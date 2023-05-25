#ifndef __DMEMMGR_INCLUDED__
#define __DMEMMGR_INCLUDED__

/*
 * pointer to video meory
 */
typedef unsigned long	FLATPTR;

/*
 * Structure for querying extended heap alignment requirements
 */

typedef struct SURFACEALIGNMENT
{
	union
	{
		struct
		{
			DWORD       dwStartAlignment;
			DWORD       dwPitchAlignment;
			DWORD       dwReserved1;
			DWORD       dwReserved2;
		} Linear;
		struct
		{
			DWORD       dwXAlignment;
			DWORD       dwYAlignment;
			DWORD       dwReserved1;
			DWORD       dwReserved2;
		} Rectangular;
	};
} SURFACEALIGNMENT_t;
typedef struct SURFACEALIGNMENT __far *LPSURFACEALIGNMENT;

typedef struct HEAPALIGNMENT
{
	DWORD                dwSize;
	DDSCAPS_t            ddsCaps;       /* Indicates which alignment fields are valid.*/
	DWORD                dwReserved;
	SURFACEALIGNMENT_t   ExecuteBuffer; /* Surfaces tagged with DDSCAPS_EXECUTEBUFFER */
	SURFACEALIGNMENT_t   Overlay;       /* Surfaces tagged with DDSCAPS_OVERLAY       */
	SURFACEALIGNMENT_t   Texture;       /* Surfaces tagged with DDSCAPS_TEXTURE       */
	SURFACEALIGNMENT_t   ZBuffer;       /* Surfaces tagged with DDSCAPS_ZBUFFER       */
	SURFACEALIGNMENT_t   AlphaBuffer;   /* Surfaces tagged with DDSCAPS_ALPHA         */
	SURFACEALIGNMENT_t   Offscreen;     /* Surfaces tagged with DDSCAPS_OFFSCREENPLAIN*/
	SURFACEALIGNMENT_t   FlipTarget;    /* Surfaces whose bits are potential primaries i.e. back buffers*/
} HEAPALIGNMENT_t;
typedef struct HEAPALIGNMENT __far *LPHEAPALIGNMENT;

/*
 * video memory manager structures
 */
typedef struct VMEML
{
	struct VMEML __far *next;
	FLATPTR		ptr;
	DWORD		  size;
} VMEML_t;

typedef struct VMEML __far* LPVMEML;
typedef struct VMEML __far *__far *LPLPVMEML;

typedef struct VMEMR
{
	struct VMEMR __far *next;
	struct VMEMR __far *prev;
	/*
	 * The pUp, pDown, pLeft and pRight members were removed in DX5
	 */
	struct VMEMR __far *pUp;
	struct VMEMR __far *pDown;
	struct VMEMR __far *pLeft;
	struct VMEMR __far *pRight;
	FLATPTR		ptr;
	DWORD	size;
	DWORD x;
	DWORD y;
	DWORD cx;
	DWORD cy;
	DWORD	flags;
	FLATPTR pBits;
} VMEMR_t;

typedef struct VMEMR __far *LPVMEMR;
typedef struct VMEMR __far *__far *LPLPVMEMR;

typedef struct VMEMHEAP
{
	DWORD             dwFlags;
	DWORD             stride;
	LPVOID		        freeList;
	LPVOID		        allocList;
	DWORD             dwTotalSize;
	FLATPTR           fpGARTLin;      /* AGP: GART linear base of heap (app. visible)   */
	FLATPTR           fpGARTDev;      /* AGP: GART device base of heap (driver visible) */
	DWORD             dwCommitedSize; /* AGP: Number of bytes commited to heap          */
	/*
	 * Extended alignment data:
	 * Filled in by DirectDraw in response to a GetHeapAlignment HAL call.
	 */
	DWORD                       dwCoalesceCount;
	HEAPALIGNMENT_t             Alignment;
} VMEMHEAP_t;

typedef VMEMHEAP_t __far *LPVMEMHEAP;

#define VMEMHEAP_LINEAR			    0x00000001l /* Heap is linear                    */
#define VMEMHEAP_RECTANGULAR		0x00000002l /* Heap is rectangular               */
#define VMEMHEAP_ALIGNMENT  		0x00000004l /* Heap has extended alignment info  */

/*
 * These legacy DLL exports don't handle nonlocal heaps
 */
extern FLATPTR WINAPI VidMemAlloc(LPVMEMHEAP pvmh, DWORD width, DWORD height);
extern void WINAPI VidMemFree(LPVMEMHEAP pvmh, FLATPTR ptr);

/*
 * This DLL export can be used by drivers to allocate aligned surfaces from heaps which
 * they have previously exposed to DDRAW.DLL. This function can allocate from nonlocal heaps.
 */
extern FLATPTR WINAPI HeapVidMemAllocAligned(struct VIDMEM* lpVidMem, DWORD dwWidth, DWORD dwHeight, LPSURFACEALIGNMENT lpAlignment , LPLONG lpNewPitch);


#endif
