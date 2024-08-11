//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

// AkTypes.h

/// \file 
/// Data type definitions.

#pragma once

#include <limits.h>

#ifndef __cplusplus
	#include <wchar.h> // wchar_t not a built-in type in C
#endif

#define AK_XBOXONE
#define AK_CPU_X86_64								///< Compiling for 64-bit x86 CPU
#define AK_USE_METRO_API

#if WINAPI_FAMILY==WINAPI_FAMILY_TV_APP
	#define AK_XBOXONE_ADK
	#define AK_XAUDIO2_FLAGS 0
#else
	#define AK_XAUDIO2_FLAGS XAUDIO2_DO_NOT_USE_SHAPE
#endif

#define AK_MOTION								///< Internal use

#define AK_71AUDIO								///< Internal use
#define AK_LFECENTER							///< Internal use
#define AK_REARCHANNELS							///< Internal use

#define AK_COMM_NO_DYNAMIC_PORTS				///< Platform does not support dynamic/ephemeral ports for communication

#if !defined(AK_XBOXONE_ADK)
	#define AK_WASAPI							///< Enable WASAPI sink
#endif

#define AK_SUPPORT_WCHAR						///< Can support wchar
#define AK_OS_WCHAR								///< Use wchar natively

#define AK_RESTRICT		__restrict				///< Refers to the __restrict compilation flag available on some platforms
#define AK_EXPECT_FALSE( _x )	(_x)
#define AkForceInline	__forceinline			///< Force inlining
#define AkNoInline		__declspec(noinline)	///< Disable inlining

#define AK_SIMD_ALIGNMENT	16					///< Platform-specific alignment requirement for SIMD data
#define AK_ALIGN_SIMD( __Declaration__ ) __declspec(align(AK_SIMD_ALIGNMENT)) __Declaration__ ///< Platform-specific alignment requirement for SIMD data
#define AK_ALIGN_DMA							///< Platform-specific data alignment for DMA transfers
#define AK_ALIGN_FASTDMA 						///< Platform-specific data alignment for faster DMA transfers
#define AK_ALIGN_SIZE_FOR_DMA( __Size__ ) (__Size__) ///< Used to align sizes to next 16 byte boundary on platfroms that require it
#define AK_BUFFER_ALIGNMENT AK_SIMD_ALIGNMENT

typedef unsigned char		AkUInt8;			///< Unsigned 8-bit integer
typedef unsigned short		AkUInt16;			///< Unsigned 16-bit integer
typedef unsigned long		AkUInt32;			///< Unsigned 32-bit integer
typedef unsigned __int64	AkUInt64;			///< Unsigned 64-bit integer

typedef __int64 AkIntPtr;						///< Integer type for pointers
typedef unsigned __int64 AkUIntPtr;				///< Integer (unsigned) type for pointers

typedef char			AkInt8;					///< Signed 8-bit integer
typedef short			AkInt16;				///< Signed 16-bit integer
typedef long   			AkInt32;				///< Signed 32-bit integer
typedef int 			AkInt;					///< Signed integer
typedef __int64			AkInt64;				///< Signed 64-bit integer

typedef wchar_t			AkOSChar;				///< Generic character string

typedef float			AkReal32;				///< 32-bit floating point
typedef double          AkReal64;				///< 64-bit floating point

typedef void *					AkThread;		///< Thread handle
typedef AkUInt32				AkThreadID;		///< Thread ID
typedef AkUInt32 (__stdcall *AkThreadRoutine)(	void* lpThreadParameter	); ///< Thread routine
typedef void *					AkEvent;		///< Event handle
typedef void *					AkFileHandle;	///< File handle
typedef wchar_t			AkUtf16;				///< Type for 2 byte chars. Used for communication
												///< with the authoring tool.
#define AK_UINT_MAX		UINT_MAX

// For strings.
#define AK_MAX_PATH     260						///< Maximum path length.

typedef AkUInt32			AkFourcc;			///< Riff chunk

/// Create Riff chunk
#define AkmmioFOURCC( ch0, ch1, ch2, ch3 )									    \
		( (AkFourcc)(AkUInt8)(ch0) | ( (AkFourcc)(AkUInt8)(ch1) << 8 ) |		\
		( (AkFourcc)(AkUInt8)(ch2) << 16 ) | ( (AkFourcc)(AkUInt8)(ch3) << 24 ) )

#define AK_BANK_PLATFORM_DATA_ALIGNMENT	(2048)	///< Required memory alignment for bank loading by memory address (see LoadBank()); equivalent to SHAPE_XMA_INPUT_BUFFER_ALIGNMENT on XboxOne.
#define AK_BANK_PLATFORM_ALLOC_TYPE		AkAPUAlloc

/// Macro that takes a string litteral and changes it to an AkOSChar string at compile time
/// \remark This is similar to the TEXT() and _T() macros that can be used to turn string litterals into wchar_t strings
/// \remark Usage: AKTEXT( "Some Text" )
#define AKTEXT(x) L ## x

/// Memory pool attributes.
/// Block allocation type determines the method used to allocate
/// a memory pool. Block management type determines the
/// method used to manage memory blocks. Note that
/// the list of values in this enum is platform-dependent.
/// \sa
/// - AkMemoryMgr::CreatePool()
/// - AK::Comm::DEFAULT_MEMORY_POOL_ATTRIBUTES
enum AkMemPoolAttributes
{
	AkNoAlloc				= 0,	///< CreatePool will not allocate memory.  You need to allocate the buffer yourself.
	AkMalloc				= 1<<0,	///< CreatePool will use AK::AllocHook() to allocate the memory block.
	AkAPUAlloc				= 1<<1,	///< CreatePool will use AK::APUAllocHook() to allocate the memory block.
	AkAllocMask				= AkNoAlloc | AkMalloc | AkAPUAlloc, ///< Block allocation type mask.

	AkFixedSizeBlocksMode	= 1<<3,					///< Block management type: Fixed-size blocks. Get blocks through GetBlock/ReleaseBlock API. If not specified, use AkAlloc/AkFree.
	AkBlockMgmtMask			= AkFixedSizeBlocksMode	///< Block management type mask.
};
#define AK_MEMPOOLATTRIBUTES

#ifdef __cplusplus
	namespace AK
	{   
		/// External allocation hook for the Memory Manager. Called by the Audiokinetic 
		/// implementation of the Memory Manager when creating a pool of type AkAPUAlloc.
		/// \aknote This needs to be defined by the client. \endaknote
		/// \return Virtual address to start of allocated memory (NULL if the system is out of memory).
		/// \sa
		/// - \ref memorymanager
		/// - AK::APUFreeHook()
		extern void * APUAllocHook( 
			size_t in_size,				///< Number of bytes to allocate.
			unsigned int in_alignment	///< Alignment in bytes (must be power of two, greater than or equal to four).
			);
	
		/// External deallocation hook for the Memory Manager. Called by the Audiokinetic 
		/// implementation of the Memory Manager when destroying a pool of type AkAPUAlloc.
		/// \aknote This needs to be defined by the client. \endaknote
		/// \sa 
		/// - \ref memorymanager
		/// - AK::APUAllocHook()
		extern void APUFreeHook( 
			void * in_pMemAddress	///< Virtual address as returned by APUAllocHook.
			);
	}
#endif
