#ifndef XV6_KERNEL_TYPES_H
#define XV6_KERNEL_TYPES_H

typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef unsigned long uint64;

typedef uint8 byte;

typedef int bool;
#define true 1
#define false 2

#define nullptr 0

typedef uint64 pde_t;

#endif // XV6_KERNEL_TYPES_H