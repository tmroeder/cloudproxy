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

#ifndef EFER_MSR_ABSTRACTION_H
#define EFER_MSR_ABSTRACTION_H


/* Function: efer_msr_set_nxe
*  Description: This function sets NXE bit in hardware EFER MSR register
*/
void efer_msr_set_nxe(void);

/* Function: efer_msr_is_nxe_bit_set
*  Description: This function checks whether NXE bit is set in EFER MSR value
*  Input:
*        efer_msr_value - 64 bit value of EFER MSR
*  Return Value: TRUE or FALSE
*/
BOOLEAN efer_msr_is_nxe_bit_set(IN UINT64 efer_msr_value);

/* Function: efer_msr_read_reg
*  Description: This function reads and returns the value of hardware EFER MSR
*  Return Value: 64 bit value of EFER MSR
*/
UINT64 efer_msr_read_reg(void);

#endif
