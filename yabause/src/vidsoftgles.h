/*  Copyright 2006 Theo Berkau

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

#ifndef VIDSOFTGLES_H
#define VIDSOFTGLES_H

#include "vdp1.h"
#include "vdp1asyncRenderer.h"

#define VIDCORE_OGLES   3

extern VideoInterface_struct VIDSoftGLES;

void VIDSoftGLESSetBilinear(int b); 

void VIDSoftGLESSetNumPriorityThreads(int num);

extern void Vdp2GetInterlaceInfo(int * start_line, int * line_increment);

void VIDSoftGLESSetNumLayerThreads(int num);

void VIDSoftGLESSetVdp1ThreadEnable(int b);

void VIDSoftGLESWaitForVdp1Thread();

#endif
