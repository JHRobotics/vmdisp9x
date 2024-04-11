/* Windows 98 Display Driver Interface */

#define VIDEOPARAMETERS 3077

typedef struct _TVGUID {
  unsigned long  Data1;
  unsigned short Data2;
  unsigned short Data3;
  unsigned char  Data4[8];
};

typedef struct _VIDEOPARAMETERS {
    struct _TVGUID Guid;
    DWORD dwOffset;
    DWORD dwCommand;
    DWORD dwFlags;
    DWORD dwMode;
    DWORD dwTVStandard;
    DWORD dwAvailableModes;
    DWORD dwAvailableTVStandard;
    DWORD dwFlickerFilter;
    DWORD dwOverScanX;
    DWORD dwOverScanY;
    DWORD dwMaxUnscaledX;
    DWORD dwMaxUnscaledY;
    DWORD dwPositionX;
    DWORD dwPositionY;
    DWORD dwBrightness;
    DWORD dwContrast;
    DWORD dwCPType;
    DWORD dwCPCommand;
    DWORD dwCPStandard;
    DWORD dwCPKey;
    BYTE  bCP_APSTriggerBits;
    BYTE  bOEMCopyProtection[256];
} VIDEOPARAMETERS_t;

#define VP_COMMAND_GET          0x0001  // return capabilities.
                                        // return dwFlags = 0 if 
                                        // not supported.
#define VP_COMMAND_SET          0x0002  // parameters set.

#define VP_FLAGS_TV_MODE        0x0001
#define VP_FLAGS_TV_STANDARD    0x0002
#define VP_FLAGS_FLICKER        0x0004
#define VP_FLAGS_OVERSCAN       0x0008
#define VP_FLAGS_MAX_UNSCALED   0x0010  // do not use on SET
#define VP_FLAGS_POSITION       0x0020
#define VP_FLAGS_BRIGHTNESS     0x0040
#define VP_FLAGS_CONTRAST       0x0080
#define VP_FLAGS_COPYPROTECT    0x0100

#define VP_MODE_WIN_GRAPHICS    0x0001  // the display is 
                                        // optimized for Windows 
                                        // FlickerFilter on and 
                                        // OverScan off
#define VP_MODE_TV_PLAYBACK     0x0002  // optimize for TV video 
                                        // playback:
                                        // FlickerFilter off and 
                                        // OverScan on

#define VP_TV_STANDARD_NTSC_M   0x0001  //        75 IRE Setup
#define VP_TV_STANDARD_NTSC_M_J 0x0002  // Japan,  0 IRE Setup
#define VP_TV_STANDARD_PAL_B    0x0004
#define VP_TV_STANDARD_PAL_D    0x0008
#define VP_TV_STANDARD_PAL_H    0x0010
#define VP_TV_STANDARD_PAL_I    0x0020
#define VP_TV_STANDARD_PAL_M    0x0040
#define VP_TV_STANDARD_PAL_N    0x0080
#define VP_TV_STANDARD_SECAM_B  0x0100
#define VP_TV_STANDARD_SECAM_D  0x0200
#define VP_TV_STANDARD_SECAM_G  0x0400
#define VP_TV_STANDARD_SECAM_H  0x0800
#define VP_TV_STANDARD_SECAM_K  0x1000
#define VP_TV_STANDARD_SECAM_K1 0x2000
#define VP_TV_STANDARD_SECAM_L  0x4000
#define VP_TV_STANDARD_WIN_VGA  0x8000    // the display can do VGA graphics
#define VP_TV_STANDARD_NTSC_433 0x00010000
#define VP_TV_STANDARD_PAL_G    0x00020000
#define VP_TV_STANDARD_PAL_60   0x00040000
#define VP_TV_STANDARD_SECAM_L1 0x00080000

#define CP_TYPE_APS_TRIGGER     0x0001  // DVD trigger bits only
#define CP_TYPE_MACROVISION     0x0002

#define VP_CP_CMD_ACTIVATE      0x0001  // CP command type
#define VP_CP_CMD_DEACTIVATE    0x0002
#define VP_CP_CMD_CHANGE        0x0004


