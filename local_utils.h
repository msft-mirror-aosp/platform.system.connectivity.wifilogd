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

// Local utilities (macros and free-standing functions).

#ifndef LOCAL_UTILS_H_
#define LOCAL_UTILS_H_

#include <limits>
#include <type_traits>

// Converts the value SRC to a value of DST_TYPE, in the range of [MIN, MAX].
// Values less than MIN are clamped to MIN, and values greater than MAX are
// clamped to MAX. Conversions are safe in the sense that the range is checked
// to be valid for both SRC and DST_TYPE, at compile-time.
//
// As compared to static_cast<>, SAFELY_CLAMP is a) more explicit, b) more
// flexible, and c) less prone to surprising conversions (e.g. -1 becoming
// UINT_MAX).
#define SAFELY_CLAMP(SRC, DST_TYPE, MIN, MAX)                                \
  local_utils::internal::SafelyClamp<decltype(SRC), DST_TYPE, MIN, MAX, MIN, \
                                     MAX>(SRC)

// While attributes are standard in C++11, the nonnull attribute is not part of
// the standard. We use a macro to abstract the nonnull attribute, to allow
// the code to compile with compilers that don't recognize nonnull.
#if defined(__clang__)
#define NONNULL [[gnu::nonnull]] /* NOLINT(whitespace/braces) */
#else
#define NONNULL
#endif

namespace android {
namespace wifilogd {
namespace local_utils {

// Returns the maximal value representable by T. Generates a compile-time
// error if T is not an integral type.
template <typename T>
constexpr T GetMaxVal() {
  // Give a useful error for non-numeric types, and avoid returning zero for
  // pointers and C-style enums (http://stackoverflow.com/a/9201960).
  static_assert(std::is_integral<T>::value,
                "GetMaxVal requires an integral type");
  return std::numeric_limits<T>::max();
}

// Returns the maximal value representable by |t_instance|. Generates a
// compile-time error if |t_instance| is not an instance of an integral type.
template <typename T>
constexpr T GetMaxVal(const T& /* t_instance */) {
  return GetMaxVal<T>();
}

namespace internal {

// Implements the functionality documented for the SAFELY_CLAMP macro.
// This function should be used via the SAFELY_CLAMP macro.
template <typename SrcType, typename DstType, SrcType MinAsSrcType,
          SrcType MaxAsSrcType, DstType MinAsDstType, DstType MaxAsDstType>
DstType SafelyClamp(SrcType input) {
  static_assert(std::is_integral<SrcType>::value,
                "source type must be integral");
  static_assert(std::is_integral<DstType>::value,
                "destination type must be integral");
  static_assert(MinAsSrcType < MaxAsSrcType, "invalid source range");
  static_assert(MinAsDstType < MaxAsDstType, "invalid destination range");
  // Clients should use the SAFELY_CLAMP macro, in which case this should never
  // happen. (When the SAFELY_CLAMP macro is used, the values can only be
  // unequal if there was a narrowing conversion. But, in that case, the value
  // should have failed to match the template, since narrowing-conversions are
  // not allowed for non-type template arguments.
  // http://stackoverflow.com/a/24346350)
  //
  // Anyway, these checks provide a fail-safe, in case clients use the template
  // function directly, and pass in inconsistent values for the range
  // definition.
  static_assert(MinAsSrcType == MinAsDstType, "inconsistent range min");
  static_assert(MaxAsSrcType == MaxAsDstType, "inconsistent range max");

  if (input < MinAsSrcType) {
    return MinAsDstType;
  } else if (input > MaxAsSrcType) {
    return MaxAsDstType;
  } else {
    // - Given that the template has matched, we know that MinAsSrcType,
    //   MaxAsSrcType, MinAsDstType, and MaxAsDstType are valid for their
    //   respective types. (See narrowing-conversion comment above.)
    // - Given the static_assert()s above, we know that a) the ranges are
    //   well-formed, and that the b) source range is identical to the
    //   destination range.
    // - Given the range checks above, we know that |input| is within the range.
    //
    // Hence, the value to be returned must be valid for DstType, and the
    // expression below has the same value as |input|.
    return static_cast<DstType>(input);
  }
}

}  // namespace internal

}  // namespace local_utils
}  // namespace wifilogd
}  // namespace android

#endif  // LOCAL_UTILS_H_
