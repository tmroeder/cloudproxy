/****************************************************************************
* Copyright (c) 2013 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
****************************************************************************/

/****************************************************************************
* INTEL CONFIDENTIAL
* Copyright 2001-2013 Intel Corporation All Rights Reserved.
*
* The source code contained or described herein and all documents related to
* the source code ("Material") are owned by Intel Corporation or its
* suppliers or licensors.  Title to the Material remains with Intel
* Corporation or its suppliers and licensors.  The Material contains trade
* secrets and proprietary and confidential information of Intel or its
* suppliers and licensors.  The Material is protected by worldwide copyright
* and trade secret laws and treaty provisions.  No part of the Material may
* be used, copied, reproduced, modified, published, uploaded, posted,
* transmitted, distributed, or disclosed in any way without Intel's prior
* express written permission.
*
* No license under any patent, copyright, trade secret or other intellectual
* property right is granted to or conferred upon you by disclosure or
* delivery of the Materials, either expressly, by implication, inducement,
* estoppel or otherwise.  Any license under such intellectual property rights
* must be express and approved by Intel in writing.
****************************************************************************/

#ifndef _LAYOUT_HOST_MEMORY_H_
#define _LAYOUT_HOST_MEMORY_H_

#include "vmm_startup.h"
#include "vmm_objects.h"

//******************************************************************************
//*
//* Perform host memory layout for multiguest environment
//*
//* Different algorithms should be used for MBR-based abd loader-based
//* uVMM load scenarios
//*
//******************************************************************************

//------------------------------------------------------------------------------
//
// init memory layout
// This function should perform init of "memory layout object" and
// init the primary guest memory layout.
//
//------------------------------------------------------------------------------
BOOLEAN init_memory_layout_from_mbr(
                    const VMM_MEMORY_LAYOUT* vmm_memory_layout,
                    GPM_HANDLE               primary_guest_gpm,
                    BOOLEAN                  are_secondary_guests_exist,
                    const VMM_APPLICATION_PARAMS_STRUCT* application_params);

//------------------------------------------------------------------------------
//
// Allocate memory for seondary guest, remove it from primary guest and
// return allocated address
//
//------------------------------------------------------------------------------
UINT64 allocate_memory_for_secondary_guest_from_mbr(
                    GPM_HANDLE               primary_guest_gpm,
                    GPM_HANDLE               secondary_guest_gpm,
                    UINT32                   required_size );

// wrappers
INLINE
BOOLEAN init_memory_layout(
                    const VMM_MEMORY_LAYOUT* vmm_memory_layout,
                    GPM_HANDLE               primary_guest_gpm,
                    BOOLEAN                  are_secondary_guests_exist,
                    const VMM_APPLICATION_PARAMS_STRUCT* application_params)
{
    return init_memory_layout_from_mbr( vmm_memory_layout,
                                        primary_guest_gpm,
                                        are_secondary_guests_exist,
                                        application_params);
}
#ifdef INCLUDE_UNUSED_CODE
#pragma warning (push)
#pragma warning (disable : 4100)
INLINE
UINT64 allocate_memory_for_secondary_guest(
                    GPM_HANDLE               primary_guest_gpm UNUSED,
                    GPM_HANDLE               secondary_guest_gpm UNUSED,
                    UINT32                   required_size UNUSED)
{
    return 0; /*allocate_memory_for_secondary_guest_from_mbr(
                                            primary_guest_gpm,
                                            secondary_guest_gpm,
                                            required_size );*/
}
#pragma warning (pop)
#endif

#endif // _LAYOUT_HOST_MEMORY_H_

