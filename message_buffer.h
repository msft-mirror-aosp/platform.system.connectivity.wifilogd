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

#ifndef MESSAGE_BUFFER_H_
#define MESSAGE_BUFFER_H_

#include <cstdint>
#include <memory>

#include "android-base/macros.h"

namespace android {
namespace wifilogd {

// A fixed-size buffer, which provides FIFO access to read and write
// a sequence of messages.
class MessageBuffer {
 public:
  // Constructs the buffer. |size| must be greater than GetHeaderSize().
  explicit MessageBuffer(size_t size);

  // Returns the size of MessageBuffer's per-message header.
  static constexpr size_t GetHeaderSize() { return sizeof(LengthHeader); }

 private:
  struct LengthHeader {
    uint16_t payload_len;
  };

  std::unique_ptr<uint8_t[]> data_;

  // MessageBuffer is a value type, so it would be semantically reasonable to
  // support copy and assign. Performance-wise, though, we should avoid
  // copies. Remove the copy constructor and the assignment operator, to
  // ensure that we don't accidentally make copies.
  DISALLOW_COPY_AND_ASSIGN(MessageBuffer);
};

}  // namespace wifilogd
}  // namespace android

#endif  // MESSAGE_BUFFER_H_
