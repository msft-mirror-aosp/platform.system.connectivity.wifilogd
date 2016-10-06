/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cinttypes>
#include <utility>

#include "android-base/logging.h"
#include "android-base/unique_fd.h"

#include "wifilogd/byte_buffer.h"
#include "wifilogd/command_processor.h"
#include "wifilogd/local_utils.h"
#include "wifilogd/protocol.h"

namespace android {
namespace wifilogd {

using ::android::base::unique_fd;

using local_utils::CopyFromBufferOrDie;
using local_utils::GetMaxVal;

CommandProcessor::CommandProcessor(size_t buffer_size_bytes)
    : current_log_buffer_(buffer_size_bytes), os_(new Os()) {}

CommandProcessor::CommandProcessor(size_t buffer_size_bytes,
                                   std::unique_ptr<Os> os)
    : current_log_buffer_(buffer_size_bytes), os_(std::move(os)) {}

bool CommandProcessor::ProcessCommand(const void* input_buffer,
                                      size_t n_bytes_read, int fd) {
  unique_fd wrapped_fd(fd);

  if (n_bytes_read < sizeof(protocol::Command)) {
    return false;
  }

  const auto& command_header =
      CopyFromBufferOrDie<protocol::Command>(input_buffer, n_bytes_read);
  switch (command_header.opcode) {
    using protocol::Opcode;
    case Opcode::kWriteAsciiMessage:
      // Copy the entire command to the log. This defers the cost of
      // validating the rest of the CommandHeader until we dump the
      // message.
      //
      // Note that most messages will be written but never read. So, in
      // the common case, the validation cost is actually eliminated,
      // rather than just deferred.
      return CopyCommandToLog(input_buffer, n_bytes_read);
  }
}

// Private methods below.

bool CommandProcessor::CopyCommandToLog(const void* command_buffer,
                                        size_t command_len_in) {
  const uint16_t command_len =
      SAFELY_CLAMP(command_len_in, uint16_t, 0, protocol::kMaxMessageSize);

  uint16_t total_size;
  static_assert(GetMaxVal(total_size) > sizeof(TimestampHeader) &&
                    GetMaxVal(total_size) - sizeof(TimestampHeader) >=
                        protocol::kMaxMessageSize,
                "total_size cannot represent some input messages");
  total_size = sizeof(TimestampHeader) + command_len;
  CHECK(current_log_buffer_.CanFitEver(total_size));

  if (!current_log_buffer_.CanFitNow(total_size)) {
    // TODO(b/31867689):
    // 1. compress current buffer
    // 2. move old buffer to linked list
    // 3. prune old buffers, if needed
    current_log_buffer_.Clear();
  }
  CHECK(current_log_buffer_.CanFitNow(total_size));

  TimestampHeader tstamp_header;
  tstamp_header.since_boot_awake_only = os_->GetTimestamp(CLOCK_MONOTONIC);
  tstamp_header.since_boot_with_sleep = os_->GetTimestamp(CLOCK_BOOTTIME);
  tstamp_header.since_epoch = os_->GetTimestamp(CLOCK_REALTIME);

  ByteBuffer<sizeof(TimestampHeader) + protocol::kMaxMessageSize> message_buf;
  message_buf.AppendOrDie(&tstamp_header, sizeof(tstamp_header));
  message_buf.AppendOrDie(command_buffer, command_len);

  bool did_write = current_log_buffer_.Append(
      message_buf.data(),
      SAFELY_CLAMP(message_buf.size(), uint16_t, 0, GetMaxVal<uint16_t>()));
  if (!did_write) {
    // Given that we checked for enough free space, Append() should
    // have succeeded. Hence, a failure here indicates a logic error,
    // rather than a runtime error.
    LOG(FATAL) << "Unexpected failure to Append()";
  }

  return true;
}

}  // namespace wifilogd
}  // namespace android
