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

// While attributes are standard in C++11, the nonnull attribute is not part of
// the standard. We use a macro to abstract the nonnull attribute, to allow
// the code to compile with compilers that don't recognize nonnull.
#if defined(__clang__)
#define NONNULL [[gnu::nonnull]] /* NOLINT(whitespace/braces) */
#else
#define NONNULL
#endif

#endif  // LOCAL_UTILS_H_
