/********************************************************************
 *                                                                  *
 * THIS FILE IS PART OF THE OggVorbis 'TREMOR' CODEC SOURCE CODE.   *
 *                                                                  *
 * USE, DISTRIBUTION AND REPRODUCTION OF THIS LIBRARY SOURCE IS     *
 * GOVERNED BY A BSD-STYLE SOURCE LICENSE INCLUDED WITH THIS SOURCE *
 * IN 'COPYING'. PLEASE READ THESE TERMS BEFORE DISTRIBUTING.       *
 *                                                                  *
 * THE OggVorbis 'TREMOR' SOURCE CODE IS (C) COPYRIGHT 1994-2002    *
 * BY THE Xiph.Org FOUNDATION http://www.xiph.org/                  *
 *                                                                  *
 ********************************************************************

 function: #ifdef jail to whip a few platforms into the UNIX ideal.

 ********************************************************************/
#ifndef _OS_TYPES_H
#define _OS_TYPES_H

#include <AK/SoundEngine/Common/AkTypes.h>

//#ifdef AK_IOS
//	#define AK_TREMOR_FIXED_POINT
//#endif

#ifdef _LOW_ACCURACY_
	#define X(n) (((((n)>>22)+1)>>1) - ((((n)>>22)+1)>>9))
	#define LOOKUP_T const unsigned char
#else
	#define X(n) (n)
	#ifdef AK_TREMOR_FIXED_POINT
		#define LOOKUP_T const ogg_int32_t
	#else
		#define LOOKUP_T const float
	#endif
#endif

#ifndef __SPU__
// Replace memory allocation with our custom memory allocator
#undef _ogg_malloc
#undef _ogg_calloc
#undef _ogg_realloc
#undef _ogg_free

#include <AK/Tools/Common/AkObject.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkMemoryMgr.h>

// MALLOC
inline static void * _ogg_malloc( unsigned int size )
{
	if ( size != 0 )
	{
		return AkAlloc( g_LEngineDefaultPoolId, size );
	}
	else
	{
		return NULL;
	}
}

// FREE
inline static void _ogg_free( void * ptr )
{
	if ( ptr )
	{
		AkFree( g_LEngineDefaultPoolId, ptr );
		ptr = NULL;
	}
}

#endif

typedef AkInt16 ogg_int16_t;
typedef AkUInt16 ogg_uint16_t;
typedef AkInt32 ogg_int32_t;
typedef AkUInt32 ogg_uint32_t;
typedef AkInt64 ogg_int64_t;

#endif  /* _OS_TYPES_H */
