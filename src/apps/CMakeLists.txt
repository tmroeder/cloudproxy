# Copyright (c) 2014, Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

include_directories(..)
include_directories(${CMAKE_BINARY_DIR})
include_directories(../third_party/google-glog/src)
include_directories(../third_party/gflags/src)
include_directories(/usr/include)

# for the chromium definitions, we need -DOS_POSIX
add_definitions(-DOS_POSIX)
include_directories(${CMAKE_SOURCE_DIR}/third_party/chromium/include)

add_custom_command(OUTPUT ${CMAKE_BINARY_DIR}/auth.cc ${CMAKE_BINARY_DIR}/auth.h
                   COMMAND go run ${CMAKE_SOURCE_DIR}/../go/apps/genauth/genauth/genauth.go -ast_file ${CMAKE_SOURCE_DIR}/../go/tao/auth/ast.go -binary_file ${CMAKE_SOURCE_DIR}/../go/tao/auth/binary.go -header_file ${CMAKE_BINARY_DIR}/auth.h -impl_file ${CMAKE_BINARY_DIR}/auth.cc
                   DEPENDS ${CMAKE_SOURCE_DIR}/../go/apps/genauth/genauth/genauth.go ${CMAKE_SOURCE_DIR}/../go/apps/genauth/cppgen.go ${CMAKE_SOURCE_DIR}/../go/apps/genauth/visitors.go)

add_library(auth ${CMAKE_BINARY_DIR}/auth.cc ${CMAKE_BINARY_DIR}/auth.h)

add_executable(encode_auth encode_auth.cc)
target_link_libraries(encode_auth tao auth)

add_executable(go_child go_child.cc)
target_link_libraries(go_child tao)
