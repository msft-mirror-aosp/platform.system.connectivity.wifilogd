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

#include <string.h>

#include <cinttypes>
#include <string>
#include <tuple>
#include <utility>

#include "android-base/logging.h"
#include "android-base/stringprintf.h"

#include "wifilogd/byte_buffer.h"
#include "wifilogd/command_processor.h"
#include "wifilogd/local_utils.h"
#include "wifilogd/protocol.h"

namespace android {
namespace wifilogd {

using ::android::base::unique_fd;

using local_utils::CopyFromBufferOrDie;
using local_utils::GetMaxVal;

namespace {
uint32_t NsecToUsec(uint32_t nsec) { return nsec / 1000; }
}  // namespace

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
    case Opcode::kDumpBuffers:
      return Dump(std::move(wrapped_fd));
  }
}

// Private methods below.

std::string CommandProcessor::TimestampHeader::ToString() const {
  const auto& awake_time = since_boot_awake_only;
  const auto& up_time = since_boot_with_sleep;
  const auto& wall_time = since_epoch;
  return base::StringPrintf("%" PRIu32 ".%06" PRIu32
                            " "
                            "%" PRIu32 ".%06" PRIu32
                            " "
                            "%" PRIu32 ".%06" PRIu32,
                            awake_time.secs, NsecToUsec(awake_time.nsecs),
                            up_time.secs, NsecToUsec(up_time.nsecs),
                            wall_time.secs, NsecToUsec(wall_time.nsecs));
}

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

bool CommandProcessor::Dump(unique_fd dump_fd) {
  const uint8_t* buf = nullptr;
  size_t buflen = 0;
  static_assert(
      GetMaxVal(buflen) - sizeof(TimestampHeader) - sizeof(protocol::Command) >=
          GetMaxVal<decltype(protocol::Command::payload_len)>(),
      "buflen cannot accommodate some messages");

  MessageBuffer::ScopedRewinder rewinder(&current_log_buffer_);
  std::tie(buf, buflen) = current_log_buffer_.ConsumeNextMessage();
  while (buf) {
    const auto& tstamp_header =
        CopyFromBufferOrDie<TimestampHeader>(buf, buflen);
    buf += sizeof(tstamp_header);
    buflen -= sizeof(tstamp_header);

    const std::string timestamp_string(tstamp_header.ToString() + '\n');
    size_t n_written;
    Os::Errno err;
    std::tie(n_written, err) =
        os_->Write(dump_fd, timestamp_string.data(), timestamp_string.size());
    if (err == EINTR) {
      // The next write will probably succeed. We dont't retry the current
      // message, however, because we want to guarantee forward progress.
      //
      // TODO(b/32098735): Increment a counter, and attempt to output that
      // counter after we've dumped all the log messages.
    } else if (err) {
      // Any error other than EINTR is considered unrecoverable.
      LOG(ERROR) << "Terminating log dump, due to " << strerror(err);
      return false;
    }

    std::tie(buf, buflen) = current_log_buffer_.ConsumeNextMessage();
  }

  return true;
}

}  // namespace wifilogd
}  // namespace android
