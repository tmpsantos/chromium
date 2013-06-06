// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_ASYNC_PIXEL_TRANSFER_MANAGER_IDLE_H_
#define GPU_COMMAND_BUFFER_SERVICE_ASYNC_PIXEL_TRANSFER_MANAGER_IDLE_H_

#include <list>

#include "gpu/command_buffer/service/async_pixel_transfer_manager.h"

namespace gpu {
class AsyncPixelTransferDelegateIdle;

class AsyncPixelTransferManagerIdle : public AsyncPixelTransferManager {
 public:
  AsyncPixelTransferManagerIdle();
  virtual ~AsyncPixelTransferManagerIdle();

  // AsyncPixelTransferManager implementation:
  virtual void BindCompletedAsyncTransfers() OVERRIDE;
  virtual void AsyncNotifyCompletion(
      const AsyncMemoryParams& mem_params,
      const CompletionCallback& callback) OVERRIDE;
  virtual uint32 GetTextureUploadCount() OVERRIDE;
  virtual base::TimeDelta GetTotalTextureUploadTime() OVERRIDE;
  virtual void ProcessMorePendingTransfers() OVERRIDE;
  virtual bool NeedsProcessMorePendingTransfers() OVERRIDE;
  virtual AsyncPixelTransferDelegate* GetAsyncPixelTransferDelegate() OVERRIDE;

  struct Task {
    Task(uint64 transfer_id, const base::Closure& task);
    ~Task();

    // This is non-zero if pixel transfer task.
    uint64 transfer_id;

    base::Closure task;
  };

  // State shared between Managers and Delegates.
  struct SharedState {
    SharedState();
    ~SharedState();
    void ProcessNotificationTasks();

    int texture_upload_count;
    base::TimeDelta total_texture_upload_time;
    std::list<Task> tasks;
  };

 private:
  SharedState shared_state_;
  scoped_ptr<AsyncPixelTransferDelegateIdle> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AsyncPixelTransferManagerIdle);
};

}  // namespace gpu

#endif  // GPU_COMMAND_BUFFER_SERVICE_ASYNC_PIXEL_TRANSFER_MANAGER_IDLE_H_
