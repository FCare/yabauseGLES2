/*  Copyright 2016 d356

    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef SH2JIT_H
#define SH2JIT_H

#define SH2CORE_JIT             5

struct Sh2JitContext
{
   u32 r[16];
   u32 sr;
   u32 gbr;
   u32 vbr;
   u32 mach;
   u32 macl;
   u32 pr;
   u32 pc;
   s32 cycles;

   u32 tmp0, tmp1;

   s32 HH, HL, LH, LL;

   s32 dest, src, ans;
};

int SH2JitInit();
void SH2JitDeInit(void);
void SH2JitReset();
void FASTCALL SH2JitExec(u32 cycles);
void SH2JitGetRegisters(sh2regs_struct *regs);
u32 SH2JitGetGPR(int num);
u32 SH2JitGetSR();
u32 SH2JitGetGBR();
u32 SH2JitGetVBR();
u32 SH2JitGetMACH();
u32 SH2JitGetMACL();
u32 SH2JitGetPR();
u32 SH2JitGetPC();
void SH2JitSetRegisters(const sh2regs_struct *regs);
void SH2JitSetGPR(int num, u32 value);
void SH2JitSetSR(u32 value);
void SH2JitSetGBR(u32 value);
void SH2JitSetVBR(u32 value);
void SH2JitSetMACH(u32 value);
void SH2JitSetMACL(u32 value);
void SH2JitSetPR(u32 value);
void SH2JitSetPC(u32 value);
void SH2JitSendInterrupt(u8 level, u8 vector);
int SH2JitGetInterrupts(interrupt_struct interrupts[MAX_INTERRUPTS]);
void SH2JitSetInterrupts(int num_interrupts,
                                 const interrupt_struct interrupts[MAX_INTERRUPTS]);

extern SH2Interface_struct SH2Jit;

#endif
