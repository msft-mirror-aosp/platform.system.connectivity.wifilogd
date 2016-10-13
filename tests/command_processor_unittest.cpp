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

#include <unistd.h>

#include <memory>
#include <string>
#include <utility>

#include "android-base/unique_fd.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "wifilogd/byte_buffer.h"
#include "wifilogd/local_utils.h"
#include "wifilogd/protocol.h"
#include "wifilogd/tests/mock_os.h"

#include "wifilogd/command_processor.h"

namespace android {
namespace wifilogd {
namespace {

using ::android::base::unique_fd;
using ::testing::_;
using ::testing::AnyNumber;
using ::testing::StrictMock;
using local_utils::GetMaxVal;

// The CommandBuffer is deliberately larger than the maximal permitted
// command, so that we can test the CommandProcessor's handling of oversized
// inputs.
using CommandBuffer = ByteBuffer<protocol::kMaxMessageSize * 2>;

constexpr size_t kBufferSizeBytes = protocol::kMaxMessageSize * 16;
constexpr size_t kMaxAsciiMessagePayloadLen = protocol::kMaxMessageSize -
                                              sizeof(protocol::Command) -
                                              sizeof(protocol::AsciiMessage);

class CommandProcessorTest : public ::testing::Test {
 public:
  CommandProcessorTest() {
    os_ = new StrictMock<MockOs>();
    command_processor_ = std::unique_ptr<CommandProcessor>(
        new CommandProcessor(kBufferSizeBytes, std::unique_ptr<Os>(os_)));
  }

 protected:
  CommandBuffer BuildAsciiMessageCommandWithSizeAdjustments(
      const std::string& tag, const std::string& message,
      ssize_t command_size_adjustment, ssize_t tag_size_adjustment,
      ssize_t message_size_adjustment) {
    protocol::AsciiMessage ascii_message_header;
    constexpr auto kMaxTagLength = GetMaxVal(ascii_message_header.tag_len);
    constexpr auto kMaxDataLength = GetMaxVal(ascii_message_header.data_len);
    EXPECT_TRUE(tag.length() <= kMaxTagLength);
    EXPECT_TRUE(message.length() <= kMaxDataLength);
    ascii_message_header.tag_len = SAFELY_CLAMP(
        tag.length() + tag_size_adjustment, uint8_t, 0, kMaxTagLength);
    ascii_message_header.data_len =
        SAFELY_CLAMP(message.length() + message_size_adjustment, uint16_t, 0,
                     kMaxDataLength);
    ascii_message_header.severity = protocol::MessageSeverity::kError;

    protocol::Command command{};
    constexpr auto kMaxPayloadLength = GetMaxVal(command.payload_len);
    size_t payload_length = sizeof(ascii_message_header) + tag.length() +
                            message.length() + command_size_adjustment;
    EXPECT_TRUE(payload_length <= kMaxPayloadLength);
    command.opcode = protocol::Opcode::kWriteAsciiMessage;
    command.payload_len =
        SAFELY_CLAMP(payload_length, uint16_t, 0, kMaxPayloadLength);

    CommandBuffer buf;
    buf.AppendOrDie(&command, sizeof(command));
    buf.AppendOrDie(&ascii_message_header, sizeof(ascii_message_header));
    buf.AppendOrDie(tag.data(), tag.length());
    buf.AppendOrDie(message.data(), message.length());
    return buf;
  }

  CommandBuffer BuildAsciiMessageCommand(const std::string& tag,
                                         const std::string& message) {
    return BuildAsciiMessageCommandWithSizeAdjustments(tag, message, 0, 0, 0);
  }

  bool SendAsciiMessageWithSizeAdjustments(const std::string& tag,
                                           const std::string& message,
                                           ssize_t command_size_adjustment,
                                           ssize_t tag_size_adjustment,
                                           ssize_t message_size_adjustment) {
    const CommandBuffer& command_buffer(
        BuildAsciiMessageCommandWithSizeAdjustments(
            tag, message, command_size_adjustment, tag_size_adjustment,
            message_size_adjustment));
    EXPECT_CALL(*os_, GetTimestamp(CLOCK_MONOTONIC));
    EXPECT_CALL(*os_, GetTimestamp(CLOCK_BOOTTIME));
    EXPECT_CALL(*os_, GetTimestamp(CLOCK_REALTIME));
    return command_processor_->ProcessCommand(
        command_buffer.data(), command_buffer.size(), Os::kInvalidFd);
  }

  bool SendAsciiMessage(const std::string& tag, const std::string& message) {
    return SendAsciiMessageWithSizeAdjustments(tag, message, 0, 0, 0);
  }

  std::unique_ptr<CommandProcessor> command_processor_;
  // We use a raw pointer to access the mock, since ownership passes
  // to |command_processor_|.
  StrictMock<MockOs>* os_;
};

}  // namespace

// A valid ASCII message should, of course, be processed successfully.
TEST_F(CommandProcessorTest, ProcessCommandOnValidAsciiMessageSucceeds) {
  EXPECT_TRUE(SendAsciiMessage("tag", "message"));
}

// If the buffer given to ProcessCommand() is shorter than a protocol::Command,
// then we discard the data.
TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageShorterThanCommandFails) {
  const CommandBuffer& command_buffer(
      BuildAsciiMessageCommand("tag", "message"));
  EXPECT_FALSE(command_processor_->ProcessCommand(
      command_buffer.data(), sizeof(protocol::Command) - 1, Os::kInvalidFd));
}

// In all other cases, we save the data we got, and will try to salvage the
// contents when dumping.
TEST_F(CommandProcessorTest, ProcessCommandOnAsciiMessageWithEmtpyTagSucceeds) {
  EXPECT_TRUE(SendAsciiMessage("", "message"));
}

TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageWithEmptyMessageSucceeds) {
  EXPECT_TRUE(SendAsciiMessage("tag", ""));
}

TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageWithEmptyTagAndMessageSucceeds) {
  EXPECT_TRUE(SendAsciiMessage("", ""));
}

TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageWithBadCommandLengthSucceeds) {
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", 1, 0, 0));
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", -1, 0, 0));
}

TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageWithBadTagLengthSucceeds) {
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", 0, 1, 0));
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", 0, -1, 0));
}

TEST_F(CommandProcessorTest,
       ProcessCommandOnAsciiMessageWithBadMessageLengthSucceeds) {
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", 0, 0, 1));
  EXPECT_TRUE(SendAsciiMessageWithSizeAdjustments("tag", "message", 0, 0, -1));
}

TEST_F(CommandProcessorTest, ProcessCommandOnOverlyLargeAsciiMessageSucceeds) {
  const std::string tag{"tag"};
  EXPECT_TRUE(SendAsciiMessage(
      tag, std::string(kMaxAsciiMessagePayloadLen - tag.size() + 1, '.')));
}

TEST_F(CommandProcessorTest, ProcessCommandSucceedsEvenAfterFillingBuffer) {
  const std::string tag{"tag"};
  const std::string message(kMaxAsciiMessagePayloadLen - tag.size(), '.');
  for (size_t cumulative_payload_bytes = 0;
       cumulative_payload_bytes <= kBufferSizeBytes;
       cumulative_payload_bytes += (tag.size() + message.size())) {
    EXPECT_TRUE(SendAsciiMessage(tag, message));
  }
}

// Strictly speaking, this is not a unit test. But there's no easy way to get
// unique_fd to call on an instance of our Os.
TEST_F(CommandProcessorTest, ProcessCommandClosesFd) {
  int pipe_fds[2];
  ASSERT_EQ(0, pipe(pipe_fds));

  const unique_fd our_fd{pipe_fds[0]};
  const int their_fd = pipe_fds[1];
  const CommandBuffer& command_buffer(
      BuildAsciiMessageCommand("tag", "message"));
  EXPECT_CALL(*os_, GetTimestamp(_)).Times(AnyNumber());
  EXPECT_TRUE(command_processor_->ProcessCommand(
      command_buffer.data(), command_buffer.size(), their_fd));
  EXPECT_EQ(-1, close(their_fd));
  EXPECT_EQ(EBADF, errno);
}

// Strictly speaking, this is not a unit test. But there's no easy way to get
// unique_fd to call on an instance of our Os.
TEST_F(CommandProcessorTest, ProcessCommandClosesFdEvenOnFailure) {
  int pipe_fds[2];
  ASSERT_EQ(0, pipe(pipe_fds));

  const unique_fd our_fd{pipe_fds[0]};
  const int their_fd = pipe_fds[1];
  const CommandBuffer command_buffer;
  EXPECT_FALSE(command_processor_->ProcessCommand(
      command_buffer.data(), command_buffer.size(), their_fd));
  EXPECT_EQ(-1, close(their_fd));
  EXPECT_EQ(EBADF, errno);
}

}  // namespace wifilogd
}  // namespace android
