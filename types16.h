#ifndef __TYPES16_H_INCLUDED__
#define __TYPES16_H_INCLUDED__

#ifndef VXD32
/* 16bit */
typedef long int32;
typedef unsigned long uint32;

typedef int int16;
typedef unsigned int uint16;

typedef signed char int8;
typedef unsigned char uint8;

#define fastcall

#else
/* 32bit */
typedef long int32;
typedef unsigned long uint32;

typedef short int16;
typedef unsigned short uint16;

typedef signed char int8;
typedef unsigned char uint8;

#endif

typedef int Bool;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#endif /* __TYPES16_H_INCLUDED__ */
