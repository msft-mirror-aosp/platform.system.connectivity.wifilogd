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

#include <cstdint>
#include <limits>

#include "gtest/gtest.h"

#include "wifilogd/local_utils.h"

namespace android {
namespace wifilogd {

using local_utils::GetMaxVal;

TEST(LocalUtilsTest, GetMaxValFromTypeIsCorrectForUnsignedTypes) {
  EXPECT_EQ(std::numeric_limits<uint8_t>::max(), GetMaxVal<uint8_t>());
  EXPECT_EQ(std::numeric_limits<uint16_t>::max(), GetMaxVal<uint16_t>());
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), GetMaxVal<uint32_t>());
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), GetMaxVal<uint64_t>());
}

TEST(LocalUtilsTest, GetMaxValFromTypeIsCorrectForSignedTypes) {
  EXPECT_EQ(std::numeric_limits<int8_t>::max(), GetMaxVal<int8_t>());
  EXPECT_EQ(std::numeric_limits<int16_t>::max(), GetMaxVal<int16_t>());
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), GetMaxVal<int32_t>());
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), GetMaxVal<int64_t>());
}

TEST(LocalUtilsTest, GetMaxValFromInstanceIsCorrectForUnsignedTypes) {
  EXPECT_EQ(std::numeric_limits<uint8_t>::max(), GetMaxVal(uint8_t{}));
  EXPECT_EQ(std::numeric_limits<uint16_t>::max(), GetMaxVal(uint16_t{}));
  EXPECT_EQ(std::numeric_limits<uint32_t>::max(), GetMaxVal(uint32_t{}));
  EXPECT_EQ(std::numeric_limits<uint64_t>::max(), GetMaxVal(uint64_t{}));
}

TEST(LocalUtilsTest, GetMaxValFromInstanceIsCorrectForSignedTypes) {
  EXPECT_EQ(std::numeric_limits<int8_t>::max(), GetMaxVal(int8_t{}));
  EXPECT_EQ(std::numeric_limits<int16_t>::max(), GetMaxVal(int16_t{}));
  EXPECT_EQ(std::numeric_limits<int32_t>::max(), GetMaxVal(int32_t{}));
  EXPECT_EQ(std::numeric_limits<int64_t>::max(), GetMaxVal(int64_t{}));
}

TEST(LocalUtilsTest, SafelyClampWorksForSameTypeClamping) {
  EXPECT_EQ(int8_t{0}, (SAFELY_CLAMP(int8_t{-1}, int8_t, 0, 2)));
  EXPECT_EQ(int8_t{0}, (SAFELY_CLAMP(int8_t{0}, int8_t, 0, 2)));
  EXPECT_EQ(int8_t{1}, (SAFELY_CLAMP(int8_t{1}, int8_t, 0, 2)));
  EXPECT_EQ(int8_t{2}, (SAFELY_CLAMP(int8_t{2}, int8_t, 0, 2)));
  EXPECT_EQ(int8_t{2}, (SAFELY_CLAMP(int8_t{3}, int8_t, 0, 2)));
}

TEST(LocalUtilsTest, SafelyClampWorksForSignedToUnsigned) {
  static_assert(std::numeric_limits<int8_t>::max() == 127,
                "upper bound is set incorrectly");
  EXPECT_EQ(uint8_t{0}, (SAFELY_CLAMP(int8_t{-1}, uint8_t, 0, 127)));
  EXPECT_EQ(uint8_t{0}, (SAFELY_CLAMP(int8_t{0}, uint8_t, 0, 127)));
  EXPECT_EQ(uint8_t{1}, (SAFELY_CLAMP(int8_t{1}, uint8_t, 0, 127)));
  EXPECT_EQ(uint8_t{127}, (SAFELY_CLAMP(int8_t{127}, uint8_t, 0, 127)));
}

TEST(LocalUtilsTest, SafelyClampWorksForUnsignedToSigned) {
  static_assert(std::numeric_limits<int8_t>::max() == 127,
                "upper bound is set incorrectly");
  EXPECT_EQ(int8_t{0}, (SAFELY_CLAMP(uint8_t{0}, int8_t, 0, 127)));
  EXPECT_EQ(int8_t{1}, (SAFELY_CLAMP(uint8_t{1}, int8_t, 0, 127)));
  EXPECT_EQ(int8_t{127}, (SAFELY_CLAMP(uint8_t{127}, int8_t, 0, 127)));
  EXPECT_EQ(int8_t{127}, (SAFELY_CLAMP(uint8_t{128}, int8_t, 0, 127)));
}

}  // namespace wifilogd
}  // namespace android
