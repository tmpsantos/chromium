// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/tools/quic/quic_server_session.h"

#include "net/quic/crypto/quic_crypto_server_config.h"
#include "net/quic/crypto/quic_random.h"
#include "net/quic/crypto/source_address_token.h"
#include "net/quic/quic_connection.h"
#include "net/quic/quic_crypto_server_stream.h"
#include "net/quic/quic_flags.h"
#include "net/quic/quic_utils.h"
#include "net/quic/test_tools/quic_config_peer.h"
#include "net/quic/test_tools/quic_connection_peer.h"
#include "net/quic/test_tools/quic_data_stream_peer.h"
#include "net/quic/test_tools/quic_sent_packet_manager_peer.h"
#include "net/quic/test_tools/quic_session_peer.h"
#include "net/quic/test_tools/quic_sustained_bandwidth_recorder_peer.h"
#include "net/quic/test_tools/quic_test_utils.h"
#include "net/tools/quic/quic_spdy_server_stream.h"
#include "net/tools/quic/test_tools/quic_test_utils.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using __gnu_cxx::vector;
using net::test::MockConnection;
using net::test::QuicConfigPeer;
using net::test::QuicConnectionPeer;
using net::test::QuicDataStreamPeer;
using net::test::QuicSentPacketManagerPeer;
using net::test::QuicSessionPeer;
using net::test::QuicSustainedBandwidthRecorderPeer;
using net::test::SupportedVersions;
using net::test::ValueRestore;
using net::test::kClientDataStreamId1;
using net::test::kClientDataStreamId2;
using net::test::kClientDataStreamId3;
using net::test::kClientDataStreamId4;
using testing::StrictMock;
using testing::_;

namespace net {
namespace tools {
namespace test {

class QuicServerSessionPeer {
 public:
  static QuicDataStream* GetIncomingDataStream(
      QuicServerSession* s, QuicStreamId id) {
    return s->GetIncomingDataStream(id);
  }
  static QuicDataStream* GetDataStream(QuicServerSession* s, QuicStreamId id) {
    return s->GetDataStream(id);
  }
  static void SetCryptoStream(QuicServerSession* s,
                              QuicCryptoServerStream* crypto_stream) {
    s->crypto_stream_.reset(crypto_stream);
  }
};

namespace {

class QuicServerSessionTest : public ::testing::TestWithParam<QuicVersion> {
 protected:
  QuicServerSessionTest()
      : crypto_config_(QuicCryptoServerConfig::TESTING,
                       QuicRandom::GetInstance()) {
    config_.SetDefaults();
    config_.set_max_streams_per_connection(3, 3);
    config_.SetInitialFlowControlWindowToSend(
        kInitialSessionFlowControlWindowForTest);
    config_.SetInitialStreamFlowControlWindowToSend(
        kInitialStreamFlowControlWindowForTest);
    config_.SetInitialSessionFlowControlWindowToSend(
        kInitialSessionFlowControlWindowForTest);

    connection_ =
        new StrictMock<MockConnection>(true, SupportedVersions(GetParam()));
    session_.reset(new QuicServerSession(config_, connection_, &owner_));
    MockClock clock;
    handshake_message_.reset(crypto_config_.AddDefaultConfig(
        QuicRandom::GetInstance(), &clock,
        QuicCryptoServerConfig::ConfigOptions()));
    session_->InitializeSession(crypto_config_);
    visitor_ = QuicConnectionPeer::GetVisitor(connection_);
  }

  QuicVersion version() const { return connection_->version(); }

  StrictMock<MockQuicServerSessionVisitor> owner_;
  StrictMock<MockConnection>* connection_;
  QuicConfig config_;
  QuicCryptoServerConfig crypto_config_;
  scoped_ptr<QuicServerSession> session_;
  scoped_ptr<CryptoHandshakeMessage> handshake_message_;
  QuicConnectionVisitorInterface* visitor_;
};

// Compares CachedNetworkParameters.
MATCHER_P(EqualsProto, network_params, "") {
  CachedNetworkParameters reference(network_params);
  return (arg->bandwidth_estimate_bytes_per_second() ==
          reference.bandwidth_estimate_bytes_per_second() &&
          arg->bandwidth_estimate_bytes_per_second() ==
          reference.bandwidth_estimate_bytes_per_second() &&
          arg->max_bandwidth_estimate_bytes_per_second() ==
          reference.max_bandwidth_estimate_bytes_per_second() &&
          arg->max_bandwidth_timestamp_seconds() ==
          reference.max_bandwidth_timestamp_seconds() &&
          arg->min_rtt_ms() == reference.min_rtt_ms() &&
          arg->previous_connection_state() ==
          reference.previous_connection_state());
}

INSTANTIATE_TEST_CASE_P(Tests, QuicServerSessionTest,
                        ::testing::ValuesIn(QuicSupportedVersions()));

TEST_P(QuicServerSessionTest, CloseStreamDueToReset) {
  // Open a stream, then reset it.
  // Send two bytes of payload to open it.
  QuicStreamFrame data1(kClientDataStreamId1, false, 0, MakeIOVector("HT"));
  vector<QuicStreamFrame> frames;
  frames.push_back(data1);
  session_->OnStreamFrames(frames);
  EXPECT_EQ(1u, session_->GetNumOpenStreams());

  // Send a reset (and expect the peer to send a RST in response).
  QuicRstStreamFrame rst1(kClientDataStreamId1, QUIC_STREAM_NO_ERROR, 0);
  EXPECT_CALL(*connection_, SendRstStream(kClientDataStreamId1,
                                          QUIC_RST_FLOW_CONTROL_ACCOUNTING, 0));
  visitor_->OnRstStream(rst1);
  EXPECT_EQ(0u, session_->GetNumOpenStreams());

  // Send the same two bytes of payload in a new packet.
  visitor_->OnStreamFrames(frames);

  // The stream should not be re-opened.
  EXPECT_EQ(0u, session_->GetNumOpenStreams());
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicServerSessionTest, NeverOpenStreamDueToReset) {
  // Send a reset (and expect the peer to send a RST in response).
  QuicRstStreamFrame rst1(kClientDataStreamId1, QUIC_STREAM_NO_ERROR, 0);
  EXPECT_CALL(*connection_, SendRstStream(kClientDataStreamId1,
                                          QUIC_RST_FLOW_CONTROL_ACCOUNTING, 0));
  visitor_->OnRstStream(rst1);
  EXPECT_EQ(0u, session_->GetNumOpenStreams());

  // Send two bytes of payload.
  QuicStreamFrame data1(kClientDataStreamId1, false, 0, MakeIOVector("HT"));
  vector<QuicStreamFrame> frames;
  frames.push_back(data1);
  visitor_->OnStreamFrames(frames);

  // The stream should never be opened, now that the reset is received.
  EXPECT_EQ(0u, session_->GetNumOpenStreams());
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicServerSessionTest, AcceptClosedStream) {
  vector<QuicStreamFrame> frames;
  // Send (empty) compressed headers followed by two bytes of data.
  frames.push_back(QuicStreamFrame(kClientDataStreamId1, false, 0,
                                   MakeIOVector("\1\0\0\0\0\0\0\0HT")));
  frames.push_back(QuicStreamFrame(kClientDataStreamId2, false, 0,
                                   MakeIOVector("\2\0\0\0\0\0\0\0HT")));
  visitor_->OnStreamFrames(frames);
  EXPECT_EQ(2u, session_->GetNumOpenStreams());

  // Send a reset (and expect the peer to send a RST in response).
  QuicRstStreamFrame rst(kClientDataStreamId1, QUIC_STREAM_NO_ERROR, 0);
  EXPECT_CALL(*connection_, SendRstStream(kClientDataStreamId1,
                                          QUIC_RST_FLOW_CONTROL_ACCOUNTING, 0));
  visitor_->OnRstStream(rst);

  // If we were tracking, we'd probably want to reject this because it's data
  // past the reset point of stream 3.  As it's a closed stream we just drop the
  // data on the floor, but accept the packet because it has data for stream 5.
  frames.clear();
  frames.push_back(
      QuicStreamFrame(kClientDataStreamId1, false, 2, MakeIOVector("TP")));
  frames.push_back(
      QuicStreamFrame(kClientDataStreamId2, false, 2, MakeIOVector("TP")));
  visitor_->OnStreamFrames(frames);
  // The stream should never be opened, now that the reset is received.
  EXPECT_EQ(1u, session_->GetNumOpenStreams());
  EXPECT_TRUE(connection_->connected());
}

TEST_P(QuicServerSessionTest, MaxNumConnections) {
  EXPECT_EQ(0u, session_->GetNumOpenStreams());
  EXPECT_TRUE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId1));
  EXPECT_TRUE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId2));
  EXPECT_TRUE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId3));
  EXPECT_CALL(*connection_, SendConnectionClose(QUIC_TOO_MANY_OPEN_STREAMS));
  EXPECT_FALSE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId4));
}

TEST_P(QuicServerSessionTest, MaxNumConnectionsImplicit) {
  EXPECT_EQ(0u, session_->GetNumOpenStreams());
  EXPECT_TRUE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId1));
  // Implicitly opens two more streams.
  EXPECT_CALL(*connection_, SendConnectionClose(QUIC_TOO_MANY_OPEN_STREAMS));
  EXPECT_FALSE(QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId4));
}

TEST_P(QuicServerSessionTest, GetEvenIncomingError) {
  // Incoming streams on the server session must be odd.
  EXPECT_CALL(*connection_, SendConnectionClose(QUIC_INVALID_STREAM_ID));
  EXPECT_EQ(NULL,
            QuicServerSessionPeer::GetIncomingDataStream(session_.get(), 4));
}

TEST_P(QuicServerSessionTest, SetFecProtectionFromConfig) {
  ValueRestore<bool> old_flag(&FLAGS_enable_quic_fec, true);

  // Set received config to have FEC connection option.
  QuicTagVector copt;
  copt.push_back(kFHDR);
  QuicConfigPeer::SetReceivedConnectionOptions(session_->config(), copt);
  session_->OnConfigNegotiated();

  // Verify that headers stream is always protected and data streams are
  // optionally protected.
  EXPECT_EQ(FEC_PROTECT_ALWAYS,
            QuicSessionPeer::GetHeadersStream(session_.get())->fec_policy());
  QuicDataStream* stream = QuicServerSessionPeer::GetIncomingDataStream(
      session_.get(), kClientDataStreamId1);
  ASSERT_TRUE(stream);
  EXPECT_EQ(FEC_PROTECT_OPTIONAL, stream->fec_policy());
}

class MockQuicCryptoServerStream : public QuicCryptoServerStream {
 public:
  explicit MockQuicCryptoServerStream(
      const QuicCryptoServerConfig& crypto_config, QuicSession* session)
      : QuicCryptoServerStream(crypto_config, session) {}
  virtual ~MockQuicCryptoServerStream() {}

  MOCK_METHOD1(SendServerConfigUpdate,
               void(const CachedNetworkParameters* cached_network_parameters));
 private:
  DISALLOW_COPY_AND_ASSIGN(MockQuicCryptoServerStream);
};

TEST_P(QuicServerSessionTest, BandwidthEstimates) {
  if (version() <= QUIC_VERSION_21) {
    return;
  }
  // Test that bandwidth estimate updates are sent to the client, only after the
  // bandwidth estimate has changes sufficiently, and enough time has passed.

  int32 bandwidth_estimate_kbytes_per_second = 123;
  int32 max_bandwidth_estimate_kbytes_per_second = 134;
  int32 max_bandwidth_estimate_timestamp = 1122334455;
  const string serving_region = "not a real region";
  session_->set_serving_region(serving_region);

  MockQuicCryptoServerStream* crypto_stream =
      new MockQuicCryptoServerStream(crypto_config_, session_.get());
  QuicServerSessionPeer::SetCryptoStream(session_.get(), crypto_stream);

  // Set some initial bandwidth values.
  QuicSentPacketManager* sent_packet_manager =
      QuicConnectionPeer::GetSentPacketManager(session_->connection());
  QuicSustainedBandwidthRecorder& bandwidth_recorder =
      QuicSentPacketManagerPeer::GetBandwidthRecorder(sent_packet_manager);
  QuicSustainedBandwidthRecorderPeer::SetBandwidthEstimate(
      &bandwidth_recorder, bandwidth_estimate_kbytes_per_second);
  QuicSustainedBandwidthRecorderPeer::SetMaxBandwidthEstimate(
      &bandwidth_recorder, max_bandwidth_estimate_kbytes_per_second,
      max_bandwidth_estimate_timestamp);

  // There will be no update sent yet - not enough time has passed.
  QuicTime now = QuicTime::Zero();
  session_->OnCongestionWindowChange(now);

  // Bandwidth estimate has now changed sufficiently but not enough time has
  // passed to send a Server Config Update.
  bandwidth_estimate_kbytes_per_second =
      bandwidth_estimate_kbytes_per_second * 1.6;
  session_->OnCongestionWindowChange(now);

  // Bandwidth estimate has now changed sufficiently and enough time has passed.
  int64 srtt_ms =
      sent_packet_manager->GetRttStats()->SmoothedRtt().ToMilliseconds();
  now = now.Add(QuicTime::Delta::FromMilliseconds(
      kMinIntervalBetweenServerConfigUpdatesRTTs * srtt_ms));

  // Verify that the proto has exactly the values we expect.
  CachedNetworkParameters expected_network_params;
  expected_network_params.set_bandwidth_estimate_bytes_per_second(
      bandwidth_recorder.BandwidthEstimate().ToBytesPerSecond());
  expected_network_params.set_max_bandwidth_estimate_bytes_per_second(
      bandwidth_recorder.MaxBandwidthEstimate().ToBytesPerSecond());
  expected_network_params.set_max_bandwidth_timestamp_seconds(
      bandwidth_recorder.MaxBandwidthTimestamp());
  expected_network_params.set_min_rtt_ms(session_->connection()
                                             ->sent_packet_manager()
                                             .GetRttStats()
                                             ->min_rtt()
                                             .ToMilliseconds());
  expected_network_params.set_previous_connection_state(
      CachedNetworkParameters::CONGESTION_AVOIDANCE);
  expected_network_params.set_serving_region(serving_region);

  EXPECT_CALL(*crypto_stream,
              SendServerConfigUpdate(EqualsProto(expected_network_params)))
      .Times(1);
  session_->OnCongestionWindowChange(now);
}

}  // namespace
}  // namespace test
}  // namespace tools
}  // namespace net
