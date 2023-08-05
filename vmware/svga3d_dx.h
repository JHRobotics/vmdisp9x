/**********************************************************
 * Copyright 2007-2017 VMware, Inc.  All rights reserved.
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
 * svga3d_dx.h --
 *
 *       SVGA 3d hardware definitions for DX10 support.
 */

#ifndef _SVGA3D_DX_H_
#define _SVGA3D_DX_H_

#define INCLUDE_ALLOW_MODULE
#define INCLUDE_ALLOW_USERLEVEL
#define INCLUDE_ALLOW_VMCORE

#define SVGA3D_MAX_QUERY 64

typedef uint32 SVGA3dSurfaceId;
typedef uint8 SVGA3dQueryTypeUint8;


#include "svga3d_limits.h"

/* Matches D3D10_DDI_INPUT_CLASSIFICATION and D3D10_INPUT_CLASSIFICATION */
#define SVGA3D_INPUT_MIN               0
#define SVGA3D_INPUT_PER_VERTEX_DATA   0
#define SVGA3D_INPUT_PER_INSTANCE_DATA 1
#define SVGA3D_INPUT_MAX               2
typedef uint32 SVGA3dInputClassification;

/* Matches D3D10DDIRESOURCE_TYPE */
#define SVGA3D_RESOURCE_TYPE_MIN      1
#define SVGA3D_RESOURCE_BUFFER        1
#define SVGA3D_RESOURCE_TEXTURE1D     2
#define SVGA3D_RESOURCE_TEXTURE2D     3
#define SVGA3D_RESOURCE_TEXTURE3D     4
#define SVGA3D_RESOURCE_TEXTURECUBE   5
#define SVGA3D_RESOURCE_TYPE_DX10_MAX 6
#define SVGA3D_RESOURCE_BUFFEREX      6
#define SVGA3D_RESOURCE_TYPE_MAX      7
typedef uint32 SVGA3dResourceType;

/* Matches D3D10_DDI_COLOR_WRITE_ENABLE and D3D10_COLOR_WRITE_ENABLE */
#define SVGA3D_COLOR_WRITE_ENABLE_RED     (1 << 0)
#define SVGA3D_COLOR_WRITE_ENABLE_GREEN   (1 << 1)
#define SVGA3D_COLOR_WRITE_ENABLE_BLUE    (1 << 2)
#define SVGA3D_COLOR_WRITE_ENABLE_ALPHA   (1 << 3)
#define SVGA3D_COLOR_WRITE_ENABLE_ALL     (SVGA3D_COLOR_WRITE_ENABLE_RED |   \
                                           SVGA3D_COLOR_WRITE_ENABLE_GREEN | \
                                           SVGA3D_COLOR_WRITE_ENABLE_BLUE |  \
                                           SVGA3D_COLOR_WRITE_ENABLE_ALPHA)
typedef uint8 SVGA3dColorWriteEnable;

/* Matches D3D10_DDI_DEPTH_WRITE_MASK and D3D10_DEPTH_WRITE_MASK */
#define SVGA3D_DEPTH_WRITE_MASK_ZERO   0
#define SVGA3D_DEPTH_WRITE_MASK_ALL    1
typedef uint8 SVGA3dDepthWriteMask;

/* Matches D3D10_DDI_FILTER and D3D10_FILTER */
#define SVGA3D_FILTER_MIP_LINEAR  (1 << 0)
#define SVGA3D_FILTER_MAG_LINEAR  (1 << 2)
#define SVGA3D_FILTER_MIN_LINEAR  (1 << 4)
#define SVGA3D_FILTER_ANISOTROPIC (1 << 6)
#define SVGA3D_FILTER_COMPARE     (1 << 7)
typedef uint32 SVGA3dFilter;

/* Matches D3D10_DDI_CULL_MODE */
#define SVGA3D_CULL_INVALID 0
#define SVGA3D_CULL_MIN     1
#define SVGA3D_CULL_NONE    1
#define SVGA3D_CULL_FRONT   2
#define SVGA3D_CULL_BACK    3
#define SVGA3D_CULL_MAX     4
typedef uint8 SVGA3dCullMode;

/* Matches D3D10_DDI_COMPARISON_FUNC */
#define SVGA3D_COMPARISON_INVALID         0
#define SVGA3D_COMPARISON_MIN             1
#define SVGA3D_COMPARISON_NEVER           1
#define SVGA3D_COMPARISON_LESS            2
#define SVGA3D_COMPARISON_EQUAL           3
#define SVGA3D_COMPARISON_LESS_EQUAL      4
#define SVGA3D_COMPARISON_GREATER         5
#define SVGA3D_COMPARISON_NOT_EQUAL       6
#define SVGA3D_COMPARISON_GREATER_EQUAL   7
#define SVGA3D_COMPARISON_ALWAYS          8
#define SVGA3D_COMPARISON_MAX             9
typedef uint8 SVGA3dComparisonFunc;

/*
 * SVGA3D_MULTISAMPLE_DISABLE disables MSAA for all primitives.
 * SVGA3D_MULTISAMPLE_DISABLE_LINE, which is supported in DX10.1,
 * disables MSAA for lines only.
 */
#define SVGA3D_MULTISAMPLE_DISABLE        0
#define SVGA3D_MULTISAMPLE_ENABLE         1
#define SVGA3D_MULTISAMPLE_DX_MAX         1
#define SVGA3D_MULTISAMPLE_DISABLE_LINE   2
#define SVGA3D_MULTISAMPLE_MAX            2
typedef uint8 SVGA3dMultisampleEnable;

#define SVGA3D_DX_MAX_VERTEXBUFFERS 32
#define SVGA3D_DX_MAX_VERTEXINPUTREGISTERS 16
#define SVGA3D_DX_SM41_MAX_VERTEXINPUTREGISTERS 32
#define SVGA3D_DX_MAX_SOTARGETS 4
#define SVGA3D_DX_MAX_SRVIEWS 128
#define SVGA3D_DX_MAX_CONSTBUFFERS 16
#define SVGA3D_DX_MAX_SAMPLERS 16

#define SVGA3D_DX_MAX_CONSTBUF_BINDING_SIZE (4096 * 4 * (uint32)sizeof(uint32))

typedef uint32 SVGA3dShaderResourceViewId;
typedef uint32 SVGA3dRenderTargetViewId;
typedef uint32 SVGA3dDepthStencilViewId;

typedef uint32 SVGA3dShaderId;
typedef uint32 SVGA3dElementLayoutId;
typedef uint32 SVGA3dSamplerId;
typedef uint32 SVGA3dBlendStateId;
typedef uint32 SVGA3dDepthStencilStateId;
typedef uint32 SVGA3dRasterizerStateId;
typedef uint32 SVGA3dQueryId;
typedef uint32 SVGA3dStreamOutputId;

typedef union {
   struct {
      float r;
      float g;
      float b;
      float a;
   };

   float value[4];
} SVGA3dRGBAFloat;

typedef
struct {
   uint32 cid;
   SVGAMobId mobid;
}
SVGAOTableDXContextEntry;

typedef
struct SVGA3dCmdDXDefineContext {
   uint32 cid;
}
SVGA3dCmdDXDefineContext;   /* SVGA_3D_CMD_DX_DEFINE_CONTEXT */

typedef
struct SVGA3dCmdDXDestroyContext {
   uint32 cid;
}
SVGA3dCmdDXDestroyContext;   /* SVGA_3D_CMD_DX_DESTROY_CONTEXT */

/*
 * Bind a DX context.
 *
 * validContents should be set to 0 for new contexts,
 * and 1 if this is an old context which is getting paged
 * back on to the device.
 *
 * For new contexts, it is recommended that the driver
 * issue commands to initialize all interesting state
 * prior to rendering.
 */
typedef
struct SVGA3dCmdDXBindContext {
   uint32 cid;
   SVGAMobId mobid;
   uint32 validContents;
}
SVGA3dCmdDXBindContext;   /* SVGA_3D_CMD_DX_BIND_CONTEXT */

/*
 * Readback a DX context.
 * (Request that the device flush the contents back into guest memory.)
 */
typedef
struct SVGA3dCmdDXReadbackContext {
   uint32 cid;
}
SVGA3dCmdDXReadbackContext;   /* SVGA_3D_CMD_DX_READBACK_CONTEXT */

/*
 * Invalidate a guest-backed context.
 */
typedef
struct SVGA3dCmdDXInvalidateContext {
   uint32 cid;
}
SVGA3dCmdDXInvalidateContext;   /* SVGA_3D_CMD_DX_INVALIDATE_CONTEXT */

typedef
struct SVGA3dCmdDXSetSingleConstantBuffer {
   uint32 slot;
   SVGA3dShaderType type;
   SVGA3dSurfaceId sid;
   uint32 offsetInBytes;
   uint32 sizeInBytes;
}
SVGA3dCmdDXSetSingleConstantBuffer;
/* SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER */

typedef
struct SVGA3dCmdDXSetShaderResources {
   uint32 startView;
   SVGA3dShaderType type;

   /*
    * Followed by a variable number of SVGA3dShaderResourceViewId's.
    */
}
SVGA3dCmdDXSetShaderResources; /* SVGA_3D_CMD_DX_SET_SHADER_RESOURCES */

typedef
struct SVGA3dCmdDXSetShader {
   SVGA3dShaderId shaderId;
   SVGA3dShaderType type;
}
SVGA3dCmdDXSetShader; /* SVGA_3D_CMD_DX_SET_SHADER */

typedef
struct SVGA3dCmdDXSetSamplers {
   uint32 startSampler;
   SVGA3dShaderType type;

   /*
    * Followed by a variable number of SVGA3dSamplerId's.
    */
}
SVGA3dCmdDXSetSamplers; /* SVGA_3D_CMD_DX_SET_SAMPLERS */

typedef
struct SVGA3dCmdDXDraw {
   uint32 vertexCount;
   uint32 startVertexLocation;
}
SVGA3dCmdDXDraw; /* SVGA_3D_CMD_DX_DRAW */

typedef
struct SVGA3dCmdDXDrawIndexed {
   uint32 indexCount;
   uint32 startIndexLocation;
   int32  baseVertexLocation;
}
SVGA3dCmdDXDrawIndexed; /* SVGA_3D_CMD_DX_DRAW_INDEXED */

typedef
struct SVGA3dCmdDXDrawInstanced {
   uint32 vertexCountPerInstance;
   uint32 instanceCount;
   uint32 startVertexLocation;
   uint32 startInstanceLocation;
}
SVGA3dCmdDXDrawInstanced; /* SVGA_3D_CMD_DX_DRAW_INSTANCED */

typedef
struct SVGA3dCmdDXDrawIndexedInstanced {
   uint32 indexCountPerInstance;
   uint32 instanceCount;
   uint32 startIndexLocation;
   int32  baseVertexLocation;
   uint32 startInstanceLocation;
}
SVGA3dCmdDXDrawIndexedInstanced; /* SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED */

typedef
struct SVGA3dCmdDXDrawIndexedInstancedIndirect {
   SVGA3dSurfaceId argsBufferSid;
   uint32 byteOffsetForArgs;
}
SVGA3dCmdDXDrawIndexedInstancedIndirect;
/* SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED_INDIRECT */

typedef
struct SVGA3dCmdDXDrawInstancedIndirect {
   SVGA3dSurfaceId argsBufferSid;
   uint32 byteOffsetForArgs;
}
SVGA3dCmdDXDrawInstancedIndirect;
/* SVGA_3D_CMD_DX_DRAW_INSTANCED_INDIRECT */

typedef
struct SVGA3dCmdDXDrawAuto {
   uint32 pad0;
}
SVGA3dCmdDXDrawAuto; /* SVGA_3D_CMD_DX_DRAW_AUTO */

typedef
struct SVGA3dCmdDXDispatch {
   uint32 threadGroupCountX;
   uint32 threadGroupCountY;
   uint32 threadGroupCountZ;
}
SVGA3dCmdDXDispatch;
/* SVGA_3D_CMD_DX_DISPATCH */

typedef
struct SVGA3dCmdDXDispatchIndirect {
   SVGA3dSurfaceId argsBufferSid;
   uint32 byteOffsetForArgs;
}
SVGA3dCmdDXDispatchIndirect;
/* SVGA_3D_CMD_DX_DISPATCH_INDIRECT */

typedef
struct SVGA3dCmdDXSetInputLayout {
   SVGA3dElementLayoutId elementLayoutId;
}
SVGA3dCmdDXSetInputLayout; /* SVGA_3D_CMD_DX_SET_INPUT_LAYOUT */

typedef
struct SVGA3dVertexBuffer {
   SVGA3dSurfaceId sid;
   uint32 stride;
   uint32 offset;
}
SVGA3dVertexBuffer;

typedef
struct SVGA3dCmdDXSetVertexBuffers {
   uint32 startBuffer;
   /* Followed by a variable number of SVGA3dVertexBuffer's. */
}
SVGA3dCmdDXSetVertexBuffers; /* SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS */

typedef
struct SVGA3dCmdDXSetIndexBuffer {
   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   uint32 offset;
}
SVGA3dCmdDXSetIndexBuffer; /* SVGA_3D_CMD_DX_SET_INDEX_BUFFER */

typedef
struct SVGA3dCmdDXSetTopology {
   SVGA3dPrimitiveType topology;
}
SVGA3dCmdDXSetTopology; /* SVGA_3D_CMD_DX_SET_TOPOLOGY */

typedef
struct SVGA3dCmdDXSetRenderTargets {
   SVGA3dDepthStencilViewId depthStencilViewId;
   /* Followed by a variable number of SVGA3dRenderTargetViewId's. */
}
SVGA3dCmdDXSetRenderTargets; /* SVGA_3D_CMD_DX_SET_RENDERTARGETS */

typedef
struct SVGA3dCmdDXSetBlendState {
   SVGA3dBlendStateId blendId;
   float blendFactor[4];
   uint32 sampleMask;
}
SVGA3dCmdDXSetBlendState; /* SVGA_3D_CMD_DX_SET_BLEND_STATE */

typedef
struct SVGA3dCmdDXSetDepthStencilState {
   SVGA3dDepthStencilStateId depthStencilId;
   uint32 stencilRef;
}
SVGA3dCmdDXSetDepthStencilState; /* SVGA_3D_CMD_DX_SET_DEPTHSTENCIL_STATE */

typedef
struct SVGA3dCmdDXSetRasterizerState {
   SVGA3dRasterizerStateId rasterizerId;
}
SVGA3dCmdDXSetRasterizerState; /* SVGA_3D_CMD_DX_SET_RASTERIZER_STATE */

/* Matches D3D10DDI_QUERY_MISCFLAG and D3D10_QUERY_MISC_FLAG */
#define SVGA3D_DXQUERY_FLAG_PREDICATEHINT (1 << 0)
typedef uint32 SVGA3dDXQueryFlags;

/*
 * The SVGADXQueryDeviceState and SVGADXQueryDeviceBits are used by the device
 * to track query state transitions, but are not intended to be used by the
 * driver.
 */
#define SVGADX_QDSTATE_INVALID   ((uint8)-1) /* Query has no state */
#define SVGADX_QDSTATE_MIN       0
#define SVGADX_QDSTATE_IDLE      0   /* Query hasn't started yet */
#define SVGADX_QDSTATE_ACTIVE    1   /* Query is actively gathering data */
#define SVGADX_QDSTATE_PENDING   2   /* Query is waiting for results */
#define SVGADX_QDSTATE_FINISHED  3   /* Query has completed */
#define SVGADX_QDSTATE_MAX       4
typedef uint8 SVGADXQueryDeviceState;

typedef
struct {
   SVGA3dQueryTypeUint8 type;
   uint16 pad0;
   SVGADXQueryDeviceState state;
   SVGA3dDXQueryFlags flags;
   SVGAMobId mobid;
   uint32 offset;
}
SVGACOTableDXQueryEntry;

typedef
struct SVGA3dCmdDXDefineQuery {
   SVGA3dQueryId queryId;
   SVGA3dQueryType type;
   SVGA3dDXQueryFlags flags;
}
SVGA3dCmdDXDefineQuery; /* SVGA_3D_CMD_DX_DEFINE_QUERY */

typedef
struct SVGA3dCmdDXDestroyQuery {
   SVGA3dQueryId queryId;
}
SVGA3dCmdDXDestroyQuery; /* SVGA_3D_CMD_DX_DESTROY_QUERY */

typedef
struct SVGA3dCmdDXBindQuery {
   SVGA3dQueryId queryId;
   SVGAMobId mobid;
}
SVGA3dCmdDXBindQuery; /* SVGA_3D_CMD_DX_BIND_QUERY */

typedef
struct SVGA3dCmdDXSetQueryOffset {
   SVGA3dQueryId queryId;
   uint32 mobOffset;
}
SVGA3dCmdDXSetQueryOffset; /* SVGA_3D_CMD_DX_SET_QUERY_OFFSET */

typedef
struct SVGA3dCmdDXBeginQuery {
   SVGA3dQueryId queryId;
}
SVGA3dCmdDXBeginQuery; /* SVGA_3D_CMD_DX_QUERY_BEGIN */

typedef
struct SVGA3dCmdDXEndQuery {
   SVGA3dQueryId queryId;
}
SVGA3dCmdDXEndQuery; /* SVGA_3D_CMD_DX_QUERY_END */

typedef
struct SVGA3dCmdDXReadbackQuery {
   SVGA3dQueryId queryId;
}
SVGA3dCmdDXReadbackQuery; /* SVGA_3D_CMD_DX_READBACK_QUERY */

typedef
struct SVGA3dCmdDXMoveQuery {
   SVGA3dQueryId queryId;
   SVGAMobId mobid;
   uint32 mobOffset;
}
SVGA3dCmdDXMoveQuery; /* SVGA_3D_CMD_DX_MOVE_QUERY */

typedef
struct SVGA3dCmdDXBindAllQuery {
   uint32 cid;
   SVGAMobId mobid;
}
SVGA3dCmdDXBindAllQuery; /* SVGA_3D_CMD_DX_BIND_ALL_QUERY */

typedef
struct SVGA3dCmdDXReadbackAllQuery {
   uint32 cid;
}
SVGA3dCmdDXReadbackAllQuery; /* SVGA_3D_CMD_DX_READBACK_ALL_QUERY */

typedef
struct SVGA3dCmdDXSetPredication {
   SVGA3dQueryId queryId;
   uint32 predicateValue;
}
SVGA3dCmdDXSetPredication; /* SVGA_3D_CMD_DX_SET_PREDICATION */

typedef
struct MKS3dDXSOState {
   uint32 offset;       /* Starting offset */
   uint32 intOffset;    /* Internal offset */
   uint32 vertexCount;  /* vertices written */
   uint32 sizeInBytes;  /* max bytes to write */
}
SVGA3dDXSOState;

/* Set the offset field to this value to append SO values to the buffer */
#define SVGA3D_DX_SO_OFFSET_APPEND ((uint32) ~0u)

typedef
struct SVGA3dSoTarget {
   SVGA3dSurfaceId sid;
   uint32 offset;
   uint32 sizeInBytes;
}
SVGA3dSoTarget;

typedef
struct SVGA3dCmdDXSetSOTargets {
   uint32 pad0;
   /* Followed by a variable number of SVGA3dSOTarget's. */
}
SVGA3dCmdDXSetSOTargets; /* SVGA_3D_CMD_DX_SET_SOTARGETS */

typedef
struct SVGA3dViewport
{
   float x;
   float y;
   float width;
   float height;
   float minDepth;
   float maxDepth;
}
SVGA3dViewport;

typedef
struct SVGA3dCmdDXSetViewports {
   uint32 pad0;
   /* Followed by a variable number of SVGA3dViewport's. */
}
SVGA3dCmdDXSetViewports; /* SVGA_3D_CMD_DX_SET_VIEWPORTS */

#define SVGA3D_DX_MAX_VIEWPORTS  16

typedef
struct SVGA3dCmdDXSetScissorRects {
   uint32 pad0;
   /* Followed by a variable number of SVGASignedRect's. */
}
SVGA3dCmdDXSetScissorRects; /* SVGA_3D_CMD_DX_SET_SCISSORRECTS */

#define SVGA3D_DX_MAX_SCISSORRECTS  16

typedef
struct SVGA3dCmdDXClearRenderTargetView {
   SVGA3dRenderTargetViewId renderTargetViewId;
   SVGA3dRGBAFloat rgba;
}
SVGA3dCmdDXClearRenderTargetView; /* SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW */

typedef
struct SVGA3dCmdDXClearDepthStencilView {
   uint16 flags;
   uint16 stencil;
   SVGA3dDepthStencilViewId depthStencilViewId;
   float depth;
}
SVGA3dCmdDXClearDepthStencilView; /* SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW */

typedef
struct SVGA3dCmdDXPredCopyRegion {
   SVGA3dSurfaceId dstSid;
   uint32 dstSubResource;
   SVGA3dSurfaceId srcSid;
   uint32 srcSubResource;
   SVGA3dCopyBox box;
}
SVGA3dCmdDXPredCopyRegion;
/* SVGA_3D_CMD_DX_PRED_COPY_REGION */

typedef
struct SVGA3dCmdDXPredCopy {
   SVGA3dSurfaceId dstSid;
   SVGA3dSurfaceId srcSid;
}
SVGA3dCmdDXPredCopy; /* SVGA_3D_CMD_DX_PRED_COPY */

typedef
struct SVGA3dCmdDXPredConvertRegion {
   SVGA3dSurfaceId dstSid;
   uint32 dstSubResource;
   SVGA3dSurfaceId srcSid;
   uint32 srcSubResource;
   SVGA3dCopyBox box;
}
SVGA3dCmdDXPredConvertRegion; /* SVGA_3D_CMD_DX_PRED_CONVERT_REGION */

typedef
struct SVGA3dCmdDXPredConvert {
   SVGA3dSurfaceId dstSid;
   SVGA3dSurfaceId srcSid;
}
SVGA3dCmdDXPredConvert; /* SVGA_3D_CMD_DX_PRED_CONVERT */

typedef
struct SVGA3dCmdDXBufferCopy {
   SVGA3dSurfaceId dest;
   SVGA3dSurfaceId src;
   uint32 destX;
   uint32 srcX;
   uint32 width;
}
SVGA3dCmdDXBufferCopy;
/* SVGA_3D_CMD_DX_BUFFER_COPY */

/*
 * Perform a surface copy between a multisample, and a non-multisampled
 * surface.
 */
typedef
struct {
   SVGA3dSurfaceId dstSid;
   uint32 dstSubResource;
   SVGA3dSurfaceId srcSid;
   uint32 srcSubResource;
   SVGA3dSurfaceFormat copyFormat;
}
SVGA3dCmdDXResolveCopy;               /* SVGA_3D_CMD_DX_RESOLVE_COPY */

/*
 * Perform a predicated surface copy between a multisample, and a
 * non-multisampled surface.
 */
typedef
struct {
   SVGA3dSurfaceId dstSid;
   uint32 dstSubResource;
   SVGA3dSurfaceId srcSid;
   uint32 srcSubResource;
   SVGA3dSurfaceFormat copyFormat;
}
SVGA3dCmdDXPredResolveCopy;           /* SVGA_3D_CMD_DX_PRED_RESOLVE_COPY */

typedef uint32 SVGA3dDXStretchBltMode;
#define SVGADX_STRETCHBLT_LINEAR         (1 << 0)
#define SVGADX_STRETCHBLT_FORCE_SRC_SRGB (1 << 1)
#define SVGADX_STRETCHBLT_MODE_MAX       (1 << 2)

typedef
struct SVGA3dCmdDXStretchBlt {
   SVGA3dSurfaceId srcSid;
   uint32 srcSubResource;
   SVGA3dSurfaceId dstSid;
   uint32 destSubResource;
   SVGA3dBox boxSrc;
   SVGA3dBox boxDest;
   SVGA3dDXStretchBltMode mode;
}
SVGA3dCmdDXStretchBlt; /* SVGA_3D_CMD_DX_STRETCHBLT */

typedef
struct SVGA3dCmdDXGenMips {
   SVGA3dShaderResourceViewId shaderResourceViewId;
}
SVGA3dCmdDXGenMips; /* SVGA_3D_CMD_DX_GENMIPS */

/*
 * Update a sub-resource in a guest-backed resource.
 * (Inform the device that the guest-contents have been updated.)
 */
typedef
struct SVGA3dCmdDXUpdateSubResource {
   SVGA3dSurfaceId sid;
   uint32 subResource;
   SVGA3dBox box;
}
SVGA3dCmdDXUpdateSubResource;   /* SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE */

/*
 * Readback a subresource in a guest-backed resource.
 * (Request the device to flush the dirty contents into the guest.)
 */
typedef
struct SVGA3dCmdDXReadbackSubResource {
   SVGA3dSurfaceId sid;
   uint32 subResource;
}
SVGA3dCmdDXReadbackSubResource;   /* SVGA_3D_CMD_DX_READBACK_SUBRESOURCE */

/*
 * Invalidate an image in a guest-backed surface.
 * (Notify the device that the contents can be lost.)
 */
typedef
struct SVGA3dCmdDXInvalidateSubResource {
   SVGA3dSurfaceId sid;
   uint32 subResource;
}
SVGA3dCmdDXInvalidateSubResource;   /* SVGA_3D_CMD_DX_INVALIDATE_SUBRESOURCE */


/*
 * Raw byte wise transfer from a buffer surface into another surface
 * of the requested box.  Supported if 3d is enabled and SVGA_CAP_DX
 * is set.  This command does not take a context.
 */
typedef
struct SVGA3dCmdDXTransferFromBuffer {
   SVGA3dSurfaceId srcSid;
   uint32 srcOffset;
   uint32 srcPitch;
   uint32 srcSlicePitch;
   SVGA3dSurfaceId destSid;
   uint32 destSubResource;
   SVGA3dBox destBox;
}
SVGA3dCmdDXTransferFromBuffer;   /* SVGA_3D_CMD_DX_TRANSFER_FROM_BUFFER */


/*
 * Raw byte wise transfer from a buffer surface into another surface
 * of the requested box.  Supported if SVGA3D_DEVCAP_DXCONTEXT is set.
 * The context is implied from the command buffer header.
 */
typedef
struct SVGA3dCmdDXPredTransferFromBuffer {
   SVGA3dSurfaceId srcSid;
   uint32 srcOffset;
   uint32 srcPitch;
   uint32 srcSlicePitch;
   SVGA3dSurfaceId destSid;
   uint32 destSubResource;
   SVGA3dBox destBox;
}
SVGA3dCmdDXPredTransferFromBuffer;
/* SVGA_3D_CMD_DX_PRED_TRANSFER_FROM_BUFFER */


typedef
struct SVGA3dCmdDXSurfaceCopyAndReadback {
   SVGA3dSurfaceId srcSid;
   SVGA3dSurfaceId destSid;
   SVGA3dCopyBox box;
}
SVGA3dCmdDXSurfaceCopyAndReadback;
/* SVGA_3D_CMD_DX_SURFACE_COPY_AND_READBACK */

/*
 * SVGA_DX_HINT_NONE: Does nothing.
 *
 * SVGA_DX_HINT_PREFETCH_OBJECT:
 * SVGA_DX_HINT_PREEVICT_OBJECT:
 *      Consumes a SVGAObjectRef, and hints that the host should consider
 *      fetching/evicting the specified object.
 *
 *      An id of SVGA3D_INVALID_ID can be used if the guest isn't sure
 *      what object was affected.  (For instance, if the guest knows that
 *      it is about to evict a DXShader, but doesn't know precisely which one,
 *      the device can still use this to help limit it's search, or track
 *      how many page-outs have happened.)
 *
 * SVGA_DX_HINT_PREFETCH_COBJECT:
 * SVGA_DX_HINT_PREEVICT_COBJECT:
 *      Same as the above, except they consume an SVGACObjectRef.
 */
typedef uint32 SVGADXHintId;
#define SVGA_DX_HINT_NONE              0
#define SVGA_DX_HINT_PREFETCH_OBJECT   1
#define SVGA_DX_HINT_PREEVICT_OBJECT   2
#define SVGA_DX_HINT_PREFETCH_COBJECT  3
#define SVGA_DX_HINT_PREEVICT_COBJECT  4
#define SVGA_DX_HINT_MAX               5

typedef
struct SVGAObjectRef {
   SVGAOTableType type;
   uint32 id;
}
SVGAObjectRef;

typedef
struct SVGACObjectRef {
   SVGACOTableType type;
   uint32 cid;
   uint32 id;
}
SVGACObjectRef;

typedef
struct SVGA3dCmdDXHint {
   SVGADXHintId hintId;

   /*
    * Followed by variable sized data depending on the hintId.
    */
}
SVGA3dCmdDXHint;
/* SVGA_3D_CMD_DX_HINT */

typedef
struct SVGA3dCmdDXBufferUpdate {
   SVGA3dSurfaceId sid;
   uint32 x;
   uint32 width;
}
SVGA3dCmdDXBufferUpdate;
/* SVGA_3D_CMD_DX_BUFFER_UPDATE */

typedef
struct SVGA3dCmdDXSetConstantBufferOffset {
   uint32 slot;
   uint32 offsetInBytes;
}
SVGA3dCmdDXSetConstantBufferOffset;

typedef SVGA3dCmdDXSetConstantBufferOffset SVGA3dCmdDXSetVSConstantBufferOffset;
/* SVGA_3D_CMD_DX_SET_VS_CONSTANT_BUFFER_OFFSET */

typedef SVGA3dCmdDXSetConstantBufferOffset SVGA3dCmdDXSetPSConstantBufferOffset;
/* SVGA_3D_CMD_DX_SET_PS_CONSTANT_BUFFER_OFFSET */

typedef SVGA3dCmdDXSetConstantBufferOffset SVGA3dCmdDXSetGSConstantBufferOffset;
/* SVGA_3D_CMD_DX_SET_GS_CONSTANT_BUFFER_OFFSET */


typedef
struct {
   union {
      struct {
         uint32 firstElement;
         uint32 numElements;
         uint32 pad0;
         uint32 pad1;
      } buffer;
      struct {
         uint32 mostDetailedMip;
         uint32 firstArraySlice;
         uint32 mipLevels;
         uint32 arraySize;
      } tex;
      struct {
         uint32 firstElement;  // D3D11DDIARG_BUFFEREX_SHADERRESOURCEVIEW
         uint32 numElements;
         uint32 flags;
         uint32 pad0;
      } bufferex;
   };
}
SVGA3dShaderResourceViewDesc;

typedef
struct {
   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;
   SVGA3dShaderResourceViewDesc desc;
   uint32 pad;
}
SVGACOTableDXSRViewEntry;

typedef
struct SVGA3dCmdDXDefineShaderResourceView {
   SVGA3dShaderResourceViewId shaderResourceViewId;

   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;

   SVGA3dShaderResourceViewDesc desc;
}
SVGA3dCmdDXDefineShaderResourceView;
/* SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW */

typedef
struct SVGA3dCmdDXDestroyShaderResourceView {
   SVGA3dShaderResourceViewId shaderResourceViewId;
}
SVGA3dCmdDXDestroyShaderResourceView;
/* SVGA_3D_CMD_DX_DESTROY_SHADERRESOURCE_VIEW */

typedef
struct SVGA3dRenderTargetViewDesc {
   union {
      struct {
         uint32 firstElement;
         uint32 numElements;
      } buffer;
      struct {
         uint32 mipSlice;
         uint32 firstArraySlice;
         uint32 arraySize;
      } tex;                    /* 1d, 2d, cube */
      struct {
         uint32 mipSlice;
         uint32 firstW;
         uint32 wSize;
      } tex3D;
   };
}
SVGA3dRenderTargetViewDesc;

typedef
struct {
   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;
   SVGA3dRenderTargetViewDesc desc;
   uint32 pad[2];
}
SVGACOTableDXRTViewEntry;

typedef
struct SVGA3dCmdDXDefineRenderTargetView {
   SVGA3dRenderTargetViewId renderTargetViewId;

   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;

   SVGA3dRenderTargetViewDesc desc;
}
SVGA3dCmdDXDefineRenderTargetView;
/* SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW */

typedef
struct SVGA3dCmdDXDestroyRenderTargetView {
   SVGA3dRenderTargetViewId renderTargetViewId;
}
SVGA3dCmdDXDestroyRenderTargetView;
/* SVGA_3D_CMD_DX_DESTROY_RENDERTARGET_VIEW */

/*
 * Create Depth-stencil view flags
 * http://msdn.microsoft.com/en-us/library/windows/hardware/ff542167(v=vs.85).aspx
 */
#define SVGA3D_DXDSVIEW_CREATE_READ_ONLY_DEPTH   0x01
#define SVGA3D_DXDSVIEW_CREATE_READ_ONLY_STENCIL 0x02
#define SVGA3D_DXDSVIEW_CREATE_FLAG_MASK         0x03
typedef uint8 SVGA3DCreateDSViewFlags;

typedef
struct {
   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;
   uint32 mipSlice;
   uint32 firstArraySlice;
   uint32 arraySize;
   SVGA3DCreateDSViewFlags flags;
   uint8 pad0;
   uint16 pad1;
   uint32 pad2;
}
SVGACOTableDXDSViewEntry;

typedef
struct SVGA3dCmdDXDefineDepthStencilView {
   SVGA3dDepthStencilViewId depthStencilViewId;

   SVGA3dSurfaceId sid;
   SVGA3dSurfaceFormat format;
   SVGA3dResourceType resourceDimension;
   uint32 mipSlice;
   uint32 firstArraySlice;
   uint32 arraySize;
   SVGA3DCreateDSViewFlags flags;  /* D3D11DDIARG_CREATEDEPTHSTENCILVIEW */
   uint8 pad0;
   uint16 pad1;
}
SVGA3dCmdDXDefineDepthStencilView;
/* SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW */

typedef
struct SVGA3dCmdDXDestroyDepthStencilView {
   SVGA3dDepthStencilViewId depthStencilViewId;
}
SVGA3dCmdDXDestroyDepthStencilView;
/* SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_VIEW */

typedef
struct SVGA3dInputElementDesc {
   uint32 inputSlot;
   uint32 alignedByteOffset;
   SVGA3dSurfaceFormat format;
   SVGA3dInputClassification inputSlotClass;
   uint32 instanceDataStepRate;
   uint32 inputRegister;
}
SVGA3dInputElementDesc;

typedef
struct {
   uint32 elid;
   uint32 numDescs;
   SVGA3dInputElementDesc descs[32];
   uint32 pad[62];
}
SVGACOTableDXElementLayoutEntry;

typedef
struct SVGA3dCmdDXDefineElementLayout {
   SVGA3dElementLayoutId elementLayoutId;
   /* Followed by a variable number of SVGA3dInputElementDesc's. */
}
SVGA3dCmdDXDefineElementLayout;
/* SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT */

typedef
struct SVGA3dCmdDXDestroyElementLayout {
   SVGA3dElementLayoutId elementLayoutId;
}
SVGA3dCmdDXDestroyElementLayout;
/* SVGA_3D_CMD_DX_DESTROY_ELEMENTLAYOUT */


#define SVGA3D_DX_MAX_RENDER_TARGETS 8

typedef
struct SVGA3dDXBlendStatePerRT {
      uint8 blendEnable;
      uint8 srcBlend;
      uint8 destBlend;
      uint8 blendOp;
      uint8 srcBlendAlpha;
      uint8 destBlendAlpha;
      uint8 blendOpAlpha;
      SVGA3dColorWriteEnable renderTargetWriteMask;
      uint8 logicOpEnable;
      uint8 logicOp;
      uint16 pad0;
}
SVGA3dDXBlendStatePerRT;

typedef
struct {
   uint8 alphaToCoverageEnable;
   uint8 independentBlendEnable;
   uint16 pad0;
   SVGA3dDXBlendStatePerRT perRT[SVGA3D_MAX_RENDER_TARGETS];
   uint32 pad1[7];
}
SVGACOTableDXBlendStateEntry;

/*
 * XXX - DX10 style (not 10.1 at this point)
 * XXX - For more information see
 *    http://msdn.microsoft.com/en-us/library/ff541919%28v=VS.85%29.aspx
 */
typedef
struct SVGA3dCmdDXDefineBlendState {
   SVGA3dBlendStateId blendId;
   uint8 alphaToCoverageEnable;
   uint8 independentBlendEnable;
   uint16 pad0;
   SVGA3dDXBlendStatePerRT perRT[SVGA3D_MAX_RENDER_TARGETS];
}
SVGA3dCmdDXDefineBlendState; /* SVGA_3D_CMD_DX_DEFINE_BLEND_STATE */

typedef
struct SVGA3dCmdDXDestroyBlendState {
   SVGA3dBlendStateId blendId;
}
SVGA3dCmdDXDestroyBlendState; /* SVGA_3D_CMD_DX_DESTROY_BLEND_STATE */

typedef
struct {
   uint8 depthEnable;
   SVGA3dDepthWriteMask depthWriteMask;
   SVGA3dComparisonFunc depthFunc;
   uint8 stencilEnable;
   uint8 frontEnable;
   uint8 backEnable;
   uint8 stencilReadMask;
   uint8 stencilWriteMask;

   uint8 frontStencilFailOp;
   uint8 frontStencilDepthFailOp;
   uint8 frontStencilPassOp;
   SVGA3dComparisonFunc frontStencilFunc;

   uint8 backStencilFailOp;
   uint8 backStencilDepthFailOp;
   uint8 backStencilPassOp;
   SVGA3dComparisonFunc backStencilFunc;
}
SVGACOTableDXDepthStencilEntry;

/*
 * XXX - For more information see
 *    http://msdn.microsoft.com/en-us/library/ff541944%28v=VS.85%29.aspx
 */
typedef
struct SVGA3dCmdDXDefineDepthStencilState {
   SVGA3dDepthStencilStateId depthStencilId;

   uint8 depthEnable;
   SVGA3dDepthWriteMask depthWriteMask;
   SVGA3dComparisonFunc depthFunc;
   uint8 stencilEnable;
   uint8 frontEnable;
   uint8 backEnable;
   uint8 stencilReadMask;
   uint8 stencilWriteMask;

   uint8 frontStencilFailOp;
   uint8 frontStencilDepthFailOp;
   uint8 frontStencilPassOp;
   SVGA3dComparisonFunc frontStencilFunc;

   uint8 backStencilFailOp;
   uint8 backStencilDepthFailOp;
   uint8 backStencilPassOp;
   SVGA3dComparisonFunc backStencilFunc;
}
SVGA3dCmdDXDefineDepthStencilState;
/* SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_STATE */

typedef
struct SVGA3dCmdDXDestroyDepthStencilState {
   SVGA3dDepthStencilStateId depthStencilId;
}
SVGA3dCmdDXDestroyDepthStencilState;
/* SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_STATE */

typedef
struct {
   uint8 fillMode;
   SVGA3dCullMode cullMode;
   uint8 frontCounterClockwise;
   uint8 provokingVertexLast;
   int32 depthBias;
   float depthBiasClamp;
   float slopeScaledDepthBias;
   uint8 depthClipEnable;
   uint8 scissorEnable;
   SVGA3dMultisampleEnable multisampleEnable;
   uint8 antialiasedLineEnable;
   float lineWidth;
   uint8 lineStippleEnable;
   uint8 lineStippleFactor;
   uint16 lineStipplePattern;
   uint32 forcedSampleCount;
}
SVGACOTableDXRasterizerStateEntry;

/*
 * XXX - For more information see
 *    http://msdn.microsoft.com/en-us/library/ff541988%28v=VS.85%29.aspx
 */
typedef
struct SVGA3dCmdDXDefineRasterizerState {
   SVGA3dRasterizerStateId rasterizerId;

   uint8 fillMode;
   SVGA3dCullMode cullMode;
   uint8 frontCounterClockwise;
   uint8 provokingVertexLast;
   int32 depthBias;
   float depthBiasClamp;
   float slopeScaledDepthBias;
   uint8 depthClipEnable;
   uint8 scissorEnable;
   SVGA3dMultisampleEnable multisampleEnable;
   uint8 antialiasedLineEnable;
   float lineWidth;
   uint8 lineStippleEnable;
   uint8 lineStippleFactor;
   uint16 lineStipplePattern;
}
SVGA3dCmdDXDefineRasterizerState;
/* SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE */

typedef
struct SVGA3dCmdDXDestroyRasterizerState {
   SVGA3dRasterizerStateId rasterizerId;
}
SVGA3dCmdDXDestroyRasterizerState;
/* SVGA_3D_CMD_DX_DESTROY_RASTERIZER_STATE */

typedef
struct {
   SVGA3dFilter filter;
   uint8 addressU;
   uint8 addressV;
   uint8 addressW;
   uint8 pad0;
   float mipLODBias;
   uint8 maxAnisotropy;
   SVGA3dComparisonFunc comparisonFunc;
   uint16 pad1;
   SVGA3dRGBAFloat borderColor;
   float minLOD;
   float maxLOD;
   uint32 pad2[6];
}
SVGACOTableDXSamplerEntry;

/*
 * XXX - For more information see
 *    http://msdn.microsoft.com/en-us/library/ff542011%28v=VS.85%29.aspx
 */
typedef
struct SVGA3dCmdDXDefineSamplerState {
   SVGA3dSamplerId samplerId;
   SVGA3dFilter filter;
   uint8 addressU;
   uint8 addressV;
   uint8 addressW;
   uint8 pad0;
   float mipLODBias;
   uint8 maxAnisotropy;
   SVGA3dComparisonFunc comparisonFunc;
   uint16 pad1;
   SVGA3dRGBAFloat borderColor;
   float minLOD;
   float maxLOD;
}
SVGA3dCmdDXDefineSamplerState; /* SVGA_3D_CMD_DX_DEFINE_SAMPLER_STATE */

typedef
struct SVGA3dCmdDXDestroySamplerState {
   SVGA3dSamplerId samplerId;
}
SVGA3dCmdDXDestroySamplerState; /* SVGA_3D_CMD_DX_DESTROY_SAMPLER_STATE */


#define SVGADX_SIGNATURE_SEMANTIC_NAME_UNDEFINED                          0
#define SVGADX_SIGNATURE_SEMANTIC_NAME_POSITION                           1
#define SVGADX_SIGNATURE_SEMANTIC_NAME_CLIP_DISTANCE                      2
#define SVGADX_SIGNATURE_SEMANTIC_NAME_CULL_DISTANCE                      3
#define SVGADX_SIGNATURE_SEMANTIC_NAME_RENDER_TARGET_ARRAY_INDEX          4
#define SVGADX_SIGNATURE_SEMANTIC_NAME_VIEWPORT_ARRAY_INDEX               5
#define SVGADX_SIGNATURE_SEMANTIC_NAME_VERTEX_ID                          6
#define SVGADX_SIGNATURE_SEMANTIC_NAME_PRIMITIVE_ID                       7
#define SVGADX_SIGNATURE_SEMANTIC_NAME_INSTANCE_ID                        8
#define SVGADX_SIGNATURE_SEMANTIC_NAME_IS_FRONT_FACE                      9
#define SVGADX_SIGNATURE_SEMANTIC_NAME_SAMPLE_INDEX                       10
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_U_EQ_0_EDGE_TESSFACTOR  11
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_V_EQ_0_EDGE_TESSFACTOR  12
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_U_EQ_1_EDGE_TESSFACTOR  13
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_V_EQ_1_EDGE_TESSFACTOR  14
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_U_INSIDE_TESSFACTOR     15
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_QUAD_V_INSIDE_TESSFACTOR     16
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_TRI_U_EQ_0_EDGE_TESSFACTOR   17
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_TRI_V_EQ_0_EDGE_TESSFACTOR   18
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_TRI_W_EQ_0_EDGE_TESSFACTOR   19
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_TRI_INSIDE_TESSFACTOR        20
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_LINE_DETAIL_TESSFACTOR       21
#define SVGADX_SIGNATURE_SEMANTIC_NAME_FINAL_LINE_DENSITY_TESSFACTOR      22
#define SVGADX_SIGNATURE_SEMANTIC_NAME_MAX                                23
typedef uint32 SVGA3dDXSignatureSemanticName;

#define SVGADX_SIGNATURE_REGISTER_COMPONENT_UNKNOWN 0
typedef uint32 SVGA3dDXSignatureRegisterComponentType;

#define SVGADX_SIGNATURE_MIN_PRECISION_DEFAULT 0
typedef uint32 SVGA3dDXSignatureMinPrecision;

typedef
struct SVGA3dDXSignatureEntry {
   uint32 registerIndex;
   SVGA3dDXSignatureSemanticName semanticName;
   uint32 mask; /* Lower 4 bits represent X, Y, Z, W channels */
   SVGA3dDXSignatureRegisterComponentType componentType;
   SVGA3dDXSignatureMinPrecision minPrecision;
}
SVGA3dDXShaderSignatureEntry;

#define SVGADX_SIGNATURE_HEADER_VERSION_0 0x08a92d12

/*
 * The SVGA3dDXSignatureHeader structure is added after the shader
 * body in the mob that is bound to the shader.  It is followed by the
 * specified number of SVGA3dDXSignatureEntry structures for each of
 * the three types of signatures in the order (input, output, patch
 * constants).
 */
typedef
struct SVGA3dDXSignatureHeader {
   uint32 headerVersion;
   uint32 numInputSignatures;
   uint32 numOutputSignatures;
   uint32 numPatchConstantSignatures;
}
SVGA3dDXShaderSignatureHeader;


typedef
struct SVGA3dCmdDXDefineShader {
   SVGA3dShaderId shaderId;
   SVGA3dShaderType type;
   uint32 sizeInBytes; /* Number of bytes of shader text. */
}
SVGA3dCmdDXDefineShader; /* SVGA_3D_CMD_DX_DEFINE_SHADER */

typedef
struct SVGACOTableDXShaderEntry {
   SVGA3dShaderType type;
   uint32 sizeInBytes;
   uint32 offsetInBytes;
   SVGAMobId mobid;
   uint32 pad[4];
}
SVGACOTableDXShaderEntry;

typedef
struct SVGA3dCmdDXDestroyShader {
   SVGA3dShaderId shaderId;
}
SVGA3dCmdDXDestroyShader; /* SVGA_3D_CMD_DX_DESTROY_SHADER */

typedef
struct SVGA3dCmdDXBindShader {
   uint32 cid;
   uint32 shid;
   SVGAMobId mobid;
   uint32 offsetInBytes;
}
SVGA3dCmdDXBindShader;   /* SVGA_3D_CMD_DX_BIND_SHADER */

typedef
struct SVGA3dCmdDXBindAllShader {
   uint32 cid;
   SVGAMobId mobid;
}
SVGA3dCmdDXBindAllShader;   /* SVGA_3D_CMD_DX_BIND_ALL_SHADER */

typedef
struct SVGA3dCmdDXCondBindAllShader {
   uint32 cid;
   SVGAMobId testMobid;
   SVGAMobId mobid;
}
SVGA3dCmdDXCondBindAllShader;   /* SVGA_3D_CMD_DX_COND_BIND_ALL_SHADER */

/*
 * The maximum number of streamout decl's in each streamout entry.
 */
#define SVGA3D_MAX_DX10_STREAMOUT_DECLS 64
#define SVGA3D_MAX_STREAMOUT_DECLS 512

typedef
struct SVGA3dStreamOutputDeclarationEntry {
   uint32 outputSlot;
   uint32 registerIndex;
   uint8  registerMask;
   uint8  pad0;
   uint16 pad1;
   uint32 stream;
}
SVGA3dStreamOutputDeclarationEntry;

typedef
struct SVGAOTableStreamOutputEntry {
   uint32 numOutputStreamEntries;
   SVGA3dStreamOutputDeclarationEntry decl[SVGA3D_MAX_DX10_STREAMOUT_DECLS];
   uint32 streamOutputStrideInBytes[SVGA3D_DX_MAX_SOTARGETS];
   uint32 rasterizedStream;
   uint32 numOutputStreamStrides;
   uint32 mobid;
   uint32 offsetInBytes;
   uint8 usesMob;
   uint8 pad0;
   uint16 pad1;
   uint32 pad2[246];
}
SVGACOTableDXStreamOutputEntry;

typedef
struct SVGA3dCmdDXDefineStreamOutput {
   SVGA3dStreamOutputId soid;
   uint32 numOutputStreamEntries;
   SVGA3dStreamOutputDeclarationEntry decl[SVGA3D_MAX_DX10_STREAMOUT_DECLS];
   uint32 streamOutputStrideInBytes[SVGA3D_DX_MAX_SOTARGETS];
   uint32 rasterizedStream;
}
SVGA3dCmdDXDefineStreamOutput; /* SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT */

/*
 * Version 2 needed in order to start validating and using the
 * rasterizedStream field.  Unfortunately the device wasn't validating
 * or using this field and the driver wasn't initializing it in shipped
 * code, so a new version of the command is needed to allow that code
 * to continue to work.  Also added new numOutputStreamStrides field.
 */

#define SVGA3D_DX_SO_NO_RASTERIZED_STREAM 0xFFFFFFFF

typedef
struct SVGA3dCmdDXDefineStreamOutputWithMob {
   SVGA3dStreamOutputId soid;
   uint32 numOutputStreamEntries;
   uint32 numOutputStreamStrides;
   uint32 streamOutputStrideInBytes[SVGA3D_DX_MAX_SOTARGETS];
   uint32 rasterizedStream;
}
SVGA3dCmdDXDefineStreamOutputWithMob;
/* SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT_WITH_MOB */

typedef
struct SVGA3dCmdDXBindStreamOutput {
   SVGA3dStreamOutputId soid;
   uint32 mobid;
   uint32 offsetInBytes;
   uint32 sizeInBytes;
}
SVGA3dCmdDXBindStreamOutput; /* SVGA_3D_CMD_DX_BIND_STREAMOUTPUT */

typedef
struct SVGA3dCmdDXDestroyStreamOutput {
   SVGA3dStreamOutputId soid;
}
SVGA3dCmdDXDestroyStreamOutput; /* SVGA_3D_CMD_DX_DESTROY_STREAMOUTPUT */

typedef
struct SVGA3dCmdDXSetStreamOutput {
   SVGA3dStreamOutputId soid;
}
SVGA3dCmdDXSetStreamOutput; /* SVGA_3D_CMD_DX_SET_STREAMOUTPUT */

typedef
struct SVGA3dCmdDXSetMinLOD {
   SVGA3dSurfaceId sid;
   float minLOD;
}
SVGA3dCmdDXSetMinLOD; /* SVGA_3D_CMD_DX_SET_MIN_LOD */

typedef
struct {
   uint64 value;
   uint32 mobId;
   uint32 mobOffset;
}
SVGA3dCmdDXMobFence64;  /* SVGA_3D_CMD_DX_MOB_FENCE_64 */

/*
 * SVGA3dCmdSetCOTable --
 *
 * This command allows the guest to bind a mob to a context-object table.
 */
typedef
struct SVGA3dCmdDXSetCOTable {
   uint32 cid;
   uint32 mobid;
   SVGACOTableType type;
   uint32 validSizeInBytes;
}
SVGA3dCmdDXSetCOTable; /* SVGA_3D_CMD_DX_SET_COTABLE */

/*
 * Guests using SVGA_3D_CMD_DX_GROW_COTABLE are promising that
 * the new COTable contains the same contents as the old one, except possibly
 * for some new invalid entries at the end.
 *
 * If there is an old cotable mob bound, it also has to still be valid.
 *
 * (Otherwise, guests should use the DXSetCOTableBase command.)
 */
typedef
struct SVGA3dCmdDXGrowCOTable {
   uint32 cid;
   uint32 mobid;
   SVGACOTableType type;
   uint32 validSizeInBytes;
}
SVGA3dCmdDXGrowCOTable; /* SVGA_3D_CMD_DX_GROW_COTABLE */

typedef
struct SVGA3dCmdDXReadbackCOTable {
   uint32 cid;
   SVGACOTableType type;
}
SVGA3dCmdDXReadbackCOTable; /* SVGA_3D_CMD_DX_READBACK_COTABLE */

typedef
struct SVGA3dCOTableData {
   uint32 mobid;
}
SVGA3dCOTableData;

typedef
struct SVGA3dBufferBinding {
   uint32 bufferId;
   uint32 stride;
   uint32 offset;
}
SVGA3dBufferBinding;

typedef
struct SVGA3dConstantBufferBinding {
   uint32 sid;
   uint32 offsetInBytes;
   uint32 sizeInBytes;
}
SVGA3dConstantBufferBinding;

typedef
struct SVGADXInputAssemblyMobFormat {
   uint32 layoutId;
   SVGA3dBufferBinding vertexBuffers[SVGA3D_DX_MAX_VERTEXBUFFERS];
   uint32 indexBufferSid;
   uint32 pad;
   uint32 indexBufferOffset;
   uint32 indexBufferFormat;
   uint32 topology;
}
SVGADXInputAssemblyMobFormat;

typedef
struct SVGADXContextMobFormat {
   SVGADXInputAssemblyMobFormat inputAssembly;

   struct {
      uint32 blendStateId;
      uint32 blendFactor[4];
      uint32 sampleMask;
      uint32 depthStencilStateId;
      uint32 stencilRef;
      uint32 rasterizerStateId;
      uint32 depthStencilViewId;
      uint32 renderTargetViewIds[SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS];
      uint32 unorderedAccessViewIds[SVGA3D_MAX_UAVIEWS];
   } renderState;

   struct {
      uint32 targets[SVGA3D_DX_MAX_SOTARGETS];
      uint32 soid;
   } streamOut;
   uint32 pad0[11];

   uint8 numViewports;
   uint8 numScissorRects;
   uint16 pad1[1];

   uint32 pad2[3];

   SVGA3dViewport viewports[SVGA3D_DX_MAX_VIEWPORTS];
   uint32 pad3[32];

   SVGASignedRect scissorRects[SVGA3D_DX_MAX_SCISSORRECTS];
   uint32 pad4[64];

   struct {
      uint32 queryID;
      uint32 value;
   } predication;
   uint32 pad5[2];

   struct {
      uint32 shaderId;
      SVGA3dConstantBufferBinding constantBuffers[SVGA3D_DX_MAX_CONSTBUFFERS];
      uint32 shaderResources[SVGA3D_DX_MAX_SRVIEWS];
      uint32 samplers[SVGA3D_DX_MAX_SAMPLERS];
   } shaderState[SVGA3D_NUM_SHADERTYPE];
   uint32 pad6[26];

   SVGA3dQueryId queryID[SVGA3D_MAX_QUERY];

   SVGA3dCOTableData cotables[SVGA_COTABLE_MAX];
   uint32 pad7[380];
}
SVGADXContextMobFormat;

#endif // _SVGA3D_DX_H_
