#ifndef __DDRAWI_H__INCLUDED__
#define __DDRAWI_H__INCLUDED__
/* DirectDraw internal header for drivers */

#define DDAPI		__loadds WINAPI
#define EXTERN_DDAPI	__export WINAPI

#define DD_ROP_SPACE		(256/32)	// space required to store ROP array

#ifndef ULONG_PTR
#define ULONG_PTR LPDWORD
#endif

#ifndef HRESULT
#define HRESULT DWORD
#endif

#define DDUNSUPPORTEDMODE		((DWORD) -1)

/*
 * maximum size of a driver name
 */
#ifndef CCHDEVICENAME
#define CCHDEVICENAME 32
#endif
#define MAX_DRIVER_NAME     CCHDEVICENAME

/*
 * stuff for drivers
 */
typedef struct GUID {
	DWORD   Data1;
	unsigned short Data2;
	unsigned short Data3;
	unsigned char Data4[8];
} GUID_t;

typedef GUID_t __far *LPGUID;

/*
 * DDSCAPS
 */
typedef struct DDSCAPS
{
	DWORD       dwCaps;         // capabilities of surface wanted
} DDSCAPS_t;

typedef struct DDSCAPS __far* LPDDSCAPS;

/*
 * Status is OK
 *
 * Issued by: DirectDraw Commands and all callbacks
 */
#define DD_OK					0


 /****************************************************************************
 *
 * DIRECTDRAW DRIVER CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Display hardware has 3D acceleration.
 */
#define DDCAPS_3D                       0x00000001l

/*
 * Indicates that DirectDraw will support only dest rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundaryDest boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYDEST        0x00000002l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeDest multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZEDEST            0x00000004l
/*
 * Indicates that DirectDraw will support only source rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundarySrc boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYSRC         0x00000008l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeSrc multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZESRC             0x00000010l

/*
 * Indicates that DirectDraw will create video memory surfaces that have a stride
 * alignment equal to DIRECTDRAWCAPS.dwAlignStride.  READ ONLY.
 */
#define DDCAPS_ALIGNSTRIDE              0x00000020l

/*
 * Display hardware is capable of blt operations.
 */
#define DDCAPS_BLT                      0x00000040l

/*
 * Display hardware is capable of asynchronous blt operations.
 */
#define DDCAPS_BLTQUEUE                 0x00000080l

/*
 * Display hardware is capable of color space conversions during the blt operation.
 */
#define DDCAPS_BLTFOURCC                0x00000100l

/*
 * Display hardware is capable of stretching during blt operations.
 */
#define DDCAPS_BLTSTRETCH               0x00000200l

/*
 * Display hardware is shared with GDI.
 */
#define DDCAPS_GDI                      0x00000400l

/*
 * Display hardware can overlay.
 */
#define DDCAPS_OVERLAY                  0x00000800l

/*
 * Set if display hardware supports overlays but can not clip them.
 */
#define DDCAPS_OVERLAYCANTCLIP          0x00001000l

/*
 * Indicates that overlay hardware is capable of color space conversions during
 * the overlay operation.
 */
#define DDCAPS_OVERLAYFOURCC            0x00002000l

/*
 * Indicates that stretching can be done by the overlay hardware.
 */
#define DDCAPS_OVERLAYSTRETCH           0x00004000l

/*
 * Indicates that unique DirectDrawPalettes can be created for DirectDrawSurfaces
 * other than the primary surface.
 */
#define DDCAPS_PALETTE                  0x00008000l

/*
 * Indicates that palette changes can be syncd with the veritcal refresh.
 */
#define DDCAPS_PALETTEVSYNC             0x00010000l

/*
 * Display hardware can return the current scan line.
 */
#define DDCAPS_READSCANLINE             0x00020000l


/*
 * This flag used to bo DDCAPS_STEREOVIEW, which is now obsolete
 */
#define DDCAPS_RESERVED1                0x00040000l

/*
 * Display hardware is capable of generating a vertical blank interrupt.
 */
#define DDCAPS_VBI                      0x00080000l

/*
 * Supports the use of z buffers with blt operations.
 */
#define DDCAPS_ZBLTS                    0x00100000l

/*
 * Supports Z Ordering of overlays.
 */
#define DDCAPS_ZOVERLAYS                0x00200000l

/*
 * Supports color key
 */
#define DDCAPS_COLORKEY                 0x00400000l

/*
 * Supports alpha surfaces
 */
#define DDCAPS_ALPHA                    0x00800000l

/*
 * colorkey is hardware assisted(DDCAPS_COLORKEY will also be set)
 */
#define DDCAPS_COLORKEYHWASSIST         0x01000000l

/*
 * no hardware support at all
 */
#define DDCAPS_NOHARDWARE               0x02000000l

/*
 * Display hardware is capable of color fill with bltter
 */
#define DDCAPS_BLTCOLORFILL             0x04000000l

/*
 * Display hardware is bank switched, and potentially very slow at
 * random access to VRAM.
 */
#define DDCAPS_BANKSWITCHED             0x08000000l

/*
 * Display hardware is capable of depth filling Z-buffers with bltter
 */
#define DDCAPS_BLTDEPTHFILL             0x10000000l

/*
 * Display hardware is capable of clipping while bltting.
 */
#define DDCAPS_CANCLIP                  0x20000000l

/*
 * Display hardware is capable of clipping while stretch bltting.
 */
#define DDCAPS_CANCLIPSTRETCHED         0x40000000l

/*
 * Display hardware is capable of bltting to or from system memory
 */
#define DDCAPS_CANBLTSYSMEM             0x80000000l

 /****************************************************************************
 *
 * MORE DIRECTDRAW DRIVER CAPABILITY FLAGS (dwCaps2)
 *
 ****************************************************************************/

/*
 * Display hardware is certified
 */
#define DDCAPS2_CERTIFIED		0x00000001l

/*
 * Driver cannot interleave 2D operations (lock and blt) to surfaces with
 * Direct3D rendering operations between calls to BeginScene() and EndScene()
 */
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002l

/*
 * Display hardware contains a video port
 */
#define DDCAPS2_VIDEOPORT		0x00000004l

/*
 * The overlay can be automatically flipped according to the video port
 * VSYNCs, providing automatic doubled buffered display of video port
 * data using an overlay
 */
#define DDCAPS2_AUTOFLIPOVERLAY		0x00000008l

/*
 * Overlay can display each field of interlaced data individually while
 * it is interleaved in memory without causing jittery artifacts.
 */
#define DDCAPS2_CANBOBINTERLEAVED	0x00000010l

/*
 * Overlay can display each field of interlaced data individually while
 * it is not interleaved in memory without causing jittery artifacts.
 */
#define DDCAPS2_CANBOBNONINTERLEAVED	0x00000020l

/*
 * The overlay surface contains color controls (brightness, sharpness, etc.)
 */
#define DDCAPS2_COLORCONTROLOVERLAY	0x00000040l

/*
 * The primary surface contains color controls (gamma, etc.)
 */
#define DDCAPS2_COLORCONTROLPRIMARY	0x00000080l

/*
 * RGBZ -> RGB supported for 16:16 RGB:Z
 */
#define DDCAPS2_CANDROPZ16BIT		0x00000100l

/*
 * Driver supports non-local video memory.
 */
#define DDCAPS2_NONLOCALVIDMEM          0x00000200l

/*
 * Dirver supports non-local video memory but has different capabilities for
 * non-local video memory surfaces. If this bit is set then so must
 * DDCAPS2_NONLOCALVIDMEM.
 */
#define DDCAPS2_NONLOCALVIDMEMCAPS      0x00000400l

/*
 * Driver neither requires nor prefers surfaces to be pagelocked when performing
 * blts involving system memory surfaces
 */
#define DDCAPS2_NOPAGELOCKREQUIRED      0x00000800l

/*
 * Driver can create surfaces which are wider than the primary surface
 */
#define DDCAPS2_WIDESURFACES            0x00001000l

/*
 * Driver supports bob without using a video port by handling the
 * DDFLIP_ODD and DDFLIP_EVEN flags specified in Flip.
 */
#define DDCAPS2_CANFLIPODDEVEN          0x00002000l

/*
 * Driver supports bob using hardware
 */
#define DDCAPS2_CANBOBHARDWARE          0x00004000l

/*
 * Driver supports bltting any FOURCC surface to another surface of the same FOURCC
 */
#define DDCAPS2_COPYFOURCC              0x00008000l

/****************************************************************************
 *
 * DIRECTDRAWSURFACE CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * This bit is reserved. It should not be specified.
 */
#define DDSCAPS_RESERVED1                       0x00000001l

/*
 * Indicates that this surface contains alpha-only information.
 * (To determine if a surface is RGBA/YUVA, the pixel format must be
 * interrogated.)
 */
#define DDSCAPS_ALPHA                           0x00000002l

/*
 * Indicates that this surface is a backbuffer.  It is generally
 * set by CreateSurface when the DDSCAPS_FLIP capability bit is set.
 * It indicates that this surface is THE back buffer of a surface
 * flipping structure.  DirectDraw supports N surfaces in a
 * surface flipping structure.  Only the surface that immediately
 * precedeces the DDSCAPS_FRONTBUFFER has this capability bit set.
 * The other surfaces are identified as back buffers by the presence
 * of the DDSCAPS_FLIP capability, their attachment order, and the
 * absence of the DDSCAPS_FRONTBUFFER and DDSCAPS_BACKBUFFER
 * capabilities.  The bit is sent to CreateSurface when a standalone
 * back buffer is being created.  This surface could be attached to
 * a front buffer and/or back buffers to form a flipping surface
 * structure after the CreateSurface call.  See AddAttachments for
 * a detailed description of the behaviors in this case.
 */
#define DDSCAPS_BACKBUFFER                      0x00000004l

/*
 * Indicates a complex surface structure is being described.  A
 * complex surface structure results in the creation of more than
 * one surface.  The additional surfaces are attached to the root
 * surface.  The complex structure can only be destroyed by
 * destroying the root.
 */
#define DDSCAPS_COMPLEX                         0x00000008l

/*
 * Indicates that this surface is a part of a surface flipping structure.
 * When it is passed to CreateSurface the DDSCAPS_FRONTBUFFER and
 * DDSCAP_BACKBUFFER bits are not set.  They are set by CreateSurface
 * on the resulting creations.  The dwBackBufferCount field in the
 * DDSURFACEDESC structure must be set to at least 1 in order for
 * the CreateSurface call to succeed.  The DDSCAPS_COMPLEX capability
 * must always be set with creating multiple surfaces through CreateSurface.
 */
#define DDSCAPS_FLIP                            0x00000010l

/*
 * Indicates that this surface is THE front buffer of a surface flipping
 * structure.  It is generally set by CreateSurface when the DDSCAPS_FLIP
 * capability bit is set.
 * If this capability is sent to CreateSurface then a standalonw front buffer
 * is created.  This surface will not have the DDSCAPS_FLIP capability.
 * It can be attached to other back buffers to form a flipping structure.
 * See AddAttachments for a detailed description of the behaviors in this
 * case.
 */
#define DDSCAPS_FRONTBUFFER                     0x00000020l

/*
 * Indicates that this surface is any offscreen surface that is not an overlay,
 * texture, zbuffer, front buffer, back buffer, or alpha surface.  It is used
 * to identify plain vanilla surfaces.
 */
#define DDSCAPS_OFFSCREENPLAIN                  0x00000040l

/*
 * Indicates that this surface is an overlay.  It may or may not be directly visible
 * depending on whether or not it is currently being overlayed onto the primary
 * surface.  DDSCAPS_VISIBLE can be used to determine whether or not it is being
 * overlayed at the moment.
 */
#define DDSCAPS_OVERLAY                         0x00000080l

/*
 * Indicates that unique DirectDrawPalette objects can be created and
 * attached to this surface.
 */
#define DDSCAPS_PALETTE                         0x00000100l

/*
 * Indicates that this surface is the primary surface.  The primary
 * surface represents what the user is seeing at the moment.
 */
#define DDSCAPS_PRIMARYSURFACE                  0x00000200l


/*
 * This flag used to be DDSCAPS_PRIMARYSURFACELEFT, which is now
 * obsolete.
 */
#define DDSCAPS_RESERVED3               0x00000400l
#define DDSCAPS_PRIMARYSURFACELEFT              0x00000000l

/*
 * Indicates that this surface memory was allocated in system memory
 */
#define DDSCAPS_SYSTEMMEMORY                    0x00000800l

/*
 * Indicates that this surface can be used as a 3D texture.  It does not
 * indicate whether or not the surface is being used for that purpose.
 */
#define DDSCAPS_TEXTURE                         0x00001000l

/*
 * Indicates that a surface may be a destination for 3D rendering.  This
 * bit must be set in order to query for a Direct3D Device Interface
 * from this surface.
 */
#define DDSCAPS_3DDEVICE                        0x00002000l

/*
 * Indicates that this surface exists in video memory.
 */
#define DDSCAPS_VIDEOMEMORY                     0x00004000l

/*
 * Indicates that changes made to this surface are immediately visible.
 * It is always set for the primary surface and is set for overlays while
 * they are being overlayed and texture maps while they are being textured.
 */
#define DDSCAPS_VISIBLE                         0x00008000l

/*
 * Indicates that only writes are permitted to the surface.  Read accesses
 * from the surface may or may not generate a protection fault, but the
 * results of a read from this surface will not be meaningful.  READ ONLY.
 */
#define DDSCAPS_WRITEONLY                       0x00010000l

/*
 * Indicates that this surface is a z buffer. A z buffer does not contain
 * displayable information.  Instead it contains bit depth information that is
 * used to determine which pixels are visible and which are obscured.
 */
#define DDSCAPS_ZBUFFER                         0x00020000l

/*
 * Indicates surface will have a DC associated long term
 */
#define DDSCAPS_OWNDC                           0x00040000l

/*
 * Indicates surface should be able to receive live video
 */
#define DDSCAPS_LIVEVIDEO                       0x00080000l

/*
 * Indicates surface should be able to have a stream decompressed
 * to it by the hardware.
 */
#define DDSCAPS_HWCODEC                         0x00100000l

/*
 * Surface is a ModeX surface.
 *
 */
#define DDSCAPS_MODEX                           0x00200000l

/*
 * Indicates surface is one level of a mip-map. This surface will
 * be attached to other DDSCAPS_MIPMAP surfaces to form the mip-map.
 * This can be done explicitly, by creating a number of surfaces and
 * attaching them with AddAttachedSurface or by implicitly by CreateSurface.
 * If this bit is set then DDSCAPS_TEXTURE must also be set.
 */
#define DDSCAPS_MIPMAP                          0x00400000l

/*
 * This bit is reserved. It should not be specified.
 */
#define DDSCAPS_RESERVED2                       0x00800000l


/*
 * Indicates that memory for the surface is not allocated until the surface
 * is loaded (via the Direct3D texture Load() function).
 */
#define DDSCAPS_ALLOCONLOAD                     0x04000000l

/*
 * Indicates that the surface will recieve data from a video port.
 */
#define DDSCAPS_VIDEOPORT                       0x08000000l

/*
 * Indicates that a video memory surface is resident in true, local video
 * memory rather than non-local video memory. If this flag is specified then
 * so must DDSCAPS_VIDEOMEMORY. This flag is mutually exclusive with
 * DDSCAPS_NONLOCALVIDMEM.
 */
#define DDSCAPS_LOCALVIDMEM                     0x10000000l

/*
 * Indicates that a video memory surface is resident in non-local video
 * memory rather than true, local video memory. If this flag is specified
 * then so must DDSCAPS_VIDEOMEMORY. This flag is mutually exclusive with
 * DDSCAPS_LOCALVIDMEM.
 */
#define DDSCAPS_NONLOCALVIDMEM                  0x20000000l

/*
 * Indicates that this surface is a standard VGA mode surface, and not a
 * ModeX surface. (This flag will never be set in combination with the
 * DDSCAPS_MODEX flag).
 */
#define DDSCAPS_STANDARDVGAMODE                 0x40000000l

/*
 * Indicates that this surface will be an optimized surface. This flag is
 * currently only valid in conjunction with the DDSCAPS_TEXTURE flag. The surface
 * will be created without any underlying video memory until loaded.
 */
#define DDSCAPS_OPTIMIZED                       0x80000000l



/*
 * This bit is reserved
 */
#define DDSCAPS2_RESERVED4                      0x00000002L
#define DDSCAPS2_HARDWAREDEINTERLACE            0x00000000L

/*
 * Indicates to the driver that this surface will be locked very frequently
 * (for procedural textures, dynamic lightmaps, etc). Surfaces with this cap
 * set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTSTATIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTDYNAMIC                    0x00000004L

/*
 * Indicates to the driver that this surface can be re-ordered/retiled on
 * load. This operation will not change the size of the texture. It is
 * relatively fast and symmetrical, since the application may lock these
 * bits (although it will take a performance hit when doing so). Surfaces
 * with this cap set must also have DDSCAPS_TEXTURE. This cap cannot be
 * used with DDSCAPS2_HINTDYNAMIC and DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_HINTSTATIC                     0x00000008L

/*
 * Indicates that the client would like this texture surface to be managed by the
 * DirectDraw/Direct3D runtime. Surfaces with this cap set must also have
 * DDSCAPS_TEXTURE set.
 */
#define DDSCAPS2_TEXTUREMANAGE                  0x00000010L

/*
 * These bits are reserved for internal use */
#define DDSCAPS2_RESERVED1                      0x00000020L
#define DDSCAPS2_RESERVED2                      0x00000040L

/*
 * Indicates to the driver that this surface will never be locked again.
 * The driver is free to optimize this surface via retiling and actual compression.
 * All calls to Lock() or Blts from this surface will fail. Surfaces with this
 * cap set must also have DDSCAPS_TEXTURE. This cap cannot be used with
 * DDSCAPS2_HINTDYNAMIC and DDSCAPS2_HINTSTATIC.
 */
#define DDSCAPS2_OPAQUE                         0x00000080L

/*
 * Applications should set this bit at CreateSurface time to indicate that they
 * intend to use antialiasing. Only valid if DDSCAPS_3DDEVICE is also set.
 */
#define DDSCAPS2_HINTANTIALIASING               0x00000100L


/*
 * This flag is used at CreateSurface time to indicate that this set of
 * surfaces is a cubic environment map
 */
#define DDSCAPS2_CUBEMAP                        0x00000200L

/*
 * These flags preform two functions:
 * - At CreateSurface time, they define which of the six cube faces are
 *   required by the application.
 * - After creation, each face in the cubemap will have exactly one of these
 *   bits set.
 */
#define DDSCAPS2_CUBEMAP_POSITIVEX              0x00000400L
#define DDSCAPS2_CUBEMAP_NEGATIVEX              0x00000800L
#define DDSCAPS2_CUBEMAP_POSITIVEY              0x00001000L
#define DDSCAPS2_CUBEMAP_NEGATIVEY              0x00002000L
#define DDSCAPS2_CUBEMAP_POSITIVEZ              0x00004000L
#define DDSCAPS2_CUBEMAP_NEGATIVEZ              0x00008000L

/*
 * This macro may be used to specify all faces of a cube map at CreateSurface time
 */
#define DDSCAPS2_CUBEMAP_ALLFACES ( DDSCAPS2_CUBEMAP_POSITIVEX |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEX |\
                                    DDSCAPS2_CUBEMAP_POSITIVEY |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEY |\
                                    DDSCAPS2_CUBEMAP_POSITIVEZ |\
                                    DDSCAPS2_CUBEMAP_NEGATIVEZ )


/*
 * This flag is an additional flag which is present on mipmap sublevels from DX7 onwards
 * It enables easier use of GetAttachedSurface rather than EnumAttachedSurfaces for surface
 * constructs such as Cube Maps, wherein there are more than one mipmap surface attached
 * to the root surface.
 * This caps bit is ignored by CreateSurface
 */
#define DDSCAPS2_MIPMAPSUBLEVEL                 0x00010000L

/* This flag indicates that the texture should be managed by D3D only */
#define DDSCAPS2_D3DTEXTUREMANAGE               0x00020000L

/* This flag indicates that the managed surface can be safely lost */
#define DDSCAPS2_DONOTPERSIST                   0x00040000L

/* indicates that this surface is part of a stereo flipping chain */
#define DDSCAPS2_STEREOSURFACELEFT              0x00080000L


/*
 * Indicates that the surface is a volume.
 * Can be combined with DDSCAPS_MIPMAP to indicate a multi-level volume
 */
#define DDSCAPS2_VOLUME                         0x00200000L

/*
 * Indicates that the surface may be locked multiple times by the application.
 * This cap cannot be used with DDSCAPS2_OPAQUE.
 */
#define DDSCAPS2_NOTUSERLOCKABLE                0x00400000L

/*
 * Indicates that the vertex buffer data can be used to render points and
 * point sprites.
 */
#define DDSCAPS2_POINTS                         0x00800000L

/*
 * Indicates that the vertex buffer data can be used to render rt pactches.
 */
#define DDSCAPS2_RTPATCHES                      0x01000000L

/*
 * Indicates that the vertex buffer data can be used to render n patches.
 */
#define DDSCAPS2_NPATCHES                       0x02000000L

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS2_RESERVED3                      0x04000000L


/*
 * Indicates that the contents of the backbuffer do not have to be preserved
 * the contents of the backbuffer after they are presented.
 */
#define DDSCAPS2_DISCARDBACKBUFFER              0x10000000L

/*
 * Indicates that all surfaces in this creation chain should be given an alpha channel.
 * This flag will be set on primary surface chains that may have no explicit pixel format
 * (and thus take on the format of the current display mode).
 * The driver should infer that all these surfaces have a format having an alpha channel.
 * (e.g. assume D3DFMT_A8R8G8B8 if the display mode is x888.)
 */
#define DDSCAPS2_ENABLEALPHACHANNEL             0x20000000L

/*
 * Indicates that all surfaces in this creation chain is extended primary surface format.
 * This flag will be set on extended primary surface chains that always have explicit pixel
 * format and the pixel format is typically GDI (Graphics Device Interface) couldn't handle,
 * thus only used with fullscreen application. (e.g. D3DFMT_A2R10G10B10 format)
 */
#define DDSCAPS2_EXTENDEDFORMATPRIMARY          0x40000000L

/*
 * Indicates that all surfaces in this creation chain is additional primary surface.
 * This flag will be set on primary surface chains which must present on the adapter
 * id provided on dwCaps4. Typically this will be used to create secondary primary surface
 * on DualView display adapter.
 */
#define DDSCAPS2_ADDITIONALPRIMARY              0x80000000L

/*
 * This is a mask that indicates the set of bits that may be set
 * at createsurface time to indicate number of samples per pixel
 * when multisampling
 */
#define DDSCAPS3_MULTISAMPLE_MASK               0x0000001FL

/*
 * This is a mask that indicates the set of bits that may be set
 * at createsurface time to indicate the quality level of rendering
 * for the current number of samples per pixel
 */
#define DDSCAPS3_MULTISAMPLE_QUALITY_MASK       0x000000E0L
#define DDSCAPS3_MULTISAMPLE_QUALITY_SHIFT      5

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS3_RESERVED1                      0x00000100L

/*
 * This bit is reserved for internal use 
 */
#define DDSCAPS3_RESERVED2                      0x00000200L

/*
 * This indicates whether this surface has light-weight miplevels
 */
#define DDSCAPS3_LIGHTWEIGHTMIPMAP              0x00000400L

/*
 * This indicates that the mipsublevels for this surface are auto-generated
 */
#define DDSCAPS3_AUTOGENMIPMAP                  0x00000800L

/*
 * This indicates that the mipsublevels for this surface are auto-generated
 */
#define DDSCAPS3_DMAP                           0x00001000L

/****************************************************************************
 *
 * DIRECTDRAW COLOR KEY CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Supports transparent blting using a color key to identify the replaceable
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLT                      0x00000001l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACE              0x00000002l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV           0x00000004l

/*
 * Supports transparent blting using a color key to identify the replaceable
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTYUV                   0x00000008l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the surface
 * being overlayed for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAY                  0x00000010l

/*
 * Supports a color space as the color key for the destination for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE          0x00000020l

/*
 * Supports a color space as the color key for the destination for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV       0x00000040l

/*
 * Supports only one active destination color key value for visible overlay
 * surfaces.
 */
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE         0x00000080l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the
 * surface being overlayed for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYYUV               0x00000100l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLT                       0x00000200l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACE               0x00000400l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV            0x00000800l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTYUV                    0x00001000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAY                   0x00002000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE           0x00004000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV        0x00008000l

/*
 * Supports only one active source color key value for visible
 * overlay surfaces.
 */
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE          0x00010000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYYUV                0x00020000l

/*
 * there are no bandwidth trade-offs for using colorkey with an overlay
 */
#define DDCKEYCAPS_NOCOSTOVERLAY                0x00040000l

/****************************************************************************
 *
 * DIRECTDRAW PIXELFORMAT FLAGS
 *
 ****************************************************************************/

/*
 * The surface has alpha channel information in the pixel format.
 */
#define DDPF_ALPHAPIXELS			0x00000001l

/*
 * The pixel format contains alpha only information
 */
#define DDPF_ALPHA				0x00000002l

/*
 * The FourCC code is valid.
 */
#define DDPF_FOURCC				0x00000004l

/*
 * The surface is 4-bit color indexed.
 */
#define DDPF_PALETTEINDEXED4			0x00000008l

/*
 * The surface is indexed into a palette which stores indices
 * into the destination surface's 8-bit palette.
 */
#define DDPF_PALETTEINDEXEDTO8			0x00000010l

/*
 * The surface is 8-bit color indexed.
 */
#define DDPF_PALETTEINDEXED8			0x00000020l

/*
 * The RGB data in the pixel format structure is valid.
 */
#define DDPF_RGB				0x00000040l

/*
 * The surface will accept pixel data in the format specified
 * and compress it during the write.
 */
#define DDPF_COMPRESSED				0x00000080l

/*
 * The surface will accept RGB data and translate it during
 * the write to YUV data.  The format of the data to be written
 * will be contained in the pixel format structure.  The DDPF_RGB
 * flag will be set.
 */
#define DDPF_RGBTOYUV				0x00000100l

/*
 * pixel format is YUV - YUV data in pixel format struct is valid
 */
#define DDPF_YUV				0x00000200l

/*
 * pixel format is a z buffer only surface
 */
#define DDPF_ZBUFFER				0x00000400l

/*
 * The surface is 1-bit color indexed.
 */
#define DDPF_PALETTEINDEXED1			0x00000800l

/*
 * The surface is 2-bit color indexed.
 */
#define DDPF_PALETTEINDEXED2			0x00001000l

/*
 * The surface contains Z information in the pixels
 */
#define DDPF_ZPIXELS				0x00002000l

/****************************************************************************
 *
 * DIRECTDRAW FX CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Uses arithmetic operations to stretch and shrink surfaces during blt
 * rather than pixel doubling techniques.  Along the Y axis.
 */
#define DDFXCAPS_BLTARITHSTRETCHY	0x00000020l

/*
 * Uses arithmetic operations to stretch during blt
 * rather than pixel doubling techniques.  Along the Y axis. Only
 * works for x1, x2, etc.
 */
#define DDFXCAPS_BLTARITHSTRETCHYN	0x00000010l

/*
 * Supports mirroring left to right in blt.
 */
#define DDFXCAPS_BLTMIRRORLEFTRIGHT	0x00000040l

/*
 * Supports mirroring top to bottom in blt.
 */
#define DDFXCAPS_BLTMIRRORUPDOWN	0x00000080l

/*
 * Supports arbitrary rotation for blts.
 */
#define DDFXCAPS_BLTROTATION		0x00000100l

/*
 * Supports 90 degree rotations for blts.
 */
#define DDFXCAPS_BLTROTATION90		0x00000200l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKX		0x00000400l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKXN		0x00000800l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKY		0x00001000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKYN		0x00002000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHX		0x00004000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHXN		0x00008000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHY		0x00010000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHYN		0x00020000l

/*
 * Uses arithmetic operations to stretch and shrink surfaces during
 * overlay rather than pixel doubling techniques.  Along the Y axis
 * for overlays.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHY	0x00040000l

/*
 * Uses arithmetic operations to stretch surfaces during
 * overlay rather than pixel doubling techniques.  Along the Y axis
 * for overlays. Only works for x1, x2, etc.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHYN	0x00000008l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKX		0x00080000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKXN	0x00100000l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKY		0x00200000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKYN	0x00400000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHX	0x00800000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHXN	0x01000000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHY	0x02000000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHYN	0x04000000l

/*
 * DirectDraw supports mirroring of overlays across the vertical axis
 */
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT	0x08000000l

/*
 * DirectDraw supports mirroring of overlays across the horizontal axis
 */
#define DDFXCAPS_OVERLAYMIRRORUPDOWN	0x10000000l

/*
 * DDPIXELFORMAT
 */
typedef struct DDPIXELFORMAT
{
	DWORD	dwSize;			// size of structure
	DWORD	dwFlags;		// pixel format flags
	DWORD	dwFourCC;		// (FOURCC code)
	union
	{
		DWORD	dwRGBBitCount;		// how many bits per pixel
		DWORD	dwYUVBitCount;		// how many bits per pixel
		DWORD	dwZBufferBitDepth;	// how many bits for z buffers
		DWORD	dwAlphaBitDepth;	// how many bits for alpha channels
	};
	union
	{
		DWORD	dwRBitMask;		// mask for red bit
		DWORD	dwYBitMask;		// mask for Y bits
	};
	union
	{
		DWORD	dwGBitMask;		// mask for green bits
		DWORD	dwUBitMask;		// mask for U bits
	};
	union
	{
		DWORD	dwBBitMask;		// mask for blue bits
		DWORD	dwVBitMask;		// mask for V bits
	};
	union
	{
		DWORD	dwRGBAlphaBitMask;	// mask for alpha channel
		DWORD	dwYUVAlphaBitMask;	// mask for alpha channel
		DWORD	dwRGBZBitMask;		// mask for Z channel
		DWORD	dwYUVZBitMask;		// mask for Z channel
	};
} DDPIXELFORMAT_t;

typedef DDPIXELFORMAT_t __far *LPDDPIXELFORMAT;

typedef struct DDCOLORKEY
{
	DWORD	dwColorSpaceLowValue;	// low boundary of color space that is to
	// be treated as Color Key, inclusive
	DWORD	dwColorSpaceHighValue;	// high boundary of color space that is
	// to be treated as Color Key, inclusive
} DDCOLORKEY_t;

typedef struct PROCESS_LIST
{
	struct PROCESS_LIST __far *lpLink;
	DWORD	dwProcessId;
	DWORD	dwRefCnt;
	DWORD	dwAlphaDepth;
	DWORD	dwZDepth;
} PROCESS_LIST_t;
typedef PROCESS_LIST_t __far *LPPROCESS_LIST;

/*
 * DDVIDEOPORTCAPS
 */
typedef struct DDVIDEOPORTCAPS
{
	DWORD dwSize;			// size of the DDVIDEOPORTCAPS structure
	DWORD dwFlags;			// indicates which fields contain data
	DWORD dwMaxWidth;			// max width of the video port field
	DWORD dwMaxVBIWidth;		// max width of the VBI data
	DWORD dwMaxHeight; 			// max height of the video port field
	DWORD dwVideoPortID;		// Video port ID (0 - (dwMaxVideoPorts -1))
	DWORD dwCaps;			// Video port capabilities
	DWORD dwFX;				// More video port capabilities
	DWORD dwNumAutoFlipSurfaces;	// Max number of autoflippable surfaces allowed
	DWORD dwAlignVideoPortBoundary;	// Byte restriction of placement within the surface
	DWORD dwAlignVideoPortPrescaleWidth;// Byte restriction of width after prescaling
	DWORD dwAlignVideoPortCropBoundary;	// Byte restriction of left cropping
	DWORD dwAlignVideoPortCropWidth;	// Byte restriction of cropping width
	DWORD dwPreshrinkXStep;		// Width can be shrunk in steps of 1/x
	DWORD dwPreshrinkYStep;		// Height can be shrunk in steps of 1/x
	DWORD dwNumVBIAutoFlipSurfaces;	// Max number of VBI autoflippable surfaces allowed
	DWORD dwNumPreferredAutoflip;	// Optimal number of autoflippable surfaces for hardware
	WORD  wNumFilterTapsX;              // Number of taps the prescaler uses in the X direction (0 - no prescale, 1 - replication, etc.)
	WORD  wNumFilterTapsY;              // Number of taps the prescaler uses in the Y direction (0 - no prescale, 1 - replication, etc.)
} DDVIDEOPORTCAPS_t;
typedef DDVIDEOPORTCAPS_t __far* LPDDVIDEOPORTCAPS;

#include "dmemmgr.h"

typedef struct DCICMD /* DDK: dciddi.h */
{
	DWORD   dwCommand;
	DWORD   dwParam1;
	DWORD   dwParam2;
	DWORD   dwVersion;
	DWORD   dwReserved;
} DCICMD_t;

typedef struct DD32BITDRIVERDATA /* DDK: ddrawi.h */
{
	char	szName[260];			// 32-bit driver name
	char	szEntryPoint[64];	// entry point
	DWORD	dwContext;			  // context to pass to entry point
} DD32BITDRIVERDATA_t;

typedef struct DDVERSIONDATA /* DDK: ddrawi.h */
{
	DWORD	dwHALVersion;			// Version of DirectDraw for which the HAL was created
	DWORD	dwReserved1;			// Reserved for future use
	DWORD	dwReserved2;			// Reserved for future use
} DDVERSIONDATA_t;

/*
 * DDKERNELCAPS
 */
typedef struct DDKERNELCAPS
{
    DWORD dwSize;			// size of the DDKERNELCAPS structure
    DWORD dwCaps;     // Contains the DDKERNELCAPS_XXX flags
    DWORD dwIRQCaps;  // Contains the DDIRQ_XXX flags
} DDKERNELCAPS;
typedef DDKERNELCAPS __far *LPDDKERNELCAPS;

/*video memory data structures */
typedef struct VIDMEM
{
	DWORD		dwFlags;	// flags
	FLATPTR		fpStart;	// start of memory chunk
	union
	{
		FLATPTR		fpEnd;		// end of memory chunk
		DWORD		dwWidth;	// width of chunk (rectanglar memory)
	};
	DDSCAPS_t		ddsCaps;		// what this memory CANNOT be used for
	DDSCAPS_t		ddsCapsAlt;	// what this memory CANNOT be used for if it must
	union
	{
		LPVMEMHEAP	lpHeap;		// heap pointer, used by DDRAW
		DWORD		dwHeight;	// height of chunk (rectanguler memory)
	};
} VIDMEM_t;

typedef struct VIDMEM __far *LPVIDMEM;

/* flags for vidmem struct */
#define VIDMEM_ISLINEAR         0x00000001l     // heap is linear
#define VIDMEM_ISRECTANGULAR	  0x00000002l     // heap is rectangular
#define VIDMEM_ISHEAP           0x00000004l     // heap is preallocated by driver
#define VIDMEM_ISNONLOCAL       0x00000008l     // heap populated with non-local video memory
#define VIDMEM_ISWC             0x00000010l     // heap populated with write combining memory

typedef struct VIDMEMINFO
{
/* 0*/ FLATPTR		fpPrimary;		// pointer to primary surface
/* 4*/ DWORD		dwFlags;		// flags
/* 8*/ DWORD		dwDisplayWidth;		// current display width
/* c*/ DWORD		dwDisplayHeight;	// current display height
/*10*/ LONG		lDisplayPitch;		// current display pitch
/*14*/ DDPIXELFORMAT_t ddpfDisplay;		// pixel format of display
/*34*/ DWORD		dwOffscreenAlign;	// byte alignment for offscreen surfaces
/*38*/ DWORD		dwOverlayAlign;		// byte alignment for overlays
/*3c*/ DWORD		dwTextureAlign;		// byte alignment for textures
/*40*/ DWORD		dwZBufferAlign;		// byte alignment for z buffers
/*44*/ DWORD		dwAlphaAlign;		// byte alignment for alpha
/*48*/ DWORD		dwNumHeaps;		// number of memory heaps in vmList
/*4c*/ LPVIDMEM		pvmList;		// array of heaps
} VIDMEMINFO_t;

typedef struct HEAPALIAS                  // PRIVATE
{
    FLATPTR  fpVidMem;                     // start of aliased vid mem
    LPVOID   lpAlias;                      // start of heap alias
    DWORD    dwAliasSize;                  // size of alias allocated
} HEAPALIAS_t;
typedef HEAPALIAS_t __far *LPHEAPALIAS;

typedef struct HEAPALIASINFO              // PRIVATE
{
    DWORD       dwRefCnt;                  // reference count of these aliases
    DWORD       dwFlags;                   // flags
    DWORD       dwNumHeaps;                // number of aliased heaps
    LPHEAPALIAS lpAliases;                 // array of heaps
} HEAPALIASINFO_t;
typedef HEAPALIASINFO_t __far *LPHEAPALIASINFO;

#define HEAPALIASINFO_MAPPEDREAL   0x00000001l // PRIVATE: heap aliases mapped to real video memory
#define HEAPALIASINFO_MAPPEDDUMMY  0x00000002l // PRIVATE: heap aliased mapped to dummy memory

/*
 * These structures contain the entry points in the display driver that
 * DDRAW will call.   Entries that the display driver does not care about
 * should be NULL.   Passed to DDRAW in DDHALINFO.
 */

/*
 * pre-declare pointers to structs containing data for DDHAL fns
 */
typedef struct DDHAL_CREATEPALETTEDATA __far *LPDDHAL_CREATEPALETTEDATA;
typedef struct DDHAL_CREATESURFACEDATA __far *LPDDHAL_CREATESURFACEDATA;
typedef struct DDHAL_CANCREATESURFACEDATA __far *LPDDHAL_CANCREATESURFACEDATA;
typedef struct DDHAL_WAITFORVERTICALBLANKDATA __far *LPDDHAL_WAITFORVERTICALBLANKDATA;
typedef struct DDHAL_DESTROYDRIVERDATA __far *LPDDHAL_DESTROYDRIVERDATA;
typedef struct DDHAL_SETMODEDATA __far *LPDDHAL_SETMODEDATA;
typedef struct DDHAL_DRVSETCOLORKEYDATA __far *LPDDHAL_DRVSETCOLORKEYDATA;
typedef struct DDHAL_GETSCANLINEDATA __far *LPDDHAL_GETSCANLINEDATA;

typedef struct DDHAL_DESTROYPALETTEDATA __far *LPDDHAL_DESTROYPALETTEDATA;
typedef struct DDHAL_SETENTRIESDATA __far *LPDDHAL_SETENTRIESDATA;

typedef struct DDHAL_BLTDATA __far *LPDDHAL_BLTDATA;
typedef struct DDHAL_LOCKDATA __far *LPDDHAL_LOCKDATA;
typedef struct DDHAL_UNLOCKDATA __far *LPDDHAL_UNLOCKDATA;
typedef struct DDHAL_UPDATEOVERLAYDATA __far *LPDDHAL_UPDATEOVERLAYDATA;
typedef struct DDHAL_SETOVERLAYPOSITIONDATA __far *LPDDHAL_SETOVERLAYPOSITIONDATA;
typedef struct DDHAL_SETPALETTEDATA __far *LPDDHAL_SETPALETTEDATA;
typedef struct DDHAL_FLIPDATA __far *LPDDHAL_FLIPDATA;
typedef struct DDHAL_DESTROYSURFACEDATA __far *LPDDHAL_DESTROYSURFACEDATA;
typedef struct DDHAL_SETCLIPLISTDATA __far *LPDDHAL_SETCLIPLISTDATA;
typedef struct DDHAL_ADDATTACHEDSURFACEDATA __far *LPDDHAL_ADDATTACHEDSURFACEDATA;
typedef struct DDHAL_SETCOLORKEYDATA __far *LPDDHAL_SETCOLORKEYDATA;
typedef struct DDHAL_GETBLTSTATUSDATA __far *LPDDHAL_GETBLTSTATUSDATA;
typedef struct DDHAL_GETFLIPSTATUSDATA __far *LPDDHAL_GETFLIPSTATUSDATA;
typedef struct DDHAL_SETEXCLUSIVEMODEDATA __far *LPDDHAL_SETEXCLUSIVEMODEDATA;
typedef struct DDHAL_FLIPTOGDISURFACEDATA __far *LPDDHAL_FLIPTOGDISURFACEDATA;

typedef struct DDHAL_CANCREATEVPORTDATA __far *LPDDHAL_CANCREATEVPORTDATA;
typedef struct DDHAL_CREATEVPORTDATA __far *LPDDHAL_CREATEVPORTDATA;
typedef struct DDHAL_FLIPVPORTDATA __far *LPDDHAL_FLIPVPORTDATA;
typedef struct DDHAL_GETVPORTCONNECTDATA __far *LPDDHAL_GETVPORTCONNECTDATA;
typedef struct DDHAL_GETVPORTBANDWIDTHDATA __far *LPDDHAL_GETVPORTBANDWIDTHDATA;
typedef struct DDHAL_GETVPORTINPUTFORMATDATA __far *LPDDHAL_GETVPORTINPUTFORMATDATA;
typedef struct DDHAL_GETVPORTOUTPUTFORMATDATA __far *LPDDHAL_GETVPORTOUTPUTFORMATDATA;
typedef struct DDHAL_GETVPORTFIELDDATA __far *LPDDHAL_GETVPORTFIELDDATA;
typedef struct DDHAL_GETVPORTLINEDATA __far *LPDDHAL_GETVPORTLINEDATA;
typedef struct DDHAL_DESTROYVPORTDATA __far *LPDDHAL_DESTROYVPORTDATA;
typedef struct DDHAL_GETVPORTFLIPSTATUSDATA __far *LPDDHAL_GETVPORTFLIPSTATUSDATA;
typedef struct DDHAL_UPDATEVPORTDATA __far *LPDDHAL_UPDATEVPORTDATA;
typedef struct DDHAL_WAITFORVPORTSYNCDATA __far *LPDDHAL_WAITFORVPORTSYNCDATA;
typedef struct DDHAL_GETVPORTSIGNALDATA __far *LPDDHAL_GETVPORTSIGNALDATA;
typedef struct DDHAL_VPORTCOLORDATA __far *LPDDHAL_VPORTCOLORDATA;

typedef struct DDHAL_COLORCONTROLDATA __far *LPDDHAL_COLORCONTROLDATA;

typedef struct DDHAL_GETAVAILDRIVERMEMORYDATA __far *LPDDHAL_GETAVAILDRIVERMEMORYDATA;
typedef struct DDHAL_UPDATENONLOCALHEAPDATA __far *LPDDHAL_UPDATENONLOCALHEAPDATA;
typedef struct DDHAL_GETHEAPALIGNMENTDATA __far *LPDDHAL_GETHEAPALIGNMENTDATA;

typedef struct DDHAL_GETDRIVERINFODATA __far *LPDDHAL_GETDRIVERINFODATA;

typedef struct DDHAL_SYNCSURFACEDATA __far *LPDDHAL_SYNCSURFACEDATA;
typedef struct DDHAL_SYNCVIDEOPORTDATA __far *LPDDHAL_SYNCVIDEOPORTDATA;

typedef struct DDHAL_CREATESURFACEEXDATA __far *LPDDHAL_CREATESURFACEEXDATA;
typedef struct DDHAL_GETDRIVERSTATEDATA __far *LPDDHAL_GETDRIVERSTATEDATA;
typedef struct DDHAL_DESTROYDDLOCALDATA __far *LPDDHAL_DESTROYDDLOCALDATA;

/*
 * DIRECTDRAW object callbacks
 */
typedef DWORD   (__far __fastcall *LPDDHAL_SETCOLORKEY)(LPDDHAL_DRVSETCOLORKEYDATA );
typedef DWORD   (__far __fastcall *LPDDHAL_CANCREATESURFACE)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD   (__far __fastcall *LPDDHAL_WAITFORVERTICALBLANK)(LPDDHAL_WAITFORVERTICALBLANKDATA );
typedef DWORD   (__far __fastcall *LPDDHAL_CREATESURFACE)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_DESTROYDRIVER)(LPDDHAL_DESTROYDRIVERDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_SETMODE)(LPDDHAL_SETMODEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_CREATEPALETTE)(LPDDHAL_CREATEPALETTEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_GETSCANLINE)(LPDDHAL_GETSCANLINEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_SETEXCLUSIVEMODE)(LPDDHAL_SETEXCLUSIVEMODEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_FLIPTOGDISURFACE)(LPDDHAL_FLIPTOGDISURFACEDATA);

typedef DWORD   (__far __fastcall *LPDDHAL_GETDRIVERINFO)(LPDDHAL_GETDRIVERINFODATA);

typedef struct DDHAL_DDCALLBACKS
{
	DWORD            dwSize;
	DWORD            dwFlags;
	LPDDHAL_DESTROYDRIVER    DestroyDriver;
	LPDDHAL_CREATESURFACE    CreateSurface;
	LPDDHAL_SETCOLORKEY      SetColorKey;
	LPDDHAL_SETMODE      SetMode;
	LPDDHAL_WAITFORVERTICALBLANK WaitForVerticalBlank;
	LPDDHAL_CANCREATESURFACE     CanCreateSurface;
	LPDDHAL_CREATEPALETTE    CreatePalette;
	LPDDHAL_GETSCANLINE      GetScanLine;
	// *** New fields for DX2 *** //
	LPDDHAL_SETEXCLUSIVEMODE     SetExclusiveMode;
	LPDDHAL_FLIPTOGDISURFACE     FlipToGDISurface;
} DDHAL_DDCALLBACKS_t;

typedef DDHAL_DDCALLBACKS_t __far *LPDDHAL_DDCALLBACKS;

#define DDCALLBACKSSIZE_V1 ( offsetof( DDHAL_DDCALLBACKS, SetExclusiveMode ) )
#define DDCALLBACKSSIZE    sizeof( DDHAL_DDCALLBACKS )

#define DDHAL_CB32_DESTROYDRIVER        0x00000001l
#define DDHAL_CB32_CREATESURFACE        0x00000002l
#define DDHAL_CB32_SETCOLORKEY          0x00000004l
#define DDHAL_CB32_SETMODE              0x00000008l
#define DDHAL_CB32_WAITFORVERTICALBLANK 0x00000010l
#define DDHAL_CB32_CANCREATESURFACE     0x00000020l
#define DDHAL_CB32_CREATEPALETTE        0x00000040l
#define DDHAL_CB32_GETSCANLINE          0x00000080l
#define DDHAL_CB32_SETEXCLUSIVEMODE     0x00000100l
#define DDHAL_CB32_FLIPTOGDISURFACE     0x00000200l

/*
 * DIRECTDRAWPALETTE object callbacks
 */
typedef DWORD   (__far __fastcall *LPDDHALPALCB_DESTROYPALETTE)(LPDDHAL_DESTROYPALETTEDATA );
typedef DWORD   (__far __fastcall *LPDDHALPALCB_SETENTRIES)(LPDDHAL_SETENTRIESDATA );

typedef struct DDHAL_DDPALETTECALLBACKS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    LPDDHALPALCB_DESTROYPALETTE DestroyPalette;
    LPDDHALPALCB_SETENTRIES SetEntries;
} DDHAL_DDPALETTECALLBACKS_t;
typedef DDHAL_DDPALETTECALLBACKS_t __far *LPDDHAL_DDPALETTECALLBACKS;

#define DDPALETTECALLBACKSSIZE  sizeof( DDHAL_DDPALETTECALLBACKS_t )

#define DDHAL_PALCB32_DESTROYPALETTE    0x00000001l
#define DDHAL_PALCB32_SETENTRIES        0x00000002l

/*
 * DIRECTDRAWSURFACE object callbacks
 */
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_LOCK)(LPDDHAL_LOCKDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_UNLOCK)(LPDDHAL_UNLOCKDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_BLT)(LPDDHAL_BLTDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_UPDATEOVERLAY)(LPDDHAL_UPDATEOVERLAYDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_SETOVERLAYPOSITION)(LPDDHAL_SETOVERLAYPOSITIONDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_SETPALETTE)(LPDDHAL_SETPALETTEDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_FLIP)(LPDDHAL_FLIPDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_DESTROYSURFACE)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_SETCLIPLIST)(LPDDHAL_SETCLIPLISTDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_ADDATTACHEDSURFACE)(LPDDHAL_ADDATTACHEDSURFACEDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_SETCOLORKEY)(LPDDHAL_SETCOLORKEYDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_GETBLTSTATUS)(LPDDHAL_GETBLTSTATUSDATA);
typedef DWORD   (__far __fastcall *LPDDHALSURFCB_GETFLIPSTATUS)(LPDDHAL_GETFLIPSTATUSDATA);


typedef struct DDHAL_DDSURFACECALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALSURFCB_DESTROYSURFACE    DestroySurface;
    LPDDHALSURFCB_FLIP          Flip;
    LPDDHALSURFCB_SETCLIPLIST       SetClipList;
    LPDDHALSURFCB_LOCK          Lock;
    LPDDHALSURFCB_UNLOCK        Unlock;
    LPDDHALSURFCB_BLT           Blt;
    LPDDHALSURFCB_SETCOLORKEY       SetColorKey;
    LPDDHALSURFCB_ADDATTACHEDSURFACE    AddAttachedSurface;
    LPDDHALSURFCB_GETBLTSTATUS      GetBltStatus;
    LPDDHALSURFCB_GETFLIPSTATUS     GetFlipStatus;
    LPDDHALSURFCB_UPDATEOVERLAY     UpdateOverlay;
    LPDDHALSURFCB_SETOVERLAYPOSITION    SetOverlayPosition;
    LPVOID              reserved4;
    LPDDHALSURFCB_SETPALETTE        SetPalette;
} DDHAL_DDSURFACECALLBACKS_t;
typedef DDHAL_DDSURFACECALLBACKS_t __far *LPDDHAL_DDSURFACECALLBACKS;

#define DDSURFACECALLBACKSSIZE sizeof( DDHAL_DDSURFACECALLBACKS_t )

#define DDHAL_SURFCB32_DESTROYSURFACE       0x00000001UL
#define DDHAL_SURFCB32_FLIP         0x00000002UL
#define DDHAL_SURFCB32_SETCLIPLIST      0x00000004UL
#define DDHAL_SURFCB32_LOCK         0x00000008UL
#define DDHAL_SURFCB32_UNLOCK           0x00000010UL
#define DDHAL_SURFCB32_BLT          0x00000020UL
#define DDHAL_SURFCB32_SETCOLORKEY      0x00000040UL
#define DDHAL_SURFCB32_ADDATTACHEDSURFACE   0x00000080UL
#define DDHAL_SURFCB32_GETBLTSTATUS         0x00000100UL
#define DDHAL_SURFCB32_GETFLIPSTATUS        0x00000200UL
#define DDHAL_SURFCB32_UPDATEOVERLAY        0x00000400UL
#define DDHAL_SURFCB32_SETOVERLAYPOSITION   0x00000800UL
#define DDHAL_SURFCB32_RESERVED4        0x00001000UL
#define DDHAL_SURFCB32_SETPALETTE       0x00002000UL

// This structure can be queried from the driver from DX5 onward
// using GetDriverInfo with GUID_MiscellaneousCallbacks

typedef DWORD   (__far __fastcall *LPDDHAL_GETAVAILDRIVERMEMORY)(LPDDHAL_GETAVAILDRIVERMEMORYDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_UPDATENONLOCALHEAP)(LPDDHAL_UPDATENONLOCALHEAPDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_GETHEAPALIGNMENT)(LPDDHAL_GETHEAPALIGNMENTDATA);
/*
 * This prototype is identical to that of GetBltStatus
 */

typedef struct DDHAL_DDMISCELLANEOUSCALLBACKS {
    DWORD                               dwSize;
    DWORD                               dwFlags;
    LPDDHAL_GETAVAILDRIVERMEMORY        GetAvailDriverMemory;
    LPDDHAL_UPDATENONLOCALHEAP          UpdateNonLocalHeap;
    LPDDHAL_GETHEAPALIGNMENT            GetHeapAlignment;
    /*
     * The GetSysmemBltStatus callback uses the same prototype as GetBltStatus.
     * It is legal to point both pointers to the same driver routine.
     */
    LPDDHALSURFCB_GETBLTSTATUS          GetSysmemBltStatus;
} DDHAL_DDMISCELLANEOUSCALLBACKS_t;
typedef DDHAL_DDMISCELLANEOUSCALLBACKS_t __far *LPDDHAL_DDMISCELLANEOUSCALLBACKS;

#define DDHAL_MISCCB32_GETAVAILDRIVERMEMORY    0x00000001l
#define DDHAL_MISCCB32_UPDATENONLOCALHEAP      0x00000002l
#define DDHAL_MISCCB32_GETHEAPALIGNMENT        0x00000004l
#define DDHAL_MISCCB32_GETSYSMEMBLTSTATUS      0x00000008l

#define DDMISCELLANEOUSCALLBACKSSIZE sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS)


// DDHAL_DDMISCELLANEOUS2CALLBACKS:
//   This structure can be queried from the driver from DX7 onward
//   using GetDriverInfo with GUID_Miscellaneous2Callbacks

typedef DWORD   (__far __fastcall *LPDDHAL_CREATESURFACEEX)(LPDDHAL_CREATESURFACEEXDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_GETDRIVERSTATE)(LPDDHAL_GETDRIVERSTATEDATA);
typedef DWORD   (__far __fastcall *LPDDHAL_DESTROYDDLOCAL)(LPDDHAL_DESTROYDDLOCALDATA);

typedef struct _DDHAL_DDMISCELLANEOUS2CALLBACKS {
    DWORD                               dwSize;
    DWORD                               dwFlags;
    LPVOID                              Reserved;
    LPDDHAL_CREATESURFACEEX             CreateSurfaceEx;
    LPDDHAL_GETDRIVERSTATE              GetDriverState;
    LPDDHAL_DESTROYDDLOCAL              DestroyDDLocal;
} DDHAL_DDMISCELLANEOUS2CALLBACKS, *LPDDHAL_DDMISCELLANEOUS2CALLBACKS;

#define DDHAL_MISC2CB32_CREATESURFACEEX        0x00000002l
#define DDHAL_MISC2CB32_GETDRIVERSTATE         0x00000004l
#define DDHAL_MISC2CB32_DESTROYDDLOCAL         0x00000008l

#define DDMISCELLANEOUS2CALLBACKSSIZE sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS)

/*
 * DIRECTDRAWEXEBUF pseudo object callbacks
 *
 * NOTE: Execute buffers are not a distinct object type, they piggy back off
 * the surface data structures and high level API. However, they have their
 * own HAL callbacks as they may have different driver semantics from "normal"
 * surfaces. They also piggy back off the HAL data structures.
 *
 * !!! NOTE: Need to resolve whether we export execute buffer copying as a
 * blit or some other from of copy instruction.
 */
typedef DWORD   (__far __fastcall *LPDDHALEXEBUFCB_CANCREATEEXEBUF)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD   (__far __fastcall *LPDDHALEXEBUFCB_CREATEEXEBUF)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD   (__far __fastcall *LPDDHALEXEBUFCB_DESTROYEXEBUF)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD   (__far __fastcall *LPDDHALEXEBUFCB_LOCKEXEBUF)(LPDDHAL_LOCKDATA);
typedef DWORD   (__far __fastcall *LPDDHALEXEBUFCB_UNLOCKEXEBUF)(LPDDHAL_UNLOCKDATA);

typedef struct DDHAL_DDEXEBUFCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALEXEBUFCB_CANCREATEEXEBUF CanCreateExecuteBuffer;
    LPDDHALEXEBUFCB_CREATEEXEBUF    CreateExecuteBuffer;
    LPDDHALEXEBUFCB_DESTROYEXEBUF   DestroyExecuteBuffer;
    LPDDHALEXEBUFCB_LOCKEXEBUF      LockExecuteBuffer;
    LPDDHALEXEBUFCB_UNLOCKEXEBUF    UnlockExecuteBuffer;
} DDHAL_DDEXEBUFCALLBACKS_t;
typedef DDHAL_DDEXEBUFCALLBACKS_t __far *LPDDHAL_DDEXEBUFCALLBACKS;

#define DDEXEBUFCALLBACKSSIZE sizeof( DDHAL_DDEXEBUFCALLBACKS )

#define DDHAL_EXEBUFCB32_CANCREATEEXEBUF    0x00000001l
#define DDHAL_EXEBUFCB32_CREATEEXEBUF       0x00000002l
#define DDHAL_EXEBUFCB32_DESTROYEXEBUF      0x00000004l
#define DDHAL_EXEBUFCB32_LOCKEXEBUF     0x00000008l
#define DDHAL_EXEBUFCB32_UNLOCKEXEBUF       0x00000010l

/*
 * DIRECTVIDEOPORT object callbacks
 */
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_CANCREATEVIDEOPORT)(LPDDHAL_CANCREATEVPORTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_CREATEVIDEOPORT)(LPDDHAL_CREATEVPORTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_FLIP)(LPDDHAL_FLIPVPORTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETBANDWIDTH)(LPDDHAL_GETVPORTBANDWIDTHDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETINPUTFORMATS)(LPDDHAL_GETVPORTINPUTFORMATDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETOUTPUTFORMATS)(LPDDHAL_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETFIELD)(LPDDHAL_GETVPORTFIELDDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETLINE)(LPDDHAL_GETVPORTLINEDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETVPORTCONNECT)(LPDDHAL_GETVPORTCONNECTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_DESTROYVPORT)(LPDDHAL_DESTROYVPORTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETFLIPSTATUS)(LPDDHAL_GETVPORTFLIPSTATUSDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_UPDATE)(LPDDHAL_UPDATEVPORTDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_WAITFORSYNC)(LPDDHAL_WAITFORVPORTSYNCDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_GETSIGNALSTATUS)(LPDDHAL_GETVPORTSIGNALDATA);
typedef DWORD (__far __fastcall *LPDDHALVPORTCB_COLORCONTROL)(LPDDHAL_VPORTCOLORDATA);

typedef struct DDHAL_DDVIDEOPORTCALLBACKS
{
	DWORD               dwSize;
	DWORD               dwFlags;
	LPDDHALVPORTCB_CANCREATEVIDEOPORT   CanCreateVideoPort;
	LPDDHALVPORTCB_CREATEVIDEOPORT      CreateVideoPort;
	LPDDHALVPORTCB_FLIP                 FlipVideoPort;
	LPDDHALVPORTCB_GETBANDWIDTH         GetVideoPortBandwidth;
	LPDDHALVPORTCB_GETINPUTFORMATS      GetVideoPortInputFormats;
	LPDDHALVPORTCB_GETOUTPUTFORMATS     GetVideoPortOutputFormats;
	LPVOID              lpReserved1;
	LPDDHALVPORTCB_GETFIELD             GetVideoPortField;
	LPDDHALVPORTCB_GETLINE              GetVideoPortLine;
	LPDDHALVPORTCB_GETVPORTCONNECT      GetVideoPortConnectInfo;
	LPDDHALVPORTCB_DESTROYVPORT         DestroyVideoPort;
	LPDDHALVPORTCB_GETFLIPSTATUS        GetVideoPortFlipStatus;
	LPDDHALVPORTCB_UPDATE               UpdateVideoPort;
	LPDDHALVPORTCB_WAITFORSYNC          WaitForVideoPortSync;
	LPDDHALVPORTCB_GETSIGNALSTATUS      GetVideoSignalStatus;
	LPDDHALVPORTCB_COLORCONTROL         ColorControl;
} DDHAL_DDVIDEOPORTCALLBACKS_t;
typedef DDHAL_DDVIDEOPORTCALLBACKS_t __far *LPDDHAL_DDVIDEOPORTCALLBACKS;

#define DDVIDEOPORTCALLBACKSSIZE sizeof( DDHAL_DDVIDEOPORTCALLBACKS )

#define DDHAL_VPORT32_CANCREATEVIDEOPORT    0x00000001l
#define DDHAL_VPORT32_CREATEVIDEOPORT           0x00000002l
#define DDHAL_VPORT32_FLIP                      0x00000004l
#define DDHAL_VPORT32_GETBANDWIDTH              0x00000008l
#define DDHAL_VPORT32_GETINPUTFORMATS           0x00000010l
#define DDHAL_VPORT32_GETOUTPUTFORMATS          0x00000020l
#define DDHAL_VPORT32_GETFIELD                  0x00000080l
#define DDHAL_VPORT32_GETLINE                   0x00000100l
#define DDHAL_VPORT32_GETCONNECT                0x00000200l
#define DDHAL_VPORT32_DESTROY                   0x00000400l
#define DDHAL_VPORT32_GETFLIPSTATUS             0x00000800l
#define DDHAL_VPORT32_UPDATE                    0x00001000l
#define DDHAL_VPORT32_WAITFORSYNC               0x00002000l
#define DDHAL_VPORT32_GETSIGNALSTATUS           0x00004000l
#define DDHAL_VPORT32_COLORCONTROL      0x00008000l

/*
 * DIRECTDRAWCOLORCONTROL object callbacks
 */
typedef DWORD (__far __fastcall *LPDDHALCOLORCB_COLORCONTROL)(LPDDHAL_COLORCONTROLDATA);

typedef struct DDHAL_DDCOLORCONTROLCALLBACKS
{
	DWORD               dwSize;
	DWORD               dwFlags;
	LPDDHALCOLORCB_COLORCONTROL         ColorControl;
} DDHAL_DDCOLORCONTROLCALLBACKS_t;
typedef DDHAL_DDCOLORCONTROLCALLBACKS_t __far *LPDDHAL_DDCOLORCONTROLCALLBACKS;

#define DDCOLORCONTROLCALLBACKSSIZE sizeof( DDHAL_DDCOLORCONTROLCALLBACKS )

#define DDHAL_COLOR_COLORCONTROL        0x00000001l

/*
 * CALLBACK RETURN VALUES
 *				        * these are values returned by the driver from the above callback routines
 */
/*
 * indicates that the display driver didn't do anything with the call
 */
#define DDHAL_DRIVER_NOTHANDLED		0x00000000L

/*
 * indicates that the display driver handled the call; HRESULT value is valid
 */
#define DDHAL_DRIVER_HANDLED		0x00000001L

/*
 * indicates that the display driver couldn't handle the call because it
 * ran out of color key hardware resources
 */
#define DDHAL_DRIVER_NOCKEYHW		0x00000002L

/*
 * DIRECTDRAWSURFACEKERNEL object callbacks
 * This structure can be queried from the driver from DX5 onward
 * using GetDriverInfo with GUID_KernelCallbacks
 */
typedef DWORD (__far __fastcall *LPDDHALKERNELCB_SYNCSURFACE)(LPDDHAL_SYNCSURFACEDATA);
typedef DWORD (__far __fastcall *LPDDHALKERNELCB_SYNCVIDEOPORT)(LPDDHAL_SYNCVIDEOPORTDATA);

typedef struct DDHAL_DDKERNELCALLBACKS
{
	DWORD                               dwSize;
	DWORD                               dwFlags;
	LPDDHALKERNELCB_SYNCSURFACE     SyncSurfaceData;
	LPDDHALKERNELCB_SYNCVIDEOPORT   SyncVideoPortData;
} DDHAL_DDKERNELCALLBACKS_t;
typedef DDHAL_DDKERNELCALLBACKS_t __far *LPDDHAL_DDKERNELCALLBACKS;

#define DDHAL_KERNEL_SYNCSURFACEDATA        0x00000001l
#define DDHAL_KERNEL_SYNCVIDEOPORTDATA      0x00000002l

#define DDKERNELCALLBACKSSIZE sizeof(DDHAL_DDKERNELCALLBACKS)

//typedef HRESULT (WINAPI *LPDDGAMMACALIBRATORPROC)( LPDDGAMMARAMP, LPBYTE);


/****************************************************************************
 *
 * DDHAL structure for motion comp callbacks
 *
 ***************************************************************************/

struct DDHAL_GETMOCOMPGUIDSDATA;
struct DDHAL_GETMOCOMPFORMATSDATA;
struct DDHAL_CREATEMOCOMPDATA;
struct DDHAL_GETMOCOMPCOMPBUFFDATA;
struct DDHAL_GETINTERNALMOCOMPDATA;
struct DDHAL_BEGINMOCOMPFRAMEDATA;
struct DDHAL_ENDMOCOMPFRAMEDATA;
struct DDHAL_RENDERMOCOMPDATA;
struct DDHAL_QUERYMOCOMPSTATUSDATA;
struct DDHAL_DESTROYMOCOMPDATA;

typedef struct DDHAL_GETMOCOMPGUIDSDATA __far *LPDDHAL_GETMOCOMPGUIDSDATA;
typedef struct DDHAL_GETMOCOMPFORMATSDATA __far *LPDDHAL_GETMOCOMPFORMATSDATA;
typedef struct DDHAL_CREATEMOCOMPDATA __far *LPDDHAL_CREATEMOCOMPDATA;
typedef struct DDHAL_GETMOCOMPCOMPBUFFDATA __far *LPDDHAL_GETMOCOMPCOMPBUFFDATA;
typedef struct DDHAL_GETINTERNALMOCOMPDATA __far *LPDDHAL_GETINTERNALMOCOMPDATA;
typedef struct DDHAL_BEGINMOCOMPFRAMEDATA __far *LPDDHAL_BEGINMOCOMPFRAMEDATA;
typedef struct DDHAL_ENDMOCOMPFRAMEDATA __far *LPDDHAL_ENDMOCOMPFRAMEDATA;
typedef struct DDHAL_RENDERMOCOMPDATA __far *LPDDHAL_RENDERMOCOMPDATA;
typedef struct DDHAL_QUERYMOCOMPSTATUSDATA __far *LPDDHAL_QUERYMOCOMPSTATUSDATA;
typedef struct DDHAL_DESTROYMOCOMPDATA __far *LPDDHAL_DESTROYMOCOMPDATA;

struct DDRAWI_DIRECTDRAW_INT;
struct DDRAWI_DIRECTDRAW_LCL;
struct DDRAWI_DDRAWPALETTE_INT;
struct DDRAWI_DDRAWCLIPPER_INT;
typedef struct DDRAWI_DIRECTDRAW_INT   __far *LPDDRAWI_DIRECTDRAW_INT;
typedef struct DDRAWI_DIRECTDRAW_LCL   __far *LPDDRAWI_DIRECTDRAW_LCL;
typedef struct DDRAWI_DDRAWPALETTE_INT __far *LPDDRAWI_DDRAWPALETTE_INT;
typedef struct DDRAWI_DDRAWCLIPPER_INT __far *LPDDRAWI_DDRAWCLIPPER_INT;

/*
 * DIRECTDRAWMOTIONCOMP object callbacks
 */
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_GETGUIDS)( LPDDHAL_GETMOCOMPGUIDSDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_GETFORMATS)( LPDDHAL_GETMOCOMPFORMATSDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_CREATE)( LPDDHAL_CREATEMOCOMPDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_GETCOMPBUFFINFO)( LPDDHAL_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_GETINTERNALINFO)( LPDDHAL_GETINTERNALMOCOMPDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_BEGINFRAME)( LPDDHAL_BEGINMOCOMPFRAMEDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_ENDFRAME)( LPDDHAL_ENDMOCOMPFRAMEDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_RENDER)( LPDDHAL_RENDERMOCOMPDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_QUERYSTATUS)( LPDDHAL_QUERYMOCOMPSTATUSDATA);
typedef DWORD (__far __fastcall *LPDDHALMOCOMPCB_DESTROY)( LPDDHAL_DESTROYMOCOMPDATA);

/*
 * structure for passing information to DDHAL GetMoCompGuids
 */
typedef struct DDHAL_GETMOCOMPGUIDSDATA
{
	LPDDRAWI_DIRECTDRAW_LCL lpDD;
	DWORD               dwNumGuids;
	LPGUID              lpGuids;
	HRESULT             ddRVal;
	LPDDHALMOCOMPCB_GETGUIDS GetMoCompGuids;
} DDHAL_GETMOCOMPGUIDSDATA_t;


typedef struct DDHAL_DDMOTIONCOMPCALLBACKS
{
	DWORD                           dwSize;
	DWORD                           dwFlags;
	LPDDHALMOCOMPCB_GETGUIDS        GetMoCompGuids;
	LPDDHALMOCOMPCB_GETFORMATS      GetMoCompFormats;
	LPDDHALMOCOMPCB_CREATE          CreateMoComp;
	LPDDHALMOCOMPCB_GETCOMPBUFFINFO GetMoCompBuffInfo;
	LPDDHALMOCOMPCB_GETINTERNALINFO GetInternalMoCompInfo;
	LPDDHALMOCOMPCB_BEGINFRAME      BeginMoCompFrame;
	LPDDHALMOCOMPCB_ENDFRAME        EndMoCompFrame;
	LPDDHALMOCOMPCB_RENDER          RenderMoComp;
	LPDDHALMOCOMPCB_QUERYSTATUS     QueryMoCompStatus;
	LPDDHALMOCOMPCB_DESTROY         DestroyMoComp;
} DDHAL_DDMOTIONCOMPCALLBACKS_t;
typedef DDHAL_DDMOTIONCOMPCALLBACKS_t __far *LPDDHAL_DDMOTIONCOMPCALLBACKS;

#define DDMOTIONCOMPCALLBACKSSIZE sizeof( DDHAL_DDMOTIONCOMPCALLBACKS_t )

#define DDHAL_MOCOMP32_GETGUIDS         0x00000001
#define DDHAL_MOCOMP32_GETFORMATS       0x00000002
#define DDHAL_MOCOMP32_CREATE           0x00000004
#define DDHAL_MOCOMP32_GETCOMPBUFFINFO  0x00000008
#define DDHAL_MOCOMP32_GETINTERNALINFO  0x00000010
#define DDHAL_MOCOMP32_BEGINFRAME       0x00000020
#define DDHAL_MOCOMP32_ENDFRAME         0x00000040
#define DDHAL_MOCOMP32_RENDER           0x00000080
#define DDHAL_MOCOMP32_QUERYSTATUS      0x00000100
#define DDHAL_MOCOMP32_DESTROY          0x00000200

typedef struct DDNONLOCALVIDMEMCAPS
{
	DWORD   dwSize;
	DWORD   dwNLVBCaps;       // driver specific capabilities for non-local->local vidmem blts
	DWORD   dwNLVBCaps2;          // more driver specific capabilities non-local->local vidmem blts
	DWORD   dwNLVBCKeyCaps;       // driver color key capabilities for non-local->local vidmem blts
	DWORD   dwNLVBFXCaps;         // driver FX capabilities for non-local->local blts
	DWORD   dwNLVBRops[DD_ROP_SPACE]; // ROPS supported for non-local->local blts
} DDNONLOCALVIDMEMCAPS_t;
typedef struct DDNONLOCALVIDMEMCAPS_t __far *LPDDNONLOCALVIDMEMCAPS;

/* contants from WinDDK 7600.16385.1 */
#define DD_VERSION              0x00000200UL
//#define DD_RUNTIME_VERSION      0x00000902UL
#define DD_RUNTIME_VERSION      0x00000700UL

/*
 * this is the HAL version.
 * the driver returns this number from QUERYESCSUPPORT/DCICOMMAND
 */
#define DD_HAL_VERSION          0x0100

/* mode information */
typedef struct DDHALMODEINFO
{
    DWORD	dwWidth;		// width (in pixels) of mode
    DWORD	dwHeight;		// height (in pixels) of mode
    LONG	lPitch;			// pitch (in bytes) of mode
    DWORD	dwBPP;			// bits per pixel
    WORD	wFlags;			// flags
    WORD	wRefreshRate;		// refresh rate
    DWORD	dwRBitMask;		// red bit mask
    DWORD	dwGBitMask;		// green bit mask
    DWORD	dwBBitMask;		// blue bit mask
    DWORD	dwAlphaBitMask;		// alpha bit mask
} DDHALMODEINFO_t;

typedef DDHALMODEINFO_t __far *LPDDHALMODEINFO;

#define DDMODEINFO_PALETTIZED   0x0001  // mode is palettized
#define DDMODEINFO_MODEX        0x0002  // mode is a modex mode
#define DDMODEINFO_UNSUPPORTED  0x0004  // mode is not supported by driver

/*
 * DDRAW interface struct
 */ 
typedef struct DDRAWI_DIRECTDRAW_INT
{
    LPVOID                      lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DIRECTDRAW_LCL     lpLcl;      // pointer to interface data
    LPDDRAWI_DIRECTDRAW_INT     lpLink;     // link to next interface
    DWORD                       dwIntRefCnt;    // interface reference count
} DDRAWI_DIRECTDRAW_INT_t;

/*
 * DDRAW version of DirectDraw object; it has data after the vtable
 *
 * all entries marked as PRIVATE are not for use by the display driver
 */
typedef struct DDHAL_CALLBACKS
{
    DDHAL_DDCALLBACKS_t         cbDDCallbacks;  // addresses in display driver for DIRECTDRAW object HAL
    DDHAL_DDSURFACECALLBACKS_t  cbDDSurfaceCallbacks; // addresses in display driver for DIRECTDRAWSURFACE object HAL
    DDHAL_DDPALETTECALLBACKS_t  cbDDPaletteCallbacks; // addresses in display driver for DIRECTDRAWPALETTE object HAL
    DDHAL_DDCALLBACKS_t         HALDD;      // HAL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS_t  HALDDSurface;   // HAL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS_t  HALDDPalette;   // HAL for DIRECTDRAWPALETTE object
    DDHAL_DDCALLBACKS_t         HELDD;      // HEL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS_t  HELDDSurface;   // HEL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS_t  HELDDPalette;   // HEL for DIRECTDRAWPALETTE object
    DDHAL_DDEXEBUFCALLBACKS_t   cbDDExeBufCallbacks; // addresses in display driver for DIRECTDRAWEXEBUF pseudo object HAL
    DDHAL_DDEXEBUFCALLBACKS_t   HALDDExeBuf;    // HAL for DIRECTDRAWEXEBUF pseudo object
    DDHAL_DDEXEBUFCALLBACKS_t   HELDDExeBuf;    // HEL for DIRECTDRAWEXEBUF preudo object
    DDHAL_DDVIDEOPORTCALLBACKS_t cbDDVideoPortCallbacks; // addresses in display driver for VideoPort object HAL
    DDHAL_DDVIDEOPORTCALLBACKS_t HALDDVideoPort; // HAL for DIRECTDRAWVIDEOPORT psuedo object
    DDHAL_DDCOLORCONTROLCALLBACKS_t cbDDColorControlCallbacks; // addresses in display driver for color control object HAL
    DDHAL_DDCOLORCONTROLCALLBACKS_t HALDDColorControl; // HAL for DIRECTDRAWCOLORCONTROL psuedo object
    DDHAL_DDMISCELLANEOUSCALLBACKS_t cbDDMiscellaneousCallbacks;
    DDHAL_DDMISCELLANEOUSCALLBACKS_t HALDDMiscellaneous;
    DDHAL_DDKERNELCALLBACKS_t     cbDDKernelCallbacks;
    DDHAL_DDKERNELCALLBACKS_t HALDDKernel;
    DDHAL_DDMOTIONCOMPCALLBACKS_t cbDDMotionCompCallbacks;
    DDHAL_DDMOTIONCOMPCALLBACKS_t HALDDMotionComp;
} DDHAL_CALLBACKS_t;

typedef DDHAL_CALLBACKS_t *LPDDHAL_CALLBACKS;

/*
 * This structure mirrors the first entries of the DDCAPS but is of a fixed
 * size and will not grow as DDCAPS grows. This is the structure your driver
 * returns in DDCOREINFO. Additional caps will be requested via a GetDriverInfo
 * call.
 */
typedef struct DDCORECAPS
{
    DWORD   dwSize;         // size of the DDDRIVERCAPS structure
    DWORD   dwCaps;         // driver specific capabilities
    DWORD   dwCaps2;        // more driver specific capabilites
    DWORD   dwCKeyCaps;     // color key capabilities of the surface
    DWORD   dwFXCaps;       // driver specific stretching and effects capabilites
    DWORD   dwFXAlphaCaps;      // alpha driver specific capabilities
    DWORD   dwPalCaps;      // palette capabilities
    DWORD   dwSVCaps;       // stereo vision capabilities
    DWORD   dwAlphaBltConstBitDepths;   // DDBD_2,4,8
    DWORD   dwAlphaBltPixelBitDepths;   // DDBD_1,2,4,8
    DWORD   dwAlphaBltSurfaceBitDepths; // DDBD_1,2,4,8
    DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
    DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
    DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
    DWORD   dwZBufferBitDepths;     // DDBD_8,16,24,32
    DWORD   dwVidMemTotal;      // total amount of video memory
    DWORD   dwVidMemFree;       // amount of free video memory
    DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
    DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
    DWORD   dwNumFourCCCodes;   // number of four cc codes
    DWORD   dwAlignBoundarySrc; // source rectangle alignment
    DWORD   dwAlignSizeSrc;     // source rectangle byte size
    DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
    DWORD   dwAlignSizeDest;    // dest rectangle byte size
    DWORD   dwAlignStrideAlign; // stride alignment
    DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
    DDSCAPS_t ddsCaps;        // DDSCAPS structure has all the general capabilities
    DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwReserved1;        // reserved
    DWORD   dwReserved2;        // reserved
    DWORD   dwReserved3;        // reserved
    DWORD   dwSVBCaps;      // driver specific capabilities for System->Vmem blts
    DWORD   dwSVBCKeyCaps;      // driver color key capabilities for System->Vmem blts
    DWORD   dwSVBFXCaps;        // driver FX capabilities for System->Vmem blts
    DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
    DWORD   dwVSBCaps;      // driver specific capabilities for Vmem->System blts
    DWORD   dwVSBCKeyCaps;      // driver color key capabilities for Vmem->System blts
    DWORD   dwVSBFXCaps;        // driver FX capabilities for Vmem->System blts
    DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
    DWORD   dwSSBCaps;      // driver specific capabilities for System->System blts
    DWORD   dwSSBCKeyCaps;      // driver color key capabilities for System->System blts
    DWORD   dwSSBFXCaps;        // driver FX capabilities for System->System blts
    DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
    DWORD   dwMaxVideoPorts;    // maximum number of usable video ports
    DWORD   dwCurrVideoPorts;   // current number of video ports used
    DWORD   dwSVBCaps2;     // more driver specific capabilities for System->Vmem blts
} DDCORECAPS_t;

typedef DDCORECAPS_t __far* LPDDCORECAPS;

struct DDRAWI_DIRECTDRAW_GBL;
struct DDRAWI_DDVIDEOPORT_LCL;
struct DDRAWI_DDVIDEOPORT_INT;
struct DDRAWI_DDRAWSURFACE_LCL;
struct DDRAWI_DDRAWSURFACE_INT;
typedef struct DDRAWI_DIRECTDRAW_GBL __far *LPDDRAWI_DIRECTDRAW_GBL;
typedef struct DDRAWI_DDVIDEOPORT_LCL __far *LPDDRAWI_DDVIDEOPORT_LCL;
typedef struct DDRAWI_DDVIDEOPORT_INT __far *LPDDRAWI_DDVIDEOPORT_INT;
typedef struct DDRAWI_DDRAWSURFACE_LCL __far *LPDDRAWI_DDRAWSURFACE_LCL;
typedef struct DDRAWI_DDRAWSURFACE_INT __far *LPDDRAWI_DDRAWSURFACE_INT;

/*
 * DDRAW surface interface struct
 */
typedef struct DDRAWI_DDRAWSURFACE_INT
{
    LPVOID                      lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DDRAWSURFACE_LCL   lpLcl;      // pointer to interface data
    LPDDRAWI_DDRAWSURFACE_INT   lpLink;     // link to next interface
    DWORD                       dwIntRefCnt;    // interface reference count
} DDRAWI_DDRAWSURFACE_INT_t;

/*
 * DBLNODE - a node in a doubly-linked list of surface interfaces
 */
typedef struct DBLNODE
{
    struct  DBLNODE __far *next;  // link to next node
    struct  DBLNODE __far *prev;  // link to previous node
    LPDDRAWI_DDRAWSURFACE_LCL           object;     // link to object
    LPDDRAWI_DDRAWSURFACE_INT           object_int; // object interface
} DBLNODE_t;
typedef DBLNODE_t __far *LPDBLNODE;

/*
 * VideoPort object interface
 */
typedef struct DDRAWI_DDVIDEOPORT_INT
{
    LPVOID			lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DDVIDEOPORT_LCL	lpLcl;		// pointer to interface data
    LPDDRAWI_DDVIDEOPORT_INT	lpLink;		// link to next interface
    DWORD			dwIntRefCnt;	// interface reference count
    DWORD			dwFlags;	// Private
} DDRAWI_DDVIDEOPORT_INT_t;

typedef struct DDRAWI_DIRECTDRAW_LCL
{
    DWORD			lpDDMore;	    // pointer to additional local data
    LPDDRAWI_DIRECTDRAW_GBL     lpGbl;		    // pointer to data
    DWORD			dwUnused0;	    // not currently used
    DWORD                       dwLocalFlags;	    // local flags (DDRAWILCL_)
    DWORD                       dwLocalRefCnt;	    // local ref cnt
    DWORD                       dwProcessId;	    // owning process id
//    IUnknown                    FAR *pUnkOuter;	    // outer IUnknown
    DWORD                       dwObsolete1;
    DWORD                       hWnd;
    DWORD			hDC;
    DWORD			dwErrorMode;
    LPDDRAWI_DDRAWSURFACE_INT	lpPrimary;
    LPDDRAWI_DDRAWSURFACE_INT	lpCB;
    DWORD			dwPreferredMode;
    //------- Fields added in Version 2.0 -------
    HINSTANCE                   hD3DInstance;	    // Handle of Direct3D's DLL.
//    IUnknown                    FAR *pD3DIUnknown;  // Direct3D's aggregated IUnknown.
    LPDDHAL_CALLBACKS		lpDDCB;		    // HAL callbacks
    DWORD			hDSVxd;		    // handle to dsound.vxd
    //------- Fields added in Version 5.0 -------
    DWORD			dwAppHackFlags;	    // app compatibilty flags
    //------- Fields added in Version 5.A -------
    DWORD			hFocusWnd;	    // Focus window set via SetCoopLevel
    DWORD                       dwHotTracking;      // Reactive menu etc setting cached while fullscreen
} DDRAWI_DIRECTDRAW_LCL_t;

#define DDRAWILCL_HASEXCLUSIVEMODE	0x00000001l
#define DDRAWILCL_ISFULLSCREEN		0x00000002l
#define DDRAWILCL_SETCOOPCALLED		0x00000004l
#define	DDRAWILCL_ACTIVEYES		0x00000008l
#define	DDRAWILCL_ACTIVENO		0x00000010l
#define DDRAWILCL_HOOKEDHWND		0x00000020l
#define DDRAWILCL_ALLOWMODEX            0x00000040l
#define DDRAWILCL_V1SCLBEHAVIOUR	0x00000080l
#define DDRAWILCL_MODEHASBEENCHANGED    0x00000100l
#define DDRAWILCL_CREATEDWINDOW         0x00000200l
#define DDRAWILCL_DIRTYDC               0x00000400l     // Set on ChangeDisplaySettings, cleared when device DC is reinited
#define DDRAWILCL_DISABLEINACTIVATE     0x00000800l

#define DDRAWI_xxxxxxxxx1		0x00000001l     // unused
#define DDRAWI_xxxxxxxxx2       	0x00000002l	// unused
#define DDRAWI_EXPLICITMONITOR		0x00000004l	// device was chosen explicitly i.e. not DDrawCreate(NULL)
#define DDRAWI_VIRTUALDESKTOP           0x00000008l     // driver is really a multi-monitor virtual desktop
#define DDRAWI_MODEX			0x00000010l	// driver is using modex
#define DDRAWI_DISPLAYDRV		0x00000020l	// driver is display driver
#define DDRAWI_FULLSCREEN		0x00000040l	// driver in fullscreen mode
#define DDRAWI_MODECHANGED		0x00000080l	// display mode has been changed
#define DDRAWI_NOHARDWARE		0x00000100l	// no driver hardware at all
#define DDRAWI_PALETTEINIT		0x00000200l	// GDI palette stuff has been initalized
#define DDRAWI_NOEMULATION		0x00000400l	// no emulation at all
#define DDRAWI_HASCKEYDESTOVERLAY 	0x00000800l	// driver has CKDestOverlay
#define DDRAWI_HASCKEYSRCOVERLAY	0x00001000l	// driver has CKSrcOverlay
#define DDRAWI_HASGDIPALETTE		0x00002000l	// GDI palette exists on primary surface
#define DDRAWI_EMULATIONINITIALIZED	0x00004000l	// emulation is initialized
#define DDRAWI_HASGDIPALETTE_EXCLUSIVE	0x00008000l 	// exclusive mode palette
#define DDRAWI_MODEXILLEGAL		0x00010000l	// modex is not supported by this hardware
#define DDRAWI_FLIPPEDTOGDI             0x00020000l     // driver has been flipped to show GDI surface
#define DDRAWI_NEEDSWIN16FORVRAMLOCK    0x00040000l     // PRIVATE: Win16 lock must be taken when locking a VRAM surface
#define DDRAWI_PDEVICEVRAMBITCLEARED    0x00080000l     // PRIVATE: the PDEVICE's VRAM bit was cleared by a lock
#define DDRAWI_STANDARDVGA              0x00100000l     // Device is using standard VGA mode (DDRAWI_MODEX will be set)
#define DDRAWI_EXTENDEDALIGNMENT        0x00200000l     // At least one heap has extended alignment. Ignore alignment in VIDMEMINFO
#define DDRAWI_CHANGINGMODE	        0x00400000l     // Currently in the middle of a mode change
#define DDRAWI_GDIDRV                   0x00800000l     // Driver is a GDI driver
#define DDRAWI_ATTACHEDTODESKTOP        0x01000000l     // Device is attached to the desktop

typedef struct DDRAWI_DIRECTDRAW_GBL
{
/*  0*/ DWORD			        dwRefCnt;	 // reference count
/*  4*/ DWORD			        dwFlags;	 // flags
/*  8*/ FLATPTR			      fpPrimaryOrig;	 // primary surf vid mem. ptr
/*  c*/ DDCORECAPS_t	    ddCaps;		 // driver caps
/*148*/ DWORD			        dwInternal1;     // Private to ddraw.dll
/*16c*/ DWORD			        dwUnused1[9];	 // not currently used
/*170*/ LPDDHAL_CALLBACKS		lpDDCBtmp;	 // HAL callbacks
/*174*/ LPDDRAWI_DDRAWSURFACE_INT	dsList;		 // PRIVATE: list of all surfaces
/*178*/ LPDDRAWI_DDRAWPALETTE_INT	palList;	 // PRIVATE: list of all palettes
/*17c*/ LPDDRAWI_DDRAWCLIPPER_INT	clipperList;	 // PRIVATE: list of all clippers
/*180*/ LPDDRAWI_DIRECTDRAW_GBL	        lp16DD;		 // PRIVATE: 16-bit ptr to this struct
/*184*/ DWORD			        dwMaxOverlays;	 // maximum number of overlays
/*188*/ DWORD			        dwCurrOverlays;	 // current number of visible overlays
/*18c*/ DWORD			        dwMonitorFrequency; // monitor frequency in current mode
/*190*/ DDCORECAPS_t	        ddHELCaps;	 // HEL capabilities
/*2cc*/ DWORD			        dwUnused2[50];	 // not currently used
/*394*/ DDCOLORKEY_t			ddckCKDestOverlay; // color key for destination overlay use
/*39c*/ DDCOLORKEY_t			ddckCKSrcOverlay; // color key for source overlay use
/*3a4*/ VIDMEMINFO_t			vmiData;	 // info about vid memory
/*3f4*/ LPVOID			        lpDriverHandle;	 // handle for use by display driver
/*   */ 						 // to call fns in DDRAW16.DLL
/*3f8*/ LPDDRAWI_DIRECTDRAW_LCL	        lpExclusiveOwner;   // PRIVATE: exclusive local object
/*3fc*/ DWORD			        dwModeIndex;	 // current mode index
/*400*/ DWORD			        dwModeIndexOrig; // original mode index
/*404*/ DWORD			        dwNumFourCC;	 // number of fourcc codes supported
/*408*/ DWORD			        FAR *lpdwFourCC; // PRIVATE: fourcc codes supported
/*40c*/ DWORD			        dwNumModes;	 // number of modes supported
/*410*/ LPDDHALMODEINFO		        lpModeInfo;	 // PRIVATE: mode information
/*424*/ PROCESS_LIST_t	        plProcessList;	 // PRIVATE: list of processes using driver
/*428*/ DWORD                           dwSurfaceLockCount; // total number of outstanding locks
/*42c*/ DWORD                           dwAliasedLockCnt; // PRIVATE: number of outstanding aliased locks
/*430*/ DWORD                           dwReserved3;     // reserved for use by display driver
/*434*/ DWORD                           hDD;             // PRIVATE: NT Kernel-mode handle (was dwFree3).
/*438*/ char			        cDriverName[MAX_DRIVER_NAME]; // Driver Name
/*444*/ DWORD			        dwReserved1;	 // reserved for use by display driver
/*448*/ DWORD			        dwReserved2;	 // reserved for use by display driver
/*44c*/ DBLNODE_t                       dbnOverlayRoot;  // The root node of the doubly-
/*   */                                                  // linked list of overlay z orders.
/*45c*/ volatile LPWORD                 lpwPDeviceFlags; // driver physical device flags
/*460*/ DWORD                           dwPDevice;       // driver physical device (16:16 pointer)
/*464*/ DWORD			        dwWin16LockCnt;	 // count on win16 holds
/*468*/ LPDDRAWI_DIRECTDRAW_LCL	        lpWin16LockOwner;   // object owning Win16 Lock
/*46c*/ DWORD			        hInstance;	 // instance handle of driver
/*470*/ DWORD			        dwEvent16;	 // 16-bit event
/*474*/ DWORD                           dwSaveNumModes;  // saved number of modes supported
/*   */ //------- Fields added in Version 2.0 -------
/*478*/ DWORD                           lpD3DGlobalDriverData;	// Global D3D Data
/*47c*/ DWORD                           lpD3DHALCallbacks;	// D3D HAL Callbacks
/*480*/ DDCORECAPS_t	        ddBothCaps;	 // logical AND of driver and HEL caps
/*   */ //------- Fields added in Version 5.0 -------
/*5bc*/ LPDDVIDEOPORTCAPS		lpDDVideoPortCaps;// Info returned by the HAL (an array if more than one video port)
/*5c0*/ LPDDRAWI_DDVIDEOPORT_INT	dvpList;	 // PRIVATE: list of all video ports
/*5c4*/ DWORD                           lpD3DHALCallbacks2;     // Post-DX3 D3D HAL callbacks
/*5c8*/ RECT			        rectDevice;	 // rectangle (in desktop coord) for device
/*5d8*/ DWORD			        cMonitors;	 // number of monitors in the system
/*5dc*/ LPVOID			        gpbmiSrc;	 // PRIVATE: used by HEL
/*5e0*/ LPVOID			        gpbmiDest;	 // PRIVATE: used by HEL
/*5e4*/ LPHEAPALIASINFO		        phaiHeapAliases; // PRIVATE: video memory heap aliases
/*5e8*/ DWORD				hKernelHandle;
/*5ec*/ DWORD				pfnNotifyProc;   // Notification proc registered w/ VDD
/*5f0*/ LPDDKERNELCAPS			lpDDKernelCaps;	 // Capabilies of kernel mode interface
/*5f4*/ LPDDNONLOCALVIDMEMCAPS		lpddNLVCaps;     // hardware non-local to local vidmem caps
/*5f8*/ LPDDNONLOCALVIDMEMCAPS		lpddNLVHELCaps;  // emulation layer non-local to local vidmem caps
/*5fc*/ LPDDNONLOCALVIDMEMCAPS		lpddNLVBothCaps; // logical AND of hardware and emulation non-local to local vidmem caps
/*600*/ DWORD                           lpD3DExtendedCaps; // extended caps for D3D
/*   */ //--------Fields added in Version 5.0A
/*604*/ DWORD                           dwDOSBoxEvent;   // Event set when returning from a DOS box
} DDRAWI_DIRECTDRAW_GBL_t;

/*
 * structure for display driver to call DDHAL_Create with
 */
typedef struct DDHALINFO
{
    DWORD                       dwSize;
    LPDDHAL_DDCALLBACKS         lpDDCallbacks;      // direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS  lpDDSurfaceCallbacks;   // surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS  lpDDPaletteCallbacks;   // palette object callbacks
    VIDMEMINFO_t                vmiData;        // video memory info
    DDCORECAPS_t                ddCaps;         // core hw specific caps
    DWORD                       dwMonitorFrequency; // monitor frequency in current mode
    LPDDHAL_GETDRIVERINFO       GetDriverInfo;          // callback to get arbitrary vtable from driver
    DWORD                       dwModeIndex;        // current mode: index into array
    LPDWORD                     lpdwFourCC;     // fourcc codes supported
    DWORD                       dwNumModes;     // number of modes supported
    LPDDHALMODEINFO             lpModeInfo;     // mode information
    DWORD                       dwFlags;        // create flags
    LPVOID                      lpPDevice;      // physical device ptr
    DWORD                       hInstance;      // instance handle of driver
    //------- Fields added in Version 2.0 -------
    ULONG_PTR                    lpD3DGlobalDriverData;  // D3D global Data
    ULONG_PTR                   lpD3DHALCallbacks;  // D3D callbacks
    LPDDHAL_DDEXEBUFCALLBACKS   lpDDExeBufCallbacks;    // Execute buffer pseudo object callbacks
} DDHALINFO_t;

typedef DDHALINFO_t __far *LPDDHALINFO;

#define DDHALINFOSIZE_V2 sizeof( DDHALINFO )

#define DDHALINFO_ISPRIMARYDISPLAY	0x00000001l	// indicates driver is primary display driver
#define DDHALINFO_MODEXILLEGAL		0x00000002l	// indicates this hardware does not support modex modes
#define DDHALINFO_GETDRIVERINFOSET      0x00000004l     // indicates that GetDriverInfo is set

typedef struct DDHAL_DESTROYDRIVERDATA
{
	LPDDRAWI_DIRECTDRAW_GBL	lpDD;	// driver struct
	HRESULT			ddRVal;	// return value
	LPDDHAL_DESTROYDRIVER	DestroyDriver;	// PRIVATE: ptr to callback
} DDHAL_DESTROYDRIVERDATA_t;

/*
 * DDRAW16.DLL entry points
 */
typedef BOOL (DDAPI *LPDDHAL_SETINFO)( LPDDHALINFO lpDDHalInfo, BOOL reset );
typedef FLATPTR (DDAPI *LPDDHAL_VIDMEMALLOC)( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, DWORD dwWidth, DWORD dwHeight );
typedef void (DDAPI *LPDDHAL_VIDMEMFREE)( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, FLATPTR fpMem );

extern BOOL DDAPI DDHAL_SetInfo( LPDDHALINFO lpDDHALInfo, BOOL reset );
extern FLATPTR DDAPI DDHAL_VidMemAlloc( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, DWORD dwWidth, DWORD dwHeight );
extern void DDAPI DDHAL_VidMemFree( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, FLATPTR fpMem );


typedef struct
{
    DWORD		dwSize;
    LPDDHAL_SETINFO	lpSetInfo;
    LPDDHAL_VIDMEMALLOC	lpVidMemAlloc;
    LPDDHAL_VIDMEMFREE	lpVidMemFree;
} DDHALDDRAWFNS;
typedef DDHALDDRAWFNS __far *LPDDHALDDRAWFNS;

/* driver functions */
BOOL DDGet32BitDriverName(DD32BITDRIVERDATA_t __far *dd32);
BOOL DDNewCallbackFns(DCICMD_t __far *lpDCICMD);
void DDGetVersion(DDVERSIONDATA_t __far *lpVer);

/* added supported functions */
DWORD DDHinstance(void);

/* DCI */
#define DCI_VERSION                     0x0100

#define DCICREATEPRIMARYSURFACE         1
#define DCICREATEOFFSCREENSURFACE       2
#define DCICREATEOVERLAYSURFACE         3
#define DCIENUMSURFACE                  4
#define DCIESCAPE                       5

/* DCI-Defined error codes */
#define DCI_OK                                  0 /* success */



/*
 * DIRECTDRAW BITDEPTH CONSTANTS
 */
#define DDBD_1			0x00004000l
#define DDBD_2			0x00002000l
#define DDBD_4			0x00001000l
#define DDBD_8			0x00000800l
#define DDBD_16			0x00000400l
#define DDBD_24			0X00000200l
#define DDBD_32			0x00000100l

#endif /* __DDRAWI_H__INCLUDED__ */
