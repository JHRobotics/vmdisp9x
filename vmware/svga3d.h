/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

/*
 * svga3d.h --
 *
 *      FIFO command level interface to the SVGA3D protocol used by
 *      the VMware SVGA device.
 */

#ifndef __SVGA3D_H__
#define __SVGA3D_H__

#include "svga.h"
#include "svga3d_reg.h"


/*
 * SVGA Device Interoperability
 */

Bool SVGA3D_Init(void);
void SVGA3D_BeginPresent(uint32 sid, SVGA3dCopyRect **rects, uint32 numRects);
void SVGA3D_BeginPresentReadback(SVGA3dRect **rects, uint32 numRects);
void SVGA3D_BlitSurfaceToScreen(const SVGA3dSurfaceImageId *srcImage,
                                const SVGASignedRect *srcRect,
                                uint32 destScreenId,
                                const SVGASignedRect *destRect);
void SVGA3D_BeginBlitSurfaceToScreen(const SVGA3dSurfaceImageId *srcImage,
                                     const SVGASignedRect *srcRect,
                                     uint32 destScreenId,
                                     const SVGASignedRect *destRect,
                                     SVGASignedRect **clipRects,
                                     uint32 numClipRects);


/*
 * Surface Management
 */

void SVGA3D_BeginDefineSurface(uint32 sid,
                               SVGA3dSurfaceFlags flags,
                               SVGA3dSurfaceFormat format,
                               SVGA3dSurfaceFace **faces,
                               SVGA3dSize **mipSizes,
                               uint32 numMipSizes);
void SVGA3D_DestroySurface(uint32 sid);
void SVGA3D_BeginSurfaceDMA(SVGA3dGuestImage *guestImage,
                            SVGA3dSurfaceImageId *hostImage,
                            SVGA3dTransferType transfer,
                            SVGA3dCopyBox **boxes,
                            uint32 numBoxes);


/*
 * Context Management
 */

void SVGA3D_DefineContext(uint32 cid);
void SVGA3D_DestroyContext(uint32 cid);


/*
 * Drawing Operations
 */

void SVGA3D_BeginClear(uint32 cid, SVGA3dClearFlag flags,
                       uint32 color, float depth, uint32 stencil,
                       SVGA3dRect **rects, uint32 numRects);
void SVGA3D_BeginDrawPrimitives(uint32 cid,
                                SVGA3dVertexDecl **decls,
                                uint32 numVertexDecls,
                                SVGA3dPrimitiveRange **ranges,
                                uint32 numRanges);

/*
 * Blits
 */

void SVGA3D_BeginSurfaceCopy(SVGA3dSurfaceImageId *src,
                             SVGA3dSurfaceImageId *dest,
                             SVGA3dCopyBox **boxes, uint32 numBoxes);

void SVGA3D_SurfaceStretchBlt(SVGA3dSurfaceImageId *src,
                              SVGA3dSurfaceImageId *dest,
                              SVGA3dBox *boxSrc, SVGA3dBox *boxDest,
                              SVGA3dStretchBltMode mode);

/*
 * Shared FFP/Shader Render State
 */

void SVGA3D_SetRenderTarget(uint32 cid, SVGA3dRenderTargetType type,
                            SVGA3dSurfaceImageId *target);
void SVGA3D_SetZRange(uint32 cid, float zMin, float zMax);
void SVGA3D_SetViewport(uint32 cid, SVGA3dRect *rect);
void SVGA3D_SetScissorRect(uint32 cid, SVGA3dRect *rect);
void SVGA3D_SetClipPlane(uint32 cid, uint32 index, const float *plane);
void SVGA3D_BeginSetTextureState(uint32 cid, SVGA3dTextureState **states,
                                 uint32 numStates);
void SVGA3D_BeginSetRenderState(uint32 cid, SVGA3dRenderState **states,
                                uint32 numStates);


/*
 * Fixed-function Render State
 */

void SVGA3D_SetTransform(uint32 cid, SVGA3dTransformType type,
                         const float *matrix);
void SVGA3D_SetMaterial(uint32 cid, SVGA3dFace face, const SVGA3dMaterial *material);
void SVGA3D_SetLightData(uint32 cid, uint32 index, const SVGA3dLightData *data);
void SVGA3D_SetLightEnabled(uint32 cid, uint32 index, Bool enabled);


/*
 * Shaders
 */

void SVGA3D_DefineShader(uint32 cid, uint32 shid, SVGA3dShaderType type,
                         const uint32 *bytecode, uint32 bytecodeLen);
void SVGA3D_DestroyShader(uint32 cid, uint32 shid, SVGA3dShaderType type);
void SVGA3D_SetShaderConst(uint32 cid, uint32 reg, SVGA3dShaderType type,
                           SVGA3dShaderConstType ctype, const void *value);
void SVGA3D_SetShader(uint32 cid, SVGA3dShaderType type, uint32 shid);

#endif /* __SVGA3D_H__ */
