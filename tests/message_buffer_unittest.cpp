/*
 * Copyright (C) 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"

#include "wifilogd/message_buffer.h"

namespace android {
namespace wifilogd {
namespace {

constexpr size_t kBufferSizeBytes = 1024;
constexpr size_t kHeaderSizeBytes = MessageBuffer::GetHeaderSize();

class MessageBufferTest : public ::testing::Test {
 public:
  MessageBufferTest() : buffer_{kBufferSizeBytes} {}

 protected:
  MessageBuffer buffer_;
};

}  // namespace

// Per
// github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#death-tests),
// death tests should be specially named.
using MessageBufferDeathTest = MessageBufferTest;

TEST_F(MessageBufferDeathTest, ConstructionOfUselesslySmallBufferCausesDeath) {
  EXPECT_DEATH(MessageBuffer{kHeaderSizeBytes}, "Check failed");
}

}  // namespace wifilogd
}  // namespace android
