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

#ifndef COMMAND_PROCESSOR_H_
#define COMMAND_PROCESSOR_H_

#include <memory>

#include "android-base/macros.h"

#include "wifilogd/local_utils.h"
#include "wifilogd/message_buffer.h"
#include "wifilogd/os.h"

namespace android {
namespace wifilogd {

class CommandProcessor {
 public:
  // Constructs a CommandProcessor with a buffer of |buffer_size_bytes|.
  explicit CommandProcessor(size_t buffer_size_bytes);

  // Constructs a CommandProcessor with a buffer of |buffer_size_bytes|.
  // The CommandProcessor will use |os| to call into operating system services.
  // This method allows tests to provide a MockOs.
  CommandProcessor(size_t buffer_size_bytes, std::unique_ptr<Os> os);

  // Processes the given command. The effect of this call depends on the
  // contents of |input_buf|.
  bool ProcessInput(NONNULL const void* input_buf, size_t n_bytes_read);

 private:
  struct TimestampHeader {
    Os::Timestamp since_boot_awake_only;
    Os::Timestamp since_boot_with_sleep;
    Os::Timestamp since_epoch;  // non-monotonic
  };

  // Copies |command_buffer| into the log buffer. Returns true if the
  // command was copied. If |command_len| exceeds protocol::kMaxMessageSize,
  // copies the first protocol::kMaxMessageSize of |command_buffer|, and returns
  // true.
  bool CopyCommandToLog(NONNULL const void* command_buffer, size_t command_len);

  // The MessageBuffer is inlined, since there's not much value to mocking
  // simple data objects. See Testing on the Toilet Episode 173.
  MessageBuffer current_log_buffer_;
  const std::unique_ptr<Os> os_;

  DISALLOW_COPY_AND_ASSIGN(CommandProcessor);
};

}  // namespace wifilogd
}  // namespace android

#endif  // COMMAND_PROCESSOR_H_