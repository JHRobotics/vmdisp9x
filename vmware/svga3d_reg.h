/**********************************************************
 * Copyright 1998-2009 VMware, Inc.  All rights reserved.
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
 * svga3d_reg.h --
 *
 *       SVGA 3D hardware definitions
 */

#ifndef _SVGA3D_REG_H_
#define _SVGA3D_REG_H_

#include "svga_reg.h"

/*
 * Extra types
 */

typedef uint16 SVGA3dLogicOp;
typedef uint32 SVGA3dSurface1Flags;
typedef uint32 SVGA3dSurface2Flags;
typedef uint64 SVGA3dSurfaceAllFlags;
typedef uint32 SVGAMobId;

#define SVGA3D_INVALID_ID ((uint32)-1)

typedef uint64 PPN64;
typedef uint32 PPN;

/*
 * Limits
 */
#define SVGA3D_NUM_TEXTURE_UNITS                32
#define SVGA3D_NUM_LIGHTS                       8

/*
 * 3D Hardware Version
 *
 *   The hardware version is stored in the SVGA_FIFO_3D_HWVERSION fifo
 *   register.   Is set by the host and read by the guest.  This lets
 *   us make new guest drivers which are backwards-compatible with old
 *   SVGA hardware revisions.  It does not let us support old guest
 *   drivers.  Good enough for now.
 *
 */

#define SVGA3D_MAKE_HWVERSION(major, minor)      (((unsigned long)(major) << 16) | ((unsigned long)(minor) & 0xFF))
#define SVGA3D_MAJOR_HWVERSION(version)          ((unsigned long)(version) >> 16)
#define SVGA3D_MINOR_HWVERSION(version)          ((unsigned long)(version) & 0xFF)

typedef enum {
   SVGA3D_HWVERSION_WS5_RC1   = SVGA3D_MAKE_HWVERSION(0, 1),
   SVGA3D_HWVERSION_WS5_RC2   = SVGA3D_MAKE_HWVERSION(0, 2),
   SVGA3D_HWVERSION_WS51_RC1  = SVGA3D_MAKE_HWVERSION(0, 3),
   SVGA3D_HWVERSION_WS6_B1    = SVGA3D_MAKE_HWVERSION(1, 1),
   SVGA3D_HWVERSION_FUSION_11 = SVGA3D_MAKE_HWVERSION(1, 4),
   SVGA3D_HWVERSION_WS65_B1   = SVGA3D_MAKE_HWVERSION(2, 0),
   SVGA3D_HWVERSION_WS8_B1    = SVGA3D_MAKE_HWVERSION(2, 1),
   SVGA3D_HWVERSION_CURRENT   = SVGA3D_HWVERSION_WS8_B1,
} SVGA3dHardwareVersion;

/*
 * Generic Types
 */

typedef uint32 SVGA3dBool; /* 32-bit Bool definition */
#define SVGA3D_NUM_CLIPPLANES                   6
#define SVGA3D_MAX_SIMULTANEOUS_RENDER_TARGETS  8
#define SVGA3D_MAX_CONTEXT_IDS                  256
#define SVGA3D_MAX_SURFACE_IDS                  (32 * 1024)

/*
 * Surface formats.
 *
 * If you modify this list, be sure to keep GLUtil.c in sync. It
 * includes the internal format definition of each surface in
 * GLUtil_ConvertSurfaceFormat, and it contains a table of
 * human-readable names in GLUtil_GetFormatName.
 */

typedef enum SVGA3dSurfaceFormat {
   SVGA3D_FORMAT_INVALID               = 0,

   SVGA3D_X8R8G8B8                     = 1,
   SVGA3D_A8R8G8B8                     = 2,

   SVGA3D_R5G6B5                       = 3,
   SVGA3D_X1R5G5B5                     = 4,
   SVGA3D_A1R5G5B5                     = 5,
   SVGA3D_A4R4G4B4                     = 6,

   SVGA3D_Z_D32                        = 7,
   SVGA3D_Z_D16                        = 8,
   SVGA3D_Z_D24S8                      = 9,
   SVGA3D_Z_D15S1                      = 10,

   SVGA3D_LUMINANCE8                   = 11,
   SVGA3D_LUMINANCE4_ALPHA4            = 12,
   SVGA3D_LUMINANCE16                  = 13,
   SVGA3D_LUMINANCE8_ALPHA8            = 14,

   SVGA3D_DXT1                         = 15,
   SVGA3D_DXT2                         = 16,
   SVGA3D_DXT3                         = 17,
   SVGA3D_DXT4                         = 18,
   SVGA3D_DXT5                         = 19,

   SVGA3D_BUMPU8V8                     = 20,
   SVGA3D_BUMPL6V5U5                   = 21,
   SVGA3D_BUMPX8L8V8U8                 = 22,
   SVGA3D_BUMPL8V8U8                   = 23,

   SVGA3D_ARGB_S10E5                   = 24,   /* 16-bit floating-point ARGB */
   SVGA3D_ARGB_S23E8                   = 25,   /* 32-bit floating-point ARGB */

   SVGA3D_A2R10G10B10                  = 26,

   /* signed formats */
   SVGA3D_V8U8                         = 27,
   SVGA3D_Q8W8V8U8                     = 28,
   SVGA3D_CxV8U8                       = 29,

   /* mixed formats */
   SVGA3D_X8L8V8U8                     = 30,
   SVGA3D_A2W10V10U10                  = 31,

   SVGA3D_ALPHA8                       = 32,

   /* Single- and dual-component floating point formats */
   SVGA3D_R_S10E5                      = 33,
   SVGA3D_R_S23E8                      = 34,
   SVGA3D_RG_S10E5                     = 35,
   SVGA3D_RG_S23E8                     = 36,

   /*
    * Any surface can be used as a buffer object, but SVGA3D_BUFFER is
    * the most efficient format to use when creating new surfaces
    * expressly for index or vertex data.
    */

   SVGA3D_BUFFER                       = 37,

   SVGA3D_Z_D24X8                      = 38,

   SVGA3D_V16U16                       = 39,

   SVGA3D_G16R16                       = 40,
   SVGA3D_A16B16G16R16                 = 41,

   /* Packed Video formats */
   SVGA3D_UYVY                         = 42,
   SVGA3D_YUY2                         = 43,

   /* Planar video formats */
   SVGA3D_NV12                         = 44,

   /* Video format with alpha */
   SVGA3D_AYUV                         = 45,

   SVGA3D_BC4_UNORM                    = 108,
   SVGA3D_BC5_UNORM                    = 111,

   /* Advanced D3D9 depth formats. */
   SVGA3D_Z_DF16                       = 118,
   SVGA3D_Z_DF24                       = 119,
   SVGA3D_Z_D24S8_INT                  = 120,

   SVGA3D_FORMAT_MAX
} SVGA3dSurfaceFormat;

typedef uint32 SVGA3dColor; /* a, r, g, b */

/*
 * These match the D3DFORMAT_OP definitions used by Direct3D. We need
 * them so that we can query the host for what the supported surface
 * operations are (when we're using the D3D backend, in particular),
 * and so we can send those operations to the guest.
 */
typedef enum {
   SVGA3DFORMAT_OP_TEXTURE                               = 0x00000001,
   SVGA3DFORMAT_OP_VOLUMETEXTURE                         = 0x00000002,
   SVGA3DFORMAT_OP_CUBETEXTURE                           = 0x00000004,
   SVGA3DFORMAT_OP_OFFSCREEN_RENDERTARGET                = 0x00000008,
   SVGA3DFORMAT_OP_SAME_FORMAT_RENDERTARGET              = 0x00000010,
   SVGA3DFORMAT_OP_ZSTENCIL                              = 0x00000040,
   SVGA3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH   = 0x00000080,

/*
 * This format can be used as a render target if the current display mode
 * is the same depth if the alpha channel is ignored. e.g. if the device
 * can render to A8R8G8B8 when the display mode is X8R8G8B8, then the
 * format op list entry for A8R8G8B8 should have this cap.
 */
   SVGA3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET  = 0x00000100,

/*
 * This format contains DirectDraw support (including Flip).  This flag
 * should not to be set on alpha formats.
 */
   SVGA3DFORMAT_OP_DISPLAYMODE                           = 0x00000400,

/*
 * The rasterizer can support some level of Direct3D support in this format
 * and implies that the driver can create a Context in this mode (for some
 * render target format).  When this flag is set, the SVGA3DFORMAT_OP_DISPLAYMODE
 * flag must also be set.
 */
   SVGA3DFORMAT_OP_3DACCELERATION                        = 0x00000800,

/*
 * This is set for a private format when the driver has put the bpp in
 * the structure.
 */
   SVGA3DFORMAT_OP_PIXELSIZE                             = 0x00001000,

/*
 * Indicates that this format can be converted to any RGB format for which
 * SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB is specified
 */
   SVGA3DFORMAT_OP_CONVERT_TO_ARGB                       = 0x00002000,

/*
 * Indicates that this format can be used to create offscreen plain surfaces.
 */
   SVGA3DFORMAT_OP_OFFSCREENPLAIN                        = 0x00004000,

/*
 * Indicated that this format can be read as an SRGB texture (meaning that the
 * sampler will linearize the looked up data)
 */
   SVGA3DFORMAT_OP_SRGBREAD                              = 0x00008000,

/*
 * Indicates that this format can be used in the bumpmap instructions
 */
   SVGA3DFORMAT_OP_BUMPMAP                               = 0x00010000,

/*
 * Indicates that this format can be sampled by the displacement map sampler
 */
   SVGA3DFORMAT_OP_DMAP                                  = 0x00020000,

/*
 * Indicates that this format cannot be used with texture filtering
 */
   SVGA3DFORMAT_OP_NOFILTER                              = 0x00040000,

/*
 * Indicates that format conversions are supported to this RGB format if
 * SVGA3DFORMAT_OP_CONVERT_TO_ARGB is specified in the source format.
 */
   SVGA3DFORMAT_OP_MEMBEROFGROUP_ARGB                    = 0x00080000,

/*
 * Indicated that this format can be written as an SRGB target (meaning that the
 * pixel pipe will DE-linearize data on output to format)
 */
   SVGA3DFORMAT_OP_SRGBWRITE                             = 0x00100000,

/*
 * Indicates that this format cannot be used with alpha blending
 */
   SVGA3DFORMAT_OP_NOALPHABLEND                          = 0x00200000,

/*
 * Indicates that the device can auto-generated sublevels for resources
 * of this format
 */
   SVGA3DFORMAT_OP_AUTOGENMIPMAP                         = 0x00400000,

/*
 * Indicates that this format can be used by vertex texture sampler
 */
   SVGA3DFORMAT_OP_VERTEXTEXTURE                         = 0x00800000,

/*
 * Indicates that this format supports neither texture coordinate wrap
 * modes, nor mipmapping
 */
   SVGA3DFORMAT_OP_NOTEXCOORDWRAPNORMIP                  = 0x01000000
} SVGA3dFormatOp;

/*
 * This structure is a conversion of SVGA3DFORMAT_OP_*.
 * Entries must be located at the same position.
 */
typedef union {
   uint32 value;
   struct {
      uint32 texture : 1;
      uint32 volumeTexture : 1;
      uint32 cubeTexture : 1;
      uint32 offscreenRenderTarget : 1;
      uint32 sameFormatRenderTarget : 1;
      uint32 unknown1 : 1;
      uint32 zStencil : 1;
      uint32 zStencilArbitraryDepth : 1;
      uint32 sameFormatUpToAlpha : 1;
      uint32 unknown2 : 1;
      uint32 displayMode : 1;
      uint32 acceleration3d : 1;
      uint32 pixelSize : 1;
      uint32 convertToARGB : 1;
      uint32 offscreenPlain : 1;
      uint32 sRGBRead : 1;
      uint32 bumpMap : 1;
      uint32 dmap : 1;
      uint32 noFilter : 1;
      uint32 memberOfGroupARGB : 1;
      uint32 sRGBWrite : 1;
      uint32 noAlphaBlend : 1;
      uint32 autoGenMipMap : 1;
      uint32 vertexTexture : 1;
      uint32 noTexCoordWrapNorMip : 1;
   };
} SVGA3dSurfaceFormatCaps;

/*
 * SVGA_3D_CMD_SETRENDERSTATE Types.  All value types
 * must fit in a uint32.
 */

typedef enum {
   SVGA3D_RS_INVALID                   = 0,
   SVGA3D_RS_ZENABLE                   = 1,     /* SVGA3dBool */
   SVGA3D_RS_ZWRITEENABLE              = 2,     /* SVGA3dBool */
   SVGA3D_RS_ALPHATESTENABLE           = 3,     /* SVGA3dBool */
   SVGA3D_RS_DITHERENABLE              = 4,     /* SVGA3dBool */
   SVGA3D_RS_BLENDENABLE               = 5,     /* SVGA3dBool */
   SVGA3D_RS_FOGENABLE                 = 6,     /* SVGA3dBool */
   SVGA3D_RS_SPECULARENABLE            = 7,     /* SVGA3dBool */
   SVGA3D_RS_STENCILENABLE             = 8,     /* SVGA3dBool */
   SVGA3D_RS_LIGHTINGENABLE            = 9,     /* SVGA3dBool */
   SVGA3D_RS_NORMALIZENORMALS          = 10,    /* SVGA3dBool */
   SVGA3D_RS_POINTSPRITEENABLE         = 11,    /* SVGA3dBool */
   SVGA3D_RS_POINTSCALEENABLE          = 12,    /* SVGA3dBool */
   SVGA3D_RS_STENCILREF                = 13,    /* uint32 */
   SVGA3D_RS_STENCILMASK               = 14,    /* uint32 */
   SVGA3D_RS_STENCILWRITEMASK          = 15,    /* uint32 */
   SVGA3D_RS_FOGSTART                  = 16,    /* float */
   SVGA3D_RS_FOGEND                    = 17,    /* float */
   SVGA3D_RS_FOGDENSITY                = 18,    /* float */
   SVGA3D_RS_POINTSIZE                 = 19,    /* float */
   SVGA3D_RS_POINTSIZEMIN              = 20,    /* float */
   SVGA3D_RS_POINTSIZEMAX              = 21,    /* float */
   SVGA3D_RS_POINTSCALE_A              = 22,    /* float */
   SVGA3D_RS_POINTSCALE_B              = 23,    /* float */
   SVGA3D_RS_POINTSCALE_C              = 24,    /* float */
   SVGA3D_RS_FOGCOLOR                  = 25,    /* SVGA3dColor */
   SVGA3D_RS_AMBIENT                   = 26,    /* SVGA3dColor */
   SVGA3D_RS_CLIPPLANEENABLE           = 27,    /* SVGA3dClipPlanes */
   SVGA3D_RS_FOGMODE                   = 28,    /* SVGA3dFogMode */
   SVGA3D_RS_FILLMODE                  = 29,    /* SVGA3dFillMode */
   SVGA3D_RS_SHADEMODE                 = 30,    /* SVGA3dShadeMode */
   SVGA3D_RS_LINEPATTERN               = 31,    /* SVGA3dLinePattern */
   SVGA3D_RS_SRCBLEND                  = 32,    /* SVGA3dBlendOp */
   SVGA3D_RS_DSTBLEND                  = 33,    /* SVGA3dBlendOp */
   SVGA3D_RS_BLENDEQUATION             = 34,    /* SVGA3dBlendEquation */
   SVGA3D_RS_CULLMODE                  = 35,    /* SVGA3dFace */
   SVGA3D_RS_ZFUNC                     = 36,    /* SVGA3dCmpFunc */
   SVGA3D_RS_ALPHAFUNC                 = 37,    /* SVGA3dCmpFunc */
   SVGA3D_RS_STENCILFUNC               = 38,    /* SVGA3dCmpFunc */
   SVGA3D_RS_STENCILFAIL               = 39,    /* SVGA3dStencilOp */
   SVGA3D_RS_STENCILZFAIL              = 40,    /* SVGA3dStencilOp */
   SVGA3D_RS_STENCILPASS               = 41,    /* SVGA3dStencilOp */
   SVGA3D_RS_ALPHAREF                  = 42,    /* float (0.0 .. 1.0) */
   SVGA3D_RS_FRONTWINDING              = 43,    /* SVGA3dFrontWinding */
   SVGA3D_RS_COORDINATETYPE            = 44,    /* SVGA3dCoordinateType */
   SVGA3D_RS_ZBIAS                     = 45,    /* float */
   SVGA3D_RS_RANGEFOGENABLE            = 46,    /* SVGA3dBool */
   SVGA3D_RS_COLORWRITEENABLE          = 47,    /* SVGA3dColorMask */
   SVGA3D_RS_VERTEXMATERIALENABLE      = 48,    /* SVGA3dBool */
   SVGA3D_RS_DIFFUSEMATERIALSOURCE     = 49,    /* SVGA3dVertexMaterial */
   SVGA3D_RS_SPECULARMATERIALSOURCE    = 50,    /* SVGA3dVertexMaterial */
   SVGA3D_RS_AMBIENTMATERIALSOURCE     = 51,    /* SVGA3dVertexMaterial */
   SVGA3D_RS_EMISSIVEMATERIALSOURCE    = 52,    /* SVGA3dVertexMaterial */
   SVGA3D_RS_TEXTUREFACTOR             = 53,    /* SVGA3dColor */
   SVGA3D_RS_LOCALVIEWER               = 54,    /* SVGA3dBool */
   SVGA3D_RS_SCISSORTESTENABLE         = 55,    /* SVGA3dBool */
   SVGA3D_RS_BLENDCOLOR                = 56,    /* SVGA3dColor */
   SVGA3D_RS_STENCILENABLE2SIDED       = 57,    /* SVGA3dBool */
   SVGA3D_RS_CCWSTENCILFUNC            = 58,    /* SVGA3dCmpFunc */
   SVGA3D_RS_CCWSTENCILFAIL            = 59,    /* SVGA3dStencilOp */
   SVGA3D_RS_CCWSTENCILZFAIL           = 60,    /* SVGA3dStencilOp */
   SVGA3D_RS_CCWSTENCILPASS            = 61,    /* SVGA3dStencilOp */
   SVGA3D_RS_VERTEXBLEND               = 62,    /* SVGA3dVertexBlendFlags */
   SVGA3D_RS_SLOPESCALEDEPTHBIAS       = 63,    /* float */
   SVGA3D_RS_DEPTHBIAS                 = 64,    /* float */


   /*
    * Output Gamma Level
    *
    * Output gamma effects the gamma curve of colors that are output from the
    * rendering pipeline.  A value of 1.0 specifies a linear color space. If the
    * value is <= 0.0, gamma correction is ignored and linear color space is
    * used.
    */

   SVGA3D_RS_OUTPUTGAMMA               = 65,    /* float */
   SVGA3D_RS_ZVISIBLE                  = 66,    /* SVGA3dBool */
   SVGA3D_RS_LASTPIXEL                 = 67,    /* SVGA3dBool */
   SVGA3D_RS_CLIPPING                  = 68,    /* SVGA3dBool */
   SVGA3D_RS_WRAP0                     = 69,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP1                     = 70,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP2                     = 71,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP3                     = 72,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP4                     = 73,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP5                     = 74,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP6                     = 75,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP7                     = 76,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP8                     = 77,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP9                     = 78,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP10                    = 79,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP11                    = 80,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP12                    = 81,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP13                    = 82,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP14                    = 83,    /* SVGA3dWrapFlags */
   SVGA3D_RS_WRAP15                    = 84,    /* SVGA3dWrapFlags */
   SVGA3D_RS_MULTISAMPLEANTIALIAS      = 85,    /* SVGA3dBool */
   SVGA3D_RS_MULTISAMPLEMASK           = 86,    /* uint32 */
   SVGA3D_RS_INDEXEDVERTEXBLENDENABLE  = 87,    /* SVGA3dBool */
   SVGA3D_RS_TWEENFACTOR               = 88,    /* float */
   SVGA3D_RS_ANTIALIASEDLINEENABLE     = 89,    /* SVGA3dBool */
   SVGA3D_RS_COLORWRITEENABLE1         = 90,    /* SVGA3dColorMask */
   SVGA3D_RS_COLORWRITEENABLE2         = 91,    /* SVGA3dColorMask */
   SVGA3D_RS_COLORWRITEENABLE3         = 92,    /* SVGA3dColorMask */
   SVGA3D_RS_SEPARATEALPHABLENDENABLE  = 93,    /* SVGA3dBool */
   SVGA3D_RS_SRCBLENDALPHA             = 94,    /* SVGA3dBlendOp */
   SVGA3D_RS_DSTBLENDALPHA             = 95,    /* SVGA3dBlendOp */
   SVGA3D_RS_BLENDEQUATIONALPHA        = 96,    /* SVGA3dBlendEquation */
   SVGA3D_RS_TRANSPARENCYANTIALIAS     = 97,    /* SVGA3dTransparencyAntialiasType */
   SVGA3D_RS_LINEAA                    = 98,    /* SVGA3dBool */
   SVGA3D_RS_LINEWIDTH                 = 99,    /* float */
   SVGA3D_RS_MAX
} SVGA3dRenderStateName;

typedef enum {
   SVGA3D_TRANSPARENCYANTIALIAS_NORMAL            = 0,
   SVGA3D_TRANSPARENCYANTIALIAS_ALPHATOCOVERAGE   = 1,
   SVGA3D_TRANSPARENCYANTIALIAS_SUPERSAMPLE       = 2,
   SVGA3D_TRANSPARENCYANTIALIAS_MAX
} SVGA3dTransparencyAntialiasType;

typedef enum {
   SVGA3D_VERTEXMATERIAL_NONE     = 0,    /* Use the value in the current material */
   SVGA3D_VERTEXMATERIAL_DIFFUSE  = 1,    /* Use the value in the diffuse component */
   SVGA3D_VERTEXMATERIAL_SPECULAR = 2,    /* Use the value in the specular component */
} SVGA3dVertexMaterial;

typedef enum {
   SVGA3D_FILLMODE_INVALID = 0,
   SVGA3D_FILLMODE_POINT   = 1,
   SVGA3D_FILLMODE_LINE    = 2,
   SVGA3D_FILLMODE_FILL    = 3,
   SVGA3D_FILLMODE_MAX
} SVGA3dFillModeType;


typedef
union {
   struct {
      uint16   mode;       /* SVGA3dFillModeType */
      uint16   face;       /* SVGA3dFace */
   };
   uint32 uintValue;
} SVGA3dFillMode;

typedef enum {
   SVGA3D_SHADEMODE_INVALID = 0,
   SVGA3D_SHADEMODE_FLAT    = 1,
   SVGA3D_SHADEMODE_SMOOTH  = 2,
   SVGA3D_SHADEMODE_PHONG   = 3,     /* Not supported */
   SVGA3D_SHADEMODE_MAX
} SVGA3dShadeMode;

typedef
union {
   struct {
      uint16 repeat;
      uint16 pattern;
   };
   uint32 uintValue;
} SVGA3dLinePattern;

typedef enum {
   SVGA3D_BLENDOP_INVALID            = 0,
   SVGA3D_BLENDOP_ZERO               = 1,
   SVGA3D_BLENDOP_ONE                = 2,
   SVGA3D_BLENDOP_SRCCOLOR           = 3,
   SVGA3D_BLENDOP_INVSRCCOLOR        = 4,
   SVGA3D_BLENDOP_SRCALPHA           = 5,
   SVGA3D_BLENDOP_INVSRCALPHA        = 6,
   SVGA3D_BLENDOP_DESTALPHA          = 7,
   SVGA3D_BLENDOP_INVDESTALPHA       = 8,
   SVGA3D_BLENDOP_DESTCOLOR          = 9,
   SVGA3D_BLENDOP_INVDESTCOLOR       = 10,
   SVGA3D_BLENDOP_SRCALPHASAT        = 11,
   SVGA3D_BLENDOP_BLENDFACTOR        = 12,
   SVGA3D_BLENDOP_INVBLENDFACTOR     = 13,
   SVGA3D_BLENDOP_MAX
} SVGA3dBlendOp;

typedef enum {
   SVGA3D_BLENDEQ_INVALID            = 0,
   SVGA3D_BLENDEQ_ADD                = 1,
   SVGA3D_BLENDEQ_SUBTRACT           = 2,
   SVGA3D_BLENDEQ_REVSUBTRACT        = 3,
   SVGA3D_BLENDEQ_MINIMUM            = 4,
   SVGA3D_BLENDEQ_MAXIMUM            = 5,
   SVGA3D_BLENDEQ_MAX
} SVGA3dBlendEquation;

typedef enum {
   SVGA3D_FRONTWINDING_INVALID = 0,
   SVGA3D_FRONTWINDING_CW      = 1,
   SVGA3D_FRONTWINDING_CCW     = 2,
   SVGA3D_FRONTWINDING_MAX
} SVGA3dFrontWinding;

typedef enum {
   SVGA3D_FACE_INVALID  = 0,
   SVGA3D_FACE_NONE     = 1,
   SVGA3D_FACE_FRONT    = 2,
   SVGA3D_FACE_BACK     = 3,
   SVGA3D_FACE_FRONT_BACK = 4,
   SVGA3D_FACE_MAX
} SVGA3dFace;

/*
 * The order and the values should not be changed
 */

typedef enum {
   SVGA3D_CMP_INVALID              = 0,
   SVGA3D_CMP_NEVER                = 1,
   SVGA3D_CMP_LESS                 = 2,
   SVGA3D_CMP_EQUAL                = 3,
   SVGA3D_CMP_LESSEQUAL            = 4,
   SVGA3D_CMP_GREATER              = 5,
   SVGA3D_CMP_NOTEQUAL             = 6,
   SVGA3D_CMP_GREATEREQUAL         = 7,
   SVGA3D_CMP_ALWAYS               = 8,
   SVGA3D_CMP_MAX
} SVGA3dCmpFunc;

/*
 * SVGA3D_FOGFUNC_* specifies the fog equation, or PER_VERTEX which allows
 * the fog factor to be specified in the alpha component of the specular
 * (a.k.a. secondary) vertex color.
 */
typedef enum {
   SVGA3D_FOGFUNC_INVALID          = 0,
   SVGA3D_FOGFUNC_EXP              = 1,
   SVGA3D_FOGFUNC_EXP2             = 2,
   SVGA3D_FOGFUNC_LINEAR           = 3,
   SVGA3D_FOGFUNC_PER_VERTEX       = 4
} SVGA3dFogFunction;

/*
 * SVGA3D_FOGTYPE_* specifies if fog factors are computed on a per-vertex
 * or per-pixel basis.
 */
typedef enum {
   SVGA3D_FOGTYPE_INVALID          = 0,
   SVGA3D_FOGTYPE_VERTEX           = 1,
   SVGA3D_FOGTYPE_PIXEL            = 2,
   SVGA3D_FOGTYPE_MAX              = 3
} SVGA3dFogType;

/*
 * SVGA3D_FOGBASE_* selects depth or range-based fog. Depth-based fog is
 * computed using the eye Z value of each pixel (or vertex), whereas range-
 * based fog is computed using the actual distance (range) to the eye.
 */
typedef enum {
   SVGA3D_FOGBASE_INVALID          = 0,
   SVGA3D_FOGBASE_DEPTHBASED       = 1,
   SVGA3D_FOGBASE_RANGEBASED       = 2,
   SVGA3D_FOGBASE_MAX              = 3
} SVGA3dFogBase;

typedef enum {
   SVGA3D_STENCILOP_INVALID        = 0,
   SVGA3D_STENCILOP_KEEP           = 1,
   SVGA3D_STENCILOP_ZERO           = 2,
   SVGA3D_STENCILOP_REPLACE        = 3,
   SVGA3D_STENCILOP_INCRSAT        = 4,
   SVGA3D_STENCILOP_DECRSAT        = 5,
   SVGA3D_STENCILOP_INVERT         = 6,
   SVGA3D_STENCILOP_INCR           = 7,
   SVGA3D_STENCILOP_DECR           = 8,
   SVGA3D_STENCILOP_MAX
} SVGA3dStencilOp;

typedef enum {
   SVGA3D_CLIPPLANE_0              = (1 << 0),
   SVGA3D_CLIPPLANE_1              = (1 << 1),
   SVGA3D_CLIPPLANE_2              = (1 << 2),
   SVGA3D_CLIPPLANE_3              = (1 << 3),
   SVGA3D_CLIPPLANE_4              = (1 << 4),
   SVGA3D_CLIPPLANE_5              = (1 << 5),
} SVGA3dClipPlanes;

typedef enum {
   SVGA3D_CLEAR_COLOR              = 0x1,
   SVGA3D_CLEAR_DEPTH              = 0x2,
   SVGA3D_CLEAR_STENCIL            = 0x4
} SVGA3dClearFlag;

typedef enum {
   SVGA3D_RT_DEPTH                 = 0,
   SVGA3D_RT_STENCIL               = 1,
   SVGA3D_RT_COLOR0                = 2,
   SVGA3D_RT_COLOR1                = 3,
   SVGA3D_RT_COLOR2                = 4,
   SVGA3D_RT_COLOR3                = 5,
   SVGA3D_RT_COLOR4                = 6,
   SVGA3D_RT_COLOR5                = 7,
   SVGA3D_RT_COLOR6                = 8,
   SVGA3D_RT_COLOR7                = 9,
   SVGA3D_RT_MAX,
   SVGA3D_RT_INVALID               = ((uint32)-1),
} SVGA3dRenderTargetType;

#define SVGA3D_MAX_RT_COLOR (SVGA3D_RT_COLOR7 - SVGA3D_RT_COLOR0 + 1)

typedef
union {
   struct {
      uint32  red   : 1;
      uint32  green : 1;
      uint32  blue  : 1;
      uint32  alpha : 1;
   };
   uint32 uintValue;
} SVGA3dColorMask;

typedef enum {
   SVGA3D_VBLEND_DISABLE            = 0,
   SVGA3D_VBLEND_1WEIGHT            = 1,
   SVGA3D_VBLEND_2WEIGHT            = 2,
   SVGA3D_VBLEND_3WEIGHT            = 3,
} SVGA3dVertexBlendFlags;

typedef enum {
   SVGA3D_WRAPCOORD_0   = 1 << 0,
   SVGA3D_WRAPCOORD_1   = 1 << 1,
   SVGA3D_WRAPCOORD_2   = 1 << 2,
   SVGA3D_WRAPCOORD_3   = 1 << 3,
   SVGA3D_WRAPCOORD_ALL = 0xF,
} SVGA3dWrapFlags;

/*
 * SVGA_3D_CMD_TEXTURESTATE Types.  All value types
 * must fit in a uint32.
 */

typedef enum {
   SVGA3D_TS_INVALID                    = 0,
   SVGA3D_TS_MIN                        = 1,
   SVGA3D_TS_BIND_TEXTURE               = 1,    /* SVGA3dSurfaceId */
   SVGA3D_TS_COLOROP                    = 2,    /* SVGA3dTextureCombiner */
   SVGA3D_TS_COLORARG1                  = 3,    /* SVGA3dTextureArgData */
   SVGA3D_TS_COLORARG2                  = 4,    /* SVGA3dTextureArgData */
   SVGA3D_TS_ALPHAOP                    = 5,    /* SVGA3dTextureCombiner */
   SVGA3D_TS_ALPHAARG1                  = 6,    /* SVGA3dTextureArgData */
   SVGA3D_TS_ALPHAARG2                  = 7,    /* SVGA3dTextureArgData */
   SVGA3D_TS_ADDRESSU                   = 8,    /* SVGA3dTextureAddress */
   SVGA3D_TS_ADDRESSV                   = 9,    /* SVGA3dTextureAddress */
   SVGA3D_TS_MIPFILTER                  = 10,   /* SVGA3dTextureFilter */
   SVGA3D_TS_MAGFILTER                  = 11,   /* SVGA3dTextureFilter */
   SVGA3D_TS_MINFILTER                  = 12,   /* SVGA3dTextureFilter */
   SVGA3D_TS_BORDERCOLOR                = 13,   /* SVGA3dColor */
   SVGA3D_TS_TEXCOORDINDEX              = 14,   /* uint32 */
   SVGA3D_TS_TEXTURETRANSFORMFLAGS      = 15,   /* SVGA3dTexTransformFlags */
   SVGA3D_TS_TEXCOORDGEN                = 16,   /* SVGA3dTextureCoordGen */
   SVGA3D_TS_BUMPENVMAT00               = 17,   /* float */
   SVGA3D_TS_BUMPENVMAT01               = 18,   /* float */
   SVGA3D_TS_BUMPENVMAT10               = 19,   /* float */
   SVGA3D_TS_BUMPENVMAT11               = 20,   /* float */
   SVGA3D_TS_TEXTURE_MIPMAP_LEVEL       = 21,   /* uint32 */
   SVGA3D_TS_TEXTURE_LOD_BIAS           = 22,   /* float */
   SVGA3D_TS_TEXTURE_ANISOTROPIC_LEVEL  = 23,   /* uint32 */
   SVGA3D_TS_ADDRESSW                   = 24,   /* SVGA3dTextureAddress */


   /*
    * Sampler Gamma Level
    *
    * Sampler gamma effects the color of samples taken from the sampler.  A
    * value of 1.0 will produce linear samples.  If the value is <= 0.0 the
    * gamma value is ignored and a linear space is used.
    */

   SVGA3D_TS_GAMMA                      = 25,   /* float */
   SVGA3D_TS_BUMPENVLSCALE              = 26,   /* float */
   SVGA3D_TS_BUMPENVLOFFSET             = 27,   /* float */
   SVGA3D_TS_COLORARG0                  = 28,   /* SVGA3dTextureArgData */
   SVGA3D_TS_ALPHAARG0                  = 29,   /* SVGA3dTextureArgData */
   SVGA3D_TS_PREGB_MAX                  = 30,   /* Max value before GBObjects */
   SVGA3D_TS_CONSTANT                   = 30,   /* SVGA3dColor */
   SVGA3D_TS_COLOR_KEY_ENABLE           = 31,   /* SVGA3dBool */
   SVGA3D_TS_COLOR_KEY                  = 32,   /* SVGA3dColor */
   SVGA3D_TS_MAX
} SVGA3dTextureStateName;

typedef enum {
   SVGA3D_TC_INVALID                   = 0,
   SVGA3D_TC_DISABLE                   = 1,
   SVGA3D_TC_SELECTARG1                = 2,
   SVGA3D_TC_SELECTARG2                = 3,
   SVGA3D_TC_MODULATE                  = 4,
   SVGA3D_TC_ADD                       = 5,
   SVGA3D_TC_ADDSIGNED                 = 6,
   SVGA3D_TC_SUBTRACT                  = 7,
   SVGA3D_TC_BLENDTEXTUREALPHA         = 8,
   SVGA3D_TC_BLENDDIFFUSEALPHA         = 9,
   SVGA3D_TC_BLENDCURRENTALPHA         = 10,
   SVGA3D_TC_BLENDFACTORALPHA          = 11,
   SVGA3D_TC_MODULATE2X                = 12,
   SVGA3D_TC_MODULATE4X                = 13,
   SVGA3D_TC_DSDT                      = 14,
   SVGA3D_TC_DOTPRODUCT3               = 15,
   SVGA3D_TC_BLENDTEXTUREALPHAPM       = 16,
   SVGA3D_TC_ADDSIGNED2X               = 17,
   SVGA3D_TC_ADDSMOOTH                 = 18,
   SVGA3D_TC_PREMODULATE               = 19,
   SVGA3D_TC_MODULATEALPHA_ADDCOLOR    = 20,
   SVGA3D_TC_MODULATECOLOR_ADDALPHA    = 21,
   SVGA3D_TC_MODULATEINVALPHA_ADDCOLOR = 22,
   SVGA3D_TC_MODULATEINVCOLOR_ADDALPHA = 23,
   SVGA3D_TC_BUMPENVMAPLUMINANCE       = 24,
   SVGA3D_TC_MULTIPLYADD               = 25,
   SVGA3D_TC_LERP                      = 26,
   SVGA3D_TC_MAX
} SVGA3dTextureCombiner;

#define SVGA3D_TC_CAP_BIT(svga3d_tc_op) (svga3d_tc_op ? (1 << (svga3d_tc_op - 1)) : 0)

typedef enum {
   SVGA3D_TEX_ADDRESS_INVALID    = 0,
   SVGA3D_TEX_ADDRESS_WRAP       = 1,
   SVGA3D_TEX_ADDRESS_MIRROR     = 2,
   SVGA3D_TEX_ADDRESS_CLAMP      = 3,
   SVGA3D_TEX_ADDRESS_BORDER     = 4,
   SVGA3D_TEX_ADDRESS_MIRRORONCE = 5,
   SVGA3D_TEX_ADDRESS_EDGE       = 6,
   SVGA3D_TEX_ADDRESS_MAX
} SVGA3dTextureAddress;

/*
 * SVGA3D_TEX_FILTER_NONE as the minification filter means mipmapping is
 * disabled, and the rasterizer should use the magnification filter instead.
 */
typedef enum {
   SVGA3D_TEX_FILTER_NONE           = 0,
   SVGA3D_TEX_FILTER_NEAREST        = 1,
   SVGA3D_TEX_FILTER_LINEAR         = 2,
   SVGA3D_TEX_FILTER_ANISOTROPIC    = 3,
   SVGA3D_TEX_FILTER_FLATCUBIC      = 4, // Deprecated, not implemented
   SVGA3D_TEX_FILTER_GAUSSIANCUBIC  = 5, // Deprecated, not implemented
   SVGA3D_TEX_FILTER_PYRAMIDALQUAD  = 6, // Not currently implemented
   SVGA3D_TEX_FILTER_GAUSSIANQUAD   = 7, // Not currently implemented
   SVGA3D_TEX_FILTER_MAX
} SVGA3dTextureFilter;

typedef enum {
   SVGA3D_TEX_TRANSFORM_OFF    = 0,
   SVGA3D_TEX_TRANSFORM_S      = (1 << 0),
   SVGA3D_TEX_TRANSFORM_T      = (1 << 1),
   SVGA3D_TEX_TRANSFORM_R      = (1 << 2),
   SVGA3D_TEX_TRANSFORM_Q      = (1 << 3),
   SVGA3D_TEX_PROJECTED        = (1 << 15),
} SVGA3dTexTransformFlags;

typedef enum {
   SVGA3D_TEXCOORD_GEN_OFF              = 0,
   SVGA3D_TEXCOORD_GEN_EYE_POSITION     = 1,
   SVGA3D_TEXCOORD_GEN_EYE_NORMAL       = 2,
   SVGA3D_TEXCOORD_GEN_REFLECTIONVECTOR = 3,
   SVGA3D_TEXCOORD_GEN_SPHERE           = 4,
   SVGA3D_TEXCOORD_GEN_MAX
} SVGA3dTextureCoordGen;

/*
 * Texture argument constants for texture combiner
 */
typedef enum {
   SVGA3D_TA_INVALID    = 0,
   SVGA3D_TA_CONSTANT   = 1,
   SVGA3D_TA_PREVIOUS   = 2,
   SVGA3D_TA_DIFFUSE    = 3,
   SVGA3D_TA_TEXTURE    = 4,
   SVGA3D_TA_SPECULAR   = 5,
   SVGA3D_TA_MAX
} SVGA3dTextureArgData;

#define SVGA3D_TM_MASK_LEN 4

/* Modifiers for texture argument constants defined above. */
typedef enum {
   SVGA3D_TM_NONE       = 0,
   SVGA3D_TM_ALPHA      = (1 << SVGA3D_TM_MASK_LEN),
   SVGA3D_TM_ONE_MINUS  = (2 << SVGA3D_TM_MASK_LEN),
} SVGA3dTextureArgModifier;

#define SVGA3D_INVALID_ID         ((uint32)-1)
#define SVGA3D_MAX_CLIP_PLANES    6

/*
 * This is the limit to the number of fixed-function texture
 * transforms and texture coordinates we can support. It does *not*
 * correspond to the number of texture image units (samplers) we
 * support!
 */
#define SVGA3D_MAX_TEXTURE_COORDS 8

/*
 * Vertex declarations
 *
 * Notes:
 *
 * SVGA3D_DECLUSAGE_POSITIONT is for pre-transformed vertices. If you
 * draw with any POSITIONT vertex arrays, the programmable vertex
 * pipeline will be implicitly disabled. Drawing will take place as if
 * no vertex shader was bound.
 */

typedef enum {
   SVGA3D_DECLUSAGE_POSITION     = 0,
   SVGA3D_DECLUSAGE_BLENDWEIGHT,       //  1
   SVGA3D_DECLUSAGE_BLENDINDICES,      //  2
   SVGA3D_DECLUSAGE_NORMAL,            //  3
   SVGA3D_DECLUSAGE_PSIZE,             //  4
   SVGA3D_DECLUSAGE_TEXCOORD,          //  5
   SVGA3D_DECLUSAGE_TANGENT,           //  6
   SVGA3D_DECLUSAGE_BINORMAL,          //  7
   SVGA3D_DECLUSAGE_TESSFACTOR,        //  8
   SVGA3D_DECLUSAGE_POSITIONT,         //  9
   SVGA3D_DECLUSAGE_COLOR,             // 10
   SVGA3D_DECLUSAGE_FOG,               // 11
   SVGA3D_DECLUSAGE_DEPTH,             // 12
   SVGA3D_DECLUSAGE_SAMPLE,            // 13
   SVGA3D_DECLUSAGE_MAX
} SVGA3dDeclUsage;

typedef enum {
   SVGA3D_DECLMETHOD_DEFAULT     = 0,
   SVGA3D_DECLMETHOD_PARTIALU,
   SVGA3D_DECLMETHOD_PARTIALV,
   SVGA3D_DECLMETHOD_CROSSUV,          // Normal
   SVGA3D_DECLMETHOD_UV,
   SVGA3D_DECLMETHOD_LOOKUP,           // Lookup a displacement map
   SVGA3D_DECLMETHOD_LOOKUPPRESAMPLED, // Lookup a pre-sampled displacement map
} SVGA3dDeclMethod;

typedef enum {
   SVGA3D_DECLTYPE_FLOAT1        =  0,
   SVGA3D_DECLTYPE_FLOAT2        =  1,
   SVGA3D_DECLTYPE_FLOAT3        =  2,
   SVGA3D_DECLTYPE_FLOAT4        =  3,
   SVGA3D_DECLTYPE_D3DCOLOR      =  4,
   SVGA3D_DECLTYPE_UBYTE4        =  5,
   SVGA3D_DECLTYPE_SHORT2        =  6,
   SVGA3D_DECLTYPE_SHORT4        =  7,
   SVGA3D_DECLTYPE_UBYTE4N       =  8,
   SVGA3D_DECLTYPE_SHORT2N       =  9,
   SVGA3D_DECLTYPE_SHORT4N       = 10,
   SVGA3D_DECLTYPE_USHORT2N      = 11,
   SVGA3D_DECLTYPE_USHORT4N      = 12,
   SVGA3D_DECLTYPE_UDEC3         = 13,
   SVGA3D_DECLTYPE_DEC3N         = 14,
   SVGA3D_DECLTYPE_FLOAT16_2     = 15,
   SVGA3D_DECLTYPE_FLOAT16_4     = 16,
   SVGA3D_DECLTYPE_MAX,
} SVGA3dDeclType;

/*
 * This structure is used for the divisor for geometry instancing;
 * it's a direct translation of the Direct3D equivalent.
 */
typedef union {
   struct {
      /*
       * For index data, this number represents the number of instances to draw.
       * For instance data, this number represents the number of
       * instances/vertex in this stream
       */
      uint32 count : 30;

      /*
       * This is 1 if this is supposed to be the data that is repeated for
       * every instance.
       */
      uint32 indexedData : 1;

      /*
       * This is 1 if this is supposed to be the per-instance data.
       */
      uint32 instanceData : 1;
   };

   uint32 value;
} SVGA3dVertexDivisor;

typedef enum {
   SVGA3D_PRIMITIVE_INVALID                     = 0,
   SVGA3D_PRIMITIVE_TRIANGLELIST                = 1,
   SVGA3D_PRIMITIVE_POINTLIST                   = 2,
   SVGA3D_PRIMITIVE_LINELIST                    = 3,
   SVGA3D_PRIMITIVE_LINESTRIP                   = 4,
   SVGA3D_PRIMITIVE_TRIANGLESTRIP               = 5,
   SVGA3D_PRIMITIVE_TRIANGLEFAN                 = 6,
   SVGA3D_PRIMITIVE_MAX
} SVGA3dPrimitiveType;

typedef enum {
   SVGA3D_COORDINATE_INVALID                   = 0,
   SVGA3D_COORDINATE_LEFTHANDED                = 1,
   SVGA3D_COORDINATE_RIGHTHANDED               = 2,
   SVGA3D_COORDINATE_MAX
} SVGA3dCoordinateType;

typedef enum {
   SVGA3D_TRANSFORM_INVALID                     = 0,
   SVGA3D_TRANSFORM_WORLD                       = 1,
   SVGA3D_TRANSFORM_VIEW                        = 2,
   SVGA3D_TRANSFORM_PROJECTION                  = 3,
   SVGA3D_TRANSFORM_TEXTURE0                    = 4,
   SVGA3D_TRANSFORM_TEXTURE1                    = 5,
   SVGA3D_TRANSFORM_TEXTURE2                    = 6,
   SVGA3D_TRANSFORM_TEXTURE3                    = 7,
   SVGA3D_TRANSFORM_TEXTURE4                    = 8,
   SVGA3D_TRANSFORM_TEXTURE5                    = 9,
   SVGA3D_TRANSFORM_TEXTURE6                    = 10,
   SVGA3D_TRANSFORM_TEXTURE7                    = 11,
   SVGA3D_TRANSFORM_WORLD1                      = 12,
   SVGA3D_TRANSFORM_WORLD2                      = 13,
   SVGA3D_TRANSFORM_WORLD3                      = 14,
   SVGA3D_TRANSFORM_MAX
} SVGA3dTransformType;

typedef enum {
   SVGA3D_LIGHTTYPE_INVALID                     = 0,
   SVGA3D_LIGHTTYPE_POINT                       = 1,
   SVGA3D_LIGHTTYPE_SPOT1                       = 2, /* 1-cone, in degrees */
   SVGA3D_LIGHTTYPE_SPOT2                       = 3, /* 2-cone, in radians */
   SVGA3D_LIGHTTYPE_DIRECTIONAL                 = 4,
   SVGA3D_LIGHTTYPE_MAX
} SVGA3dLightType;

typedef enum {
   SVGA3D_CUBEFACE_POSX                         = 0,
   SVGA3D_CUBEFACE_NEGX                         = 1,
   SVGA3D_CUBEFACE_POSY                         = 2,
   SVGA3D_CUBEFACE_NEGY                         = 3,
   SVGA3D_CUBEFACE_POSZ                         = 4,
   SVGA3D_CUBEFACE_NEGZ                         = 5,
} SVGA3dCubeFace;

typedef enum {
   SVGA3D_SHADERTYPE_INVALID                    = 0,
   SVGA3D_SHADERTYPE_MIN                        = 1,
   SVGA3D_SHADERTYPE_VS                         = 1,
   SVGA3D_SHADERTYPE_PS                         = 2,
   SVGA3D_SHADERTYPE_PREDX_MAX                  = 3,
   SVGA3D_SHADERTYPE_GS                         = 3,
   SVGA3D_SHADERTYPE_DX10_MAX                   = 4,
   SVGA3D_SHADERTYPE_HS                         = 4,
   SVGA3D_SHADERTYPE_DS                         = 5,
   SVGA3D_SHADERTYPE_CS                         = 6,
   SVGA3D_SHADERTYPE_MAX                        = 7
} SVGA3dShaderType;

#define SVGA3D_NUM_SHADERTYPE_PREDX \
   (SVGA3D_SHADERTYPE_PREDX_MAX - SVGA3D_SHADERTYPE_MIN)

#define SVGA3D_NUM_SHADERTYPE_DX10 \
   (SVGA3D_SHADERTYPE_DX10_MAX - SVGA3D_SHADERTYPE_MIN)

#define SVGA3D_NUM_SHADERTYPE \
   (SVGA3D_SHADERTYPE_MAX - SVGA3D_SHADERTYPE_MIN)

/*
 * Register limits for shader consts.
 */
#define SVGA3D_CONSTREG_MAX            256
#define SVGA3D_CONSTINTREG_MAX         16
#define SVGA3D_CONSTBOOLREG_MAX        16

typedef enum {
   SVGA3D_CONST_TYPE_FLOAT                      = 0,
   SVGA3D_CONST_TYPE_INT                        = 1,
   SVGA3D_CONST_TYPE_BOOL                       = 2,
} SVGA3dShaderConstType;

#define SVGA3D_MAX_SURFACE_FACES                6

typedef enum {
   SVGA3D_STRETCH_BLT_POINT                     = 0,
   SVGA3D_STRETCH_BLT_LINEAR                    = 1,
   SVGA3D_STRETCH_BLT_MAX
} SVGA3dStretchBltMode;

typedef enum {
   SVGA3D_QUERYTYPE_OCCLUSION                   = 0,
   SVGA3D_QUERYTYPE_MAX
} SVGA3dQueryType;

typedef enum {
   SVGA3D_QUERYSTATE_PENDING     = 0,      /* Waiting on the host (set by guest) */
   SVGA3D_QUERYSTATE_SUCCEEDED   = 1,      /* Completed successfully (set by host) */
   SVGA3D_QUERYSTATE_FAILED      = 2,      /* Completed unsuccessfully (set by host) */
   SVGA3D_QUERYSTATE_NEW         = 3,      /* Never submitted (For guest use only) */
} SVGA3dQueryState;

typedef enum {
   SVGA3D_WRITE_HOST_VRAM        = 1,
   SVGA3D_READ_HOST_VRAM         = 2,
} SVGA3dTransferType;

/*
 * The maximum number of vertex arrays we're guaranteed to support in
 * SVGA_3D_CMD_DRAWPRIMITIVES.
 */
#define SVGA3D_MAX_VERTEX_ARRAYS   32

/*
 * The maximum number of primitive ranges we're guaranteed to support
 * in SVGA_3D_CMD_DRAWPRIMITIVES.
 */
#define SVGA3D_MAX_DRAW_PRIMITIVE_RANGES 32

/*
 * Identifiers for commands in the command FIFO.
 *
 * IDs between 1000 and 1039 (inclusive) were used by obsolete versions of
 * the SVGA3D protocol and remain reserved; they should not be used in the
 * future.
 *
 * IDs between 1040 and 1999 (inclusive) are available for use by the
 * current SVGA3D protocol.
 *
 * FIFO clients other than SVGA3D should stay below 1000, or at 2000
 * and up.
 */

#define SVGA_3D_CMD_LEGACY_BASE            1000
#define SVGA_3D_CMD_BASE                   1040

#define SVGA_3D_CMD_SURFACE_DEFINE         SVGA_3D_CMD_BASE + 0     // Deprecated
#define SVGA_3D_CMD_SURFACE_DESTROY        SVGA_3D_CMD_BASE + 1
#define SVGA_3D_CMD_SURFACE_COPY           SVGA_3D_CMD_BASE + 2
#define SVGA_3D_CMD_SURFACE_STRETCHBLT     SVGA_3D_CMD_BASE + 3
#define SVGA_3D_CMD_SURFACE_DMA            SVGA_3D_CMD_BASE + 4
#define SVGA_3D_CMD_CONTEXT_DEFINE         SVGA_3D_CMD_BASE + 5
#define SVGA_3D_CMD_CONTEXT_DESTROY        SVGA_3D_CMD_BASE + 6
#define SVGA_3D_CMD_SETTRANSFORM           SVGA_3D_CMD_BASE + 7
#define SVGA_3D_CMD_SETZRANGE              SVGA_3D_CMD_BASE + 8
#define SVGA_3D_CMD_SETRENDERSTATE         SVGA_3D_CMD_BASE + 9
#define SVGA_3D_CMD_SETRENDERTARGET        SVGA_3D_CMD_BASE + 10
#define SVGA_3D_CMD_SETTEXTURESTATE        SVGA_3D_CMD_BASE + 11
#define SVGA_3D_CMD_SETMATERIAL            SVGA_3D_CMD_BASE + 12
#define SVGA_3D_CMD_SETLIGHTDATA           SVGA_3D_CMD_BASE + 13
#define SVGA_3D_CMD_SETLIGHTENABLED        SVGA_3D_CMD_BASE + 14
#define SVGA_3D_CMD_SETVIEWPORT            SVGA_3D_CMD_BASE + 15
#define SVGA_3D_CMD_SETCLIPPLANE           SVGA_3D_CMD_BASE + 16
#define SVGA_3D_CMD_CLEAR                  SVGA_3D_CMD_BASE + 17
#define SVGA_3D_CMD_PRESENT                SVGA_3D_CMD_BASE + 18    // Deprecated
#define SVGA_3D_CMD_SHADER_DEFINE          SVGA_3D_CMD_BASE + 19
#define SVGA_3D_CMD_SHADER_DESTROY         SVGA_3D_CMD_BASE + 20
#define SVGA_3D_CMD_SET_SHADER             SVGA_3D_CMD_BASE + 21
#define SVGA_3D_CMD_SET_SHADER_CONST       SVGA_3D_CMD_BASE + 22
#define SVGA_3D_CMD_DRAW_PRIMITIVES        SVGA_3D_CMD_BASE + 23
#define SVGA_3D_CMD_SETSCISSORRECT         SVGA_3D_CMD_BASE + 24
#define SVGA_3D_CMD_BEGIN_QUERY            SVGA_3D_CMD_BASE + 25
#define SVGA_3D_CMD_END_QUERY              SVGA_3D_CMD_BASE + 26
#define SVGA_3D_CMD_WAIT_FOR_QUERY         SVGA_3D_CMD_BASE + 27
#define SVGA_3D_CMD_PRESENT_READBACK       SVGA_3D_CMD_BASE + 28    // Deprecated
#define SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN SVGA_3D_CMD_BASE + 29
#define SVGA_3D_CMD_SURFACE_DEFINE_V2      SVGA_3D_CMD_BASE + 30
#define SVGA_3D_CMD_GENERATE_MIPMAPS       SVGA_3D_CMD_BASE + 31
#define SVGA_3D_CMD_ACTIVATE_SURFACE       SVGA_3D_CMD_BASE + 40
#define SVGA_3D_CMD_DEACTIVATE_SURFACE     SVGA_3D_CMD_BASE + 41
//#define SVGA_3D_CMD_MAX                    SVGA_3D_CMD_BASE + 42

/* GPU10 commands */
#define SVGA_3D_CMD_SCREEN_DMA		1082
#define SVGA_3D_CMD_DEAD1		1083
#define SVGA_3D_CMD_DEAD2		1084

#define SVGA_3D_CMD_DEAD12		1085
#define SVGA_3D_CMD_DEAD13		1086
#define SVGA_3D_CMD_DEAD14		1087
#define SVGA_3D_CMD_DEAD15		1088
#define SVGA_3D_CMD_DEAD16		1089
#define SVGA_3D_CMD_DEAD17		1090

#define SVGA_3D_CMD_SET_OTABLE_BASE		1091
#define SVGA_3D_CMD_READBACK_OTABLE		1092

#define SVGA_3D_CMD_DEFINE_GB_MOB		1093
#define SVGA_3D_CMD_DESTROY_GB_MOB		1094
#define SVGA_3D_CMD_DEAD3		1095
#define SVGA_3D_CMD_UPDATE_GB_MOB_MAPPING		1096

#define SVGA_3D_CMD_DEFINE_GB_SURFACE		1097
#define SVGA_3D_CMD_DESTROY_GB_SURFACE		1098
#define SVGA_3D_CMD_BIND_GB_SURFACE		1099
#define SVGA_3D_CMD_COND_BIND_GB_SURFACE		1100
#define SVGA_3D_CMD_UPDATE_GB_IMAGE		1101
#define SVGA_3D_CMD_UPDATE_GB_SURFACE		1102
#define SVGA_3D_CMD_READBACK_GB_IMAGE		1103
#define SVGA_3D_CMD_READBACK_GB_SURFACE		1104
#define SVGA_3D_CMD_INVALIDATE_GB_IMAGE		1105
#define SVGA_3D_CMD_INVALIDATE_GB_SURFACE		1106

#define SVGA_3D_CMD_DEFINE_GB_CONTEXT		1107
#define SVGA_3D_CMD_DESTROY_GB_CONTEXT		1108
#define SVGA_3D_CMD_BIND_GB_CONTEXT		1109
#define SVGA_3D_CMD_READBACK_GB_CONTEXT		1110
#define SVGA_3D_CMD_INVALIDATE_GB_CONTEXT		1111

#define SVGA_3D_CMD_DEFINE_GB_SHADER		1112
#define SVGA_3D_CMD_DESTROY_GB_SHADER		1113
#define SVGA_3D_CMD_BIND_GB_SHADER		1114

#define SVGA_3D_CMD_SET_OTABLE_BASE64		1115

#define SVGA_3D_CMD_BEGIN_GB_QUERY		1116
#define SVGA_3D_CMD_END_GB_QUERY		1117
#define SVGA_3D_CMD_WAIT_FOR_GB_QUERY		1118

#define SVGA_3D_CMD_NOP		1119

#define SVGA_3D_CMD_ENABLE_GART		1120
#define SVGA_3D_CMD_DISABLE_GART		1121
#define SVGA_3D_CMD_MAP_MOB_INTO_GART		1122
#define SVGA_3D_CMD_UNMAP_GART_RANGE		1123

#define SVGA_3D_CMD_DEFINE_GB_SCREENTARGET		1124
#define SVGA_3D_CMD_DESTROY_GB_SCREENTARGET		1125
#define SVGA_3D_CMD_BIND_GB_SCREENTARGET		1126
#define SVGA_3D_CMD_UPDATE_GB_SCREENTARGET		1127

#define SVGA_3D_CMD_READBACK_GB_IMAGE_PARTIAL		1128
#define SVGA_3D_CMD_INVALIDATE_GB_IMAGE_PARTIAL		1129

#define SVGA_3D_CMD_SET_GB_SHADERCONSTS_INLINE		1130

#define SVGA_3D_CMD_GB_SCREEN_DMA		1131
#define SVGA_3D_CMD_BIND_GB_SURFACE_WITH_PITCH		1132
#define SVGA_3D_CMD_GB_MOB_FENCE		1133
#define SVGA_3D_CMD_DEFINE_GB_SURFACE_V2		1134
#define SVGA_3D_CMD_DEFINE_GB_MOB64		1135
#define SVGA_3D_CMD_REDEFINE_GB_MOB64		1136
#define SVGA_3D_CMD_NOP_ERROR		1137

#define SVGA_3D_CMD_SET_VERTEX_STREAMS		1138
#define SVGA_3D_CMD_SET_VERTEX_DECLS		1139
#define SVGA_3D_CMD_SET_VERTEX_DIVISORS		1140
#define SVGA_3D_CMD_DRAW		1141
#define SVGA_3D_CMD_DRAW_INDEXED		1142

#define SVGA_3D_CMD_DX_MIN		1143
#define SVGA_3D_CMD_DX_DEFINE_CONTEXT		1143
#define SVGA_3D_CMD_DX_DESTROY_CONTEXT		1144
#define SVGA_3D_CMD_DX_BIND_CONTEXT		1145
#define SVGA_3D_CMD_DX_READBACK_CONTEXT		1146
#define SVGA_3D_CMD_DX_INVALIDATE_CONTEXT		1147
#define SVGA_3D_CMD_DX_SET_SINGLE_CONSTANT_BUFFER		1148
#define SVGA_3D_CMD_DX_SET_SHADER_RESOURCES		1149
#define SVGA_3D_CMD_DX_SET_SHADER		1150
#define SVGA_3D_CMD_DX_SET_SAMPLERS		1151
#define SVGA_3D_CMD_DX_DRAW		1152
#define SVGA_3D_CMD_DX_DRAW_INDEXED		1153
#define SVGA_3D_CMD_DX_DRAW_INSTANCED		1154
#define SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED		1155
#define SVGA_3D_CMD_DX_DRAW_AUTO		1156
#define SVGA_3D_CMD_DX_SET_INPUT_LAYOUT		1157
#define SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS		1158
#define SVGA_3D_CMD_DX_SET_INDEX_BUFFER		1159
#define SVGA_3D_CMD_DX_SET_TOPOLOGY		1160
#define SVGA_3D_CMD_DX_SET_RENDERTARGETS		1161
#define SVGA_3D_CMD_DX_SET_BLEND_STATE		1162
#define SVGA_3D_CMD_DX_SET_DEPTHSTENCIL_STATE		1163
#define SVGA_3D_CMD_DX_SET_RASTERIZER_STATE		1164
#define SVGA_3D_CMD_DX_DEFINE_QUERY		1165
#define SVGA_3D_CMD_DX_DESTROY_QUERY		1166
#define SVGA_3D_CMD_DX_BIND_QUERY		1167
#define SVGA_3D_CMD_DX_SET_QUERY_OFFSET		1168
#define SVGA_3D_CMD_DX_BEGIN_QUERY		1169
#define SVGA_3D_CMD_DX_END_QUERY		1170
#define SVGA_3D_CMD_DX_READBACK_QUERY		1171
#define SVGA_3D_CMD_DX_SET_PREDICATION		1172
#define SVGA_3D_CMD_DX_SET_SOTARGETS		1173
#define SVGA_3D_CMD_DX_SET_VIEWPORTS		1174
#define SVGA_3D_CMD_DX_SET_SCISSORRECTS		1175
#define SVGA_3D_CMD_DX_CLEAR_RENDERTARGET_VIEW		1176
#define SVGA_3D_CMD_DX_CLEAR_DEPTHSTENCIL_VIEW		1177
#define SVGA_3D_CMD_DX_PRED_COPY_REGION		1178
#define SVGA_3D_CMD_DX_PRED_COPY		1179
#define SVGA_3D_CMD_DX_PRESENTBLT		1180
#define SVGA_3D_CMD_DX_GENMIPS		1181
#define SVGA_3D_CMD_DX_UPDATE_SUBRESOURCE		1182
#define SVGA_3D_CMD_DX_READBACK_SUBRESOURCE		1183
#define SVGA_3D_CMD_DX_INVALIDATE_SUBRESOURCE		1184
#define SVGA_3D_CMD_DX_DEFINE_SHADERRESOURCE_VIEW		1185
#define SVGA_3D_CMD_DX_DESTROY_SHADERRESOURCE_VIEW		1186
#define SVGA_3D_CMD_DX_DEFINE_RENDERTARGET_VIEW		1187
#define SVGA_3D_CMD_DX_DESTROY_RENDERTARGET_VIEW		1188
#define SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW		1189
#define SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_VIEW		1190
#define SVGA_3D_CMD_DX_DEFINE_ELEMENTLAYOUT		1191
#define SVGA_3D_CMD_DX_DESTROY_ELEMENTLAYOUT		1192
#define SVGA_3D_CMD_DX_DEFINE_BLEND_STATE		1193
#define SVGA_3D_CMD_DX_DESTROY_BLEND_STATE		1194
#define SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_STATE		1195
#define SVGA_3D_CMD_DX_DESTROY_DEPTHSTENCIL_STATE		1196
#define SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE		1197
#define SVGA_3D_CMD_DX_DESTROY_RASTERIZER_STATE		1198
#define SVGA_3D_CMD_DX_DEFINE_SAMPLER_STATE		1199
#define SVGA_3D_CMD_DX_DESTROY_SAMPLER_STATE		1200
#define SVGA_3D_CMD_DX_DEFINE_SHADER		1201
#define SVGA_3D_CMD_DX_DESTROY_SHADER		1202
#define SVGA_3D_CMD_DX_BIND_SHADER		1203
#define SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT		1204
#define SVGA_3D_CMD_DX_DESTROY_STREAMOUTPUT		1205
#define SVGA_3D_CMD_DX_SET_STREAMOUTPUT		1206
#define SVGA_3D_CMD_DX_SET_COTABLE		1207
#define SVGA_3D_CMD_DX_READBACK_COTABLE		1208
#define SVGA_3D_CMD_DX_BUFFER_COPY		1209
#define SVGA_3D_CMD_DX_TRANSFER_FROM_BUFFER		1210
#define SVGA_3D_CMD_DX_SURFACE_COPY_AND_READBACK		1211
#define SVGA_3D_CMD_DX_MOVE_QUERY		1212
#define SVGA_3D_CMD_DX_BIND_ALL_QUERY		1213
#define SVGA_3D_CMD_DX_READBACK_ALL_QUERY		1214
#define SVGA_3D_CMD_DX_PRED_TRANSFER_FROM_BUFFER		1215
#define SVGA_3D_CMD_DX_MOB_FENCE_64		1216
#define SVGA_3D_CMD_DX_BIND_ALL_SHADER		1217
#define SVGA_3D_CMD_DX_HINT		1218
#define SVGA_3D_CMD_DX_BUFFER_UPDATE		1219
#define SVGA_3D_CMD_DX_SET_VS_CONSTANT_BUFFER_OFFSET		1220
#define SVGA_3D_CMD_DX_SET_PS_CONSTANT_BUFFER_OFFSET		1221
#define SVGA_3D_CMD_DX_SET_GS_CONSTANT_BUFFER_OFFSET		1222
#define SVGA_3D_CMD_DX_SET_HS_CONSTANT_BUFFER_OFFSET		1223
#define SVGA_3D_CMD_DX_SET_DS_CONSTANT_BUFFER_OFFSET		1224
#define SVGA_3D_CMD_DX_SET_CS_CONSTANT_BUFFER_OFFSET		1225

#define SVGA_3D_CMD_DX_COND_BIND_ALL_SHADER		1226
#define SVGA_3D_CMD_DX_MAX		1227

#define SVGA_3D_CMD_SCREEN_COPY		1227

#define SVGA_3D_CMD_RESERVED1		1228
#define SVGA_3D_CMD_RESERVED2		1229
#define SVGA_3D_CMD_RESERVED3		1230
#define SVGA_3D_CMD_RESERVED4		1231
#define SVGA_3D_CMD_RESERVED5		1232
#define SVGA_3D_CMD_RESERVED6		1233
#define SVGA_3D_CMD_RESERVED7		1234
#define SVGA_3D_CMD_RESERVED8		1235

#define SVGA_3D_CMD_GROW_OTABLE		1236
#define SVGA_3D_CMD_DX_GROW_COTABLE		1237
#define SVGA_3D_CMD_INTRA_SURFACE_COPY		1238

#define SVGA_3D_CMD_DEFINE_GB_SURFACE_V3		1239

#define SVGA_3D_CMD_DX_RESOLVE_COPY		1240
#define SVGA_3D_CMD_DX_PRED_RESOLVE_COPY		1241
#define SVGA_3D_CMD_DX_PRED_CONVERT_REGION		1242
#define SVGA_3D_CMD_DX_PRED_CONVERT		1243
#define SVGA_3D_CMD_WHOLE_SURFACE_COPY		1244

#define SVGA_3D_CMD_DX_DEFINE_UA_VIEW		1245
#define SVGA_3D_CMD_DX_DESTROY_UA_VIEW		1246
#define SVGA_3D_CMD_DX_CLEAR_UA_VIEW_UINT		1247
#define SVGA_3D_CMD_DX_CLEAR_UA_VIEW_FLOAT		1248
#define SVGA_3D_CMD_DX_COPY_STRUCTURE_COUNT		1249
#define SVGA_3D_CMD_DX_SET_UA_VIEWS		1250

#define SVGA_3D_CMD_DX_DRAW_INDEXED_INSTANCED_INDIRECT		1251
#define SVGA_3D_CMD_DX_DRAW_INSTANCED_INDIRECT		1252
#define SVGA_3D_CMD_DX_DISPATCH		1253
#define SVGA_3D_CMD_DX_DISPATCH_INDIRECT		1254

#define SVGA_3D_CMD_WRITE_ZERO_SURFACE		1255
#define SVGA_3D_CMD_UPDATE_ZERO_SURFACE		1256
#define SVGA_3D_CMD_DX_TRANSFER_TO_BUFFER		1257
#define SVGA_3D_CMD_DX_SET_STRUCTURE_COUNT		1258

#define SVGA_3D_CMD_LOGICOPS_BITBLT		1259
#define SVGA_3D_CMD_LOGICOPS_TRANSBLT		1260
#define SVGA_3D_CMD_LOGICOPS_STRETCHBLT		1261
#define SVGA_3D_CMD_LOGICOPS_COLORFILL		1262
#define SVGA_3D_CMD_LOGICOPS_ALPHABLEND		1263
#define SVGA_3D_CMD_LOGICOPS_CLEARTYPEBLEND		1264

#define SVGA_3D_CMD_DX_COPY_COTABLE_INTO_MOB		1265

#define SVGA_3D_CMD_UPDATE_GB_SCREENTARGET_V2		1266

#define SVGA_3D_CMD_DEFINE_GB_SURFACE_V4		1267
#define SVGA_3D_CMD_DX_SET_CS_UA_VIEWS		1268
#define SVGA_3D_CMD_DX_SET_MIN_LOD		1269

#define SVGA_3D_CMD_DX_DEFINE_DEPTHSTENCIL_VIEW_V2		1272
#define SVGA_3D_CMD_DX_DEFINE_STREAMOUTPUT_WITH_MOB		1273
#define SVGA_3D_CMD_DX_SET_SHADER_IFACE		1274
#define SVGA_3D_CMD_DX_BIND_STREAMOUTPUT		1275
#define SVGA_3D_CMD_SURFACE_STRETCHBLT_NON_MS_TO_MS		1276
#define SVGA_3D_CMD_DX_BIND_SHADER_IFACE		1277

#define SVGA_3D_CMD_UPDATE_GB_SCREENTARGET_MOVE		1278

#define SVGA_3D_CMD_DX_PRED_STAGING_COPY		1281
#define SVGA_3D_CMD_DX_STAGING_COPY		1282
#define SVGA_3D_CMD_DX_PRED_STAGING_COPY_REGION		1283
#define SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS_V2		1284
#define SVGA_3D_CMD_DX_SET_INDEX_BUFFER_V2		1285
#define SVGA_3D_CMD_DX_SET_VERTEX_BUFFERS_OFFSET_AND_SIZE		1286
#define SVGA_3D_CMD_DX_SET_INDEX_BUFFER_OFFSET_AND_SIZE		1287
#define SVGA_3D_CMD_DX_DEFINE_RASTERIZER_STATE_V2		1288
#define SVGA_3D_CMD_DX_PRED_STAGING_CONVERT_REGION		1289
#define SVGA_3D_CMD_DX_PRED_STAGING_CONVERT		1290
#define SVGA_3D_CMD_DX_STAGING_BUFFER_COPY		1291

#define SVGA_3D_CMD_MAX		1303
#define SVGA_3D_CMD_FUTURE_MAX		3000

/*
 * Common substructures used in multiple FIFO commands:
 */

typedef struct {
   union {
      struct {
         uint16  function;       // SVGA3dFogFunction
         uint8   type;           // SVGA3dFogType
         uint8   base;           // SVGA3dFogBase
      };
      uint32     uintValue;
   };
} SVGA3dFogMode;

/*
 * Uniquely identify one image (a 1D/2D/3D array) from a surface. This
 * is a surface ID as well as face/mipmap indices.
 */

typedef
struct SVGA3dSurfaceImageId {
   uint32               sid;
   uint32               face;
   uint32               mipmap;
} SVGA3dSurfaceImageId;

typedef
struct SVGA3dGuestImage {
   SVGAGuestPtr         ptr;

   /*
    * A note on interpretation of pitch: This value of pitch is the
    * number of bytes between vertically adjacent image
    * blocks. Normally this is the number of bytes between the first
    * pixel of two adjacent scanlines. With compressed textures,
    * however, this may represent the number of bytes between
    * compression blocks rather than between rows of pixels.
    *
    * XXX: Compressed textures currently must be tightly packed in guest memory.
    *
    * If the image is 1-dimensional, pitch is ignored.
    *
    * If 'pitch' is zero, the SVGA3D device calculates a pitch value
    * assuming each row of blocks is tightly packed.
    */
   uint32 pitch;
} SVGA3dGuestImage;


/*
 * FIFO command format definitions:
 */

/*
 * The data size header following cmdNum for every 3d command
 */
typedef
struct {
   uint32               id;
   uint32               size;
} SVGA3dCmdHeader;

/*
 * A surface is a hierarchy of host VRAM surfaces: 1D, 2D, or 3D, with
 * optional mipmaps and cube faces.
 */

typedef
struct {
   uint32               width;
   uint32               height;
   uint32               depth;
} SVGA3dSize;

/*
 * Guest-backed objects definitions.
 */
typedef enum {
   SVGA_OTABLE_MOB             = 0,
   SVGA_OTABLE_MIN             = 0,
   SVGA_OTABLE_SURFACE         = 1,
   SVGA_OTABLE_CONTEXT         = 2,
   SVGA_OTABLE_SHADER          = 3,
   SVGA_OTABLE_SCREENTARGET    = 4,

   SVGA_OTABLE_DX9_MAX         = 5,

   SVGA_OTABLE_DXCONTEXT       = 5,
   SVGA_OTABLE_DX_MAX          = 6,

   SVGA_OTABLE_RESERVED1       = 6,
   SVGA_OTABLE_RESERVED2       = 7,

   /*
    * Additions to this table need to be tied to HW-version features and
    * checkpointed accordingly.
    */
   SVGA_OTABLE_DEVEL_MAX       = 8,
   SVGA_OTABLE_MAX             = 8
} SVGAOTableType;

typedef enum {
   SVGA_COTABLE_MIN             = 0,
   SVGA_COTABLE_RTVIEW          = 0,
   SVGA_COTABLE_DSVIEW          = 1,
   SVGA_COTABLE_SRVIEW          = 2,
   SVGA_COTABLE_ELEMENTLAYOUT   = 3,
   SVGA_COTABLE_BLENDSTATE      = 4,
   SVGA_COTABLE_DEPTHSTENCIL    = 5,
   SVGA_COTABLE_RASTERIZERSTATE = 6,
   SVGA_COTABLE_SAMPLER         = 7,
   SVGA_COTABLE_STREAMOUTPUT    = 8,
   SVGA_COTABLE_DXQUERY         = 9,
   SVGA_COTABLE_DXSHADER        = 10,
   SVGA_COTABLE_DX10_MAX        = 11,
   SVGA_COTABLE_UAVIEW          = 11,
   SVGA_COTABLE_MAX             = 12,
} SVGACOTableType;


/*
 * The largest size (number of entries) allowed in a COTable.
 */
#define SVGA_COTABLE_MAX_IDS (MAX_UINT16 - 2)

typedef enum SVGAMobFormat {
   SVGA3D_MOBFMT_INVALID     = SVGA3D_INVALID_ID,
   SVGA3D_MOBFMT_PTDEPTH_0   = 0,
   SVGA3D_MOBFMT_MIN         = 0,
   SVGA3D_MOBFMT_PTDEPTH_1   = 1,
   SVGA3D_MOBFMT_PTDEPTH_2   = 2,
   SVGA3D_MOBFMT_RANGE       = 3,
   SVGA3D_MOBFMT_PTDEPTH64_0 = 4,
   SVGA3D_MOBFMT_PTDEPTH64_1 = 5,
   SVGA3D_MOBFMT_PTDEPTH64_2 = 6,
   SVGA3D_MOBFMT_PREDX_MAX   = 7,
   SVGA3D_MOBFMT_EMPTY       = 7,
   SVGA3D_MOBFMT_MAX,

   /*
    * This isn't actually used by the guest, but is a mob-format used
    * internally by the SVGA device (and is therefore not binary compatible).
    */
   SVGA3D_MOBFMT_HB,
} SVGAMobFormat;

#define SVGA3D_MOB_EMPTY_BASE 1

/*
 * Multisample pattern types.
 */

typedef enum SVGA3dMSPattern {
   SVGA3D_MS_PATTERN_NONE     = 0,
   SVGA3D_MS_PATTERN_MIN      = 0,
   SVGA3D_MS_PATTERN_STANDARD = 1,
   SVGA3D_MS_PATTERN_CENTER   = 2,
   SVGA3D_MS_PATTERN_MAX      = 3,
} SVGA3dMSPattern;

/*
 * Precision settings for each sample.
 */

typedef enum SVGA3dMSQualityLevel {
   SVGA3D_MS_QUALITY_NONE = 0,
   SVGA3D_MS_QUALITY_MIN  = 0,
   SVGA3D_MS_QUALITY_FULL = 1,
   SVGA3D_MS_QUALITY_MAX  = 2,
} SVGA3dMSQualityLevel;


typedef enum {
   SVGA3D_SURFACE_CUBEMAP              = (1 << 0),
   SVGA3D_SURFACE_HINT_STATIC          = (1 << 1),
   SVGA3D_SURFACE_HINT_DYNAMIC         = (1 << 2),
   SVGA3D_SURFACE_HINT_INDEXBUFFER     = (1 << 3),
   SVGA3D_SURFACE_HINT_VERTEXBUFFER    = (1 << 4),
   SVGA3D_SURFACE_HINT_TEXTURE         = (1 << 5),
   SVGA3D_SURFACE_HINT_RENDERTARGET    = (1 << 6),
   SVGA3D_SURFACE_HINT_DEPTHSTENCIL    = (1 << 7),
   SVGA3D_SURFACE_HINT_WRITEONLY       = (1 << 8),
   SVGA3D_SURFACE_MASKABLE_ANTIALIAS   = (1 << 9),
   SVGA3D_SURFACE_AUTOGENMIPMAPS       = (1 << 10),
} SVGA3dSurfaceFlags;

typedef
struct {
   uint32               numMipLevels;
} SVGA3dSurfaceFace;

typedef
struct {
   uint32                      sid;
   SVGA3dSurfaceFlags          surfaceFlags;
   SVGA3dSurfaceFormat         format;
   /*
    * If surfaceFlags has SVGA3D_SURFACE_CUBEMAP bit set, all SVGA3dSurfaceFace
    * structures must have the same value of numMipLevels field.
    * Otherwise, all but the first SVGA3dSurfaceFace structures must have the
    * numMipLevels set to 0.
    */
   SVGA3dSurfaceFace           face[SVGA3D_MAX_SURFACE_FACES];
   /*
    * Followed by an SVGA3dSize structure for each mip level in each face.
    *
    * A note on surface sizes: Sizes are always specified in pixels,
    * even if the true surface size is not a multiple of the minimum
    * block size of the surface's format. For example, a 3x3x1 DXT1
    * compressed texture would actually be stored as a 4x4x1 image in
    * memory.
    */
} SVGA3dCmdDefineSurface;       /* SVGA_3D_CMD_SURFACE_DEFINE */

typedef
struct {
   uint32                      sid;
   SVGA3dSurfaceFlags          surfaceFlags;
   SVGA3dSurfaceFormat         format;
   /*
    * If surfaceFlags has SVGA3D_SURFACE_CUBEMAP bit set, all SVGA3dSurfaceFace
    * structures must have the same value of numMipLevels field.
    * Otherwise, all but the first SVGA3dSurfaceFace structures must have the
    * numMipLevels set to 0.
    */
   SVGA3dSurfaceFace           face[SVGA3D_MAX_SURFACE_FACES];
   uint32                      multisampleCount;
   SVGA3dTextureFilter         autogenFilter;
   /*
    * Followed by an SVGA3dSize structure for each mip level in each face.
    *
    * A note on surface sizes: Sizes are always specified in pixels,
    * even if the true surface size is not a multiple of the minimum
    * block size of the surface's format. For example, a 3x3x1 DXT1
    * compressed texture would actually be stored as a 4x4x1 image in
    * memory.
    */
} SVGA3dCmdDefineSurface_v2;     /* SVGA_3D_CMD_SURFACE_DEFINE_V2 */

typedef
struct {
   uint32               sid;
} SVGA3dCmdDestroySurface;      /* SVGA_3D_CMD_SURFACE_DESTROY */

typedef
struct {
   uint32               cid;
} SVGA3dCmdDefineContext;       /* SVGA_3D_CMD_CONTEXT_DEFINE */

typedef
struct {
   uint32               cid;
} SVGA3dCmdDestroyContext;      /* SVGA_3D_CMD_CONTEXT_DESTROY */

typedef
struct {
   uint32               cid;
   SVGA3dClearFlag      clearFlag;
   uint32               color;
   float                depth;
   uint32               stencil;
   /* Followed by variable number of SVGA3dRect structures */
} SVGA3dCmdClear;               /* SVGA_3D_CMD_CLEAR */

typedef
struct SVGA3dCopyRect {
   uint32               x;
   uint32               y;
   uint32               w;
   uint32               h;
   uint32               srcx;
   uint32               srcy;
} SVGA3dCopyRect;

typedef
struct SVGA3dCopyBox {
   uint32               x;
   uint32               y;
   uint32               z;
   uint32               w;
   uint32               h;
   uint32               d;
   uint32               srcx;
   uint32               srcy;
   uint32               srcz;
} SVGA3dCopyBox;

typedef
struct {
   uint32               x;
   uint32               y;
   uint32               w;
   uint32               h;
} SVGA3dRect;

typedef
struct {
   uint32               x;
   uint32               y;
   uint32               z;
   uint32               w;
   uint32               h;
   uint32               d;
} SVGA3dBox;

typedef
struct {
   uint32               x;
   uint32               y;
   uint32               z;
} SVGA3dPoint;

typedef
struct {
   SVGA3dLightType      type;
   SVGA3dBool           inWorldSpace;
   float                diffuse[4];
   float                specular[4];
   float                ambient[4];
   float                position[4];
   float                direction[4];
   float                range;
   float                falloff;
   float                attenuation0;
   float                attenuation1;
   float                attenuation2;
   float                theta;
   float                phi;
} SVGA3dLightData;

typedef
struct {
   uint32               sid;
   /* Followed by variable number of SVGA3dCopyRect structures */
} SVGA3dCmdPresent;             /* SVGA_3D_CMD_PRESENT */

typedef
struct {
   SVGA3dRenderStateName   state;
   union {
      uint32               uintValue;
      float                floatValue;
   };
} SVGA3dRenderState;

typedef
struct {
   uint32               cid;
   /* Followed by variable number of SVGA3dRenderState structures */
} SVGA3dCmdSetRenderState;      /* SVGA_3D_CMD_SETRENDERSTATE */

typedef
struct {
   uint32                 cid;
   SVGA3dRenderTargetType type;
   SVGA3dSurfaceImageId   target;
} SVGA3dCmdSetRenderTarget;     /* SVGA_3D_CMD_SETRENDERTARGET */

typedef
struct {
   SVGA3dSurfaceImageId  src;
   SVGA3dSurfaceImageId  dest;
   /* Followed by variable number of SVGA3dCopyBox structures */
} SVGA3dCmdSurfaceCopy;               /* SVGA_3D_CMD_SURFACE_COPY */

typedef
struct {
   SVGA3dSurfaceImageId  src;
   SVGA3dSurfaceImageId  dest;
   SVGA3dBox             boxSrc;
   SVGA3dBox             boxDest;
   SVGA3dStretchBltMode  mode;
} SVGA3dCmdSurfaceStretchBlt;         /* SVGA_3D_CMD_SURFACE_STRETCHBLT */

typedef
struct {
   /*
    * If the discard flag is present in a surface DMA operation, the host may
    * discard the contents of the current mipmap level and face of the target
    * surface before applying the surface DMA contents.
    */
   uint32 discard : 1;

   /*
    * If the unsynchronized flag is present, the host may perform this upload
    * without syncing to pending reads on this surface.
    */
   uint32 unsynchronized : 1;

   /*
    * Guests *MUST* set the reserved bits to 0 before submitting the command
    * suffix as future flags may occupy these bits.
    */
   uint32 reserved : 30;
} SVGA3dSurfaceDMAFlags;

typedef
struct {
   SVGA3dGuestImage      guest;
   SVGA3dSurfaceImageId  host;
   SVGA3dTransferType    transfer;
   /*
    * Followed by variable number of SVGA3dCopyBox structures. For consistency
    * in all clipping logic and coordinate translation, we define the
    * "source" in each copyBox as the guest image and the
    * "destination" as the host image, regardless of transfer
    * direction.
    *
    * For efficiency, the SVGA3D device is free to copy more data than
    * specified. For example, it may round copy boxes outwards such
    * that they lie on particular alignment boundaries.
    */
} SVGA3dCmdSurfaceDMA;                /* SVGA_3D_CMD_SURFACE_DMA */

/*
 * SVGA3dCmdSurfaceDMASuffix --
 *
 *    This is a command suffix that will appear after a SurfaceDMA command in
 *    the FIFO.  It contains some extra information that hosts may use to
 *    optimize performance or protect the guest.  This suffix exists to preserve
 *    backwards compatibility while also allowing for new functionality to be
 *    implemented.
 */

typedef
struct {
   uint32 suffixSize;

   /*
    * The maximum offset is used to determine the maximum offset from the
    * guestPtr base address that will be accessed or written to during this
    * surfaceDMA.  If the suffix is supported, the host will respect this
    * boundary while performing surface DMAs.
    *
    * Defaults to MAX_UINT32
    */
   uint32 maximumOffset;

   /*
    * A set of flags that describes optimizations that the host may perform
    * while performing this surface DMA operation.  The guest should never rely
    * on behaviour that is different when these flags are set for correctness.
    *
    * Defaults to 0
    */
   SVGA3dSurfaceDMAFlags flags;
} SVGA3dCmdSurfaceDMASuffix;

/*
 * SVGA_3D_CMD_DRAW_PRIMITIVES --
 *
 *   This command is the SVGA3D device's generic drawing entry point.
 *   It can draw multiple ranges of primitives, optionally using an
 *   index buffer, using an arbitrary collection of vertex buffers.
 *
 *   Each SVGA3dVertexDecl defines a distinct vertex array to bind
 *   during this draw call. The declarations specify which surface
 *   the vertex data lives in, what that vertex data is used for,
 *   and how to interpret it.
 *
 *   Each SVGA3dPrimitiveRange defines a collection of primitives
 *   to render using the same vertex arrays. An index buffer is
 *   optional.
 */

typedef
struct {
   /*
    * A range hint is an optional specification for the range of indices
    * in an SVGA3dArray that will be used. If 'last' is zero, it is assumed
    * that the entire array will be used.
    *
    * These are only hints. The SVGA3D device may use them for
    * performance optimization if possible, but it's also allowed to
    * ignore these values.
    */
   uint32               first;
   uint32               last;
} SVGA3dArrayRangeHint;

typedef
struct {
   /*
    * Define the origin and shape of a vertex or index array. Both
    * 'offset' and 'stride' are in bytes. The provided surface will be
    * reinterpreted as a flat array of bytes in the same format used
    * by surface DMA operations. To avoid unnecessary conversions, the
    * surface should be created with the SVGA3D_BUFFER format.
    *
    * Index 0 in the array starts 'offset' bytes into the surface.
    * Index 1 begins at byte 'offset + stride', etc. Array indices may
    * not be negative.
    */
   uint32               surfaceId;
   uint32               offset;
   uint32               stride;
} SVGA3dArray;

typedef
struct {
   /*
    * Describe a vertex array's data type, and define how it is to be
    * used by the fixed function pipeline or the vertex shader. It
    * isn't useful to have two VertexDecls with the same
    * VertexArrayIdentity in one draw call.
    */
   SVGA3dDeclType       type;
   SVGA3dDeclMethod     method;
   SVGA3dDeclUsage      usage;
   uint32               usageIndex;
} SVGA3dVertexArrayIdentity;

typedef
struct {
   SVGA3dVertexArrayIdentity  identity;
   SVGA3dArray                array;
   SVGA3dArrayRangeHint       rangeHint;
} SVGA3dVertexDecl;

typedef
struct {
   /*
    * Define a group of primitives to render, from sequential indices.
    *
    * The value of 'primitiveType' and 'primitiveCount' imply the
    * total number of vertices that will be rendered.
    */
   SVGA3dPrimitiveType  primType;
   uint32               primitiveCount;

   /*
    * Optional index buffer. If indexArray.surfaceId is
    * SVGA3D_INVALID_ID, we render without an index buffer. Rendering
    * without an index buffer is identical to rendering with an index
    * buffer containing the sequence [0, 1, 2, 3, ...].
    *
    * If an index buffer is in use, indexWidth specifies the width in
    * bytes of each index value. It must be less than or equal to
    * indexArray.stride.
    *
    * (Currently, the SVGA3D device requires index buffers to be tightly
    * packed. In other words, indexWidth == indexArray.stride)
    */
   SVGA3dArray          indexArray;
   uint32               indexWidth;

   /*
    * Optional index bias. This number is added to all indices from
    * indexArray before they are used as vertex array indices. This
    * can be used in multiple ways:
    *
    *  - When not using an indexArray, this bias can be used to
    *    specify where in the vertex arrays to begin rendering.
    *
    *  - A positive number here is equivalent to increasing the
    *    offset in each vertex array.
    *
    *  - A negative number can be used to render using a small
    *    vertex array and an index buffer that contains large
    *    values. This may be used by some applications that
    *    crop a vertex buffer without modifying their index
    *    buffer.
    *
    * Note that rendering with a negative bias value may be slower and
    * use more memory than rendering with a positive or zero bias.
    */
   int32                indexBias;
} SVGA3dPrimitiveRange;

typedef
struct {
   uint32               cid;
   uint32               numVertexDecls;
   uint32               numRanges;

   /*
    * There are two variable size arrays after the
    * SVGA3dCmdDrawPrimitives structure. In order,
    * they are:
    *
    * 1. SVGA3dVertexDecl, quantity 'numVertexDecls', but no more than
    *    SVGA3D_MAX_VERTEX_ARRAYS;
    * 2. SVGA3dPrimitiveRange, quantity 'numRanges', but no more than
    *    SVGA3D_MAX_DRAW_PRIMITIVE_RANGES;
    * 3. Optionally, SVGA3dVertexDivisor, quantity 'numVertexDecls' (contains
    *    the frequency divisor for the corresponding vertex decl).
    */
} SVGA3dCmdDrawPrimitives;      /* SVGA_3D_CMD_DRAWPRIMITIVES */

typedef
struct {
   uint32                   stage;
   SVGA3dTextureStateName   name;
   union {
      uint32                value;
      float                 floatValue;
   };
} SVGA3dTextureState;

typedef
struct {
   uint32               cid;
   /* Followed by variable number of SVGA3dTextureState structures */
} SVGA3dCmdSetTextureState;      /* SVGA_3D_CMD_SETTEXTURESTATE */

typedef
struct {
   uint32                   cid;
   SVGA3dTransformType      type;
   float                    matrix[16];
} SVGA3dCmdSetTransform;          /* SVGA_3D_CMD_SETTRANSFORM */

typedef
struct {
   float                min;
   float                max;
} SVGA3dZRange;

typedef
struct {
   uint32               cid;
   SVGA3dZRange         zRange;
} SVGA3dCmdSetZRange;             /* SVGA_3D_CMD_SETZRANGE */

typedef
struct {
   float                diffuse[4];
   float                ambient[4];
   float                specular[4];
   float                emissive[4];
   float                shininess;
} SVGA3dMaterial;

typedef
struct {
   uint32               cid;
   SVGA3dFace           face;
   SVGA3dMaterial       material;
} SVGA3dCmdSetMaterial;           /* SVGA_3D_CMD_SETMATERIAL */

typedef
struct {
   uint32               cid;
   uint32               index;
   SVGA3dLightData      data;
} SVGA3dCmdSetLightData;           /* SVGA_3D_CMD_SETLIGHTDATA */

typedef
struct {
   uint32               cid;
   uint32               index;
   uint32               enabled;
} SVGA3dCmdSetLightEnabled;      /* SVGA_3D_CMD_SETLIGHTENABLED */

typedef
struct {
   uint32               cid;
   SVGA3dRect           rect;
} SVGA3dCmdSetViewport;           /* SVGA_3D_CMD_SETVIEWPORT */

typedef
struct {
   uint32               cid;
   SVGA3dRect           rect;
} SVGA3dCmdSetScissorRect;         /* SVGA_3D_CMD_SETSCISSORRECT */

typedef
struct {
   uint32               cid;
   uint32               index;
   float                plane[4];
} SVGA3dCmdSetClipPlane;           /* SVGA_3D_CMD_SETCLIPPLANE */

typedef
struct {
   uint32               cid;
   uint32               shid;
   SVGA3dShaderType     type;
   /* Followed by variable number of DWORDs for shader bycode */
} SVGA3dCmdDefineShader;           /* SVGA_3D_CMD_SHADER_DEFINE */

typedef
struct {
   uint32               cid;
   uint32               shid;
   SVGA3dShaderType     type;
} SVGA3dCmdDestroyShader;         /* SVGA_3D_CMD_SHADER_DESTROY */

typedef
struct {
   uint32                  cid;
   uint32                  reg;     /* register number */
   SVGA3dShaderType        type;
   SVGA3dShaderConstType   ctype;
   uint32                  values[4];
} SVGA3dCmdSetShaderConst;        /* SVGA_3D_CMD_SET_SHADER_CONST */

typedef
struct {
   uint32               cid;
   SVGA3dShaderType     type;
   uint32               shid;
} SVGA3dCmdSetShader;             /* SVGA_3D_CMD_SET_SHADER */

typedef
struct {
   uint32               cid;
   SVGA3dQueryType      type;
} SVGA3dCmdBeginQuery;           /* SVGA_3D_CMD_BEGIN_QUERY */

typedef
struct {
   uint32               cid;
   SVGA3dQueryType      type;
   SVGAGuestPtr         guestResult;  /* Points to an SVGA3dQueryResult structure */
} SVGA3dCmdEndQuery;                  /* SVGA_3D_CMD_END_QUERY */

typedef
struct {
   uint32               cid;          /* Same parameters passed to END_QUERY */
   SVGA3dQueryType      type;
   SVGAGuestPtr         guestResult;
} SVGA3dCmdWaitForQuery;              /* SVGA_3D_CMD_WAIT_FOR_QUERY */

typedef
struct {
   uint32               totalSize;    /* Set by guest before query is ended. */
   SVGA3dQueryState     state;        /* Set by host or guest. See SVGA3dQueryState. */
   union {                            /* Set by host on exit from PENDING state */
      uint32            result32;
   };
} SVGA3dQueryResult;

/*
 * SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN --
 *
 *    This is a blit from an SVGA3D surface to a Screen Object. Just
 *    like GMR-to-screen blits, this blit may be directed at a
 *    specific screen or to the virtual coordinate space.
 *
 *    The blit copies from a rectangular region of an SVGA3D surface
 *    image to a rectangular region of a screen or screens.
 *
 *    This command takes an optional variable-length list of clipping
 *    rectangles after the body of the command. If no rectangles are
 *    specified, there is no clipping region. The entire destRect is
 *    drawn to. If one or more rectangles are included, they describe
 *    a clipping region. The clip rectangle coordinates are measured
 *    relative to the top-left corner of destRect.
 *
 *    This clipping region serves multiple purposes:
 *
 *      - It can be used to perform an irregularly shaped blit more
 *        efficiently than by issuing many separate blit commands.
 *
 *      - It is equivalent to allowing blits with non-integer
 *        source coordinates. You could blit just one half-pixel
 *        of a source, for example, by specifying a larger
 *        destination rectangle than you need, then removing
 *        part of it using a clip rectangle.
 *
 * Availability:
 *    SVGA_FIFO_CAP_SCREEN_OBJECT
 *
 * Limitations:
 *
 *    - Currently, no backend supports blits from a mipmap or face
 *      other than the first one.
 */

typedef
struct {
   SVGA3dSurfaceImageId srcImage;
   SVGASignedRect       srcRect;
   uint32               destScreenId; /* Screen ID or SVGA_ID_INVALID for virt. coords */
   SVGASignedRect       destRect;     /* Supports scaling if src/rest different size */
   /* Clipping: zero or more SVGASignedRects follow */
} SVGA3dCmdBlitSurfaceToScreen;         /* SVGA_3D_CMD_BLIT_SURFACE_TO_SCREEN */

typedef
struct {
   uint32               sid;
   SVGA3dTextureFilter  filter;
} SVGA3dCmdGenerateMipmaps;             /* SVGA_3D_CMD_GENERATE_MIPMAPS */


/*
 * Capability query index.
 *
 * Notes:
 *
 *   1. SVGA3D_DEVCAP_MAX_TEXTURES reflects the maximum number of
 *      fixed-function texture units available. Each of these units
 *      work in both FFP and Shader modes, and they support texture
 *      transforms and texture coordinates. The host may have additional
 *      texture image units that are only usable with shaders.
 *
 *   2. The BUFFER_FORMAT capabilities are deprecated, and they always
 *      return TRUE. Even on physical hardware that does not support
 *      these formats natively, the SVGA3D device will provide an emulation
 *      which should be invisible to the guest OS.
 *
 *      In general, the SVGA3D device should support any operation on
 *      any surface format, it just may perform some of these
 *      operations in software depending on the capabilities of the
 *      available physical hardware.
 *
 *      XXX: In the future, we will add capabilities that describe in
 *      detail what formats are supported in hardware for what kinds
 *      of operations.
 */

typedef enum {
   SVGA3D_DEVCAP_3D                                = 0,
   SVGA3D_DEVCAP_MAX_LIGHTS                        = 1,
   SVGA3D_DEVCAP_MAX_TEXTURES                      = 2,  /* See note (1) */
   SVGA3D_DEVCAP_MAX_CLIP_PLANES                   = 3,
   SVGA3D_DEVCAP_VERTEX_SHADER_VERSION             = 4,
   SVGA3D_DEVCAP_VERTEX_SHADER                     = 5,
   SVGA3D_DEVCAP_FRAGMENT_SHADER_VERSION           = 6,
   SVGA3D_DEVCAP_FRAGMENT_SHADER                   = 7,
   SVGA3D_DEVCAP_MAX_RENDER_TARGETS                = 8,
   SVGA3D_DEVCAP_S23E8_TEXTURES                    = 9,
   SVGA3D_DEVCAP_S10E5_TEXTURES                    = 10,
   SVGA3D_DEVCAP_MAX_FIXED_VERTEXBLEND             = 11,
   SVGA3D_DEVCAP_D16_BUFFER_FORMAT                 = 12, /* See note (2) */
   SVGA3D_DEVCAP_D24S8_BUFFER_FORMAT               = 13, /* See note (2) */
   SVGA3D_DEVCAP_D24X8_BUFFER_FORMAT               = 14, /* See note (2) */
   SVGA3D_DEVCAP_QUERY_TYPES                       = 15,
   SVGA3D_DEVCAP_TEXTURE_GRADIENT_SAMPLING         = 16,
   SVGA3D_DEVCAP_MAX_POINT_SIZE                    = 17,
   SVGA3D_DEVCAP_MAX_SHADER_TEXTURES               = 18,
   SVGA3D_DEVCAP_MAX_TEXTURE_WIDTH                 = 19,
   SVGA3D_DEVCAP_MAX_TEXTURE_HEIGHT                = 20,
   SVGA3D_DEVCAP_MAX_VOLUME_EXTENT                 = 21,
   SVGA3D_DEVCAP_MAX_TEXTURE_REPEAT                = 22,
   SVGA3D_DEVCAP_MAX_TEXTURE_ASPECT_RATIO          = 23,
   SVGA3D_DEVCAP_MAX_TEXTURE_ANISOTROPY            = 24,
   SVGA3D_DEVCAP_MAX_PRIMITIVE_COUNT               = 25,
   SVGA3D_DEVCAP_MAX_VERTEX_INDEX                  = 26,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_INSTRUCTIONS    = 27,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_INSTRUCTIONS  = 28,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEMPS           = 29,
   SVGA3D_DEVCAP_MAX_FRAGMENT_SHADER_TEMPS         = 30,
   SVGA3D_DEVCAP_TEXTURE_OPS                       = 31,
   SVGA3D_DEVCAP_SURFACEFMT_X8R8G8B8               = 32,
   SVGA3D_DEVCAP_SURFACEFMT_A8R8G8B8               = 33,
   SVGA3D_DEVCAP_SURFACEFMT_A2R10G10B10            = 34,
   SVGA3D_DEVCAP_SURFACEFMT_X1R5G5B5               = 35,
   SVGA3D_DEVCAP_SURFACEFMT_A1R5G5B5               = 36,
   SVGA3D_DEVCAP_SURFACEFMT_A4R4G4B4               = 37,
   SVGA3D_DEVCAP_SURFACEFMT_R5G6B5                 = 38,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE16            = 39,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8_ALPHA8      = 40,
   SVGA3D_DEVCAP_SURFACEFMT_ALPHA8                 = 41,
   SVGA3D_DEVCAP_SURFACEFMT_LUMINANCE8             = 42,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D16                  = 43,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8                = 44,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24X8                = 45,
   SVGA3D_DEVCAP_SURFACEFMT_DXT1                   = 46,
   SVGA3D_DEVCAP_SURFACEFMT_DXT2                   = 47,
   SVGA3D_DEVCAP_SURFACEFMT_DXT3                   = 48,
   SVGA3D_DEVCAP_SURFACEFMT_DXT4                   = 49,
   SVGA3D_DEVCAP_SURFACEFMT_DXT5                   = 50,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPX8L8V8U8           = 51,
   SVGA3D_DEVCAP_SURFACEFMT_A2W10V10U10            = 52,
   SVGA3D_DEVCAP_SURFACEFMT_BUMPU8V8               = 53,
   SVGA3D_DEVCAP_SURFACEFMT_Q8W8V8U8               = 54,
   SVGA3D_DEVCAP_SURFACEFMT_CxV8U8                 = 55,
   SVGA3D_DEVCAP_SURFACEFMT_R_S10E5                = 56,
   SVGA3D_DEVCAP_SURFACEFMT_R_S23E8                = 57,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S10E5               = 58,
   SVGA3D_DEVCAP_SURFACEFMT_RG_S23E8               = 59,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S10E5             = 60,
   SVGA3D_DEVCAP_SURFACEFMT_ARGB_S23E8             = 61,
   SVGA3D_DEVCAP_MAX_VERTEX_SHADER_TEXTURES        = 63,

   /*
    * Note that MAX_SIMULTANEOUS_RENDER_TARGETS is a maximum count of color
    * render targets.  This does no include the depth or stencil targets.
    */
   SVGA3D_DEVCAP_MAX_SIMULTANEOUS_RENDER_TARGETS   = 64,

   SVGA3D_DEVCAP_SURFACEFMT_V16U16                 = 65,
   SVGA3D_DEVCAP_SURFACEFMT_G16R16                 = 66,
   SVGA3D_DEVCAP_SURFACEFMT_A16B16G16R16           = 67,
   SVGA3D_DEVCAP_SURFACEFMT_UYVY                   = 68,
   SVGA3D_DEVCAP_SURFACEFMT_YUY2                   = 69,
   SVGA3D_DEVCAP_MULTISAMPLE_NONMASKABLESAMPLES    = 70,
   SVGA3D_DEVCAP_MULTISAMPLE_MASKABLESAMPLES       = 71,
   SVGA3D_DEVCAP_ALPHATOCOVERAGE                   = 72,
   SVGA3D_DEVCAP_SUPERSAMPLE                       = 73,
   SVGA3D_DEVCAP_AUTOGENMIPMAPS                    = 74,
   SVGA3D_DEVCAP_SURFACEFMT_NV12                   = 75,
   SVGA3D_DEVCAP_SURFACEFMT_AYUV                   = 76,

   /*
    * This is the maximum number of SVGA context IDs that the guest
    * can define using SVGA_3D_CMD_CONTEXT_DEFINE.
    */
   SVGA3D_DEVCAP_MAX_CONTEXT_IDS                   = 77,

   /*
    * This is the maximum number of SVGA surface IDs that the guest
    * can define using SVGA_3D_CMD_SURFACE_DEFINE*.
    */
   SVGA3D_DEVCAP_MAX_SURFACE_IDS                   = 78,

   SVGA3D_DEVCAP_SURFACEFMT_Z_DF16                 = 79,
   SVGA3D_DEVCAP_SURFACEFMT_Z_DF24                 = 80,
   SVGA3D_DEVCAP_SURFACEFMT_Z_D24S8_INT            = 81,

   SVGA3D_DEVCAP_SURFACEFMT_BC4_UNORM              = 82,
   SVGA3D_DEVCAP_SURFACEFMT_BC5_UNORM              = 83,

   /*
    * Don't add new caps into the previous section; the values in this
    * enumeration must not change. You can put new values right before
    * SVGA3D_DEVCAP_MAX.
    */
   SVGA3D_DEVCAP_MAX                                  /* This must be the last index. */
} SVGA3dDevCapIndex;

typedef union {
   Bool   b;
   uint32 u;
   int32  i;
   float  f;
} SVGA3dDevCapResult;

/*******************************************************************************
 *
 *  G P U   1 0
 *
 ******************************************************************************/

/* https://elixir.bootlin.com/linux/v5.2-rc3/source/drivers/gpu/drm/vmwgfx/device_include/svga3d_cmd.h */

/*
 * Screen DMA command
 *
 * Available with SVGA_FIFO_CAP_SCREEN_OBJECT_2.  The SVGA_CAP_3D device
 * cap bit is not required.
 *
 * - refBuffer and destBuffer are 32bit BGRX; refBuffer and destBuffer could
 *   be different, but it is required that guest makes sure refBuffer has
 *   exactly the same contents that were written to when last time screen DMA
 *   command is received by host.
 *
 * - changemap is generated by lib/blit, and it has the changes from last
 *   received screen DMA or more.
 */

typedef struct SVGA3dCmdScreenDMA {
   uint32 screenId;
   SVGAGuestImage refBuffer;
   SVGAGuestImage destBuffer;
   SVGAGuestImage changeMap;
}
SVGA3dCmdScreenDMA;        /* SVGA_3D_CMD_SCREEN_DMA */

/*
 * Logic ops
 */

#define SVGA3D_LOTRANSBLT_HONORALPHA     (0x01)
#define SVGA3D_LOSTRETCHBLT_MIRRORX      (0x01)
#define SVGA3D_LOSTRETCHBLT_MIRRORY      (0x02)
#define SVGA3D_LOALPHABLEND_SRCHASALPHA  (0x01)

typedef
struct SVGA3dCmdLogicOpsBitBlt {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId src;
   SVGA3dSurfaceImageId dst;
   SVGA3dLogicOp logicOp;
   /* Followed by variable number of SVGA3dCopyBox structures */
}
SVGA3dCmdLogicOpsBitBlt;   /* SVGA_3D_CMD_LOGICOPS_BITBLT */


typedef
struct SVGA3dCmdLogicOpsTransBlt {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId src;
   SVGA3dSurfaceImageId dst;
   uint32 color;
   uint32 flags;
   SVGA3dBox srcBox;
   SVGA3dBox dstBox;
}
SVGA3dCmdLogicOpsTransBlt;   /* SVGA_3D_CMD_LOGICOPS_TRANSBLT */


typedef
struct SVGA3dCmdLogicOpsStretchBlt {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId src;
   SVGA3dSurfaceImageId dst;
   uint16 mode;
   uint16 flags;
   SVGA3dBox srcBox;
   SVGA3dBox dstBox;
}
SVGA3dCmdLogicOpsStretchBlt;   /* SVGA_3D_CMD_LOGICOPS_STRETCHBLT */


typedef
struct SVGA3dCmdLogicOpsColorFill {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId dst;
   uint32 color;
   SVGA3dLogicOp logicOp;
   /* Followed by variable number of SVGA3dRect structures. */
}
SVGA3dCmdLogicOpsColorFill;   /* SVGA_3D_CMD_LOGICOPS_COLORFILL */


typedef
struct SVGA3dCmdLogicOpsAlphaBlend {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId src;
   SVGA3dSurfaceImageId dst;
   uint32 alphaVal;
   uint32 flags;
   SVGA3dBox srcBox;
   SVGA3dBox dstBox;
}
SVGA3dCmdLogicOpsAlphaBlend;   /* SVGA_3D_CMD_LOGICOPS_ALPHABLEND */

#define SVGA3D_CLEARTYPE_INVALID_GAMMA_INDEX 0xFFFFFFFF

#define SVGA3D_CLEARTYPE_GAMMA_WIDTH  512
#define SVGA3D_CLEARTYPE_GAMMA_HEIGHT 16

typedef
struct SVGA3dCmdLogicOpsClearTypeBlend {
   /*
    * All LogicOps surfaces are one-level
    * surfaces so mipmap & face should always
    * be zero.
    */
   SVGA3dSurfaceImageId tmp;
   SVGA3dSurfaceImageId dst;
   SVGA3dSurfaceImageId gammaSurf;
   SVGA3dSurfaceImageId alphaSurf;
   uint32 gamma;
   uint32 color;
   uint32 color2;
   int32 alphaOffsetX;
   int32 alphaOffsetY;
   /* Followed by variable number of SVGA3dBox structures */
}
SVGA3dCmdLogicOpsClearTypeBlend;   /* SVGA_3D_CMD_LOGICOPS_CLEARTYPEBLEND */


/*
 * Guest-backed objects definitions.
 */

typedef
struct {
   SVGAMobFormat ptDepth;
   uint32 sizeInBytes;
   PPN64 base;
}
SVGAOTableMobEntry;
#define SVGA3D_OTABLE_MOB_ENTRY_SIZE (sizeof(SVGAOTableMobEntry))

typedef
struct {
   SVGA3dSurfaceFormat format;
   SVGA3dSurface1Flags surface1Flags;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
   SVGAMobId mobid;
   uint32 arraySize;
   uint32 mobPitch;
   SVGA3dSurface2Flags surface2Flags;
   uint8 multisamplePattern;
   uint8 qualityLevel;
   uint8  pad0[2];
   uint32 pad1[3];
}
SVGAOTableSurfaceEntry;
#define SVGA3D_OTABLE_SURFACE_ENTRY_SIZE (sizeof(SVGAOTableSurfaceEntry))

typedef
struct {
   uint32 cid;
   SVGAMobId mobid;
}
SVGAOTableContextEntry;
#define SVGA3D_OTABLE_CONTEXT_ENTRY_SIZE (sizeof(SVGAOTableContextEntry))

typedef
struct {
   SVGA3dShaderType type;
   uint32 sizeInBytes;
   uint32 offsetInBytes;
   SVGAMobId mobid;
}
SVGAOTableShaderEntry;
#define SVGA3D_OTABLE_SHADER_ENTRY_SIZE (sizeof(SVGAOTableShaderEntry))

#define SVGA_STFLAG_PRIMARY  (1 << 0)
#define SVGA_STFLAG_RESERVED (1 << 1) /* Added with cap SVGA_CAP_HP_CMD_QUEUE */
typedef uint32 SVGAScreenTargetFlags;

typedef
struct {
   SVGA3dSurfaceImageId image;
   uint32 width;
   uint32 height;
   int32 xRoot;
   int32 yRoot;
   SVGAScreenTargetFlags flags;
   uint32 dpi;
   uint32 pad[7];
}
SVGAOTableScreenTargetEntry;
#define SVGA3D_OTABLE_SCREEN_TARGET_ENTRY_SIZE \
	(sizeof(SVGAOTableScreenTargetEntry))

typedef
struct {
   float value[4];
}
SVGA3dShaderConstFloat;

typedef
struct {
   int32 value[4];
}
SVGA3dShaderConstInt;

typedef
struct {
   uint32 value;
}
SVGA3dShaderConstBool;

typedef
struct {
   uint16 streamOffset;
   uint8 stream;
   uint8 type;
   uint8 methodUsage;
   uint8 usageIndex;
}
SVGAGBVertexElement;

typedef
struct {
   uint32 sid;
   uint16 stride;
   uint32 offset;
}
SVGAGBVertexStream;
typedef
struct {
   SVGA3dRect viewport;
   SVGA3dRect scissorRect;
   SVGA3dZRange zRange;

   SVGA3dSurfaceImageId renderTargets[SVGA3D_RT_MAX];
   SVGAGBVertexElement decl1[4];

   uint32 renderStates[SVGA3D_RS_MAX];
   SVGAGBVertexElement decl2[18];
   uint32 pad0[2];

   struct {
      SVGA3dFace face;
      SVGA3dMaterial material;
   } material;

   float clipPlanes[SVGA3D_NUM_CLIPPLANES][4];
   float matrices[SVGA3D_TRANSFORM_MAX][16];

   SVGA3dBool lightEnabled[SVGA3D_NUM_LIGHTS];
   SVGA3dLightData lightData[SVGA3D_NUM_LIGHTS];

   /*
    * Shaders currently bound
    */
   uint32 shaders[SVGA3D_NUM_SHADERTYPE_PREDX];
   SVGAGBVertexElement decl3[10];
   uint32 pad1[3];

   uint32 occQueryActive;
   uint32 occQueryValue;

   /*
    * Int/Bool Shader constants
    */
   SVGA3dShaderConstInt pShaderIValues[SVGA3D_CONSTINTREG_MAX];
   SVGA3dShaderConstInt vShaderIValues[SVGA3D_CONSTINTREG_MAX];
   uint16 pShaderBValues;
   uint16 vShaderBValues;


   SVGAGBVertexStream streams[SVGA3D_MAX_VERTEX_ARRAYS];
   SVGA3dVertexDivisor divisors[SVGA3D_MAX_VERTEX_ARRAYS];
   uint32 numVertexDecls;
   uint32 numVertexStreams;
   uint32 numVertexDivisors;
   uint32 pad2[30];

   /*
    * Texture Stages
    *
    * SVGA3D_TS_INVALID through SVGA3D_TS_CONSTANT are in the
    * textureStages array.
    * SVGA3D_TS_COLOR_KEY is in tsColorKey.
    */
   uint32 tsColorKey[SVGA3D_NUM_TEXTURE_UNITS];
   uint32 textureStages[SVGA3D_NUM_TEXTURE_UNITS][SVGA3D_TS_CONSTANT + 1];
   uint32 tsColorKeyEnable[SVGA3D_NUM_TEXTURE_UNITS];

   /*
    * Float Shader constants.
    */
   SVGA3dShaderConstFloat pShaderFValues[SVGA3D_CONSTREG_MAX];
   SVGA3dShaderConstFloat vShaderFValues[SVGA3D_CONSTREG_MAX];
}
SVGAGBContextData;
#define SVGA3D_CONTEXT_DATA_SIZE (sizeof(SVGAGBContextData))

/*
 * SVGA3dCmdSetOTableBase --
 *
 * This command allows the guest to specify the base PPN of the
 * specified object table.
 */

typedef
struct {
   SVGAOTableType type;
   PPN baseAddress;
   uint32 sizeInBytes;
   uint32 validSizeInBytes;
   SVGAMobFormat ptDepth;
}
SVGA3dCmdSetOTableBase;  /* SVGA_3D_CMD_SET_OTABLE_BASE */

typedef
struct {
   SVGAOTableType type;
   PPN64 baseAddress;
   uint32 sizeInBytes;
   uint32 validSizeInBytes;
   SVGAMobFormat ptDepth;
}
SVGA3dCmdSetOTableBase64;  /* SVGA_3D_CMD_SET_OTABLE_BASE64 */

/*
 * Guests using SVGA_3D_CMD_GROW_OTABLE are promising that
 * the new OTable contains the same contents as the old one, except possibly
 * for some new invalid entries at the end.
 *
 * (Otherwise, guests should use one of the SetOTableBase commands.)
 */
typedef
struct {
   SVGAOTableType type;
   PPN64 baseAddress;
   uint32 sizeInBytes;
   uint32 validSizeInBytes;
   SVGAMobFormat ptDepth;
}
SVGA3dCmdGrowOTable;  /* SVGA_3D_CMD_GROW_OTABLE */

typedef
struct {
   SVGAOTableType type;
}
SVGA3dCmdReadbackOTable;  /* SVGA_3D_CMD_READBACK_OTABLE */

/*
 * Define a memory object (Mob) in the OTable.
 */

typedef
struct SVGA3dCmdDefineGBMob {
   SVGAMobId mobid;
   SVGAMobFormat ptDepth;
   PPN base;
   uint32 sizeInBytes;
}
SVGA3dCmdDefineGBMob;   /* SVGA_3D_CMD_DEFINE_GB_MOB */


/*
 * Destroys an object in the OTable.
 */

typedef
struct SVGA3dCmdDestroyGBMob {
   SVGAMobId mobid;
}
SVGA3dCmdDestroyGBMob;   /* SVGA_3D_CMD_DESTROY_GB_MOB */


/*
 * Define a memory object (Mob) in the OTable with a PPN64 base.
 */

typedef
struct SVGA3dCmdDefineGBMob64 {
   SVGAMobId mobid;
   SVGAMobFormat ptDepth;
   PPN64 base;
   uint32 sizeInBytes;
}
SVGA3dCmdDefineGBMob64;   /* SVGA_3D_CMD_DEFINE_GB_MOB64 */

/*
 * Redefine an object in the OTable with PPN64 base.
 */

typedef
struct SVGA3dCmdRedefineGBMob64 {
   SVGAMobId mobid;
   SVGAMobFormat ptDepth;
   PPN64 base;
   uint32 sizeInBytes;
}
SVGA3dCmdRedefineGBMob64;   /* SVGA_3D_CMD_REDEFINE_GB_MOB64 */

/*
 * Notification that the page tables have been modified.
 */

typedef
struct SVGA3dCmdUpdateGBMobMapping {
   SVGAMobId mobid;
}
SVGA3dCmdUpdateGBMobMapping;   /* SVGA_3D_CMD_UPDATE_GB_MOB_MAPPING */

/*
 * Define a guest-backed surface.
 */

typedef
struct SVGA3dCmdDefineGBSurface {
   uint32 sid;
   SVGA3dSurface1Flags surfaceFlags;
   SVGA3dSurfaceFormat format;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
}
SVGA3dCmdDefineGBSurface;   /* SVGA_3D_CMD_DEFINE_GB_SURFACE */

/*
 * Defines a guest-backed surface, adding the arraySize field.
 */
typedef
struct SVGA3dCmdDefineGBSurface_v2 {
   uint32 sid;
   SVGA3dSurface1Flags surfaceFlags;
   SVGA3dSurfaceFormat format;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
   uint32 arraySize;
   uint32 pad;
}
SVGA3dCmdDefineGBSurface_v2;   /* SVGA_3D_CMD_DEFINE_GB_SURFACE_V2 */

/*
 * Defines a guest-backed surface, adding the larger flags.
 */
typedef
struct SVGA3dCmdDefineGBSurface_v3 {
   uint32 sid;
   SVGA3dSurfaceAllFlags surfaceFlags;
   SVGA3dSurfaceFormat format;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dMSPattern multisamplePattern;
   SVGA3dMSQualityLevel qualityLevel;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
   uint32 arraySize;
}
SVGA3dCmdDefineGBSurface_v3;   /* SVGA_3D_CMD_DEFINE_GB_SURFACE_V3 */

/*
 * Defines a guest-backed surface, adding buffer byte stride.
 */
typedef
struct SVGA3dCmdDefineGBSurface_v4 {
   uint32 sid;
   SVGA3dSurfaceAllFlags surfaceFlags;
   SVGA3dSurfaceFormat format;
   uint32 numMipLevels;
   uint32 multisampleCount;
   SVGA3dMSPattern multisamplePattern;
   SVGA3dMSQualityLevel qualityLevel;
   SVGA3dTextureFilter autogenFilter;
   SVGA3dSize size;
   uint32 arraySize;
   uint32 bufferByteStride;
}
SVGA3dCmdDefineGBSurface_v4;   /* SVGA_3D_CMD_DEFINE_GB_SURFACE_V4 */

/*
 * Destroy a guest-backed surface.
 */

typedef
struct SVGA3dCmdDestroyGBSurface {
   uint32 sid;
}
SVGA3dCmdDestroyGBSurface;   /* SVGA_3D_CMD_DESTROY_GB_SURFACE */

/*
 * Bind a guest-backed surface to a mob.
 */

typedef
struct SVGA3dCmdBindGBSurface {
   uint32 sid;
   SVGAMobId mobid;
}
SVGA3dCmdBindGBSurface;   /* SVGA_3D_CMD_BIND_GB_SURFACE */

typedef
struct SVGA3dCmdBindGBSurfaceWithPitch {
   uint32 sid;
   SVGAMobId mobid;
   uint32 baseLevelPitch;
}
SVGA3dCmdBindGBSurfaceWithPitch;   /* SVGA_3D_CMD_BIND_GB_SURFACE_WITH_PITCH */

/*
 * Conditionally bind a mob to a guest-backed surface if testMobid
 * matches the currently bound mob.  Optionally issue a
 * readback/update on the surface while it is still bound to the old
 * mobid if the mobid is changed by this command.
 */

#define SVGA3D_COND_BIND_GB_SURFACE_FLAG_READBACK (1 << 0)
#define SVGA3D_COND_BIND_GB_SURFACE_FLAG_UPDATE   (1 << 1)

typedef
struct SVGA3dCmdCondBindGBSurface {
   uint32 sid;
   SVGAMobId testMobid;
   SVGAMobId mobid;
   uint32 flags;
}
SVGA3dCmdCondBindGBSurface;          /* SVGA_3D_CMD_COND_BIND_GB_SURFACE */

/*
 * Update an image in a guest-backed surface.
 * (Inform the device that the guest-contents have been updated.)
 */

typedef
struct SVGA3dCmdUpdateGBImage {
   SVGA3dSurfaceImageId image;
   SVGA3dBox box;
}
SVGA3dCmdUpdateGBImage;   /* SVGA_3D_CMD_UPDATE_GB_IMAGE */

/*
 * Update an entire guest-backed surface.
 * (Inform the device that the guest-contents have been updated.)
 */

typedef
struct SVGA3dCmdUpdateGBSurface {
   uint32 sid;
}
SVGA3dCmdUpdateGBSurface;   /* SVGA_3D_CMD_UPDATE_GB_SURFACE */

/*
 * Readback an image in a guest-backed surface.
 * (Request the device to flush the dirty contents into the guest.)
 */

typedef
struct SVGA3dCmdReadbackGBImage {
   SVGA3dSurfaceImageId image;
}
SVGA3dCmdReadbackGBImage;   /* SVGA_3D_CMD_READBACK_GB_IMAGE */

/*
 * Readback an entire guest-backed surface.
 * (Request the device to flush the dirty contents into the guest.)
 */

typedef
struct SVGA3dCmdReadbackGBSurface {
   uint32 sid;
}
SVGA3dCmdReadbackGBSurface;   /* SVGA_3D_CMD_READBACK_GB_SURFACE */

/*
 * Readback a sub rect of an image in a guest-backed surface.  After
 * issuing this command the driver is required to issue an update call
 * of the same region before issuing any other commands that reference
 * this surface or rendering is not guaranteed.
 */

typedef
struct SVGA3dCmdReadbackGBImagePartial {
   SVGA3dSurfaceImageId image;
   SVGA3dBox box;
   uint32 invertBox;
}
SVGA3dCmdReadbackGBImagePartial; /* SVGA_3D_CMD_READBACK_GB_IMAGE_PARTIAL */


/*
 * Invalidate an image in a guest-backed surface.
 * (Notify the device that the contents can be lost.)
 */

typedef
struct SVGA3dCmdInvalidateGBImage {
   SVGA3dSurfaceImageId image;
}
SVGA3dCmdInvalidateGBImage;   /* SVGA_3D_CMD_INVALIDATE_GB_IMAGE */

/*
 * Invalidate an entire guest-backed surface.
 * (Notify the device that the contents if all images can be lost.)
 */

typedef
struct SVGA3dCmdInvalidateGBSurface {
   uint32 sid;
}
SVGA3dCmdInvalidateGBSurface; /* SVGA_3D_CMD_INVALIDATE_GB_SURFACE */

/*
 * Invalidate a sub rect of an image in a guest-backed surface.  After
 * issuing this command the driver is required to issue an update call
 * of the same region before issuing any other commands that reference
 * this surface or rendering is not guaranteed.
 */

typedef
struct SVGA3dCmdInvalidateGBImagePartial {
   SVGA3dSurfaceImageId image;
   SVGA3dBox box;
   uint32 invertBox;
}
SVGA3dCmdInvalidateGBImagePartial; /* SVGA_3D_CMD_INVALIDATE_GB_IMAGE_PARTIAL */


/*
 * Define a guest-backed context.
 */

typedef
struct SVGA3dCmdDefineGBContext {
   uint32 cid;
}
SVGA3dCmdDefineGBContext;   /* SVGA_3D_CMD_DEFINE_GB_CONTEXT */

/*
 * Destroy a guest-backed context.
 */

typedef
struct SVGA3dCmdDestroyGBContext {
   uint32 cid;
}
SVGA3dCmdDestroyGBContext;   /* SVGA_3D_CMD_DESTROY_GB_CONTEXT */

/*
 * Bind a guest-backed context.
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
struct SVGA3dCmdBindGBContext {
   uint32 cid;
   SVGAMobId mobid;
   uint32 validContents;
}
SVGA3dCmdBindGBContext;   /* SVGA_3D_CMD_BIND_GB_CONTEXT */

/*
 * Readback a guest-backed context.
 * (Request that the device flush the contents back into guest memory.)
 */

typedef
struct SVGA3dCmdReadbackGBContext {
   uint32 cid;
}
SVGA3dCmdReadbackGBContext;   /* SVGA_3D_CMD_READBACK_GB_CONTEXT */

/*
 * Invalidate a guest-backed context.
 */
typedef
struct SVGA3dCmdInvalidateGBContext {
   uint32 cid;
}
SVGA3dCmdInvalidateGBContext;   /* SVGA_3D_CMD_INVALIDATE_GB_CONTEXT */

/*
 * Define a guest-backed shader.
 */

typedef
struct SVGA3dCmdDefineGBShader {
   uint32 shid;
   SVGA3dShaderType type;
   uint32 sizeInBytes;
}
SVGA3dCmdDefineGBShader;   /* SVGA_3D_CMD_DEFINE_GB_SHADER */

/*
 * Bind a guest-backed shader.
 */

typedef
struct SVGA3dCmdBindGBShader {
   uint32 shid;
   SVGAMobId mobid;
   uint32 offsetInBytes;
}
SVGA3dCmdBindGBShader;   /* SVGA_3D_CMD_BIND_GB_SHADER */

/*
 * Destroy a guest-backed shader.
 */

typedef
struct SVGA3dCmdDestroyGBShader {
   uint32 shid;
}
SVGA3dCmdDestroyGBShader;   /* SVGA_3D_CMD_DESTROY_GB_SHADER */

typedef
struct {
   uint32                  cid;
   uint32                  regStart;
   SVGA3dShaderType        shaderType;
   SVGA3dShaderConstType   constType;

   /*
    * Followed by a variable number of shader constants.
    *
    * Note that FLOAT and INT constants are 4-dwords in length, while
    * BOOL constants are 1-dword in length.
    */
}
SVGA3dCmdSetGBShaderConstInline;   /* SVGA_3D_CMD_SET_GB_SHADERCONSTS_INLINE */


typedef
struct {
   uint32               cid;
   SVGA3dQueryType      type;
}
SVGA3dCmdBeginGBQuery;           /* SVGA_3D_CMD_BEGIN_GB_QUERY */

typedef
struct {
   uint32               cid;
   SVGA3dQueryType      type;
   SVGAMobId mobid;
   uint32 offset;
}
SVGA3dCmdEndGBQuery;                  /* SVGA_3D_CMD_END_GB_QUERY */


/*
 * SVGA_3D_CMD_WAIT_FOR_GB_QUERY --
 *
 *    The semantics of this command are identical to the
 *    SVGA_3D_CMD_WAIT_FOR_QUERY except that the results are written
 *    to a Mob instead of a GMR.
 */

typedef
struct {
   uint32               cid;
   SVGA3dQueryType      type;
   SVGAMobId mobid;
   uint32 offset;
}
SVGA3dCmdWaitForGBQuery;          /* SVGA_3D_CMD_WAIT_FOR_GB_QUERY */


typedef
struct {
   SVGAMobId mobid;
   uint32 mustBeZero;
   uint32 initialized;
}
SVGA3dCmdEnableGart;              /* SVGA_3D_CMD_ENABLE_GART */

typedef
struct {
   SVGAMobId mobid;
   uint32 gartOffset;
}
SVGA3dCmdMapMobIntoGart;          /* SVGA_3D_CMD_MAP_MOB_INTO_GART */


typedef
struct {
   uint32 gartOffset;
   uint32 numPages;
}
SVGA3dCmdUnmapGartRange;          /* SVGA_3D_CMD_UNMAP_GART_RANGE */


/*
 * Screen Targets
 */

typedef
struct {
   uint32 stid;
   uint32 width;
   uint32 height;
   int32 xRoot;
   int32 yRoot;
   SVGAScreenTargetFlags flags;

   /*
    * The physical DPI that the guest expects this screen displayed at.
    *
    * Guests which are not DPI-aware should set this to zero.
    */
   uint32 dpi;
}
SVGA3dCmdDefineGBScreenTarget;    /* SVGA_3D_CMD_DEFINE_GB_SCREENTARGET */

typedef
struct {
   uint32 stid;
}
SVGA3dCmdDestroyGBScreenTarget;  /* SVGA_3D_CMD_DESTROY_GB_SCREENTARGET */

typedef
struct {
   uint32 stid;
   SVGA3dSurfaceImageId image;
}
SVGA3dCmdBindGBScreenTarget;  /* SVGA_3D_CMD_BIND_GB_SCREENTARGET */

typedef
struct {
   uint32 stid;
   SVGA3dRect rect;
}
SVGA3dCmdUpdateGBScreenTarget;  /* SVGA_3D_CMD_UPDATE_GB_SCREENTARGET */

typedef
struct SVGA3dCmdGBScreenDMA {
   uint32 screenId;
   uint32 dead;
   SVGAMobId destMobID;
   uint32 destPitch;
   SVGAMobId changeMapMobID;
}
SVGA3dCmdGBScreenDMA;        /* SVGA_3D_CMD_GB_SCREEN_DMA */

typedef
struct {
   uint32 value;
   uint32 mobId;
   uint32 mobOffset;
}
SVGA3dCmdGBMobFence;  /* SVGA_3D_CMD_GB_MOB_FENCE */

typedef
struct {
   uint32 stid;
   SVGA3dSurfaceImageId dest;

   uint32 statusMobId;
   uint32 statusMobOffset;

   /* Reserved fields */
   uint32 mustBeInvalidId;
   uint32 mustBeZero;
}
SVGA3dCmdScreenCopy;  /* SVGA_3D_CMD_SCREEN_COPY */

#define SVGA_SCREEN_COPY_STATUS_FAILURE 0x00
#define SVGA_SCREEN_COPY_STATUS_SUCCESS 0x01
#define SVGA_SCREEN_COPY_STATUS_INVALID 0xFFFFFFFF


#endif /* _SVGA3D_REG_H_ */
