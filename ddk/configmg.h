#ifndef __CONFIGMG_H__INCLUDED__
#define __CONFIGMG_H__INCLUDED__
/* Definitions for the configuration manager. */

typedef DWORD CONFIGRET;                /* Standardized return value. */

typedef DWORD LOG_CONF;                 /* Logical configuration. */
typedef LOG_CONF FAR *PLOG_CONF;        /* Far pointer to logical configuration. */

typedef DWORD RES_DES;                  /* Resource descriptor. */
typedef RES_DES FAR *PRES_DES;          /* Far pointer to resource descriptor. */

typedef DWORD DEVNODE;                  /* Devnode. */

typedef DWORD ULONG;

typedef void FAR *PFARVOID;

typedef ULONG RESOURCEID;               /* Resource type ID. */
typedef RESOURCEID FAR *PRESOURCEID;    /* Far pointer to resource type ID. */

typedef struct {
    WORD            MD_Count;
    WORD            MD_Type;
    ULONG           MD_Alloc_Base;
    ULONG           MD_Alloc_End;
    WORD            MD_Flags;
    WORD            MD_Reserved;
} MEM_DES, *PMEM_DES;

#define CR_SUCCESS      0
#define ALLOC_LOG_CONF  2
#define ResType_Mem     1

#endif /* __CONFIGMG_H__INCLUDED__ */
