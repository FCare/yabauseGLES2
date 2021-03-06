/*  Copyright 2005-2006 Guillaume Duhamel
    Copyright 2005-2006 Theo Berkau
    Copyright 2011-2015 Shinya Miyamoto(devmiyax)

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

#ifdef HAVE_LIBGLES

#include <stdlib.h>
#include <math.h>
#include "ygles2.h"
#include "yui.h"
#include "vidshared.h"
#include "debug.h"

static int YglEsCalcTextureQ( float   *pnts,float *q);

#define PI 3.1415926535897932384626433832795f

#define ATLAS_BIAS (0.025f)

void YglEsScalef(YglEsMatrix *result, GLfloat sx, GLfloat sy, GLfloat sz)
{
    result->m[0][0] *= sx;
    result->m[0][1] *= sx;
    result->m[0][2] *= sx;
    result->m[0][3] *= sx;

    result->m[1][0] *= sy;
    result->m[1][1] *= sy;
    result->m[1][2] *= sy;
    result->m[1][3] *= sy;

    result->m[2][0] *= sz;
    result->m[2][1] *= sz;
    result->m[2][2] *= sz;
    result->m[2][3] *= sz;
}

void YglEsTranslatef(YglEsMatrix *result, GLfloat tx, GLfloat ty, GLfloat tz)
{
    result->m[0][3] += (result->m[0][0] * tx + result->m[0][1] * ty + result->m[0][2] * tz);
    result->m[1][3] += (result->m[1][0] * tx + result->m[1][1] * ty + result->m[1][2] * tz);
    result->m[2][3] += (result->m[2][0] * tx + result->m[2][1] * ty + result->m[2][2] * tz);
    result->m[3][3] += (result->m[3][0] * tx + result->m[3][1] * ty + result->m[3][2] * tz);
}

void YglEsRotatef(YglEsMatrix *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
   GLfloat sinAngle, cosAngle;
   GLfloat mag = sqrtf(x * x + y * y + z * z);

   sinAngle = sinf ( angle * PI / 180.0f );
   cosAngle = cosf ( angle * PI / 180.0f );
   if ( mag > 0.0f )
   {
      GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;
      GLfloat oneMinusCos;
      YglEsMatrix rotMat;

      x /= mag;
      y /= mag;
      z /= mag;

      xx = x * x;
      yy = y * y;
      zz = z * z;
      xy = x * y;
      yz = y * z;
      zx = z * x;
      xs = x * sinAngle;
      ys = y * sinAngle;
      zs = z * sinAngle;
      oneMinusCos = 1.0f - cosAngle;

      rotMat.m[0][0] = (oneMinusCos * xx) + cosAngle;
      rotMat.m[0][1] = (oneMinusCos * xy) - zs;
      rotMat.m[0][2] = (oneMinusCos * zx) + ys;
      rotMat.m[0][3] = 0.0F;

      rotMat.m[1][0] = (oneMinusCos * xy) + zs;
      rotMat.m[1][1] = (oneMinusCos * yy) + cosAngle;
      rotMat.m[1][2] = (oneMinusCos * yz) - xs;
      rotMat.m[1][3] = 0.0F;

      rotMat.m[2][0] = (oneMinusCos * zx) - ys;
      rotMat.m[2][1] = (oneMinusCos * yz) + xs;
      rotMat.m[2][2] = (oneMinusCos * zz) + cosAngle;
      rotMat.m[2][3] = 0.0F;

      rotMat.m[3][0] = 0.0F;
      rotMat.m[3][1] = 0.0F;
      rotMat.m[3][2] = 0.0F;
      rotMat.m[3][3] = 1.0F;

      YglEsMatrixMultiply( result, &rotMat, result );
   }
}

void YglEsFrustum(YglEsMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float       deltaX = right - left;
    float       deltaY = top - bottom;
    float       deltaZ = farZ - nearZ;
    YglEsMatrix    frust;

    if ( (nearZ <= 0.0f) || (farZ <= 0.0f) ||
         (deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f) )
         return;

    frust.m[0][0] = 2.0f * nearZ / deltaX;
    frust.m[0][1] = frust.m[0][2] = frust.m[0][3] = 0.0f;

    frust.m[1][1] = 2.0f * nearZ / deltaY;
    frust.m[1][0] = frust.m[1][2] = frust.m[1][3] = 0.0f;

    frust.m[2][0] = (right + left) / deltaX;
    frust.m[2][1] = (top + bottom) / deltaY;
    frust.m[2][2] = -(nearZ + farZ) / deltaZ;
    frust.m[2][3] = -1.0f;

    frust.m[3][2] = -2.0f * nearZ * farZ / deltaZ;
    frust.m[3][0] = frust.m[3][1] = frust.m[3][3] = 0.0f;

    YglEsMatrixMultiply(result, &frust, result);
}


void YglEsPerspective(YglEsMatrix *result, float fovy, float aspect, float nearZ, float farZ)
{
   GLfloat frustumW, frustumH;

   frustumH = tanf( fovy / 360.0f * PI ) * nearZ;
   frustumW = frustumH * aspect;

   YglEsFrustum( result, -frustumW, frustumW, -frustumH, frustumH, nearZ, farZ );
}

void YglEsOrtho(YglEsMatrix *result, float left, float right, float bottom, float top, float nearZ, float farZ)
{
    float       deltaX = right - left;
    float       deltaY = top - bottom;
    float       deltaZ = farZ - nearZ;
    YglEsMatrix    ortho;

    if ( (deltaX == 0.0f) || (deltaY == 0.0f) || (deltaZ == 0.0f) )
        return;

    YglEsLoadIdentity(&ortho);
    ortho.m[0][0] = 2.0f / deltaX;
    ortho.m[0][3] = -(right + left) / deltaX;
    ortho.m[1][1] = 2.0f / deltaY;
    ortho.m[1][3] = -(top + bottom) / deltaY;
    ortho.m[2][2] = -2.0f / deltaZ;
    ortho.m[2][3] = -(nearZ + farZ) / deltaZ;

    YglEsMatrixMultiply(result, &ortho, result);
}

void YglEsTransform(YglEsMatrix *mtx, float * inXyz, float * outXyz )
{
    outXyz[0] = inXyz[0] * mtx->m[0][0] + inXyz[0] * mtx->m[0][1]  + inXyz[0] * mtx->m[0][2] + mtx->m[0][3];
    outXyz[1] = inXyz[0] * mtx->m[1][0] + inXyz[0] * mtx->m[1][1]  + inXyz[0] * mtx->m[1][2] + mtx->m[1][3];
    outXyz[2] = inXyz[0] * mtx->m[2][0] + inXyz[0] * mtx->m[2][1]  + inXyz[0] * mtx->m[2][2] + mtx->m[2][3];
}

void YglEsMatrixMultiply(YglEsMatrix *result, YglEsMatrix *srcA, YglEsMatrix *srcB)
{
    YglEsMatrix    tmp;
    int         i;

    for (i=0; i<4; i++)
    {
        tmp.m[i][0] =   (srcA->m[i][0] * srcB->m[0][0]) +
                        (srcA->m[i][1] * srcB->m[1][0]) +
                        (srcA->m[i][2] * srcB->m[2][0]) +
                        (srcA->m[i][3] * srcB->m[3][0]) ;

        tmp.m[i][1] =   (srcA->m[i][0] * srcB->m[0][1]) +
                        (srcA->m[i][1] * srcB->m[1][1]) +
                        (srcA->m[i][2] * srcB->m[2][1]) +
                        (srcA->m[i][3] * srcB->m[3][1]) ;

        tmp.m[i][2] =   (srcA->m[i][0] * srcB->m[0][2]) +
                        (srcA->m[i][1] * srcB->m[1][2]) +
                        (srcA->m[i][2] * srcB->m[2][2]) +
                        (srcA->m[i][3] * srcB->m[3][2]) ;

        tmp.m[i][3] =   (srcA->m[i][0] * srcB->m[0][3]) +
                        (srcA->m[i][1] * srcB->m[1][3]) +
                        (srcA->m[i][2] * srcB->m[2][3]) +
                        (srcA->m[i][3] * srcB->m[3][3]) ;
    }
    memcpy(result, &tmp, sizeof(YglEsMatrix));
}


void YglEsLoadIdentity(YglEsMatrix *result)
{
    memset(result, 0x0, sizeof(YglEsMatrix));
    result->m[0][0] = 1.0f;
    result->m[1][1] = 1.0f;
    result->m[2][2] = 1.0f;
    result->m[3][3] = 1.0f;
}


YglEsTextureManager * YglEsTM;
YglEs * _YglEs;

typedef struct
{
   float s, t, r, q;
} texturecoordinate_struct;


extern int GlHeight;
extern int GlWidth;
extern int vdp1cor;
extern int vdp1cog;
extern int vdp1cob;


#define STD_Q2 (1.0f)
#define EPS (1e-10)
#define EQ(a,b) (abs((a)-(b)) < EPS)
#define IS_ZERO(a) ( (a) < EPS && (a) > -EPS)

// AXB = |A||B|sin
static INLINE float cross2d( float veca[2], float vecb[2] )
{
   return (veca[0]*vecb[1])-(vecb[0]*veca[1]);
}

/*-----------------------------------------
    b1+--+ a1
     /  / \
    /  /   \
  a2+-+-----+b2
      ans

  get intersection point for opssite edge.
--------------------------------------------*/
int FASTCALL YglEsIntersectionOppsiteEdge(float * a1, float * a2, float * b1, float * b2, float * out )
{
  float veca[2];
  float vecb[2];
  float vecc[2];
  float d1;
  float d2;

  veca[0]=a2[0]-a1[0];
  veca[1]=a2[1]-a1[1];
  vecb[0]=b1[0]-a1[0];
  vecb[1]=b1[1]-a1[1];
  vecc[0]=b2[0]-a1[0];
  vecc[1]=b2[1]-a1[1];
  d1 = cross2d(vecb,vecc);
  if( IS_ZERO(d1) ) return -1;
  d2 = cross2d(vecb,veca);

  out[0] = a1[0]+vecc[0]*d2/d1;
  out[1] = a1[1]+vecc[1]*d2/d1;

  return 0;
}





int YglEsCalcTextureQ(
   float   *pnts,
   float *q
)
{
   float p1[2],p2[2],p3[2],p4[2],o[2];
   float   q1, q3, q4, qw;
   float   dx, w;
   float   ww;

   // fast calculation for triangle
   if (( pnts[2*0+0] == pnts[2*1+0] ) && ( pnts[2*0+1] == pnts[2*1+1] )) {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;

   } else if (( pnts[2*1+0] == pnts[2*2+0] ) && ( pnts[2*1+1] == pnts[2*2+1] ))  {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   } else if (( pnts[2*2+0] == pnts[2*3+0] ) && ( pnts[2*2+1] == pnts[2*3+1] ))  {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   } else if (( pnts[2*3+0] == pnts[2*0+0] ) && ( pnts[2*3+1] == pnts[2*0+1] )) {
      q[0] = 1.0f;
      q[1] = 1.0f;
      q[2] = 1.0f;
      q[3] = 1.0f;
      return 0;
   }

   p1[0]=pnts[0];
   p1[1]=pnts[1];
   p2[0]=pnts[2];
   p2[1]=pnts[3];
   p3[0]=pnts[4];
   p3[1]=pnts[5];
   p4[0]=pnts[6];
   p4[1]=pnts[7];

   // calcurate Q1
   if( YglEsIntersectionOppsiteEdge( p3, p1, p2, p4,  o ) == 0 )
   {
      dx = o[0]-p1[0];
      if( !IS_ZERO(dx) )
      {
         w = p3[0]-p2[0];
         if( !IS_ZERO(w) )
          q1 = fabs(dx/w);
         else
          q1 = 0.0f;
      }else{
         w = p3[1] - p2[1];
         if ( !IS_ZERO(w) )
         {
            ww = ( o[1] - p1[1] );
            if ( !IS_ZERO(ww) )
               q1 = fabs(ww / w);
            else
               q1 = 0.0f;
         } else {
            q1 = 0.0f;
         }
      }
   }else{
      q1 = 1.0f;
   }

   /* q2 = 1.0f; */

   // calcurate Q3
   if( YglEsIntersectionOppsiteEdge( p1, p3, p2,p4,  o ) == 0 )
   {
      dx = o[0]-p3[0];
      if( !IS_ZERO(dx) )
      {
         w = p1[0]-p2[0];
         if( !IS_ZERO(w) )
          q3 = fabs(dx/w);
         else
          q3 = 0.0f;
      }else{
         w = p1[1] - p2[1];
         if ( !IS_ZERO(w) )
         {
            ww = ( o[1] - p3[1] );
            if ( !IS_ZERO(ww) )
               q3 = fabs(ww / w);
            else
               q3 = 0.0f;
         } else {
            q3 = 0.0f;
         }
      }
   }else{
      q3 = 1.0f;
   }


   // calcurate Q4
   if( YglEsIntersectionOppsiteEdge( p3, p1, p4, p2,  o ) == 0 )
   {
      dx = o[0]-p1[0];
      if( !IS_ZERO(dx) )
      {
         w = p3[0]-p4[0];
         if( !IS_ZERO(w) )
          qw = fabs(dx/w);
         else
          qw = 0.0f;
      }else{
         w = p3[1] - p4[1];
         if ( !IS_ZERO(w) )
         {
            ww = ( o[1] - p1[1] );
            if ( !IS_ZERO(ww) )
               qw = fabs(ww / w);
            else
               qw = 0.0f;
         } else {
            qw = 0.0f;
         }
      }
      if ( !IS_ZERO(qw) )
      {
         w   = qw / q1;
      }
      else
      {
         w   = 0.0f;
      }
      if ( IS_ZERO(w) ) {
         q4 = 1.0f;
      } else {
         q4 = 1.0f / w;
      }
   }else{
      q4 = 1.0f;
   }

   qw = q1;
   if ( qw < 1.0f )   /* q2 = 1.0f */
      qw = 1.0f;
   if ( qw < q3 )
      qw = q3;
   if ( qw < q4 )
      qw = q4;

   if ( 1.0f != qw )
   {
      qw      = 1.0f / qw;

      q[0]   = q1 * qw;
      q[1]   = 1.0f * qw;
      q[2]   = q3 * qw;
      q[3]   = q4 * qw;
   }
   else
   {
      q[0]   = q1;
      q[1]   = 1.0f;
      q[2]   = q3;
      q[3]   = q4;
   }
   return 0;
}



//////////////////////////////////////////////////////////////////////////////

void YglEsTMInit(unsigned int w, unsigned int h) {
   YglEsTM = (YglEsTextureManager *) malloc(sizeof(YglEsTextureManager));
   YglEsTM->texture = (unsigned int *) malloc(sizeof(unsigned int) * w * h);
   memset(YglEsTM->texture,0,sizeof(unsigned int) * w * h);
   YglEsTM->width = w;
   YglEsTM->height = h;

   YglEsTMReset();
}

//////////////////////////////////////////////////////////////////////////////

void YglEsTMDeInit(void) {
   //free(YglEsTM->texture);
    if( YglEsTM->texture != NULL ) {
        glUnmapBuffer (GL_PIXEL_UNPACK_BUFFER);
        YglEsTM->texture = NULL;
    }

   free(YglEsTM);
}

//////////////////////////////////////////////////////////////////////////////

void YglEsTMReset(void) {
   YglEsTM->currentX = 0;
   YglEsTM->currentY = 0;
   YglEsTM->yMax = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglEsTMAllocate(YglEsTexture * output, unsigned int w, unsigned int h, unsigned int * x, unsigned int * y) {
   if ((YglEsTM->height - YglEsTM->currentY) < h) {
      fprintf(stderr, "can't allocate texture: %dx%d\n", w, h);
      *x = *y = 0;
      output->w = 0;
      output->textdata = YglEsTM->texture;
      return;
   }

   if ((YglEsTM->width - YglEsTM->currentX) >= w) {
      *x = YglEsTM->currentX;
      *y = YglEsTM->currentY;
      output->w = YglEsTM->width - w;
      output->textdata = YglEsTM->texture + YglEsTM->currentY * YglEsTM->width + YglEsTM->currentX;
      YglEsTM->currentX += w;
      //YglEsTM->currentX += 0x0F;
      //YglEsTM->currentX &= ~(0x0F);

      if ((YglEsTM->currentY + h) > YglEsTM->yMax)
      {
         YglEsTM->yMax = YglEsTM->currentY + h;
         //YglEsTM->yMax += 0x0F;
         //YglEsTM->yMax &= ~(0x0F);
      }
   }
   else {
      YglEsTM->currentX = 0;
      YglEsTM->currentY = YglEsTM->yMax;
      YglEsTMAllocate(output, w, h, x, y);
   }
}



void VIDOGLESVdp1ReadFrameBuffer(u32 type, u32 addr, void * out) {
  if (_YglEs->smallfbo == 0) {

    glGenFramebuffers(1, &_YglEs->smallfbo);
    YGLLOG("glGenFramebuffers %d\n", _YglEs->smallfbo );
    glGenTextures(1, &_YglEs->smallfbotex);
    YGLLOG("glGenTextures %d\n",_YglEs->smallfbotex );
    glBindTexture(GL_TEXTURE_2D, _YglEs->smallfbotex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _YglEs->rwidth, _YglEs->rheight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    YGLLOG("glTexImage2D %d\n",_YglEs->smallfbotex );

    glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->smallfbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->smallfbotex, 0);

    glGenBuffers(1, &_YglEs->vdp1pixelBufferID);
     YGLLOG("glGenBuffers %d\n",_YglEs->vdp1pixelBufferID);
     if( _YglEs->vdp1pixelBufferID == 0 ){
        YGLLOG("Fail to glGenBuffers %X",glGetError());
     }
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _YglEs->vdp1pixelBufferID);
    glBufferData(GL_PIXEL_PACK_BUFFER, _YglEs->rwidth*_YglEs->rheight * 4, NULL, GL_DYNAMIC_READ);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  if (_YglEs->pFrameBuffer == NULL){
#if 0
    glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->vdp1fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[((_YglEs->drawframe ^ 0x01) & 0x01)], 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, _YglEs->vdp1fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, _YglEs->smallfbo);
    glBlitFramebuffer(0, 0, GlWidth, GlHeight, 0, 0, _YglEs->rwidth, _YglEs->rheight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#else
	YglEsBlitFramebuffer(_YglEs->vdp1FrameBuff[_YglEs->readframe], _YglEs->smallfbo, (float)_YglEs->rwidth / (float)GlWidth, (float)_YglEs->rheight / (float)GlHeight);
#endif
    glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->smallfbo);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, _YglEs->vdp1pixelBufferID);
    YGLLOG("glReadPixels %d\n",_YglEs->vdp1pixelBufferID);
    glReadPixels(0, 0, _YglEs->rwidth, _YglEs->rheight, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    YGLLOG("VIDOGLESVdp1ReadFrameBuffer %d\n", _YglEs->drawframe);
    _YglEs->pFrameBuffer = (unsigned int *)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, _YglEs->rwidth *  _YglEs->rheight * 4, GL_MAP_READ_BIT);
    glBindFramebuffer(GL_FRAMEBUFFER,0);

    if (_YglEs->pFrameBuffer==NULL)
    {
      switch (type)
      {
      case 1:
        *(u16*)out = 0x0000;
        break;
      case 2:
        *(u32*)out = 0x00000000;
        break;
      }
      return;
    }

  }

  {
  const int Line = (addr >> 10); // *((float)(GlHeight) / (float)_YglEs->rheight);
  const int Pix = ((addr & 0x3FF) >> 1); // *((float)(GlWidth) / (float)_YglEs->rwidth);
  const int index = (_YglEs->rheight - 1 - Line)*(_YglEs->rwidth* 4) + Pix * 4;

  switch (type)
  {
  case 1:
    {
      u8 r = *((u8*)(_YglEs->pFrameBuffer) + index);
      u8 g = *((u8*)(_YglEs->pFrameBuffer) + index + 1);
      u8 b = *((u8*)(_YglEs->pFrameBuffer) + index + 2);
      //*(u16*)out = ((val & 0x1f) << 10) | ((val >> 1) & 0x3e0) | ((val >> 11) & 0x1F) | 0x8000;
      *(u16*)out = ((r >> 3) & 0x1f) | (((g >> 3) & 0x1f) << 5) | (((b >> 3) & 0x1F)<<10) | 0x8000;
    }
    break;
  case 2:
    {

      u32 r = *((u8*)(_YglEs->pFrameBuffer) + index);
      u32 g = *((u8*)(_YglEs->pFrameBuffer) + index + 1);
      u32 b = *((u8*)(_YglEs->pFrameBuffer) + index + 2);
      u32 r2 = *((u8*)(_YglEs->pFrameBuffer) + index+4);
      u32 g2 = *((u8*)(_YglEs->pFrameBuffer) + index + 5);
      u32 b2 = *((u8*)(_YglEs->pFrameBuffer) + index + 6);

      if (r != 0)
      {
        int a=0;
      }

      /*  BBBBBGGGGGRRRRR */
      //*(u16*)out = ((val & 0x1f) << 10) | ((val >> 1) & 0x3e0) | ((val >> 11) & 0x1F) | 0x8000;
      *(u32*)out = (((r2 >> 3) & 0x1f) | (((g2 >> 3) & 0x1f) << 5) | (((b2 >> 3) & 0x1F)<<10) | 0x8000)  |
                  ((((r  >> 3) & 0x1f) | (((g  >> 3) & 0x1f) << 5) | (((b  >> 3) & 0x1F)<<10) | 0x8000) << 16) ;

    }
    break;
  }
  }

}


//////////////////////////////////////////////////////////////////////////////

int YglEsGLInit(int width, int height) {
   int status;
   GLuint error;

   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

   YglEsLoadIdentity(&_YglEs->mtxModelView);
   YglEsOrtho(&_YglEs->mtxModelView,0.0f, 320.0f, 224.0f, 0.0f, 10.0f, 0.0f);

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   glDisable(GL_DEPTH_TEST);
   glDepthFunc(GL_GEQUAL);
   glClearDepthf(0.0f);
   
   glCullFace(GL_FRONT_AND_BACK);
   glDisable(GL_CULL_FACE);
   glDisable(GL_DITHER);

   glGetError();

   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   YGLLOG("YglEsGLInit(%d,%d)\n",GlWidth,GlHeight );

   if( _YglEs->texture == 0 )
      glGenTextures(1, &_YglEs->texture);

  glGenBuffers(1, &_YglEs->pixelBufferID);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->pixelBufferID);
  glBufferData(GL_PIXEL_UNPACK_BUFFER, width * height * 4, NULL, GL_STREAM_DRAW);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

   glBindTexture(GL_TEXTURE_2D, _YglEs->texture);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
   if( (error = glGetError()) != GL_NO_ERROR )
   {
      YGLLOG("Fail to init YglEsTM->texture %04X", error);
      return -1;
   }
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glBindTexture(GL_TEXTURE_2D, _YglEs->texture);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->pixelBufferID);

   YglEsTM->texture = (unsigned int *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, width * height * 4, GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT);
   if( (error = glGetError()) != GL_NO_ERROR )
   {
      YGLLOG("Fail to init YglEsTM->texture %04X", error);
      return -1;
   }
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

   if( _YglEs->vdp1FrameBuff != 0 ) glDeleteTextures(2,_YglEs->vdp1FrameBuff);
   glGenTextures(2,_YglEs->vdp1FrameBuff);
   glBindTexture(GL_TEXTURE_2D,_YglEs->vdp1FrameBuff[0]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlWidth, GlHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,NULL);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glBindTexture(GL_TEXTURE_2D,_YglEs->vdp1FrameBuff[1]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlWidth, GlHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,NULL);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);


   _YglEs->pFrameBuffer = NULL;

   if( strstr((const char*)glGetString(GL_EXTENSIONS),"packed_depth_stencil") != NULL )
   {
      if( _YglEs->rboid_depth != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_depth);
      glGenRenderbuffers(1, &_YglEs->rboid_depth);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_depth);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_DEPTH24_STENCIL8, GlWidth, GlHeight);
      _YglEs->rboid_stencil = _YglEs->rboid_depth;

   }else{
      if( _YglEs->rboid_depth != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_depth);
      glGenRenderbuffers(1, &_YglEs->rboid_depth);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_depth);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_DEPTH_COMPONENT16, GlWidth, GlHeight);

      if( _YglEs->rboid_stencil != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_stencil);
      glGenRenderbuffers(1, &_YglEs->rboid_stencil);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_stencil);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_STENCIL_INDEX8, GlWidth, GlHeight);
   }


   glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->vdp1fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[0], 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_depth);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_stencil);
   status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if( status != GL_FRAMEBUFFER_COMPLETE )
   {
      YGLLOG("YglEsGLInit:Framebuffer status = %08X\n", status );
   }
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->vdp1fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[1], 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_depth);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_stencil);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   glBindFramebuffer(GL_FRAMEBUFFER, 0 );
   glBindTexture(GL_TEXTURE_2D,_YglEs->texture);



   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YglEsScreenInit(int r, int g, int b, int d) {

   return 0;
/*
   YuiSetVideoAttribute(RED_SIZE, r);
   YuiSetVideoAttribute(GREEN_SIZE, g);
   YuiSetVideoAttribute(BLUE_SIZE, b);
   YuiSetVideoAttribute(DEPTH_SIZE, d);
   return (YuiSetVideoMode(320, 224, 32, 0) == 0);
*/
}

void YuiSetVideoAttribute(int type, int val){return;}


//////////////////////////////////////////////////////////////////////////////


int YglEsInit(int width, int height, unsigned int depth) {
   unsigned int i,j;
   GLuint status;
   void * dataPointer=NULL;

   YGLLOG("YglEsInit(%d,%d,%d);",width,height,depth );

   YglEsTMInit(width, height);

   if ((_YglEs = (YglEs *) malloc(sizeof(YglEs))) == NULL)
      return -1;

   memset(_YglEs,0,sizeof(YglEs));

   _YglEs->depth = depth;
   _YglEs->rwidth = 320;
   _YglEs->rheight = 240;

   if ((_YglEs->levels = (YglEsLevel *) malloc(sizeof(YglEsLevel) * (depth+1))) == NULL)
      return -1;

   memset(_YglEs->levels,0,sizeof(YglEsLevel) * (depth+1) );
   for(i = 0;i < (depth+1) ;i++) {
     _YglEs->levels[i].prgcurrent = 0;
     _YglEs->levels[i].uclipcurrent = 0;
     _YglEs->levels[i].prgcount = 1;
     _YglEs->levels[i].prg = (YglEsProgram*)malloc(sizeof(YglEsProgram)*_YglEs->levels[i].prgcount);
     memset(  _YglEs->levels[i].prg,0,sizeof(YglEsProgram)*_YglEs->levels[i].prgcount);
     if( _YglEs->levels[i].prg == NULL ) return -1;

     for(j = 0;j < _YglEs->levels[i].prgcount; j++) {
        _YglEs->levels[i].prg[j].prg=0;
        _YglEs->levels[i].prg[j].currentQuad = 0;
        _YglEs->levels[i].prg[j].maxQuad = 12 * 2000;
        if ((_YglEs->levels[i].prg[j].quads = (float *) malloc(_YglEs->levels[i].prg[j].maxQuad * sizeof(float))) == NULL)
            return -1;

        if ((_YglEs->levels[i].prg[j].textcoords = (float *) malloc(_YglEs->levels[i].prg[j].maxQuad * sizeof(float) * 2)) == NULL)
            return -1;

        if ((_YglEs->levels[i].prg[j].vertexAttribute = (float *) malloc(_YglEs->levels[i].prg[j].maxQuad * sizeof(float)*2)) == NULL)
            return -1;
      }
   }

#if defined(_USEGLEW_)
   glewInit();
#endif
   YglEsGLInit(width, height);

   if( YglEsProgramInit() != 0 )
   {
      YuiErrorMsg("Fail to YglEsProgramInit\n");
      return -1;
   }

   _YglEs->drawframe = 0;
   _YglEs->readframe = 1;

   glGenTextures(2,_YglEs->vdp1FrameBuff);
   glBindTexture(GL_TEXTURE_2D,_YglEs->vdp1FrameBuff[0]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlWidth, GlHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,NULL);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

   glBindTexture(GL_TEXTURE_2D,_YglEs->vdp1FrameBuff[1]);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, GlWidth, GlHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,NULL);
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    if( strstr((const char*)glGetString(GL_EXTENSIONS),"packed_depth_stencil") != NULL )
   {
      if( _YglEs->rboid_depth != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_depth);
      glGenRenderbuffers(1, &_YglEs->rboid_depth);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_depth);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_DEPTH24_STENCIL8, GlWidth, GlHeight);
      _YglEs->rboid_stencil = _YglEs->rboid_depth;

   }else{
      if( _YglEs->rboid_depth != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_depth);
      glGenRenderbuffers(1, &_YglEs->rboid_depth);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_depth);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_DEPTH_COMPONENT16, GlWidth, GlHeight);

      if( _YglEs->rboid_stencil != 0 ) glDeleteRenderbuffers(1,&_YglEs->rboid_stencil);
      glGenRenderbuffers(1, &_YglEs->rboid_stencil);
      glBindRenderbuffer(GL_RENDERBUFFER,_YglEs->rboid_stencil);
      glRenderbufferStorage(GL_RENDERBUFFER,  GL_STENCIL_INDEX8, GlWidth, GlHeight);
   }

   glGenFramebuffers(1,&_YglEs->vdp1fbo);
   glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->vdp1fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[0], 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_depth);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_stencil);
   status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if( status != GL_FRAMEBUFFER_COMPLETE )
   {
      YGLLOG("YglEsInit: Framebuffer status = %08X\n", status );
      return -1;
   }

   _YglEs->smallfbo = 0;
   _YglEs->smallfbotex = 0;

   glBindFramebuffer(GL_FRAMEBUFFER, 0 );

   _YglEs->st = 0;
   _YglEs->msglength = 0;

   return 0;
}


//////////////////////////////////////////////////////////////////////////////

void YglEsDeInit(void) {
   unsigned int i,j;

   YglEsTMDeInit();

   if (_YglEs)
   {
      if (_YglEs->levels)
      {
         for (i = 0; i < (_YglEs->depth+1); i++)
         {
         for (j = 0; j < _YglEs->levels[i].prgcount; j++)
         {
            if (_YglEs->levels[i].prg[j].quads)
            free(_YglEs->levels[i].prg[j].quads);
            if (_YglEs->levels[i].prg[j].textcoords)
            free(_YglEs->levels[i].prg[j].textcoords);
            if (_YglEs->levels[i].prg[j].vertexAttribute)
            free(_YglEs->levels[i].prg[j].vertexAttribute);
         }
         free(_YglEs->levels[i].prg);
         }
         free(_YglEs->levels);
      }

      free(_YglEs);
   }

}

void YglEsStartWindow( vdp2draw_struct * info, int win0, int logwin0, int win1, int logwin1, int mode )
{
   YglEsLevel   *level;
   YglEsProgram *program;
   level = &_YglEs->levels[info->priority];
   YglEsProgramChange(level,PG_VDP2_STARTWINDOW);
   program = &level->prg[level->prgcurrent];
   program->bwin0 = win0;
   program->logwin0 = logwin0;
   program->bwin1 = win1;
   program->logwin1 = logwin1;
   program->winmode = mode;

}

void YglEsEndWindow( vdp2draw_struct * info )
{
   YglEsLevel   *level;
   level = &_YglEs->levels[info->priority];
   YglEsProgramChange(level,PG_VDP2_ENDWINDOW);
}


//////////////////////////////////////////////////////////////////////////////

YglEsProgram * YglEsGetProgram( YglEsSprite * input, int prg )
{
   YglEsLevel   *level;
   YglEsProgram *program;
   float checkval;

   if (input->priority > 8) {
      VDP1LOG("sprite with priority %d\n", input->priority);
      return NULL;
   }

   level = &_YglEs->levels[input->priority];

   level->blendmode |= (input->blendmode&0x03);

   if( input->uclipmode != level->uclipcurrent )
   {
      if( input->uclipmode == 0x02 || input->uclipmode == 0x03 )
      {
         YglEsProgramChange(level,PG_VFP1_STARTUSERCLIP);
         program = &level->prg[level->prgcurrent];
         program->uClipMode = input->uclipmode;
         if( level->ux1 != Vdp1Regs->userclipX1 || level->uy1 != Vdp1Regs->userclipY1 ||
            level->ux2 != Vdp1Regs->userclipX2 || level->uy2 != Vdp1Regs->userclipY2 )
         {
            program->ux1=Vdp1Regs->userclipX1;
            program->uy1=Vdp1Regs->userclipY1;
            program->ux2=Vdp1Regs->userclipX2;
            program->uy2=Vdp1Regs->userclipY2;
            level->ux1=Vdp1Regs->userclipX1;
            level->uy1=Vdp1Regs->userclipY1;
            level->ux2=Vdp1Regs->userclipX2;
            level->uy2=Vdp1Regs->userclipY2;
         }else{
            program->ux1=-1;
            program->uy1=-1;
            program->ux2=-1;
            program->uy2=-1;
         }
      }else{
         YglEsProgramChange(level,PG_VFP1_ENDUSERCLIP);
         program = &level->prg[level->prgcurrent];
         program->uClipMode = input->uclipmode;
      }
      level->uclipcurrent = input->uclipmode;

   }

   checkval = (float)(input->cor) / 255.0f;
   if (checkval != level->prg[level->prgcurrent].color_offset_val[0])
   {
	   YglEsProgramChange(level, prg);
   } else if( level->prg[level->prgcurrent].prgid != prg ) {
      YglEsProgramChange(level,prg);
   }
// for polygon debug
//   else if (prg == PG_VFP1_GOURAUDSAHDING ){
//	   YglEsProgramChange(level, prg);
//   }
   program = &level->prg[level->prgcurrent];

   if (program->currentQuad == program->maxQuad) {
      program->maxQuad += 12*128;
	  program->quads = (float *)realloc(program->quads, program->maxQuad * sizeof(float));
      program->textcoords = (float *) realloc(program->textcoords, program->maxQuad * sizeof(float) * 2);
      program->vertexAttribute = (float *) realloc(program->vertexAttribute, program->maxQuad * sizeof(float)*2);
      YglEsCacheReset();
   }

   return program;
}

void YglEsQuadOffset(YglEsSprite * input, YglEsTexture * output, YglEsCache * c, int cx, int cy, float sx, float sy ) {
	unsigned int x, y;
	YglEsProgram *program;
	texturecoordinate_struct *tmp;
	int prg = PG_NORMAL;
	float * pos;
	//float * vtxa;

	int vHeight;

	if ((input->blendmode & 0x03) == 2)
	{
		prg = PG_VDP2_ADDBLEND;
	}

  if (input->linescreen){
    prg = PG_LINECOLOR_INSERT;
  }

  
	program = YglEsGetProgram(input, prg);
	if (program == NULL) return;


	program->color_offset_val[0] = (float)(input->cor) / 255.0f;
	program->color_offset_val[1] = (float)(input->cog) / 255.0f;
	program->color_offset_val[2] = (float)(input->cob) / 255.0f;
	program->color_offset_val[3] = 0;
	//info->cor

	vHeight = input->vertices[5] - input->vertices[1];

	pos = program->quads + program->currentQuad;
	pos[0] = (input->vertices[0] - cx) * sx;
	pos[1] = input->vertices[1] * sy;
	pos[2] = (input->vertices[2] - cx) * sx;
	pos[3] = input->vertices[3] * sy;
	pos[4] = (input->vertices[4] - cx) * sx;
	pos[5] = input->vertices[5] * sy;
	pos[6] = (input->vertices[0] - cx) * sx;
	pos[7] = (input->vertices[1]) * sy;
	pos[8] = (input->vertices[4] - cx)*sx;
	pos[9] = input->vertices[5] * sy;
	pos[10] = (input->vertices[6] - cx) * sx;
	pos[11] = input->vertices[7] * sy;

	// vtxa = (program->vertexAttribute + (program->currentQuad * 2));
	// memset(vtxa,0,sizeof(float)*24);

	tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));

	program->currentQuad += 12;
	YglEsTMAllocate(output, input->w, input->h, &x, &y);
	if (output->textdata == NULL){
		abort();
	}


	tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

	/*
	0 +---+ 1
	  |   |
	  +---+ 2
	3 +---+
	  |   |
	5 +---+ 4
	*/

	if (input->flip & 0x1) {
		tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w) - ATLAS_BIAS;
		tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ATLAS_BIAS;
	}
	else {
		tmp[0].s = tmp[3].s = tmp[5].s = (float)(x)+ATLAS_BIAS;
		tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w) - ATLAS_BIAS;
	}
	if (input->flip & 0x2) {
		tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h - cy) - ATLAS_BIAS;
		tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h - (cy+vHeight) ) + ATLAS_BIAS;
	}
	else {
		tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + cy ) + ATLAS_BIAS;
		tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + (cy + vHeight)) - ATLAS_BIAS;
	}

	c->x = x; 
	c->y = y; 

	tmp[0].q = 1.0f;
	tmp[1].q = 1.0f;
	tmp[2].q = 1.0f;
	tmp[3].q = 1.0f;
	tmp[4].q = 1.0f;
	tmp[5].q = 1.0f;
}



float * YglEsQuad(YglEsSprite * input, YglEsTexture * output, YglEsCache * c) {
   unsigned int x, y;
   YglEsProgram *program;
   texturecoordinate_struct *tmp;
   float q[4];
   int prg = PG_NORMAL;
   float * pos;
   //float * vtxa;


   if( (input->blendmode&0x03) == 2 )
   {
      prg = PG_VDP2_ADDBLEND;
   }else if( input->blendmode == 0x80 )
   {
      prg = PG_VFP1_HALFTRANS;
   }else if( input->priority == 8 )
   {
      prg = PG_VDP1_NORMAL;
   }

   if (input->linescreen){
     prg = PG_LINECOLOR_INSERT;
   }

   program = YglEsGetProgram(input,prg);
   if( program == NULL ) return NULL;


   program->color_offset_val[0] = (float)(input->cor)/255.0f;
   program->color_offset_val[1] = (float)(input->cog)/255.0f;
   program->color_offset_val[2] = (float)(input->cob)/255.0f;
   program->color_offset_val[3] = 0;
   //info->cor

   pos = program->quads + program->currentQuad;
   pos[0] = input->vertices[0];
   pos[1] = input->vertices[1];
   pos[2] = input->vertices[2];
   pos[3] = input->vertices[3];
   pos[4] = input->vertices[4];
   pos[5] = input->vertices[5];
   pos[6] = input->vertices[0];
   pos[7] = input->vertices[1];
   pos[8] = input->vertices[4];
   pos[9] = input->vertices[5];
   pos[10] = input->vertices[6];
   pos[11] = input->vertices[7];

  // vtxa = (program->vertexAttribute + (program->currentQuad * 2));
  // memset(vtxa,0,sizeof(float)*24);

   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));

   program->currentQuad += 12;
   YglEsTMAllocate(output, input->w, input->h, &x, &y);
   if (output->textdata == NULL){
	   abort();
   }


   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

   /*
     0 +---+ 1
       |   |
       +---+ 2
     3 +---+
       |   |
     5 +---+ 4
   */

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w) - ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ ATLAS_BIAS;
   } else {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x) + ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w)-ATLAS_BIAS;
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h)-ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y)+ATLAS_BIAS;
   } else {
	   tmp[0].t = tmp[1].t = tmp[3].t = (float)(y) + ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h)-ATLAS_BIAS;
   }

   if( c != NULL )
   {
      switch(input->flip) {
        case 0:
			c->x = *(program->textcoords + ((program->currentQuad - 12) * 2));   // upper left coordinates(0)
			c->y = *(program->textcoords + ((program->currentQuad - 12) * 2) + 1); // upper left coordinates(0)
          break;
        case 1:
			c->x = *(program->textcoords + ((program->currentQuad - 10) * 2));   // upper left coordinates(0)
			c->y = *(program->textcoords + ((program->currentQuad - 10) * 2) + 1); // upper left coordinates(0)
          break;
       case 2:
		   c->x = *(program->textcoords + ((program->currentQuad - 2) * 2));   // upper left coordinates(0)
		   c->y = *(program->textcoords + ((program->currentQuad - 2) * 2) + 1); // upper left coordinates(0)
          break;
       case 3:
		   c->x = *(program->textcoords + ((program->currentQuad - 4) * 2));   // upper left coordinates(0)
		   c->y = *(program->textcoords + ((program->currentQuad - 4) * 2) + 1); // upper left coordinates(0)
          break;
      }
   }


   if( input->dst == 1 )
   {
      YglEsCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[0];
      tmp[3].t *= q[0];
      tmp[4].s *= q[2];
      tmp[4].t *= q[2];
      tmp[5].s *= q[3];
      tmp[5].t *= q[3];
      tmp[0].q = q[0];
      tmp[1].q = q[1];
      tmp[2].q = q[2];
      tmp[3].q = q[0];
      tmp[4].q = q[2];
      tmp[5].q = q[3];
   }else{
      tmp[0].q = 1.0f;
      tmp[1].q = 1.0f;
      tmp[2].q = 1.0f;
      tmp[3].q = 1.0f;
      tmp[4].q = 1.0f;
      tmp[5].q = 1.0f;
   }


   return 0;
}

//////////////////////////////////////////////////////////////////////////////

int YglEsQuadGrowShading(YglEsSprite * input, YglEsTexture * output, float * colors,YglEsCache * c) {
   unsigned int x, y;
   YglEsProgram *program;
   texturecoordinate_struct *tmp;
   float * vtxa;
   float q[4];
   int prg = PG_VFP1_GOURAUDSAHDING;
   float * pos;


   if( (input->blendmode&0x03) == 2 )
   {
      prg = PG_VDP2_ADDBLEND;
   }else if( input->blendmode == 0x80 )
   {
      prg = PG_VFP1_GOURAUDSAHDING_HALFTRANS;
   }

   if (input->linescreen){
     prg = PG_LINECOLOR_INSERT;
   }


   program = YglEsGetProgram(input,prg);
   if( program == NULL ) return -1;
   //YGLLOG( "program->quads = %X,%X,%d/%d\n",program->quads,program->vertexBuffer,program->currentQuad,program->maxQuad );
   if( program->quads == NULL ) {
       int a=0;
   }

   program->color_offset_val[0] = (float)(input->cor)/255.0f;
   program->color_offset_val[1] = (float)(input->cog)/255.0f;
   program->color_offset_val[2] = (float)(input->cob)/255.0f;
   program->color_offset_val[3] = 0;

   // Vertex
   pos = program->quads + program->currentQuad;

/*
   float dx = input->vertices[4] - input->vertices[0];
   float dy = input->vertices[5] - input->vertices[1];
 
   if (dx < 0.0 && dy < 0.0 ){
	   pos[0] = input->vertices[2*1 + 0]; // 1
	   pos[1] = input->vertices[2*1 + 1];
	   pos[2] = input->vertices[2 * 0 + 0]; // 0
	   pos[3] = input->vertices[2 * 0 + 1];
	   pos[4] = input->vertices[2 * 2 + 0]; // 2
	   pos[5] = input->vertices[2 * 2 + 1];
	   pos[6] = input->vertices[2 * 1 + 0]; // 1
	   pos[7] = input->vertices[2 * 1 + 1];
	   pos[8] = input->vertices[2 * 2 + 0]; // 2
	   pos[9] = input->vertices[2 * 2 + 1];
	   pos[10] = input->vertices[2 * 3 + 0]; //3
	   pos[11] = input->vertices[2 * 3 + 1];
   }
   else
*/  
   {
	   pos[0] = input->vertices[0];
	   pos[1] = input->vertices[1];
	   pos[2] = input->vertices[2];
	   pos[3] = input->vertices[3];
	   pos[4] = input->vertices[4];
	   pos[5] = input->vertices[5];
	   pos[6] = input->vertices[0];
	   pos[7] = input->vertices[1];
	   pos[8] = input->vertices[4];
	   pos[9] = input->vertices[5];
	   pos[10] = input->vertices[6];
	   pos[11] = input->vertices[7];
   }


   // Color
   vtxa = (program->vertexAttribute + (program->currentQuad * 2));
   if( colors == NULL ) {
      memset(vtxa,0,sizeof(float)*24);
   } else {
	   vtxa[0] = colors[0];
	   vtxa[1] = colors[1];
	   vtxa[2] = colors[2];
	   vtxa[3] = colors[3];

	   vtxa[4] = colors[4];
	   vtxa[5] = colors[5];
	   vtxa[6] = colors[6];
	   vtxa[7] = colors[7];

	   vtxa[8] = colors[8];
	   vtxa[9] = colors[9];
	   vtxa[10] = colors[10];
	   vtxa[11] = colors[11];

	   vtxa[12] = colors[0];
	   vtxa[13] = colors[1];
	   vtxa[14] = colors[2];
	   vtxa[15] = colors[3];

	   vtxa[16] = colors[8];
	   vtxa[17] = colors[9];
	   vtxa[18] = colors[10];
	   vtxa[19] = colors[11];

	   vtxa[20] = colors[12];
	   vtxa[21] = colors[13];
	   vtxa[22] = colors[14];
	   vtxa[23] = colors[15];
   }

   // texture
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));

   program->currentQuad += 12;

   YglEsTMAllocate(output, input->w, input->h, &x, &y);

   tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w)-ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ATLAS_BIAS;
   } else {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x)+ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w)-ATLAS_BIAS;
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h)-ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y)+ATLAS_BIAS;
   } else {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y)+ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h)-ATLAS_BIAS;
   }

   if( c != NULL )
   {
      switch(input->flip) {
        case 0:
          c->x = *(program->textcoords + ((program->currentQuad - 12) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 12) * 2)+1); // upper left coordinates(0)
          break;
        case 1:
          c->x = *(program->textcoords + ((program->currentQuad - 10) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 10) * 2)+1); // upper left coordinates(0)
          break;
       case 2:
          c->x = *(program->textcoords + ((program->currentQuad - 2) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 2) * 2)+1); // upper left coordinates(0)
          break;
       case 3:
          c->x = *(program->textcoords + ((program->currentQuad - 4) * 2));   // upper left coordinates(0)
          c->y = *(program->textcoords + ((program->currentQuad - 4) * 2)+1); // upper left coordinates(0)
          break;
      }
   }




   if( input->dst == 1 )
   {
      YglEsCalcTextureQ(input->vertices,q);

      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[0];
      tmp[3].t *= q[0];
      tmp[4].s *= q[2];
      tmp[4].t *= q[2];
      tmp[5].s *= q[3];
      tmp[5].t *= q[3];

      tmp[0].q = q[0];
      tmp[1].q = q[1];
      tmp[2].q = q[2];
      tmp[3].q = q[0];
      tmp[4].q = q[2];
      tmp[5].q = q[3];
   }else{
      tmp[0].q = 1.0f;
      tmp[1].q = 1.0f;
      tmp[2].q = 1.0f;
      tmp[3].q = 1.0f;
      tmp[4].q = 1.0f;
      tmp[5].q = 1.0f;
   }

   return 0;
}

//////////////////////////////////////////////////////////////////////////////
void YglEsCachedQuadOffset(YglEsSprite * input, YglEsCache * cache, int cx, int cy, float sx, float sy ) {
	YglEsProgram * program;
	unsigned int x, y;
	texturecoordinate_struct *tmp;
	float * pos;
	float * vtxa;
	int vHeight;

	int prg = PG_NORMAL;

	if ((input->blendmode & 0x03) == 2)
	{
		prg = PG_VDP2_ADDBLEND;
	}

  if (input->linescreen){
    prg = PG_LINECOLOR_INSERT;
  }

	program = YglEsGetProgram(input, prg);
	if (program == NULL) return;

	program->color_offset_val[0] = (float)(input->cor) / 255.0f;
	program->color_offset_val[1] = (float)(input->cog) / 255.0f;
	program->color_offset_val[2] = (float)(input->cob) / 255.0f;
	program->color_offset_val[3] = 0;

	x = cache->x;
	y = cache->y;

	// Vertex
	vHeight = input->vertices[5] - input->vertices[1];
	pos = program->quads + program->currentQuad;
	pos[0] = (input->vertices[0] - cx) * sx;
	pos[1] = input->vertices[1] * sy;
	pos[2] = (input->vertices[2] - cx) * sx;
	pos[3] = input->vertices[3] * sy;
	pos[4] = (input->vertices[4] - cx) * sx;
	pos[5] = input->vertices[5] * sy;
	pos[6] = (input->vertices[0] - cx) * sx;
	pos[7] = (input->vertices[1]) * sy;
	pos[8] = (input->vertices[4] - cx)*sx;
	pos[9] = input->vertices[5] * sy;
	pos[10] = (input->vertices[6] - cx) * sx;
	pos[11] = input->vertices[7] * sy;

	// Color
	tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
	vtxa = (program->vertexAttribute + (program->currentQuad * 2));
	memset(vtxa, 0, sizeof(float) * 24);

	program->currentQuad += 12;

	tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

	if (input->flip & 0x1) {
		tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w) - ATLAS_BIAS;
		tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ATLAS_BIAS;
	}
	else {
		tmp[0].s = tmp[3].s = tmp[5].s = (float)(x)+ATLAS_BIAS;
		tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w) - ATLAS_BIAS;
	}
	if (input->flip & 0x2) {
		tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h - cy) - ATLAS_BIAS;
		tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h - (cy + vHeight)) + ATLAS_BIAS;
	}
	else {
		tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + cy) + ATLAS_BIAS;
		tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + (cy + vHeight)) - ATLAS_BIAS;
	}

	tmp[0].q = 1.0f;
	tmp[1].q = 1.0f;
	tmp[2].q = 1.0f;
	tmp[3].q = 1.0f;
	tmp[4].q = 1.0f;
	tmp[5].q = 1.0f;

}

void YglEsCachedQuad(YglEsSprite * input, YglEsCache * cache) {
   YglEsProgram * program;
   unsigned int x,y;
   texturecoordinate_struct *tmp;
   float q[4];
   float * pos;
   float * vtxa;

   int prg = PG_NORMAL;

   if( (input->blendmode&0x03) == 2 )
   {
      prg = PG_VDP2_ADDBLEND;
   }else if( input->blendmode == 0x80 )
   {
      prg = PG_VFP1_HALFTRANS;
   }else if( input->priority == 8 )
   {
      prg = PG_VDP1_NORMAL;
   }

   if (input->linescreen){
     prg = PG_LINECOLOR_INSERT;
   }

   program = YglEsGetProgram(input,prg);
   if( program == NULL ) return;

   program->color_offset_val[0] = (float)(input->cor)/255.0f;
   program->color_offset_val[1] = (float)(input->cog)/255.0f;
   program->color_offset_val[2] = (float)(input->cob)/255.0f;
   program->color_offset_val[3] = 0;

   x = cache->x;
   y = cache->y;

   // Vertex
   pos = program->quads + program->currentQuad;
   pos[0] = input->vertices[0];
   pos[1] = input->vertices[1];
   pos[2] = input->vertices[2];
   pos[3] = input->vertices[3];
   pos[4] = input->vertices[4];
   pos[5] = input->vertices[5];
   pos[6] = input->vertices[0];
   pos[7] = input->vertices[1];
   pos[8] = input->vertices[4];
   pos[9] = input->vertices[5];
   pos[10] = input->vertices[6];
   pos[11] = input->vertices[7];

   // Color
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));
   vtxa = (program->vertexAttribute + (program->currentQuad * 2));
   memset(vtxa,0,sizeof(float)*24);

   program->currentQuad += 12;

  tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w)-ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ATLAS_BIAS;
   } else {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x)+ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w)-ATLAS_BIAS;
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h)-ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y)+ATLAS_BIAS;
   } else {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y)+ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h)-ATLAS_BIAS;
   }



   if( input->dst == 1 )
   {
      YglEsCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[0];
      tmp[3].t *= q[0];
      tmp[4].s *= q[2];
      tmp[4].t *= q[2];
      tmp[5].s *= q[3];
      tmp[5].t *= q[3];

      tmp[0].q = q[0];
      tmp[1].q = q[1];
      tmp[2].q = q[2];
      tmp[3].q = q[0];
      tmp[4].q = q[2];
      tmp[5].q = q[3];
   }else{
      tmp[0].q = 1.0f;
      tmp[1].q = 1.0f;
      tmp[2].q = 1.0f;
      tmp[3].q = 1.0f;
      tmp[4].q = 1.0f;
      tmp[5].q = 1.0f;
   }

}

//////////////////////////////////////////////////////////////////////////////

void YglEsCacheQuadGrowShading(YglEsSprite * input, float * colors,YglEsCache * cache) {
   YglEsProgram * program;
   unsigned int x,y;
   texturecoordinate_struct *tmp;
   float q[4];
   int prg = PG_VFP1_GOURAUDSAHDING;
   int currentpg = 0;
   float * vtxa;
   float *pos;


  if( (input->blendmode&0x03) == 2 )
   {
      prg = PG_VDP2_ADDBLEND;
   }else if( input->blendmode == 0x80 )
   {
      prg = PG_VFP1_GOURAUDSAHDING_HALFTRANS;
   }

   if (input->linescreen){
     prg = PG_LINECOLOR_INSERT;
   }

   program = YglEsGetProgram(input,prg);
   if( program == NULL ) return;

   program->color_offset_val[0] = (float)(input->cor)/255.0f;
   program->color_offset_val[1] = (float)(input->cog)/255.0f;
   program->color_offset_val[2] = (float)(input->cob)/255.0f;
   program->color_offset_val[3] = 0;

   x = cache->x;
   y = cache->y;

   // Vertex
   pos = program->quads + program->currentQuad;
   pos[0] = input->vertices[0];
   pos[1] = input->vertices[1];
   pos[2] = input->vertices[2];
   pos[3] = input->vertices[3];
   pos[4] = input->vertices[4];
   pos[5] = input->vertices[5];
   pos[6] = input->vertices[0];
   pos[7] = input->vertices[1];
   pos[8] = input->vertices[4];
   pos[9] = input->vertices[5];
   pos[10] = input->vertices[6];
   pos[11] = input->vertices[7];

   // Color
   vtxa = (program->vertexAttribute + (program->currentQuad * 2));
   if( colors == NULL )
   {
        memset(vtxa,0,sizeof(float)*24);
   }else{
       vtxa[0] = colors[0];
       vtxa[1] = colors[1];
       vtxa[2] = colors[2];
       vtxa[3] = colors[3];

       vtxa[4] = colors[4];
       vtxa[5] = colors[5];
       vtxa[6] = colors[6];
       vtxa[7] = colors[7];

       vtxa[8] = colors[8];
       vtxa[9] = colors[9];
       vtxa[10] = colors[10];
       vtxa[11] = colors[11];

       vtxa[12] = colors[0];
       vtxa[13] = colors[1];
       vtxa[14] = colors[2];
       vtxa[15] = colors[3];

       vtxa[16] = colors[8];
       vtxa[17] = colors[9];
       vtxa[18] = colors[10];
       vtxa[19] = colors[11];

       vtxa[20] = colors[12];
       vtxa[21] = colors[13];
       vtxa[22] = colors[14];
       vtxa[23] = colors[15];
   }

   // Texture
   tmp = (texturecoordinate_struct *)(program->textcoords + (program->currentQuad * 2));

   program->currentQuad += 12;

  tmp[0].r = tmp[1].r = tmp[2].r = tmp[3].r = tmp[4].r = tmp[5].r = 0; // these can stay at 0

   if (input->flip & 0x1) {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x + input->w)-ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x)+ATLAS_BIAS;
   } else {
      tmp[0].s = tmp[3].s = tmp[5].s = (float)(x)+ATLAS_BIAS;
      tmp[1].s = tmp[2].s = tmp[4].s = (float)(x + input->w)-ATLAS_BIAS;
   }
   if (input->flip & 0x2) {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y + input->h)-ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y)+ATLAS_BIAS;
   } else {
      tmp[0].t = tmp[1].t = tmp[3].t = (float)(y)+ATLAS_BIAS;
      tmp[2].t = tmp[4].t = tmp[5].t = (float)(y + input->h)-ATLAS_BIAS;
   }

   if( input->dst == 1 )
   {
      YglEsCalcTextureQ(input->vertices,q);
      tmp[0].s *= q[0];
      tmp[0].t *= q[0];
      tmp[1].s *= q[1];
      tmp[1].t *= q[1];
      tmp[2].s *= q[2];
      tmp[2].t *= q[2];
      tmp[3].s *= q[0];
      tmp[3].t *= q[0];
      tmp[4].s *= q[2];
      tmp[4].t *= q[2];
      tmp[5].s *= q[3];
      tmp[5].t *= q[3];
      tmp[0].q = q[0];
      tmp[1].q = q[1];
      tmp[2].q = q[2];
      tmp[3].q = q[0];
      tmp[4].q = q[2];
      tmp[5].q = q[3];
   }else{
      tmp[0].q = 1.0f;
      tmp[1].q = 1.0f;
      tmp[2].q = 1.0f;
      tmp[3].q = 1.0f;
      tmp[4].q = 1.0f;
      tmp[5].q = 1.0f;
   }
}


//////////////////////////////////////////////////////////////////////////////
void YglEsRenderVDP1(void) {

   YglEsLevel * level;
   GLuint cprg=0;
   int j;
   int status;

   if (_YglEs->pFrameBuffer != NULL) {
     _YglEs->pFrameBuffer = NULL;
     glBindBuffer(GL_PIXEL_PACK_BUFFER, _YglEs->vdp1pixelBufferID);
     glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
     glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
   }
   YGLLOG("YglEsRenderVDP1 %d, PTMR = %d\n", _YglEs->drawframe, Vdp1Regs->PTMR);

   level = &(_YglEs->levels[_YglEs->depth]);
   glDisable(GL_STENCIL_TEST);
   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _YglEs->texture);
   if (YglEsTM->texture != NULL) {
     glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->pixelBufferID);
     glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, YglEsTM->width, YglEsTM->yMax, GL_RGBA, GL_UNSIGNED_BYTE, 0);
     glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
     YglEsTM->texture = NULL;
   }

   cprg = -1;

   glBindFramebuffer(GL_FRAMEBUFFER, _YglEs->vdp1fbo);
   glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[_YglEs->drawframe], 0);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_depth);
   glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, _YglEs->rboid_stencil);
   status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
   if( status != GL_FRAMEBUFFER_COMPLETE )
   {
      YGLLOG("YglEsRenderVDP1: Framebuffer status = %08X\n", status );
      return;
   }else{
      //YGLLOG("Framebuffer status OK = %08X\n", status );
   }

   // Many regressions to Enable it
   //if(  ((Vdp1Regs->TVMR & 0x08) && (Vdp1Regs->FBCR&0x03)==0x03) || ((Vdp1Regs->FBCR & 2) == 0) || Vdp1External.manualerase)
   {
     u16 color;
     int priority;
     u16 alpha;
#if 0
     h = (Vdp1Regs->EWRR & 0x1FF) + 1;
     if (h > vdp1height) h = vdp1height;
     w = ((Vdp1Regs->EWRR >> 6) & 0x3F8) + 8;
     if (w > vdp1width) w = vdp1width;

     if (vdp1pixelsize == 2)
     {
       for (i2 = (Vdp1Regs->EWLR & 0x1FF); i2 < h; i2++)
       {
         for (i = ((Vdp1Regs->EWLR >> 6) & 0x1F8); i < w; i++)
           ((u16 *)vdp1backframebuffer)[(i2 * vdp1width) + i] = Vdp1Regs->EWDR;
       }
     }
     else
     {
       for (i2 = (Vdp1Regs->EWLR & 0x1FF); i2 < h; i2++)
       {
         for (i = ((Vdp1Regs->EWLR >> 6) & 0x1F8); i < w; i++)
           vdp1backframebuffer[(i2 * vdp1width) + i] = Vdp1Regs->EWDR & 0xFF;
       }
     }
#endif

     color = Vdp1Regs->EWDR;
     priority = 0;

     if (color & 0x8000)
       priority = Vdp2Regs->PRISA & 0x7;
     else
     {
       int shadow, colorcalc;
       Vdp1ProcessSpritePixel(Vdp2Regs->SPCTL & 0xF, &color, &shadow, &priority, &colorcalc);
#ifdef WORDS_BIGENDIAN
       priority = ((u8 *)&Vdp2Regs->PRISA)[priority ^ 1] & 0x7;
#else
       priority = ((u8 *)&Vdp2Regs->PRISA)[priority] & 0x7;
#endif
     }

     if (color == 0)
     {
       alpha = 0;
       priority = 0;
     }
     else{
       alpha = 0xF8;
     }
     alpha |= priority;

     glClearColor((color & 0x1F) / 31.0f, ((color >> 5) & 0x1F) / 31.0f, ((color >> 10) & 0x1F) / 31.0f, alpha / 255.0f);
     glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
     Vdp1External.manualerase = 0;
     YGLLOG("YglEsRenderVDP1: clear %d\n", _YglEs->drawframe);

   }



   glDisable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glCullFace(GL_FRONT_AND_BACK);
   glDisable(GL_CULL_FACE);

   for( j=0;j<(level->prgcurrent+1); j++ )
   {
      if( level->prg[j].prgid != cprg )
      {
         cprg = level->prg[j].prgid;
         glUseProgram(level->prg[j].prg);
      }
      if(level->prg[j].setupUniform)
      {
         level->prg[j].setupUniform((void*)&level->prg[j]);
      }
        
      if( level->prg[j].currentQuad != 0 )
      {
         glUniformMatrix4fv(level->prg[j].mtxModelView, 1, GL_FALSE, (GLfloat*)&_YglEs->mtxModelView.m[0][0]);
         glVertexAttribPointer(level->prg[j].vertexp, 2, GL_FLOAT, GL_FALSE, 0, (GLvoid *)level->prg[j].quads);
         glVertexAttribPointer(level->prg[j].texcoordp,4,GL_FLOAT,GL_FALSE,0,(GLvoid *)level->prg[j].textcoords );
         if( level->prg[j].vaid != 0 ) {
            glVertexAttribPointer(level->prg[j].vaid,4, GL_FLOAT, GL_FALSE, 0, level->prg[j].vertexAttribute);
         }
printf("prog %d %d (%d)\n", level->prg[j].prgid, level->prg[j].currentQuad, __LINE__);
	 glDrawArrays(GL_TRIANGLES, 0, level->prg[j].currentQuad/2);
         level->prg[j].currentQuad = 0;
      }

      if( level->prg[j].cleanupUniform )
      {
         level->prg[j].cleanupUniform((void*)&level->prg[j]);
      }

   }
   level->prgcurrent = 0;
   
#if 0
   if ( (((Vdp1Regs->TVMR & 0x08)==0) && ((Vdp1Regs->FBCR & 0x03)==0x03) )
	)
   {
	   u32 current_drawframe = 0;
	   current_drawframe = _YglEs->drawframe;
	   _YglEs->drawframe = _YglEs->readframe;
	   _YglEs->readframe = current_drawframe;
     Vdp1External.manualchange = 0;
     YGLLOG("YglEsRenderVDP1: swap drawframe =%d readframe = %d\n", _YglEs->drawframe, _YglEs->readframe);
   }
#endif
   if ((((Vdp1Regs->TVMR & 0x08) == 0) && ((Vdp1Regs->FBCR & 0x03) == 0x03)) ||
	   ((Vdp1Regs->FBCR & 2) == 0) || 
	   Vdp1External.manualchange)
   {
	   u32 current_drawframe = 0;
	   current_drawframe = _YglEs->drawframe;
	   _YglEs->drawframe = _YglEs->readframe;
	   _YglEs->readframe = current_drawframe;
	   Vdp1External.manualchange = 0;
	   YGLLOG("YglEsRenderVDP1: swap drawframe =%d readframe = %d\n", _YglEs->drawframe, _YglEs->readframe);
   }

   // glFlush(); need??
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);

}

void YglEsDmyRenderVDP1(void) {

    Vdp1External.manualerase = 0;

    if ( (((Vdp1Regs->TVMR & 0x08)==0) && ((Vdp1Regs->FBCR & 0x03)==0x03) ) ||
          ((Vdp1Regs->FBCR & 2) == 0) ||
          Vdp1External.manualchange )
    {
		u32 current_drawframe = 0;
		current_drawframe = _YglEs->drawframe;
		_YglEs->drawframe = _YglEs->readframe;
		_YglEs->readframe = current_drawframe;

      Vdp1External.manualchange = 0;
      YGLLOG("YglEsRenderVDP1: swap drawframe =%d readframe = %d\n", _YglEs->drawframe, _YglEs->readframe);
    }
}

void YglEsNeedToUpdateWindow()
{
   _YglEs->bUpdateWindow = 1;
}

void YglEsSetVdp2Window()
{
    int bwin0,bwin1;
   //if( _YglEs->bUpdateWindow && (_YglEs->win0_vertexcnt != 0 || _YglEs->win1_vertexcnt != 0 ) )

    bwin0 = (Vdp2Regs->WCTLC >> 9) &0x01;
    bwin1 = (Vdp2Regs->WCTLC >> 11) &0x01;
   if( (_YglEs->win0_vertexcnt != 0 || _YglEs->win1_vertexcnt != 0 )  )
   {

     YglEs_uniformWindow(&_YglEs->windowpg);
     glUniformMatrix4fv( _YglEs->windowpg.mtxModelView, 1, GL_FALSE, (GLfloat*) &_YglEs->mtxModelView.m[0][0] );

      //
     glColorMask(GL_FALSE,GL_FALSE,GL_FALSE,GL_FALSE);
     glDepthMask(GL_FALSE);
     glDisable(GL_TEXTURE_2D);
     glDisable(GL_DEPTH_TEST);

     //glClearStencil(0);
     //glClear(GL_STENCIL_BUFFER_BIT);
     glEnable(GL_STENCIL_TEST);
     glDisable(GL_TEXTURE_2D);

     glStencilOp(GL_REPLACE,GL_REPLACE,GL_REPLACE);

      if( _YglEs->win0_vertexcnt != 0 )
      {
           glStencilMask(0x01);
           glStencilFunc(GL_ALWAYS,0x01,0x01);
           glVertexAttribPointer(_YglEs->windowpg.vertexp,2,GL_INT, GL_FALSE,0,(GLvoid *)_YglEs->win0v );
printf("render (%d)\n", __LINE__);
           glDrawArrays(GL_TRIANGLE_STRIP,0,_YglEs->win0_vertexcnt);
      }

      if( _YglEs->win1_vertexcnt != 0 )
      {
          glStencilMask(0x02);
          glStencilFunc(GL_ALWAYS,0x02,0x02);
          glVertexAttribPointer(_YglEs->windowpg.vertexp,2,GL_INT, GL_FALSE,0,(GLvoid *)_YglEs->win1v );
printf("render (%d)\n", __LINE__);
          glDrawArrays(GL_TRIANGLE_STRIP,0,_YglEs->win1_vertexcnt);
      }

      glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
      glDepthMask(GL_TRUE);
      glEnable(GL_DEPTH_TEST);
      glDisable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);
      glStencilFunc(GL_ALWAYS,0,0xFF);
      glStencilMask(0xFFFFFFFF);

      _YglEs->bUpdateWindow = 0;
   }
   return;
}


void YglEsRenderFrameBuffer( int from , int to ) {

   GLint   vertices[12];
   GLfloat texcord[12];
   float offsetcol[4];
   int bwin0,bwin1,logwin0,logwin1,winmode;

   // Out of range, do nothing
   if( _YglEs->vdp1_maxpri < from ) return;
   if( _YglEs->vdp1_minpri > to ) return;

   //YGLLOG("YglEsRenderFrameBuffer: %d to %d\n", from , to );

   offsetcol[0] = vdp1cor / 255.0f;
   offsetcol[1] = vdp1cog / 255.0f;
   offsetcol[2] = vdp1cob / 255.0f;
   offsetcol[3] = 0.0f;

   if ( (Vdp2Regs->CCCTL & 0x540) == 0x140 ){
       // Sprite Add Color
       YglEs_uniformVDP2DrawFramebuffer_addcolor(&_YglEs->renderfb, (float)(from) / 10.0f, (float)(to) / 10.0f, offsetcol);
   }else if (Vdp2Regs->LNCLEN & 0x20){
	YglEs_uniformVDP2DrawFramebuffer_linecolor(&_YglEs->renderfb, (float)(from) / 10.0f, (float)(to) / 10.0f, offsetcol);
   }
   else{
     YglEs_uniformVDP2DrawFramebuffer(&_YglEs->renderfb, (float)(from) / 10.0f, (float)(to) / 10.0f, offsetcol);
   }
   glBindTexture(GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[_YglEs->readframe]);
   //glBindTexture(GL_TEXTURE_2D, _YglEs->vdp1FrameBuff[_YglEs->drawframe]);

   YGLLOG("YglEsRenderFrameBuffer: %d to %d: fb %d\n", from, to, _YglEs->readframe);

   // Window Mode
   bwin0 = (Vdp2Regs->WCTLC >> 9) &0x01;
   logwin0 = (Vdp2Regs->WCTLC >> 8) & 0x01;
   bwin1 = (Vdp2Regs->WCTLC >> 11) &0x01;
   logwin1 = (Vdp2Regs->WCTLC >> 10) & 0x01;
   winmode    = (Vdp2Regs->WCTLC >> 15 ) & 0x01;

   if( bwin0 || bwin1 )
   {
      glEnable(GL_STENCIL_TEST);
      glStencilOp(GL_KEEP,GL_KEEP,GL_KEEP);

      if( bwin0 && !bwin1 )
      {
         if( logwin0 )
         {
            glStencilFunc(GL_EQUAL,0x01,0x01);
         }else{
            glStencilFunc(GL_NOTEQUAL,0x01,0x01);
         }
      }else if( !bwin0 && bwin1 ) {

         if( logwin1 )
         {
            glStencilFunc(GL_EQUAL,0x02,0x02);
         }else{
            glStencilFunc(GL_NOTEQUAL,0x02,0x02);
         }
      }else if( bwin0 && bwin1 ) {
         // and
         if( winmode == 0x0 )
         {
			 if (logwin0 == 1 && logwin1 == 1){ // show inside
				glStencilFunc(GL_EQUAL, 0x03, 0x03);
			}
			 else if(logwin0 == 0 && logwin1 == 0) {
				glStencilFunc(GL_NOTEQUAL, 0x03, 0x03);
			 }
			 else{
				glStencilFunc(GL_ALWAYS, 0x00, 0x00);
			 }

         // OR
         }else if( winmode == 0x01 )
         {
			 // OR
			 if (logwin0 == 1 && logwin1 == 1){ // show inside
				 glStencilFunc(GL_LEQUAL, 0x01, 0x03);
			 }
			 else if (logwin0 == 0 && logwin1 == 0) {
				 glStencilFunc(GL_GREATER, 0x01, 0x03);
			 }
			 else{
				 glStencilFunc(GL_ALWAYS, 0x00, 0x00);
			 }
         }
      }
   }

   // render
   vertices[0] = 0;
   vertices[1] = 0;
   vertices[2] = _YglEs->rwidth+1;
   vertices[3] = 0;
   vertices[4] = _YglEs->rwidth+1;
   vertices[5] = _YglEs->rheight+1;

   vertices[6] = 0;
   vertices[7] = 0;
   vertices[8] = _YglEs->rwidth+1;
   vertices[9] = _YglEs->rheight+1;
   vertices[10] = 0;
   vertices[11] = _YglEs->rheight+1;


   texcord[0] = 0.0f;
   texcord[1] = 1.0f;
   texcord[2] = 1.0f;
   texcord[3] = 1.0f;
   texcord[4] = 1.0f;
   texcord[5] = 0.0f;

   texcord[6] = 0.0f;
   texcord[7] = 1.0f;
   texcord[8] = 1.0f;
   texcord[9] = 0.0f;
   texcord[10] = 0.0f;
   texcord[11] = 0.0f;

   glUniformMatrix4fv( _YglEs->renderfb.mtxModelView, 1, GL_FALSE, (GLfloat*)&_YglEs->mtxModelView.m[0][0] );
   glVertexAttribPointer(_YglEs->renderfb.vertexp,2,GL_INT, GL_FALSE,0,(GLvoid *)vertices );
   glVertexAttribPointer(_YglEs->renderfb.texcoordp,2,GL_FLOAT,GL_FALSE,0,(GLvoid *)texcord );
printf("render (%d)\n", __LINE__);
   glDrawArrays(GL_TRIANGLES, 0, 6);

   if( bwin0 || bwin1 )
   {
      glDisable(GL_STENCIL_TEST);
      glStencilFunc(GL_ALWAYS,0,0xFF);
   }
}

void YglEsSetClearColor(float r, float g, float b){
	_YglEs->clear_r = r;
	_YglEs->clear_g = g;
	_YglEs->clear_b = b;
}


void YglEsRender(void) {
   YglEsLevel * level;
   GLuint cprg=0;
   int from = 0;
   int to   = 0;
   YglEsMatrix mtx;
   YglEsMatrix dmtx;
   unsigned int i,j;

   YGLLOG("YglEsRender\n");

   glBindFramebuffer(GL_FRAMEBUFFER,0);

   glClearColor(_YglEs->clear_r, _YglEs->clear_g, _YglEs->clear_b, 1.0f);
   glClearDepthf(0.0f);
   glDepthMask(GL_TRUE);
   glEnable(GL_DEPTH_TEST);
   glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);

   glActiveTexture(GL_TEXTURE0);
   glBindTexture(GL_TEXTURE_2D, _YglEs->texture);
   if (YglEsTM->texture != NULL) {
     glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->pixelBufferID);
     glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
     glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, YglEsTM->width, YglEsTM->yMax, GL_RGBA, GL_UNSIGNED_BYTE, 0);
     glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
     YglEsTM->texture = NULL;
   }
   
#if 0 // Test
   ShaderDrawTest();
#else
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   //YglEsRenderVDP1();

   YglEsLoadIdentity(&mtx);

   cprg = -1;

   YglEsSetVdp2Window();

   YglEsTranslatef(&mtx,0.0f,0.0f,-1.0f);

   for(i = 0;i < _YglEs->depth;i++)
   {
      level = _YglEs->levels + i;

         if( level->blendmode != 0 )
         {
            to = i;

            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			if(Vdp1External.disptoggle&0x01) YglEsRenderFrameBuffer(from, to);
            from = to;

            // clean up
            cprg = -1;
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, _YglEs->texture);
         }

         glDisable(GL_STENCIL_TEST);
         for( j=0;j<(level->prgcurrent+1); j++ )
         {
            if( level->prg[j].prgid != cprg )
            {
               cprg = level->prg[j].prgid;
               glUseProgram(level->prg[j].prg);


            }
            if(level->prg[j].setupUniform)
            {
               level->prg[j].setupUniform((void*)&level->prg[j]);
            }

            YglEsMatrixMultiply(&dmtx, &mtx, &_YglEs->mtxModelView);
            
  		    if( level->prg[j].currentQuad != 0 )
			    {
#if 0
            if (level->blendmode == 0){
              glDisable(GL_BLEND);
            }
            else if (level->blendmode == 1){
              glEnable(GL_BLEND);
              glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            else if (level->blendmode == 2){
              glEnable(GL_BLEND);
              glBlendFunc(GL_ONE, GL_ONE);
            }
#endif
					glUniformMatrix4fv(level->prg[j].mtxModelView, 1, GL_FALSE, (GLfloat*)&dmtx.m[0][0]);
				    glVertexAttribPointer(level->prg[j].vertexp,2,GL_FLOAT, GL_FALSE,0,(GLvoid *)level->prg[j].quads );
				    glVertexAttribPointer(level->prg[j].texcoordp,4,GL_FLOAT,GL_FALSE,0,(GLvoid *)level->prg[j].textcoords );
				    if( level->prg[j].vaid != 0 ) { glVertexAttribPointer(level->prg[j].vaid,4, GL_FLOAT, GL_FALSE, 0, level->prg[j].vertexAttribute); }
                    glDrawArrays(GL_TRIANGLES, 0, level->prg[j].currentQuad/2);
                    level->prg[j].currentQuad = 0;
			    }

          if( level->prg[j].cleanupUniform )
          {
               level->prg[j].cleanupUniform((void*)&level->prg[j]);
          }

         }
         level->prgcurrent = 0;

         YglEsTranslatef(&mtx,0.0f,0.0f,0.1f);

   }

   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   if (Vdp1External.disptoggle & 0x01) YglEsRenderFrameBuffer(from, 8);

#endif
   glDisable(GL_TEXTURE_2D);
   glUseProgram(0);
   glGetError();
   glBindBuffer(GL_ARRAY_BUFFER, 0);
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER,0);
   glDisableVertexAttribArray(0);
   glDisableVertexAttribArray(1);
   glDisableVertexAttribArray(2);
   glDisable(GL_DEPTH_TEST);
   glDisable(GL_SCISSOR_TEST);
   YuiSwapBuffers();
   glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->pixelBufferID);
   YglEsTM->texture = (unsigned int*)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 2048 * 1024 * 4, GL_MAP_WRITE_BIT);
   if (YglEsTM->texture == NULL){
	   abort();
   }
#if 0
   if ( ((Vdp1Regs->FBCR & 2) == 0) )
   {
	   YabThreadLock(_YglEs->mutex);
	   u32 current_drawframe = 0;
	   current_drawframe = _YglEs->drawframe;
	   _YglEs->drawframe = _YglEs->readframe;
	   _YglEs->readframe = current_drawframe;
	   Vdp1External.manualchange = 0;
	   YGLLOG("YglEsRenderVDP1: swap drawframe =%d readframe = %d\n", _YglEs->drawframe, _YglEs->readframe);
	   YabThreadUnLock(_YglEs->mutex);
   }
#endif
   return;
}

//////////////////////////////////////////////////////////////////////////////

void YglEsReset(void) {
   YglEsLevel * level;
   unsigned int i,j;

   YglEsTMReset();

   for(i = 0;i < (_YglEs->depth+1) ;i++) {
     level = _YglEs->levels + i;
     level->blendmode  = 0;
     level->prgcurrent = 0;
     level->uclipcurrent = 0;
     level->ux1 = 0;
     level->uy1 = 0;
     level->ux2 = 0;
     level->uy2 = 0;
     for( j=0; j< level->prgcount; j++ )
     {
         _YglEs->levels[i].prg[j].currentQuad = 0;
     }
   }
   _YglEs->msglength = 0;
}

//////////////////////////////////////////////////////////////////////////////

void YglEsShowTexture(void) {
   _YglEs->st = !_YglEs->st;
}

u32 * YglEsGetLineColorPointer(){
  int error;
  if (_YglEs->lincolor_tex == 0){
    glGetError();
    glGenTextures(1, &_YglEs->lincolor_tex);

    glGenBuffers(1, &_YglEs->linecolor_pbo);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->linecolor_pbo);
    glBufferData(GL_PIXEL_UNPACK_BUFFER, 512 * 4, NULL, GL_STREAM_DRAW);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

    glBindTexture(GL_TEXTURE_2D, _YglEs->lincolor_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 512, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    if ((error = glGetError()) != GL_NO_ERROR)
    {
      YGLLOG("Fail to init lincolor_tex %04X", error);
      return NULL;
    }
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  }

  glBindTexture(GL_TEXTURE_2D, _YglEs->lincolor_tex);
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->linecolor_pbo);
  _YglEs->lincolor_buf = (u32 *)glMapBufferRange(GL_PIXEL_UNPACK_BUFFER, 0, 512 * 4, GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT);
  if ((error = glGetError()) != GL_NO_ERROR)
  {
    YGLLOG("Fail to init YglEsTM->texture %04X", error);
    return NULL;
  }
  glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);

  return _YglEs->lincolor_buf;
}

void YglEsSetLineColor(u32 * pbuf, int size){

  glBindTexture(GL_TEXTURE_2D, _YglEs->lincolor_tex);
  //if (_YglEs->lincolor_buf == pbuf) {
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, _YglEs->linecolor_pbo);
    glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size, 1, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
    _YglEs->lincolor_buf = NULL;
  //}
  glBindTexture(GL_TEXTURE_2D, 0 );
  return;
}

//////////////////////////////////////////////////////////////////////////////

void YglEsChangeResolution(int w, int h) {
   YglEsLoadIdentity(&_YglEs->mtxModelView);
   YglEsOrtho(&_YglEs->mtxModelView, 0.0f, (float)w, (float)h, 0.0f, 10.0f, 0.0f);

   if( _YglEs->rwidth != w || _YglEs->rheight != h ) {
       if (_YglEs->smallfbo != 0) {
         glDeleteFramebuffers(1, &_YglEs->smallfbo);
         _YglEs->smallfbo = 0;
         glDeleteTextures(1, &_YglEs->smallfbotex);
         _YglEs->smallfbotex = 0;
         glDeleteBuffers(1, &_YglEs->vdp1pixelBufferID);
         _YglEs->vdp1pixelBufferID = 0;
         _YglEs->pFrameBuffer = NULL;
       }
   }

   _YglEs->rwidth = w;
   _YglEs->rheight = h;

}

//////////////////////////////////////////////////////////////////////////////

void YglEsOnScreenDebugMessage(char *string, ...) {
   va_list arglist;

   va_start(arglist, string);
   vsprintf(_YglEs->message, string, arglist);
   va_end(arglist);
   _YglEs->msglength = (int)strlen(_YglEs->message);
}

#endif
