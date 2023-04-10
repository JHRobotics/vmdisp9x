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
 * svga3d.c --
 *
 *      FIFO command level interface to the SVGA3D protocol used by
 *      the VMware SVGA device.
 */

#include <string.h>
#include "svga_all.h"

#ifdef DBGPRINT
void dbg_printf( const char *s, ... );
#else
#define dbg_printf(...)
#endif

/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_Init --
 *
 *      Perform 3D-only initialization for the SVGA device.  This
 *      function negotiates SVGA3D protocol versions between the host
 *      and the guest.
 *
 *      The 3D protocol negotiation works as follows:
 *
 *      1. Before enabling the FIFO, the guest writes its 3D protocol
 *         version to the SVGA_FIFO_GUEST_3D_HWVERSION register.
 *         We do this step in SVGA_Init().
 *
 *      2. When the FIFO is enabled, the host checks our 3D version
 *         for compatibility. If the host can support our protocol,
 *         the host puts its protocol version in SVGA_FIFO_3D_HWVERSION.
 *         If the host cannot support our protocol, it leaves the
 *         version set to zero.
 *
 *      This function must be called after the FIFO is enabled.
 *      (After SVGA_SetMode.)
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Panic if the host is incompatible with 3D.
 *
 *----------------------------------------------------------------------
 */

Bool
SVGA3D_Init(void)
{
   SVGA3dHardwareVersion hwVersion;

   if (!(gSVGA.capabilities & SVGA_CAP_EXTENDED_FIFO)) {
      dbg_printf("3D requires the Extended FIFO capability.");
   }

   if (SVGA_HasFIFOCap(SVGA_FIFO_CAP_3D_HWVERSION_REVISED)) {
      hwVersion = gSVGA.fifoMem[SVGA_FIFO_3D_HWVERSION_REVISED];
   } else {
      if (gSVGA.fifoMem[SVGA_FIFO_MIN] <= sizeof(uint32) * SVGA_FIFO_GUEST_3D_HWVERSION) {
         dbg_printf("GUEST_3D_HWVERSION register not present.");
      }
      hwVersion = gSVGA.fifoMem[SVGA_FIFO_3D_HWVERSION];
   }

   /*
    * Check the host's version, make sure we're binary compatible.
    */

   if (hwVersion == 0) {
      dbg_printf("3D disabled by host.");
      return FALSE;
   }
   if (hwVersion < SVGA3D_HWVERSION_WS65_B1) {
      dbg_printf("Host SVGA3D protocol is too old, not binary compatible.");
      return FALSE;
   }
   
   return TRUE;
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_FIFOReserve --
 *
 *      Reserve space for an SVGA3D FIFO command.
 *
 *      The 2D SVGA commands have been around for a while, so they
 *      have a rather asymmetric structure. The SVGA3D protocol is
 *      more uniform: each command begins with a header containing the
 *      command number and the full size.
 *
 *      This is a convenience wrapper around SVGA_FIFOReserve. We
 *      reserve space for the whole command, and write the header.
 *
 *      This function must be paired with SVGA_FIFOCommitAll().
 *
 * Results:
 *      Returns a pointer to the space reserved for command-specific
 *      data. It must be 'cmdSize' bytes long.
 *
 * Side effects:
 *      Begins a FIFO reservation.
 *
 *----------------------------------------------------------------------
 */

void *
SVGA3D_FIFOReserve(uint32 cmd,      // IN
                   uint32 cmdSize)  // IN
{
   SVGA3dCmdHeader __far *header;

   header = SVGA_FIFOReserve(sizeof *header + cmdSize);
   header->id = cmd;
   header->size = cmdSize;

   return &header[1];
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginDefineSurface --
 *
 *      Begin a SURFACE_DEFINE command. This reserves space for it in
 *      the FIFO, and returns pointers to the command's faces and
 *      mipsizes arrays.
 *
 *      This function must be paired with SVGA_FIFOCommitAll().
 *      The faces and mipSizes arrays are initialized to zero.
 *
 *      This creates a "surface" object in the SVGA3D device,
 *      with the provided surface ID (sid). Surfaces are generic
 *      containers for host VRAM objects like textures, vertex
 *      buffers, and depth/stencil buffers.
 *
 *      Surfaces are hierarchial:
 *
 *        - Surface may have multiple faces (for cube maps)
 *
 *          - Each face has a list of mipmap levels
 *
 *             - Each mipmap image may have multiple volume
 *               slices, if the image is three dimensional.
 *
 *                - Each slice is a 2D array of 'blocks'
 *
 *                   - Each block may be one or more pixels.
 *                     (Usually 1, more for DXT or YUV formats.)
 *
 *      Surfaces are generic host VRAM objects. The SVGA3D device
 *      may optimize surfaces according to the format they were
 *      created with, but this format does not limit the ways in
 *      which the surface may be used. For example, a depth surface
 *      can be used as a texture, or a floating point image may
 *      be used as a vertex buffer. Some surface usages may be
 *      lower performance, due to software emulation, but any
 *      usage should work with any surface.
 *
 *      If 'sid' is already defined, the old surface is deleted
 *      and this new surface replaces it.
 *
 *      Surface IDs are arbitrary small non-negative integers,
 *      global to the entire SVGA device.
 *
 * Results:
 *      Returns pointers to arrays allocated in the FIFO for 'faces'
 *      and 'mipSizes'.
 *
 * Side effects:
 *      Begins a FIFO reservation.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginDefineSurface(uint32 sid,                  // IN
                          SVGA3dSurfaceFlags flags,    // IN
                          SVGA3dSurfaceFormat format,  // IN
                          SVGA3dSurfaceFace **faces,   // OUT
                          SVGA3dSize **mipSizes,       // OUT
                          uint32 numMipSizes)          // IN
{
   SVGA3dCmdDefineSurface *cmd;

   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SURFACE_DEFINE, sizeof *cmd +
                            sizeof **mipSizes * numMipSizes);

   cmd->sid = sid;
   cmd->surfaceFlags = flags;
   cmd->format = format;

   *faces = &cmd->face[0];
   *mipSizes = (SVGA3dSize*) &cmd[1];

   memset(*faces, 0, sizeof **faces * SVGA3D_MAX_SURFACE_FACES);
   memset(*mipSizes, 0, sizeof **mipSizes * numMipSizes);
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_DestroySurface --
 *
 *      Release the host VRAM encapsulated by a particular surface ID.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_DestroySurface(uint32 sid)  // IN
{
   SVGA3dCmdDestroySurface *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SURFACE_DESTROY, sizeof *cmd);
   cmd->sid = sid;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginSurfaceDMA--
 *
 *      Begin a SURFACE_DMA command. This reserves space for it in
 *      the FIFO, and returns a pointer to the command's box array.
 *      This function must be paired with SVGA_FIFOCommitAll().
 *
 *      When the SVGA3D device asynchronously processes this FIFO
 *      command, a DMA operation is performed between host VRAM and
 *      a generic SVGAGuestPtr. The guest pointer may refer to guest
 *      VRAM (provided by the SVGA PCI device) or to guest system
 *      memory that has been set up as a Guest Memory Region (GMR)
 *      by the SVGA device.
 *
 *      The guest's DMA buffer must remain valid (not freed, paged out,
 *      or overwritten) until the host has finished processing this
 *      command. The guest can determine that the host has finished
 *      by using the SVGA device's FIFO Fence mechanism.
 *
 *      The guest's image buffer can be an arbitrary size and shape.
 *      Guest image data is interpreted according to the SVGA3D surface
 *      format specified when the surface was defined.
 *
 *      The caller may optionally define the guest image's pitch.
 *      guestImage->pitch can either be zero (assume image is tightly
 *      packed) or it must be the number of bytes between vertically
 *      adjacent image blocks.
 *
 *      The provided copybox list specifies which regions of the source
 *      image are to be copied, and where they appear on the destination.
 *
 *      NOTE: srcx/srcy are always on the guest image and x/y are
 *      always on the host image, regardless of the actual transfer
 *      direction!
 *
 *      For efficiency, the SVGA3D device is free to copy more data
 *      than specified. For example, it may round copy boxes outwards
 *      such that they lie on particular alignment boundaries.
 *
 *      The box array is initialized with zeroes.
 *
 * Results:
 *      Returns pointers to a box array allocated in the FIFO.
 *
 * Side effects:
 *      Begins a FIFO reservation.
 *      Begins a FIFO command which will eventually perform DMA.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginSurfaceDMA(SVGA3dGuestImage *guestImage,     // IN
                       SVGA3dSurfaceImageId *hostImage,  // IN
                       SVGA3dTransferType transfer,      // IN
                       SVGA3dCopyBox **boxes,            // OUT
                       uint32 numBoxes)                  // IN
{
   SVGA3dCmdSurfaceDMA *cmd;
   uint32 boxesSize = sizeof **boxes * numBoxes;

   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SURFACE_DMA, sizeof *cmd + boxesSize);

   cmd->guest = *guestImage;
   cmd->host = *hostImage;
   cmd->transfer = transfer;
   *boxes = (SVGA3dCopyBox*) &cmd[1];

   memset(*boxes, 0, boxesSize);
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_DefineContext --
 *
 *      Create a new context, to be referred to with the provided ID.
 *
 *      Context objects encapsulate all render state, and shader
 *      objects are per-context.
 *
 *      Surfaces are not per-context. The same surface can be shared
 *      between multiple contexts, and surface operations can occur
 *      without a context.
 *
 *      If the provided context ID already existed, it is redefined.
 *
 *      Context IDs are arbitrary small non-negative integers,
 *      global to the entire SVGA device.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_DefineContext(uint32 cid)  // IN
{
   SVGA3dCmdDefineContext *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_CONTEXT_DEFINE, sizeof *cmd);
   cmd->cid = cid;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_DestroyContext --
 *
 *      Delete a context created with SVGA3D_DefineContext.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_DestroyContext(uint32 cid)  // IN
{
   SVGA3dCmdDestroyContext *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_CONTEXT_DESTROY, sizeof *cmd);
   cmd->cid = cid;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetRenderTarget --
 *
 *      Bind a surface object to a particular render target attachment
 *      point on the current context. Render target attachment points
 *      exist for color buffers, a depth buffer, and a stencil buffer.
 *
 *      The SVGA3D device is quite lenient about the types of surfaces
 *      that may be used as render targets. The color buffers must
 *      all be the same size, but the depth and stencil buffers do not
 *      have to be the same size as the color buffer. All attachments
 *      are optional.
 *
 *      Some combinations of render target formats may require software
 *      emulation, depending on the capabilities of the host graphics
 *      API and graphics hardware.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetRenderTarget(uint32 cid,                    // IN
                       SVGA3dRenderTargetType type,   // IN
                       SVGA3dSurfaceImageId *target)  // IN
{
   SVGA3dCmdSetRenderTarget *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETRENDERTARGET, sizeof *cmd);
   cmd->cid = cid;
   cmd->type = type;
   cmd->target = *target;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetTransform --
 *
 *      Set one of the current context's transform matrices.
 *      The 'type' parameter should be an SVGA3D_TRANSFORM_* constant.
 *
 *      All transformation matrices follow Direct3D-style coordinate
 *      system semantics.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */


void
SVGA3D_SetTransform(uint32 cid,                // IN
                    SVGA3dTransformType type,  // IN
                    const float *matrix)       // IN
{
   SVGA3dCmdSetTransform *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETTRANSFORM, sizeof *cmd);
   cmd->cid = cid;
   cmd->type = type;
   memcpy(&cmd->matrix[0], matrix, sizeof(float) * 16);
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetMaterial --
 *
 *      Set all material data for one SVGA3dFace.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetMaterial(uint32 cid,                      // IN
                   SVGA3dFace face,                 // IN
                   const SVGA3dMaterial *material)  // IN
{
   SVGA3dCmdSetMaterial *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETMATERIAL, sizeof *cmd);
   cmd->cid = cid;
   cmd->face = face;
   memcpy(&cmd->material, material, sizeof *material);
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetLightEnabled --
 *
 *      Enable or disable one fixed-function light.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetLightEnabled(uint32 cid,    // IN
                       uint32 index,  // IN
                       Bool enabled)  // IN
{
   SVGA3dCmdSetLightEnabled *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETLIGHTENABLED, sizeof *cmd);
   cmd->cid = cid;
   cmd->index = index;
   cmd->enabled = enabled;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetLightData --
 *
 *      Set all state for one fixed-function light.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetLightData(uint32 cid,                   // IN
                    uint32 index,                 // IN
                    const SVGA3dLightData *data)  // IN
{
   SVGA3dCmdSetLightData *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETLIGHTDATA, sizeof *cmd);
   cmd->cid = cid;
   cmd->index = index;
   memcpy(&cmd->data, data, sizeof *data);
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_DefineShader --
 *
 *      Upload the bytecode for a new shader. The bytecode is "SVGA3D
 *      format", which is theoretically a binary-compatible superset
 *      of Microsoft's DirectX shader bytecode. In practice, the
 *      SVGA3D bytecode doesn't yet have any extensions to DirectX's
 *      bytecode format.
 *
 *      The SVGA3D device supports shader models 1.1 through 2.0.
 *
 *      The caller chooses a shader ID (small positive integer) by
 *      which this shader will be identified in future commands. This
 *      ID is in a namespace which is per-context and per-shader-type.
 *
 *      'bytecodeLen' is specified in bytes. It must be a multiple of 4.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_DefineShader(uint32 cid,                   // IN
                    uint32 shid,                  // IN
                    SVGA3dShaderType type,        // IN
                    const uint32 *bytecode,       // IN
                    uint32 bytecodeLen)           // IN
{
   SVGA3dCmdDefineShader *cmd;

   if (bytecodeLen & 3) {
      dbg_printf("Shader bytecode length isn't a multiple of 32 bits!");
   }

   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SHADER_DEFINE, sizeof *cmd + bytecodeLen);
   cmd->cid = cid;
   cmd->shid = shid;
   cmd->type = type;
   memcpy(&cmd[1], bytecode, bytecodeLen);
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_DestroyShader --
 *
 *      Delete a shader that was created by SVGA3D_DefineShader. If
 *      the shader was the current vertex or pixel shader for its
 *      context, rendering results are undefined until a new shader is
 *      bound.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_DestroyShader(uint32 cid,             // IN
                     uint32 shid,            // IN
                     SVGA3dShaderType type)  // IN
{
   SVGA3dCmdDestroyShader *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SHADER_DESTROY, sizeof *cmd);
   cmd->cid = cid;
   cmd->shid = shid;
   cmd->type = type;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetShaderConst --
 *
 *      Set the value of a shader constant.
 *
 *      Shader constants are analogous to uniform variables in GLSL,
 *      except that they belong to the render context rather than to
 *      an individual shader.
 *
 *      Constants may have one of three types: A 4-vector of floats,
 *      a 4-vector of integers, or a single boolean flag.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetShaderConst(uint32 cid,                   // IN
                      uint32 reg,                   // IN
                      SVGA3dShaderType type,        // IN
                      SVGA3dShaderConstType ctype,  // IN
                      const void *value)            // IN
{
   SVGA3dCmdSetShaderConst *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SET_SHADER_CONST, sizeof *cmd);
   cmd->cid = cid;
   cmd->reg = reg;
   cmd->type = type;
   cmd->ctype = ctype;

   switch (ctype) {

   case SVGA3D_CONST_TYPE_FLOAT:
   case SVGA3D_CONST_TYPE_INT:
      memcpy(&cmd->values, value, sizeof cmd->values);
      break;

   case SVGA3D_CONST_TYPE_BOOL:
      memset(&cmd->values, 0, sizeof cmd->values);
      cmd->values[0] = *(uint32*)value;
      break;

   default:
      dbg_printf("Bad shader constant type.");
      break;

   }
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetShader --
 *
 *      Switch active shaders. This binds a new vertex or pixel shader
 *      to the specified context.
 *
 *      A shader ID of SVGA3D_INVALID_ID unbinds any shader, switching
 *      back to the fixed function vertex or pixel pipeline.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetShader(uint32 cid,             // IN
                 SVGA3dShaderType type,  // IN
                 uint32 shid)            // IN
{
   SVGA3dCmdSetShader *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SET_SHADER, sizeof *cmd);
   cmd->cid = cid;
   cmd->type = type;
   cmd->shid = shid;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginPresent --
 *
 *      Begin a PRESENT command. This reserves space for it in the
 *      FIFO, and returns a pointer to the command's rectangle array.
 *      This function must be paired with SVGA_FIFOCommitAll().
 *
 *      Present is the SVGA3D device's way of transferring fully
 *      rendered images to the 2D portion of the SVGA device. Present
 *      is a hardware accelerated surface-to-screen blit.
 *
 *      Important caveats:
 *
 *      - Present may or may not modify the guest-visible 2D
 *        framebuffer. If you need to read back rendering results,
 *        use an surface DMA transfer.
 *
 *      - Present is a good place to think about flow control
 *        between the host and the guest. If the host and guest
 *        have no other form of synchronization, the FIFO could
 *        fill with many frames worth of rendering commands. The
 *        host will visibly lag the guest. Depending on the
 *        size of the FIFO and the size of the frames, it could
 *        lag by hours! It is advisable to use FIFO fences in order
 *        to guarantee that no more than one or two Presents are
 *        queued in the FIFO at any given time.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May or may not write to the 2D framebuffer.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginPresent(uint32 sid,              // IN
                    SVGA3dCopyRect **rects,  // OUT
                    uint32 numRects)         // IN
{
   SVGA3dCmdPresent *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_PRESENT, sizeof *cmd +
                            sizeof **rects * numRects);
   cmd->sid = sid;
   *rects = (SVGA3dCopyRect*) &cmd[1];
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginClear --
 *
 *      Begin a CLEAR command. This reserves space for it in the FIFO,
 *      and returns a pointer to the command's rectangle array.  This
 *      function must be paired with SVGA_FIFOCommitAll().
 *
 *      Clear is a rendering operation which fills a list of
 *      rectangles with constant values on all render target types
 *      indicated by 'flags'.
 *
 *      Clear is not affected by clipping, depth test, or other
 *      render state which affects the fragment pipeline.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May write to attached render target surfaces.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginClear(uint32 cid,             // IN
                  SVGA3dClearFlag flags,  // IN
                  uint32 color,           // IN
                  float depth,            // IN
                  uint32 stencil,         // IN
                  SVGA3dRect **rects,     // OUT
                  uint32 numRects)        // IN
{
   SVGA3dCmdClear *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_CLEAR, sizeof *cmd +
                            sizeof **rects * numRects);
   cmd->cid = cid;
   cmd->clearFlag = flags;
   cmd->color = color;
   cmd->depth = depth;
   cmd->stencil = stencil;
   *rects = (SVGA3dRect*) &cmd[1];
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginDrawPrimitives --
 *
 *      Begin a DRAW_PRIMITIVES command. This reserves space for it in
 *      the FIFO, and returns a pointer to the command's arrays.
 *      This function must be paired with SVGA_FIFOCommitAll().
 *
 *      Drawing commands consist of two variable-length arrays:
 *      SVGA3dVertexDecl elements declare a set of vertex buffers to
 *      use while rendering, and SVGA3dPrimitiveRange elements specify
 *      groups of primitives each with an optional index buffer.
 *
 *      The decls and ranges arrays are initialized to zero.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      May write to attached render target surfaces.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginDrawPrimitives(uint32 cid,                    // IN
                           SVGA3dVertexDecl **decls,      // OUT
                           uint32 numVertexDecls,         // IN
                           SVGA3dPrimitiveRange **ranges, // OUT
                           uint32 numRanges)              // IN
{
   SVGA3dCmdDrawPrimitives *cmd;
   SVGA3dVertexDecl *declArray;
   SVGA3dPrimitiveRange *rangeArray;
   uint32 declSize = sizeof **decls * numVertexDecls;
   uint32 rangeSize = sizeof **ranges * numRanges;

   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_DRAW_PRIMITIVES, sizeof *cmd +
                            declSize + rangeSize);

   cmd->cid = cid;
   cmd->numVertexDecls = numVertexDecls;
   cmd->numRanges = numRanges;

   declArray = (SVGA3dVertexDecl*) &cmd[1];
   rangeArray = (SVGA3dPrimitiveRange*) &declArray[numVertexDecls];

   memset(declArray, 0, declSize);
   memset(rangeArray, 0, rangeSize);

   *decls = declArray;
   *ranges = rangeArray;
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginSurfaceCopy --
 *
 *      Begin a SURFACE_COPY command. This reserves space for it in
 *      the FIFO, and returns a pointer to the command's arrays.  This
 *      function must be paired with SVGA_FIFOCommitAll().
 *
 *      The box array is initialized with zeroes.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Asynchronously copies a list of boxes from surface to surface.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginSurfaceCopy(SVGA3dSurfaceImageId *src,   // IN
                        SVGA3dSurfaceImageId *dest,  // IN
                        SVGA3dCopyBox **boxes,       // OUT
                        uint32 numBoxes)             // IN
{
   SVGA3dCmdSurfaceCopy *cmd;
   uint32 boxesSize = sizeof **boxes * numBoxes;

   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SURFACE_COPY, sizeof *cmd + boxesSize);

   cmd->src = *src;
   cmd->dest = *dest;
   *boxes = (SVGA3dCopyBox*) &cmd[1];

   memset(*boxes, 0, boxesSize);
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SurfaceStretchBlt --
 *
 *      Issue a SURFACE_STRETCHBLT command: an asynchronous
 *      surface-to-surface blit, with scaling.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Asynchronously copies one box from surface to surface.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SurfaceStretchBlt(SVGA3dSurfaceImageId *src,   // IN
                         SVGA3dSurfaceImageId *dest,  // IN
                         SVGA3dBox *boxSrc,           // IN
                         SVGA3dBox *boxDest,          // IN
                         SVGA3dStretchBltMode mode)   // IN
{
   SVGA3dCmdSurfaceStretchBlt *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SURFACE_STRETCHBLT, sizeof *cmd);
   cmd->src = *src;
   cmd->dest = *dest;
   cmd->boxSrc = *boxSrc;
   cmd->boxDest = *boxDest;
   cmd->mode = mode;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetViewport --
 *
 *      Set the current context's viewport rectangle. The viewport
 *      is clipped to the dimensions of the current render target,
 *      then all rendering is clipped to the viewport.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetViewport(uint32 cid,        // IN
                   SVGA3dRect *rect)  // IN
{
   SVGA3dCmdSetViewport *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETVIEWPORT, sizeof *cmd);
   cmd->cid = cid;
   cmd->rect = *rect;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_SetZRange --
 *
 *      Set the range of the depth buffer to use. 'min' and 'max'
 *      are values between 0.0 and 1.0.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_SetZRange(uint32 cid,  // IN
                 float zMin,  // IN
                 float zMax)  // IN
{
   SVGA3dCmdSetZRange *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETZRANGE, sizeof *cmd);
   cmd->cid = cid;
   cmd->zRange.min = zMin;
   cmd->zRange.max = zMax;
   SVGA_FIFOCommitAll();
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginSetTextureState --
 *
 *      Begin a SETTEXTURESTATE command. This reserves space for it in
 *      the FIFO, and returns a pointer to the command's texture state
 *      array.  This function must be paired with SVGA_FIFOCommitAll().
 *
 *      This command sets rendering state which is per-texture-unit.
 *
 *      XXX: Individual texture states need documentation. However,
 *           they are very similar to the texture states defined by
 *           Direct3D. The D3D documentation is a good starting point
 *           for understanding SVGA3D texture states.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginSetTextureState(uint32 cid,                   // IN
                            SVGA3dTextureState **states,  // OUT
                            uint32 numStates)             // IN
{
   SVGA3dCmdSetTextureState *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETTEXTURESTATE, sizeof *cmd +
                            sizeof **states * numStates);
   cmd->cid = cid;
   *states = (SVGA3dTextureState*) &cmd[1];
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginSetRenderState --
 *
 *      Begin a SETRENDERSTATE command. This reserves space for it in
 *      the FIFO, and returns a pointer to the command's texture state
 *      array.  This function must be paired with SVGA_FIFOCommitAll().
 *
 *      This command sets rendering state which is global to the context.
 *
 *      XXX: Individual render states need documentation. However,
 *           they are very similar to the render states defined by
 *           Direct3D. The D3D documentation is a good starting point
 *           for understanding SVGA3D render states.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginSetRenderState(uint32 cid,                  // IN
                           SVGA3dRenderState **states,  // OUT
                           uint32 numStates)            // IN
{
   SVGA3dCmdSetRenderState *cmd;
   cmd = SVGA3D_FIFOReserve(SVGA_3D_CMD_SETRENDERSTATE, sizeof *cmd +
                            sizeof **states * numStates);
   cmd->cid = cid;
   *states = (SVGA3dRenderState*) &cmd[1];
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginPresentReadback --
 *
 *      Begin a PRESENT_READBACK command. This reserves space for it
 *      in the FIFO, and returns a pointer to the command's SVGA3dRect
 *      array.  This function must be paired with
 *      SVGA_FIFOCommitAll().
 *
 *      This command will update the 2D framebuffer with the most
 *      recently presented data in the supplied regions.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      None.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginPresentReadback(SVGA3dRect **rects,  // OUT
                            uint32 numRects)     // IN
{
   void *cmd;
   cmd = (void *) SVGA3D_FIFOReserve(SVGA_3D_CMD_PRESENT_READBACK,
                                     sizeof **rects * numRects);
   *rects = (SVGA3dRect*) cmd;
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BeginBlitSurfaceToScreen --
 *
 *      Begin a BLIT_SURFACE_TO_SCREEN command. This reserves space
 *      for it in the FIFO, and optionally returns a pointer to the
 *      command's clip rectangle array.  This function must be paired
 *      with SVGA_FIFOCommitAll().
 *
 *      Copy an SVGA3D surface image to a Screen Object.  This command
 *      requires the SVGA Screen Object capability to be present.
 *      Copies a rectangular region of a surface image to a
 *      rectangular region of a screen or a portion of the virtual
 *      coordinate space that contains all screens.
 *
 *      If destScreenId is SVGA_ID_INVALID, the destination rectangle
 *      is in virtual coordinates rather than in screen coordinates.
 *
 *      If the source and destination rectangle are different sizes,
 *      the image will be scaled using a backend-specific algorithm.
 *
 *      This command may optionally include a clip rectangle list. If
 *      zero rectangles are specified, there is no clip region. The
 *      entire destination rectangle is drawn. If one or more
 *      rectangles are specified, they describe a destination clipping
 *      region in the same coordinate system as destRect.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes to the Screen Object.
 *      Begins a new FIFO command.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BeginBlitSurfaceToScreen(const SVGA3dSurfaceImageId *srcImage,  // IN
                                const SVGASignedRect *srcRect,         // IN
                                uint32 destScreenId,                   // IN (optional)
                                const SVGASignedRect *destRect,        // IN
                                SVGASignedRect **clipRects,            // IN (optional)
                                uint32 numClipRects)                   // IN (optional)
{
   SVGA3dCmdBlitSurfaceToScreen *cmd =
      SVGA3D_FIFOReserve(SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN,
                         sizeof *cmd + numClipRects * sizeof(SVGASignedRect));

   cmd->srcImage = *srcImage;
   cmd->srcRect = *srcRect;
   cmd->destScreenId = destScreenId;
   cmd->destRect = *destRect;

   if (clipRects) {
      *clipRects = (SVGASignedRect*) &cmd[1];
   }
}


/*
 *----------------------------------------------------------------------
 *
 * SVGA3D_BlitSurfaceToScreen --
 *
 *      This is a convenience wrapper around
 *      SVGA3D_BeginBlitSurfaceToScreen, for callers who do not
 *      require a clip rectangle list.
 *
 * Results:
 *      None.
 *
 * Side effects:
 *      Writes to the Screen Object.
 *
 *----------------------------------------------------------------------
 */

void
SVGA3D_BlitSurfaceToScreen(const SVGA3dSurfaceImageId *srcImage,  // IN
                           const SVGASignedRect *srcRect,         // IN
                           uint32 destScreenId,                   // IN (optional)
                           const SVGASignedRect *destRect)        // IN
{
   SVGA3D_BeginBlitSurfaceToScreen(srcImage, srcRect, destScreenId, destRect, NULL, 0);
   SVGA_FIFOCommitAll();
}
