/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#include "stdafx.h" 

#include "AkFXMemAlloc.h"

#include <AK/SoundEngine/Common/AkMemoryMgr.h>

AkFXMemAlloc AkFXMemAlloc::m_instanceUpper;
AkFXMemAlloc AkFXMemAlloc::m_instanceLower;

#if defined (AK_MEMDEBUG)

// debug malloc
void * AkFXMemAlloc::dMalloc( 
    size_t   in_uSize,		// size
    const char*	 in_pszFile,// file name
	AkUInt32 in_uLine		// line number
	)
{
#ifdef AK_PS3
	// FX memory needs to always be 16-byte aligned on PS3
	// Allocation size rounded upwards to nearest 16 byte boundary to avoid roundtrip DMA corruption errors
	return AK::MemoryMgr::dMalign( m_poolId, AK_ALIGN_SIZE_FOR_DMA( in_uSize ), 16, in_pszFile, in_uLine ); 
#else
	return AK::MemoryMgr::dMalign( m_poolId, in_uSize, 16, in_pszFile, in_uLine );
#endif
}

#endif

// release malloc
void * AkFXMemAlloc::Malloc( 
    size_t in_uSize		// size
    )
{
#ifdef AK_PS3
	// FX memory needs to always be 16-byte aligned on PS3
	// Allocation size rounded upwards to nearest 16 byte boundary to avoid roundtrip DMA corruption errors
	return AK::MemoryMgr::Malign( m_poolId, AK_ALIGN_SIZE_FOR_DMA( in_uSize ), 16 ); 
#else
	return AK::MemoryMgr::Malign( m_poolId, in_uSize, 16 );
#endif
}

void AkFXMemAlloc::Free(
    void * in_pMemAddress	// starting address
    )
{
	AK::MemoryMgr::Falign( m_poolId, in_pMemAddress );
}
