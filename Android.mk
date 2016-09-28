# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)
wifilogd_cpp_flags := -Wall -Wextra -Weffc++ -Weverything \
    -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-padded -Werror
wifilogd_gtest_cpp_flags := -Wno-undef -Wno-missing-noreturn \
    -Wno-shift-sign-overflow -Wno-used-but-marked-unused -Wno-deprecated \
    -Wno-weak-vtables -Wno-sign-conversion -Wno-global-constructors \
    -Wno-covered-switch-default

wifilogd_parent_dir := $(LOCAL_PATH)/../
wifilogd_includes := \
    $(wifilogd_parent_dir)

###
### wifilogd static library
###
include $(CLEAR_VARS)
LOCAL_MODULE := libwifilogd
LOCAL_CPPFLAGS := $(wifilogd_cpp_flags)
LOCAL_C_INCLUDES := $(wifilogd_includes)
LOCAL_SRC_FILES := \
    message_buffer.cpp
LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog
include $(BUILD_STATIC_LIBRARY)

###
### wifilogd unit tests.
###
include $(CLEAR_VARS)
LOCAL_MODULE := wifilogd_unit_test
LOCAL_CPPFLAGS := $(wifilogd_cpp_flags) $(wifilogd_gtest_cpp_flags)
LOCAL_C_INCLUDES := $(wifilogd_includes)
LOCAL_SRC_FILES := \
    tests/message_buffer_unittest.cpp \
    tests/main.cpp
LOCAL_STATIC_LIBRARIES := \
    libgtest \
    libwifilogd
LOCAL_SHARED_LIBRARIES := \
    libbase \
    liblog
include $(BUILD_NATIVE_TEST)
