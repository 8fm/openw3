/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#ifndef RED_THREADS_TYPES_H
#define RED_THREADS_TYPES_H
#pragma once

namespace Red { namespace Threads {

#if defined( RED_THREADS_PLATFORM_WINDOWS_API ) 
	typedef DWORD				TSpinCount;
	typedef DWORD				TTimespec;
#elif defined( RED_THREADS_PLATFORM_ORBIS_API )
	typedef int					TSpinCount;
	typedef	SceKernelUseconds	TTimespec; // a Uint32
#else
#	error Platform not defined
#endif

	using Red::System::Bool;
	using Red::System::Int8;
	using Red::System::Uint8;
	using Red::System::Int16;
	using Red::System::Uint16;
	using Red::System::Int32;
	using Red::System::Uint32;
	using Red::System::Int64;
	using Red::System::Uint64;
// 	using Red::System::Float;
// 	using Red::System::Double;
// 	using Red::System::UniChar;
	using Red::System::AnsiChar;
	using Red::System::MemSize;
	using Red::System::MemDiff;
	using Red::System::MemInt;
	using Red::System::MemUint;

	typedef Uint32	TStackSize; // Unsigned 32 bit LCD between the Windows and Pthreads API

#ifdef RED_PLATFORM_ORBIS
	typedef Red::System::Uint64 TAffinityMask;
#elif defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
	typedef DWORD_PTR			TAffinityMask;
#else
#error Platform not supported
#endif

} } // namespace Red { namespace Threads {

#endif // RED_THREADS_TYPES_H