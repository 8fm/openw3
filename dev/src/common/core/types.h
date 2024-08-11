/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#ifndef _CORE_TYPES_H_
#define _CORE_TYPES_H_

#include "../redSystem/types.h"
#include "../redSystem/guid.h"
#include "../redMath/random/random.h"
#include "../redMath/random/standardRand.h"

typedef Red::System::Bool Bool;
typedef Red::System::Int8 Int8;
typedef Red::System::Uint8 Uint8;
typedef Red::System::Int16 Int16;
typedef Red::System::Uint16 Uint16;
typedef Red::System::Int32 Int32;
typedef Red::System::Uint32 Uint32;
typedef Red::System::Int64 Int64;
typedef Red::System::Uint64 Uint64;
typedef Red::System::Int32 Int32;
typedef Red::System::Uint32 Uint32;
typedef Red::System::Float Float;
typedef Red::System::Double Double;
typedef Red::System::UniChar UniChar;
typedef Red::System::AnsiChar AnsiChar;
typedef Red::System::Char Char;
typedef Red::System::MemInt MemInt;
typedef Red::System::MemUint MemUint;
typedef Red::System::MemSize MemSize;

typedef Red::System::Uint32 TMemSize;

typedef Red::System::GUID CGUID;

typedef Red::Math::Random::Generator< Red::Math::Random::StandardRand > CStandardRand;

#endif
