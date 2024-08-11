
#ifndef TMTYPES_H
#define TMTYPES_H

#ifdef _MSC_VER 
#pragma warning(disable:4121)
#pragma warning(disable:4127)
#endif

#ifndef __RADRR_COREH__
#include "rrCore.h"
#endif

// This is documentation stuff
#undef EXPTYPE
#define EXPTYPE
#undef EXPGROUP
#define EXPGROUP(x)
EXPGROUP(TMAPI)

EXPTYPE typedef S8  TmI8;   // Signed 8-bit integer
EXPTYPE typedef U8  TmU8;   // Unsigned 8-bit integer
EXPTYPE typedef S16 TmI16;  // Signed 16-bit integer
EXPTYPE typedef U16 TmU16;  // Unsigned 16-bit integer
EXPTYPE typedef S32 TmI32;  // Signed 32-bit integer
EXPTYPE typedef U32 TmU32;  // Unsigned 32-bit integer
EXPTYPE typedef F32 TmF32;  // IEEE 32-bit float
EXPTYPE typedef F64 TmF64;  // IEEE 64-bit float (double)
EXPTYPE typedef S64 TmI64;  // Signed 64-bit integer
EXPTYPE typedef U64 TmU64;  // Unsigned 64-bit integer
EXPTYPE typedef RAD_UINTa TmIntPtr; // Minimal size guaranteed to hold a pointer

typedef struct _TmFormatCode
{
    TmU32 fc_format;
    TmU32 fc_fbits;
    char const *fc_ptr;
} TmFormatCode;

// This is documentation stuff
#undef EXPGROUP
#define EXPGROUP()
EXPGROUP()
#undef EXPGROUP

#endif //TMTYPES_H
