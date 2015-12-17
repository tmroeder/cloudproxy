// Copyright (c) 2014, Google Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package tpm

import (
	"fmt"
)

// A Handle is a 32-bit unsigned integer.
type Handle uint32

// A commandHeader is the header for a TPM command.
type commandHeader struct {
	Tag  uint16
	Size uint32
	Cmd  uint32
}

// String returns a string version of a commandHeader
func (ch commandHeader) String() string {
	return fmt.Sprintf("commandHeader{Tag: %x, Size: %x, Cmd: %x}", ch.Tag, ch.Size, ch.Cmd)
}

// A responseHeader is a header for TPM responses.
type responseHeader struct {
	Tag  uint16
	Size uint32
	Res  uint32
}

/*
typedef struct {
  TPM2B_AUTH           userAuth;
  TPM2B_SENSITIVE_DATA data;
} TPMS_SENSITIVE_CREATE;

// Table 182 - TPMT_PUBLIC_PARMS Structure
typedef struct {
  TPMI_ALG_PUBLIC   type;
  TPMU_PUBLIC_PARMS parameters;
} TPMT_PUBLIC_PARMS;

// Table 183 - TPMT_PUBLIC Structure
typedef struct {
  TPMI_ALG_PUBLIC   type;
  TPMI_ALG_HASH     nameAlg;
  TPMA_OBJECT       objectAttributes;
  TPM2B_DIGEST      authPolicy;
  TPMU_PUBLIC_PARMS parameters;
  TPMU_PUBLIC_ID    unique;
} TPMT_PUBLIC;

typedef struct {
  TPMI_ALG_PUBLIC   type;
  TPMU_PUBLIC_PARMS parameters;
} TPMT_PUBLIC_PARMS;

// Table 184 - TPM2B_PUBLIC Structure
typedef struct {
  uint16_t    size;
  TPMT_PUBLIC publicArea;
} TPM2B_PUBLIC;

typedef struct {
  uint32_t TPMA_NV_PPWRITE        : 1;
  uint32_t TPMA_NV_OWNERWRITE     : 1;
  uint32_t TPMA_NV_AUTHWRITE      : 1;
  uint32_t TPMA_NV_POLICYWRITE    : 1;
  uint32_t TPMA_NV_COUNTER        : 1;
  uint32_t TPMA_NV_BITS           : 1;
  uint32_t TPMA_NV_EXTEND         : 1;
  uint32_t reserved7_9            : 3;
  uint32_t TPMA_NV_POLICY_DELETE  : 1;
  uint32_t TPMA_NV_WRITELOCKED    : 1;
  uint32_t TPMA_NV_WRITEALL       : 1;
  uint32_t TPMA_NV_WRITEDEFINE    : 1;
  uint32_t TPMA_NV_WRITE_STCLEAR  : 1;
  uint32_t TPMA_NV_GLOBALLOCK     : 1;
  uint32_t TPMA_NV_PPREAD         : 1;
  uint32_t TPMA_NV_OWNERREAD      : 1;
  uint32_t TPMA_NV_AUTHREAD       : 1;
  uint32_t TPMA_NV_POLICYREAD     : 1;
  uint32_t reserved20_24          : 5;
  uint32_t TPMA_NV_NO_DA          : 1;
  uint32_t TPMA_NV_ORDERLY        : 1;
  uint32_t TPMA_NV_CLEAR_STCLEAR  : 1;
  uint32_t TPMA_NV_READLOCKED     : 1;
  uint32_t TPMA_NV_WRITTEN        : 1;
  uint32_t TPMA_NV_PLATFORMCREATE : 1;
  uint32_t TPMA_NV_READ_STCLEAR   : 1;
} TPMA_NV;

typedef struct {
  TPMI_ALG_KEYEDHASH_SCHEME scheme;
  TPMU_SCHEME_KEYEDHASH     details;
} TPMT_KEYEDHASH_SCHEME;

// RSA Key
type RsaKey {
	TPMT_SYM_DEF_OBJECT symmetric;
TPMI_ALG_SYM_OBJECT algorithm;
  TPMU_SYM_KEY_BITS   keyBits;
  TPMU_SYM_MODE       mode;
	TPMT_RSA_SCHEME     scheme;
	TPMI_RSA_KEY_BITS   keyBits;
	uint32_t            exponent;
}

// Public key
type PublicKey {
}
*/


