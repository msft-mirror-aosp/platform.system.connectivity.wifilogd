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

#include <algorithm>
#include <cstdint>

#include "android-base/logging.h"

#include "wifilogd/local_utils.h"
#include "wifilogd/os.h"

namespace android {
namespace wifilogd {

using local_utils::GetMaxVal;

namespace {
constexpr auto kMaxNanoSeconds = 1000 * 1000 * 1000 - 1;
}

Os::Os() : raw_os_(new RawOs()) {}
Os::Os(std::unique_ptr<RawOs> raw_os) : raw_os_(std::move(raw_os)) {}
Os::~Os() {}

Os::Timestamp Os::GetTimestamp(clockid_t clock_id) const {
  struct timespec now_timespec;
  int failed = raw_os_->ClockGettime(clock_id, &now_timespec);
  if (failed) {
    LOG(FATAL) << "Unexpected error: " << std::strerror(errno);
  }
  CHECK(now_timespec.tv_nsec <= kMaxNanoSeconds);

  Timestamp now_timestamp;
  now_timestamp.secs = SAFELY_CLAMP(
      now_timespec.tv_sec, uint32_t, 0,
      // The upper-bound comes from the source-type on 32-bit platforms,
      // and the dest-type on 64-bit platforms. Using min(), we can figure out
      // which type to use for the upper bound, without resorting to macros.
      std::min(static_cast<uintmax_t>(GetMaxVal(now_timestamp.secs)),
               static_cast<uintmax_t>(GetMaxVal(now_timespec.tv_sec))));
  now_timestamp.nsecs =
      SAFELY_CLAMP(now_timespec.tv_nsec, uint32_t, 0, kMaxNanoSeconds);
  return now_timestamp;
}

}  // namespace wifilogd
}  // namespace android
