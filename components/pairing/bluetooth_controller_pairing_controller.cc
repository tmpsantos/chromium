// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/pairing/bluetooth_controller_pairing_controller.h"

#include "base/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "components/pairing/bluetooth_pairing_constants.h"
#include "components/pairing/pairing_api.pb.h"
#include "device/bluetooth/bluetooth_adapter_factory.h"
#include "device/bluetooth/bluetooth_discovery_session.h"
#include "net/base/io_buffer.h"

namespace {
const int kReceiveSize = 16384;
}

namespace pairing_chromeos {

BluetoothControllerPairingController::BluetoothControllerPairingController()
    : current_stage_(STAGE_NONE),
      got_initial_status_(false),
      proto_decoder_(new ProtoDecoder(this)),
      ptr_factory_(this) {
}

BluetoothControllerPairingController::~BluetoothControllerPairingController() {
  Reset();
}

device::BluetoothDevice* BluetoothControllerPairingController::GetController() {
  DCHECK(!controller_device_id_.empty());
  device::BluetoothDevice* device = adapter_->GetDevice(controller_device_id_);
  if (!device) {
    LOG(ERROR) << "Lost connection to controller.";
    ChangeStage(STAGE_ESTABLISHING_CONNECTION_ERROR);
  }

  return device;
}

void BluetoothControllerPairingController::ChangeStage(Stage new_stage) {
  if (current_stage_ == new_stage)
    return;
  current_stage_ = new_stage;
  FOR_EACH_OBSERVER(ControllerPairingController::Observer, observers_,
                    PairingStageChanged(new_stage));
}

void BluetoothControllerPairingController::Reset() {
  got_initial_status_ = false;
  controller_device_id_.clear();
  discovery_session_.reset();

  if (socket_) {
    socket_->Close();
    socket_ = NULL;
  }

  if (adapter_) {
    adapter_->RemoveObserver(this);
    adapter_ = NULL;
  }
}

void BluetoothControllerPairingController::DeviceFound(
    device::BluetoothDevice* device) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(thread_checker_.CalledOnValidThread());
  if (StartsWith(device->GetName(), base::ASCIIToUTF16(kDeviceNamePrefix),
                 false)) {
    discovered_devices_.insert(device->GetAddress());
    FOR_EACH_OBSERVER(ControllerPairingController::Observer, observers_,
                      DiscoveredDevicesListChanged());
  }
}

void BluetoothControllerPairingController::DeviceLost(
    device::BluetoothDevice* device) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(thread_checker_.CalledOnValidThread());
  std::set<std::string>::iterator ix =
      discovered_devices_.find(device->GetAddress());
  if (ix != discovered_devices_.end()) {
    discovered_devices_.erase(ix);
    FOR_EACH_OBSERVER(ControllerPairingController::Observer, observers_,
                      DiscoveredDevicesListChanged());
  }
}

void BluetoothControllerPairingController::OnSetPowered() {
  DCHECK(thread_checker_.CalledOnValidThread());
  adapter_->StartDiscoverySession(
      base::Bind(&BluetoothControllerPairingController::OnStartDiscoverySession,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnGetAdapter(
    scoped_refptr<device::BluetoothAdapter> adapter) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!adapter_);
  adapter_ = adapter;
  adapter_->AddObserver(this);

  if (adapter_->IsPowered()) {
    OnSetPowered();
  } else {
    adapter_->SetPowered(
        true,
        base::Bind(&BluetoothControllerPairingController::OnSetPowered,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothControllerPairingController::OnError,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothControllerPairingController::OnStartDiscoverySession(
    scoped_ptr<device::BluetoothDiscoverySession> discovery_session) {
  DCHECK(thread_checker_.CalledOnValidThread());
  discovery_session_ = discovery_session.Pass();
  ChangeStage(STAGE_DEVICES_DISCOVERY);

  device::BluetoothAdapter::DeviceList device_list = adapter_->GetDevices();
  for (device::BluetoothAdapter::DeviceList::iterator ix = device_list.begin();
       ix != device_list.end(); ++ix) {
    DeviceFound(*ix);
  }
}

void BluetoothControllerPairingController::OnConnect() {
  DCHECK(thread_checker_.CalledOnValidThread());
  device::BluetoothDevice* device = GetController();
  if (device) {
    device->ConnectToService(
        device::BluetoothUUID(kPairingServiceUUID),
        base::Bind(&BluetoothControllerPairingController::OnConnectToService,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(&BluetoothControllerPairingController::OnErrorWithMessage,
                   ptr_factory_.GetWeakPtr()));
  }
}

void BluetoothControllerPairingController::OnConnectToService(
    scoped_refptr<device::BluetoothSocket> socket) {
  DCHECK(thread_checker_.CalledOnValidThread());
  socket_ = socket;

  socket_->Receive(
      kReceiveSize,
      base::Bind(&BluetoothControllerPairingController::OnReceiveComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnReceiveError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnSendComplete(int bytes_sent) {}

void BluetoothControllerPairingController::OnReceiveComplete(
    int bytes, scoped_refptr<net::IOBuffer> io_buffer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  proto_decoder_->DecodeIOBuffer(bytes, io_buffer);

  socket_->Receive(
      kReceiveSize,
      base::Bind(&BluetoothControllerPairingController::OnReceiveComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnReceiveError,
                 ptr_factory_.GetWeakPtr()));
}

void BluetoothControllerPairingController::OnError() {
  // TODO(zork): Add a stage for initialization error. (http://crbug.com/405744)
  LOG(ERROR) << "Pairing initialization failed";
  Reset();
}

void BluetoothControllerPairingController::OnErrorWithMessage(
    const std::string& message) {
  // TODO(zork): Add a stage for initialization error. (http://crbug.com/405744)
  LOG(ERROR) << message;
  Reset();
}

void BluetoothControllerPairingController::OnConnectError(
    device::BluetoothDevice::ConnectErrorCode error_code) {
  DCHECK(thread_checker_.CalledOnValidThread());
  device::BluetoothDevice* device = GetController();

  if (device && device->IsPaired()) {
    // The connection attempt is only used to start the pairing between the
    // devices.  If the connection fails, it's not a problem as long as pairing
    // was successful.
    OnConnect();
  }
}

void BluetoothControllerPairingController::OnReceiveError(
    device::BluetoothSocket::ErrorReason reason,
    const std::string& error_message) {
  LOG(ERROR) << reason << ", " << error_message;
  Reset();
}

void BluetoothControllerPairingController::AddObserver(
    ControllerPairingController::Observer* observer) {
  observers_.AddObserver(observer);
}

void BluetoothControllerPairingController::RemoveObserver(
    ControllerPairingController::Observer* observer) {
  observers_.RemoveObserver(observer);
}

ControllerPairingController::Stage
BluetoothControllerPairingController::GetCurrentStage() {
  return current_stage_;
}

void BluetoothControllerPairingController::StartPairing() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(current_stage_ == STAGE_NONE ||
         current_stage_ == STAGE_DEVICE_NOT_FOUND ||
         current_stage_ == STAGE_ESTABLISHING_CONNECTION_ERROR ||
         current_stage_ == STAGE_HOST_ENROLLMENT_ERROR);
  // TODO(zork): Add a stage for no bluetooth. (http://crbug.com/405744)
  if (!device::BluetoothAdapterFactory::IsBluetoothAdapterAvailable()) {
    ChangeStage(STAGE_DEVICE_NOT_FOUND);
    return;
  }

  device::BluetoothAdapterFactory::GetAdapter(
      base::Bind(&BluetoothControllerPairingController::OnGetAdapter,
                 ptr_factory_.GetWeakPtr()));

}

ControllerPairingController::DeviceIdList
BluetoothControllerPairingController::GetDiscoveredDevices() {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  return DeviceIdList(discovered_devices_.begin(), discovered_devices_.end());
}

void BluetoothControllerPairingController::ChooseDeviceForPairing(
    const std::string& device_id) {
  DCHECK_EQ(current_stage_, STAGE_DEVICES_DISCOVERY);
  DCHECK(discovered_devices_.count(device_id));
  discovery_session_.reset();
  controller_device_id_ = device_id;

  device::BluetoothDevice* device = GetController();

  if (device) {
    ChangeStage(STAGE_ESTABLISHING_CONNECTION);
    if (device->IsPaired()) {
      OnConnect();
    } else {
      device->Connect(
          this,
          base::Bind(&BluetoothControllerPairingController::OnConnect,
                     ptr_factory_.GetWeakPtr()),
          base::Bind(&BluetoothControllerPairingController::OnConnectError,
                     ptr_factory_.GetWeakPtr()));
    }
  }
}

void BluetoothControllerPairingController::RepeatDiscovery() {
  DCHECK(current_stage_ == STAGE_DEVICE_NOT_FOUND ||
         current_stage_ == STAGE_ESTABLISHING_CONNECTION_ERROR ||
         current_stage_ == STAGE_HOST_ENROLLMENT_ERROR);
  Reset();
  StartPairing();
}

std::string BluetoothControllerPairingController::GetConfirmationCode() {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CODE_CONFIRMATION);
  DCHECK(!confirmation_code_.empty());
  return confirmation_code_;
}

void BluetoothControllerPairingController::SetConfirmationCodeIsCorrect(
    bool correct) {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CODE_CONFIRMATION);

  device::BluetoothDevice* device = GetController();
  if (!device)
    return;

  if (correct) {
    device->ConfirmPairing();
    // Once pairing is confirmed, the connection will either be successful, or
    // fail.  Either case is acceptable as long as the devices are paired.
  } else {
    device->RejectPairing();
    controller_device_id_.clear();
    RepeatDiscovery();
  }
}

void BluetoothControllerPairingController::SetHostConfiguration(
    bool accepted_eula,
    const std::string& lang,
    const std::string& timezone,
    bool send_reports,
    const std::string& keyboard_layout) {
  // TODO(zork): Get configuration from UI and send to Host.
  // (http://crbug.com/405744)
}

void BluetoothControllerPairingController::OnAuthenticationDone(
    const std::string& domain,
    const std::string& auth_token) {
  DCHECK_EQ(current_stage_, STAGE_WAITING_FOR_CREDENTIALS);

  pairing_api::PairDevices pair_devices;
  pair_devices.set_api_version(kPairingAPIVersion);
  pair_devices.mutable_parameters()->set_admin_access_token(auth_token);

  int size = 0;
  scoped_refptr<net::IOBuffer> io_buffer(
      ProtoDecoder::SendPairDevices(pair_devices, &size));

  socket_->Send(
      io_buffer, size,
      base::Bind(&BluetoothControllerPairingController::OnSendComplete,
                 ptr_factory_.GetWeakPtr()),
      base::Bind(&BluetoothControllerPairingController::OnErrorWithMessage,
                 ptr_factory_.GetWeakPtr()));
  ChangeStage(STAGE_HOST_ENROLLMENT_IN_PROGRESS);
}

void BluetoothControllerPairingController::StartSession() {
  DCHECK_EQ(current_stage_, STAGE_PAIRING_DONE);
  ChangeStage(STAGE_FINISHED);
}

// ProtoDecoder::Observer:
void BluetoothControllerPairingController::OnHostStatusMessage(
    const pairing_api::HostStatus& message) {
  if (got_initial_status_) {
    // TODO(zork): Check that the domain matches. (http://crbug.com/405761)
    // TODO(zork): Handling updating stages (http://crbug.com/405754).
    pairing_api::CompleteSetup complete_setup;
    complete_setup.set_api_version(kPairingAPIVersion);
    // TODO(zork): Get AddAnother from UI (http://crbug.com/405757)
    complete_setup.mutable_parameters()->set_add_another(false);

    int size = 0;
    scoped_refptr<net::IOBuffer> io_buffer(
        ProtoDecoder::SendCompleteSetup(complete_setup, &size));

    socket_->Send(
        io_buffer, size,
        base::Bind(&BluetoothControllerPairingController::OnSendComplete,
                   ptr_factory_.GetWeakPtr()),
        base::Bind(
            &BluetoothControllerPairingController::OnErrorWithMessage,
            ptr_factory_.GetWeakPtr()));
    ChangeStage(STAGE_PAIRING_DONE);
  } else {
    got_initial_status_ = true;

    // TODO(zork): Check domain. (http://crbug.com/405761)
    ChangeStage(STAGE_WAITING_FOR_CREDENTIALS);
  }
}

void BluetoothControllerPairingController::OnConfigureHostMessage(
    const pairing_api::ConfigureHost& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnPairDevicesMessage(
    const pairing_api::PairDevices& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnCompleteSetupMessage(
    const pairing_api::CompleteSetup& message) {
  NOTREACHED();
}

void BluetoothControllerPairingController::OnErrorMessage(
    const pairing_api::Error& message) {
  LOG(ERROR) << message.parameters().code() << ", " <<
      message.parameters().description();
  ChangeStage(STAGE_HOST_ENROLLMENT_ERROR);
}

void BluetoothControllerPairingController::DeviceAdded(
    device::BluetoothAdapter* adapter,
    device::BluetoothDevice* device) {
  DCHECK_EQ(adapter, adapter_.get());
  DeviceFound(device);
}

void BluetoothControllerPairingController::DeviceRemoved(
    device::BluetoothAdapter* adapter,
    device::BluetoothDevice* device) {
  DCHECK_EQ(adapter, adapter_.get());
  DeviceLost(device);
}

void BluetoothControllerPairingController::RequestPinCode(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::RequestPasskey(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::DisplayPinCode(
    device::BluetoothDevice* device,
    const std::string& pincode) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::DisplayPasskey(
    device::BluetoothDevice* device,
    uint32 passkey) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::KeysEntered(
    device::BluetoothDevice* device,
    uint32 entered) {
  // Disallow unknown device.
  device->RejectPairing();
}

void BluetoothControllerPairingController::ConfirmPasskey(
    device::BluetoothDevice* device,
    uint32 passkey) {
  confirmation_code_ = base::StringPrintf("%06d", passkey);
  ChangeStage(STAGE_WAITING_FOR_CODE_CONFIRMATION);
}

void BluetoothControllerPairingController::AuthorizePairing(
    device::BluetoothDevice* device) {
  // Disallow unknown device.
  device->RejectPairing();
}

}  // namespace pairing_chromeos
