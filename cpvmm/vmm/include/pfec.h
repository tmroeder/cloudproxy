/*
 * Copyright (c) 2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef PFEC_H
#define PFEC_H

typedef union {
  struct {
    UINT32  present:1;
    UINT32  is_write:1;
    UINT32  is_user:1;
    UINT32  is_reserved:1;
    UINT32  is_fetch:1;
    UINT32  reserved:27;
  } bits;
  UINT64  value;
} VMM_PFEC;

#define VMM_PFEC_NUM_OF_USED_BITS 5

#endif // PFEC_H
