/*
 * 
 *  Copyright (c) 2013 Intel Corporation
 * 
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */


#include "guest_cpu_internal.h"
#include "vmm_defs.h"

// pointer to the array of pointers to the GUEST_CPU_SAVE_AREA_PREFIX
extern GUEST_CPU_SAVE_AREA** g_guest_regs_save_area;

// Define initial part of GUEST_CPU_SAVE_AREA structure
typedef struct {
    VMM_GP_REGISTERS gp; 
    VMM_XMM_REGISTERS xmm;
} PACKED GUEST_CPU_SAVE_AREA_PREFIX; 


/*
 * This functions are part of the GUEST_CPU class.  They are called by
 * assembler-lever VmExit/VmResume functions to save all registers that are not
 * saved in VMCS but may be used immediately by C-language VMM code.
 * The following registers are NOT saved here
 *   RIP            part of VMCS RSP            part of VMCS RFLAGS         part
 *   of VMCS segment regs   part of VMCS control regs   saved in C-code later
 *   debug regs     saved in C-code later FP/MMX regs    saved in C-code later
 * Assumptions: No free registers except of RSP/RFLAGS FS contains host CPU id
 * (should be calculated)
 *   can tell the cpu id is %rax register
 * Assumption - no free registers on entry, all are saved on exit
 */
// CHECK(JLM)
void gcpu_save_registers(void) 
{
    UINT64                  cpuid= 0;
    UINT64                  oldrbx = 0ULL;
    GUEST_CPU_SAVE_AREA*    save_area= NULL;

    cpuid = hw_cpu_id();
    save_area= g_guest_regs_save_area[cpuid];

    asm volatile (
        "\tmovq   %%rbx, %[oldrbx]\n"
        "\tmovq   %[save_area], %%rbx\n"
        "\tmovq   %%rax, (%%rbx)\n"
        "\tmovq   %[oldrbx], %%rax\n"
        "\tmovq   %%rax, 8(%%rbx)\n"
        "\tmovq   %%rcx, 16(%%rbx)\n"
        "\tmovq   %%rdx, 24(%%rbx)\n"
        "\tmovq   %%rdi, 32(%%rbx)\n"
        "\tmovq   %%rsi, 40(%%rbx)\n"
        "\tmovq   %%rbp, 48(%%rbx)\n"
        "\tmovq   %%r8, 64(%%rbx)\n"
        "\tmovq   %%r9, 72(%%rbx)\n"
        "\tmovq   %%r10, 80(%%rbx)\n"
        "\tmovq   %%r11, 88(%%rbx)\n"
        "\tmovq   %%r12, 96(%%rbx)\n"
        "\tmovq   %%r13, 104(%%rbx)\n"
        "\tmovq   %%r14, 112(%%rbx)\n"
        "\tmovq   %%r15, 120(%%rbx)\n"
        // JLM: how does %rsp get set?
        "\tmovaps %%xmm0, 144(%%rbx)\n"
        "\tmovaps %%xmm1, 152(%%rbx)\n"
        "\tmovaps %%xmm2, 160(%%rbx)\n"
        "\tmovaps %%xmm3, 168(%%rbx)\n"
        "\tmovaps %%xmm4, 176(%%rbx)\n"
        "\tmovaps %%xmm5, 182(%%rbx)\n"
        "\tmovq   (%%rbx), %%rax\n"
        "\tmovq %[oldrbx], %%rbx\n"
    : [oldrbx] "=m" (oldrbx)
    : [cpuid] "m" (cpuid), [save_area] "p" (save_area)
    :);

    return;
}



// Assumption - all free registers on entry, no free registers on exit
void gcpu_restore_registers(void) 
{
    UINT64                  cpuid= 0;
    GUEST_CPU_SAVE_AREA*    save_area= NULL;

    cpuid = hw_cpu_id();
    save_area= g_guest_regs_save_area[cpuid];

    // restore all XMM first
    asm volatile (
        "\tmovq   %[save_area], %%rbx\n"
        "\tmovaps 144(%%rbx), %%xmm0\n"
        "\tmovaps 152(%%rbx), %%xmm1\n"
        "\tmovaps 160(%%rbx), %%xmm2\n"
        "\tmovaps 168(%%rbx), %%xmm3\n"
        "\tmovaps 176(%%rbx), %%xmm4\n"
        "\tmovaps 182(%%rbx), %%xmm5\n"
        // rbx is restored at the end
        "\tmovq   16(%%rbx), %%rcx\n"
        "\tmovq   24(%%rbx), %%rdx\n"
        "\tmovq   32(%%rbx), %%rdi\n"
        "\tmovq   40(%%rbx),%%rsi\n"
        // rsp is not restored
        "\tmovq   64(%%rbx), %%r8\n"
        "\tmovq   72(%%rbx), %%r9\n"
        "\tmovq   80(%%rbx), %%r10\n"
        "\tmovq   88(%%rbx), %%r11\n"
        "\tmovq   96(%%rbx), %%r12\n"
        "\tmovq   104(%%rbx), %%r13\n"
        "\tmovq   112(%%rbx), %%r14\n"
        "\tmovq   120(%%rbx), %%r15\n"
        "\tmovq   (%%rbx), %%rax\n"
        "\tmovq   48(%%rbx), %%rbp\n"
        "\tmovq   8(%%rbx), %%rbx\n"
    : 
    : [save_area] "p" (save_area)
    :);

   return;
}



