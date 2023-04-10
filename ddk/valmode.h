
/* Return values for ValidateMode. */
#define VALMODE_YES		    0   /* Mode is good. */
#define VALMODE_NO_WRONGDRV 1   /* Hardware not supported by driver. */
#define VALMODE_NO_NOMEM	2   /* Insufficient video memory. */
#define VALMODE_NO_NODAC	3   /* DAC cannot handle bit depth. */
#define VALMODE_NO_UNKNOWN	4   /* Some other problem. */


/* Structure describing a display mode. */
typedef struct {
    UINT    dvmSize;    /* Size of this struct. */
    UINT    dvmBpp;     /* Mode color depth. */
    int     dvmXRes;    /* Mode X resolution. */
    int     dvmYRes;    /* Mode Y resolution. */
} DISPVALMODE;

/* Must be exported by name from driver. Recommended ordinal is 700. */
extern UINT WINAPI ValidateMode( DISPVALMODE FAR *lpMode );

