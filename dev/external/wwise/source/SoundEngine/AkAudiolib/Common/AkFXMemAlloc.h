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
#ifndef _AK_FX_MEM_ALLOC_H_
#define _AK_FX_MEM_ALLOC_H_

#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>

class AkFXMemAlloc
	: public AK::IAkPluginMemAlloc
{
public:
	inline AkFXMemAlloc() : m_poolId( AK_INVALID_POOL_ID ) {}
	
	inline void SetPoolId( AkMemPoolId in_poolId ) { m_poolId = in_poolId; }

	static inline AkFXMemAlloc * GetUpper() { return &m_instanceUpper; }
	static inline AkFXMemAlloc * GetLower() { return &m_instanceLower; }

	// AK::IAkPluginMemAlloc

	    virtual void * Malloc( 
            size_t in_uSize		// size
            );
	    
        virtual void Free(
            void * in_pMemAddress	// starting address
            );

#if defined (AK_MEMDEBUG)
	    virtual void * dMalloc( 
            size_t in_ulSize,		// size
            const char* in_pszFile,	// file name
		    AkUInt32 in_uLine		// line number
		    );
#endif

private:
	AkMemPoolId m_poolId;

	static AkFXMemAlloc m_instanceUpper;
	static AkFXMemAlloc m_instanceLower;
};
#endif
