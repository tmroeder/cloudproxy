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

#ifndef _UVMM_CRT_H_
#define _UVMM_CRT_H_

#include "common_libc.h"


// Set of CRT-like routines to be used in VMM environment

#define EOF (-1)


// Console I/O functions
//
// The normal version of putc() and puts() perform locking of the output as a
// critical resource, in order to avoid intermingling of printed lines.
// The "nolock" version of these functions just print to the output.  They
// should be used in places, such as exception handlers, where there's a danger
// of deadlock (should they be called while the output is locked).

void    vmm_libc_init(void);
int     vmm_puts(const char *string);
int     vmm_puts_nolock(const char *string);
UINT8   vmm_putc(UINT8 Char);
UINT8   vmm_putc_nolock(UINT8 Char);
UINT8   vmm_getc(void);

//
// Test function, active only if #ifdef'ed in

void    vmm_print_test(UINT32 id);

// vmm_printf() is declared in the common_libc.h
// If uses global buffers and use locks for avoid cluttered prints on COM port

//
// vmm_printf_nolock()
//
// Like vmm_printf() but uses buffers on the stack and does not use locks
//
// Use it in the NMI handler to avoid deadlocks
//
int vmm_printf_nolock( const char *format, ... );

#endif // _UVMM_CRT_H_
