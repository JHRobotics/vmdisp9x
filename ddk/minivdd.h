
/* Functions callable via VDD's API entry point, usually called by display
 * drivers.
 */
#define MINIVDD_SVC_BASE_OFFSET             0x80
#define VDD_DRIVER_REGISTER                 (0 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_DRIVER_UNREGISTER               (1 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SAVE_DRIVER_STATE               (2 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_REGISTER_DISPLAY_DRIVER_INFO    (3 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_REGISTER_SSB_FLAGS              (4 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_GET_DISPLAY_CONFIG              (5 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_PRE_MODE_CHANGE                 (6 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_POST_MODE_CHANGE                (7 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SET_USER_FLAGS                  (8 + MINIVDD_SVC_BASE_OFFSET)
#define VDD_SET_BUSY_FLAG_ADDR              (9 + MINIVDD_SVC_BASE_OFFSET)

/* The DISPLAYINFO structure for querying Registry information. */
typedef struct {
    WORD    diHdrSize;
    WORD    diInfoFlags;
    DWORD   diDevNodeHandle;
    char    diDriverName[16];
    WORD    diXRes;
    WORD    diYRes;
    WORD    diDPI;
    BYTE    diPlanes;
    BYTE    diBpp;
    WORD    diRefreshRateMax;
    WORD    diRefreshRateMin;
    WORD    diLowHorz;
    WORD    diHighHorz;
    WORD    diLowVert;
    WORD    diHighVert;
    DWORD   diMonitorDevNodeHandle;
    BYTE    diHorzSyncPolarity;
    BYTE    diVertSyncPolarity;
} DISPLAYINFO;

/* diInfoFlags */
#define RETURNED_DATA_IS_STALE          0x001   /* VDD couldn't read Registry, data could be old. */
#define MINIVDD_FAILED_TO_LOAD          0x002   /* MiniVDD did not load, probably bad config. */
#define MINIVDD_CHIP_ID_DIDNT_MATCH     0x004   /* ChipID mismatch, probably bad config. */
#define REGISTRY_BPP_NOT_VALID          0x008   /* BPP could not be read from Registry. */
#define REGISTRY_RESOLUTION_NOT_VALID   0x010   /* Resolution could not be read from Registry. */
#define REGISTRY_DPI_NOT_VALID          0x020   /* DPI could not be read from Registry. */
#define MONITOR_DEVNODE_NOT_ACTIVE      0x040   /* Devnode not there, no refresh rate data. */
#define MONITOR_INFO_NOT_VALID          0x080   /* Refresh rate data could not be read. */
#define MONITOR_INFO_DISABLED_BY_USER   0x100   /* Refresh rate data not valid. */
#define REFRESH_RATE_MAX_ONLY           0x200   /* Only diRefreshRateMax is valid. */
#define CARD_VDD_LOADED_OK              0x400   /* Second MiniVDD loaded fine. */

/* Funcrions callable in a mini-VDD. */
#define REGISTER_DISPLAY_DRIVER     0
#define GET_VDD_BANK                1
#define SET_VDD_BANK                2
#define RESET_BANK                  3
#define PRE_HIRES_TO_VGA            4
#define POST_HIRES_TO_VGA           5
#define PRE_VGA_TO_HIRES            6
#define POST_VGA_TO_HIRES           7
#define SAVE_REGISTERS              8
#define RESTORE_REGISTERS           9
#define MODIFY_REGISTER_STATE       10
#define ACCESS_VGA_MEMORY_MODE      11
#define ACCESS_LINEAR_MEMORY_MODE   12
#define ENABLE_TRAPS                13
#define DISABLE_TRAPS               14
#define MAKE_HARDWARE_NOT_BUSY      15
#define VIRTUALIZE_CRTC_IN          16
#define VIRTUALIZE_CRTC_OUT         17
#define VIRTUALIZE_SEQUENCER_IN     18
#define VIRTUALIZE_SEQUENCER_OUT    19
#define VIRTUALIZE_GCR_IN           20
#define VIRTUALIZE_GCR_OUT          21
#define SET_LATCH_BANK              22
#define RESET_LATCH_BANK            23
#define SAVE_LATCHES                24
#define RESTORE_LATCHES             25
#define DISPLAY_DRIVER_DISABLING    26
#define SELECT_PLANE                27
#define PRE_CRTC_MODE_CHANGE        28
#define POST_CRTC_MODE_CHANGE       29
#define VIRTUALIZE_DAC_OUT          30
#define VIRTUALIZE_DAC_IN           31
#define GET_CURRENT_BANK_WRITE      32
#define GET_CURRENT_BANK_READ       33
#define SET_BANK                    34
#define CHECK_HIRES_MODE            35
#define GET_TOTAL_VRAM_SIZE         36
#define GET_BANK_SIZE               37
#define SET_HIRES_MODE              38
#define PRE_HIRES_SAVE_RESTORE      39
#define POST_HIRES_SAVE_RESTORE     40
#define VESA_SUPPORT                41
#define GET_CHIP_ID                 42
#define CHECK_SCREEN_SWITCH_OK      43
#define VIRTUALIZE_BLTER_IO         44
#define SAVE_MESSAGE_MODE_STATE     45
#define SAVE_FORCED_PLANAR_STATE    46
#define VESA_CALL_POST_PROCESSING   47
#define PRE_INT_10_MODE_SET         48

#define NBR_MINI_VDD_FUNCTIONS      49

/* Port sizes. */
#define BYTE_LENGTHED               1
#define WORD_LENGTHED               2

/* Flag bits. */
#define GOING_TO_WINDOWS_MODE       1
#define GOING_TO_VGA_MODE           2
#define DISPLAY_DRIVER_DISABLED     4
#define IN_WINDOWS_HIRES_MODE       8

