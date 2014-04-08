/*
 * Copyright (c) 2013 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 *
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#include <types.h>
#include "vmm_defs.h"
#include "common_libc.h"


//  force compiler intrinsics to use our code
void memset (void *str, int c, int n) {
    vmm_memset (str, c, n);
    return;
}


void memcpy(void *str1, void *str2, int n) {
    vmm_memcpy(str1, str2, n);
    return; 
}


int strlen (const char *str) {
    return vmm_strlen(str);
}


void *memmove(void *dest, const void *src, int n) {
    return vmm_memmove(dest, src, n);
}


void vmm_lock_xchg_qword (UINT64 *dst, UINT64 *src) 
{
    // CHECK(JLM)
    asm volatile(
        "\tmovq %[src], %%r8\n"
        "\tmovq %[src], %%rdx\n"
        "\tmovq %[dst], %%rcx\n"
        "\tmovq %%r8, (%%rdx)\n"
        "\tlock xchg %%r8, (%%rcx)\n"
    :
    : [dst] "m" (dst), [src] "m" (src)
    :"rcx", "rdx", "r8");
}


void vmm_lock_xchg_byte (UINT8 *dst, UINT8 *src) 
{
    asm volatile(
        "\tmovq %[src], %%rdx\n"
        "\tmovq %[dst], %%rcx\n"
        "\tmovb (%%rdx), %%bl\n"
        "\tlock xchg %%bl, (%%rcx)\n" // byte exchange
    :
    : [dst] "m" (dst), [src] "m" (src)
    :"rcx", "rbx", "rdx");
}

