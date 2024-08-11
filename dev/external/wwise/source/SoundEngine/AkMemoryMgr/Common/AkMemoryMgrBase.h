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

//////////////////////////////////////////////////////////////////////
//
// AkMemoryMgrBase.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _MEMORY_MGR_BASE_H_
#define _MEMORY_MGR_BASE_H_

#include <AK/SoundEngine/Common/AkMemoryMgr.h>
#include <AK/Tools/Common/AkListBare.h>

#ifdef AK_WII
#include <AK/Tools/Wii/AkInterruptLock.h>
#define AUTOLOCKPOOL( _pool ) AkAutoInterruptLock lock
#else
#include <AK/Tools/Common/AkAutoLock.h>
#include <AK/Tools/Common/AkLock.h>
#define AUTOLOCKPOOL( _pool ) AkAutoLock<CAkLock> PoolLock( _pool->lock )
#endif

#define FREE_BLOCK_FLAG			0xFBFBFBFB
#define ALLOCATED_BLOCK_FLAG	0xABABABAB

struct AkLinkedBuffer
{
	AkLinkedBuffer * pNextItem;
};
typedef AkListBare<AkLinkedBuffer> BuffersList;

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------
struct AkMemPool
{
	AkMemPool()
	{
		Init();
	}

	void Init();

	void StatsAdd( 
		void* in_pvMemAddress
		, AkUInt32 in_uBlockSize
#ifdef AK_MEMDEBUG
		, const char*szFile = 0
		, AkUInt32	ulLine = 0
#endif
		);
	void StatsSusbtract( void* in_pvMemAddress, AkUInt32 in_uBlockSize );

	AkUInt32		ulNumBlocks;			// number of blocks
	AkUInt32		ulBlockSize;			// how big our blocks are
	char*         pcAllocAddress;			// address used during allocation ( different from pcMemAddress when BlockAlign != 0 )
	char*			pcMemAddress;			// starting address used for block allocation
	BuffersList		listBuffers;			// Fixed-sized blocks pool buffer list.
	AkUInt32/*AkMemPoolAttributes*/ eAttributes;	// pool attribute flags.
	AkUInt32		bAllocated :1;			// has to be freed when done
#ifndef AK_WII
	CAkLock	    	lock;					// pool-level memory lock
#endif
	void			*pTlsfPool;				// points to start of memory pool for tlsf management
	AkUInt32		ulAlign;				// auto-memory alignment
	AkUInt32        ulUsed;					// Current memory total allocation (in bytes)
	AkUInt32		ulTotalAvailable;		// Total Pool actual memory space.

	// Profile info
#ifndef AK_OPTIMIZED

	AkUInt32		ulAllocs;									// Number of Alloc calls since start
	AkUInt32		ulFrees;									// Number of Free calls since start
	AkUInt32        ulPeakUsed;				// Peak memory allocation (in bytes)
	AkOSChar		ptcName[AK_MAX_MEM_POOL_NAME_SIZE];	        // Name of pool
    bool            bDoMonitor;									// Flag for monitoring (do not monitor is false)
#endif

};

//----------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------

namespace AK
{
	namespace MemoryMgr
	{
		extern AKRESULT InitBase( AkInt32 in_iNumPools );

		extern AkInt32		s_iMaxNumPools;
		extern AkInt32		s_iNumPools;
		extern AkMemPool*	s_pMemPools; 

#ifndef AK_OPTIMIZED
		void CheckForMemUsage( AkMemPool* in_pMemPool );

		void CheckPool(AkMemPoolId in_poolId);
		void CheckPointer(AkMemPoolId in_poolId, void * in_pMemAddress);
		
		void CheckAllPools();
#endif
	}
}

#endif // _MEMORY_MGR_BASE_H_

