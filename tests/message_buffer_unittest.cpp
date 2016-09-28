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

#include <array>

#include "gtest/gtest.h"

#include "wifilogd/message_buffer.h"

namespace android {
namespace wifilogd {
namespace {

constexpr size_t kBufferSizeBytes = 1024;
constexpr size_t kHeaderSizeBytes = MessageBuffer::GetHeaderSize();
constexpr std::array<uint8_t, 1> kSmallestMessage{};
constexpr std::array<uint8_t, kBufferSizeBytes - kHeaderSizeBytes>
    kLargestMessage{};

class MessageBufferTest : public ::testing::Test {
 public:
  MessageBufferTest() : buffer_{kBufferSizeBytes} {}

 protected:
  MessageBuffer buffer_;
};

}  // namespace

TEST_F(MessageBufferTest, AppendMinimalOnEmptyBufferSucceeds) {
  EXPECT_TRUE(buffer_.Append(kSmallestMessage.data(), kSmallestMessage.size()));
}

TEST_F(MessageBufferTest, AppendMaximalOnEmptyBufferSucceeds) {
  EXPECT_TRUE(buffer_.Append(kLargestMessage.data(), kLargestMessage.size()));
}

TEST_F(MessageBufferTest, AppendUnalignedMessagesDoesNotCrash) {
  // Odd-length messages should trigger alignment problems, if any such
  // problems exist. We'll need more than one, though, since the first header
  // might be aligned by default.
  constexpr std::array<uint8_t, 1> message{};
  while (buffer_.CanFitNow(message.size())) {
    ASSERT_TRUE(buffer_.Append(message.data(), message.size()));
  }
}

TEST_F(MessageBufferTest, AppendLargerThanBufferFails) {
  constexpr std::array<uint8_t, kBufferSizeBytes + 1> oversized_message{};
  EXPECT_FALSE(
      buffer_.Append(oversized_message.data(), oversized_message.size()));
}

TEST_F(MessageBufferTest, AppendLargerThanFreeSpaceFails) {
  constexpr size_t expected_free = kBufferSizeBytes - kHeaderSizeBytes;
  ASSERT_FALSE(buffer_.CanFitNow(expected_free + 1));

  constexpr std::array<uint8_t, expected_free + 1> oversized_message{};
  EXPECT_FALSE(
      buffer_.Append(oversized_message.data(), oversized_message.size()));
}

TEST_F(MessageBufferTest, AppendMultipleMessagesToFillBufferDoesNotCrash) {
  constexpr std::array<uint8_t, kHeaderSizeBytes> message{};
  static_assert(kBufferSizeBytes % (kHeaderSizeBytes + message.size()) == 0,
                "messages will not fill buffer to capacity");
  for (size_t i = 0; i < kBufferSizeBytes / (kHeaderSizeBytes + message.size());
       ++i) {
    ASSERT_TRUE(buffer_.Append(message.data(), message.size()));
  }
  ASSERT_EQ(0U, buffer_.GetFreeSize());
}

TEST_F(MessageBufferTest, CanFitNowIsCorrectOnFreshBuffer) {
  EXPECT_TRUE(buffer_.CanFitNow(kLargestMessage.size()));
  EXPECT_FALSE(buffer_.CanFitNow(kLargestMessage.size() + 1));
}

TEST_F(MessageBufferTest, CanFitNowIsCorrectAfterSmallWrite) {
  ASSERT_TRUE(buffer_.Append(kSmallestMessage.data(), kSmallestMessage.size()));

  constexpr size_t expected_free =
      kBufferSizeBytes - (kSmallestMessage.size() + kHeaderSizeBytes) -
      kHeaderSizeBytes;
  EXPECT_TRUE(buffer_.CanFitNow(expected_free));
  EXPECT_FALSE(buffer_.CanFitNow(expected_free + 1));
}

TEST_F(MessageBufferTest, CanFitNowIsCorrectOnFullBuffer) {
  ASSERT_TRUE(buffer_.Append(kLargestMessage.data(), kLargestMessage.size()));
  EXPECT_FALSE(buffer_.CanFitNow(1));
}

// Per
// github.com/google/googletest/blob/master/googletest/docs/AdvancedGuide.md#death-tests),
// death tests should be specially named.
using MessageBufferDeathTest = MessageBufferTest;

TEST_F(MessageBufferDeathTest, AppendZeroBytesCausesDeath) {
  constexpr std::array<uint8_t, 1> message{};
  EXPECT_DEATH(buffer_.Append(message.data(), 0), "Check failed");
}

TEST_F(MessageBufferDeathTest, ConstructionOfUselesslySmallBufferCausesDeath) {
  EXPECT_DEATH(MessageBuffer{kHeaderSizeBytes}, "Check failed");
}

}  // namespace wifilogd
}  // namespace android
