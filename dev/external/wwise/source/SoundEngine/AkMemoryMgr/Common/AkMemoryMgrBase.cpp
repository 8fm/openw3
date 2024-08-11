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
// AkMemoryMgrBase.cpp
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h" 

#include "AkMemoryMgrBase.h"
#include "AkMonitor.h"

// Header to the actual memory manager
#include "tlsf/tlsf.h"

#if defined (AK_MEMDEBUG)
#include "AkMemTracker.h"

AkMemTracker g_memTracker;
#endif

#ifdef _DEBUG
#define MSTC_STRESS_MEMORY
#endif

#ifdef MSTC_STRESS_MEMORY
////////////////////////////////////////////////////////////////
// Special memory error detection stress mode.
////////////////////////////////////////////////////////////////
#include "AkRandom.h"
static int g_MSTC_GraceDelay = 4000;
static int g_MSTC_AllocCounter = 0;
static unsigned int g_MSTC_Fail_chances = 100; // 100 means 1 %;  1 means 100%.
static unsigned int g_MSTC_SequenceFailMin = 1; //must be greater than zero.
static unsigned int g_MSTC_SequenceFailMax = 1; //must be greater of equal than g_MSTC_SequenceFailMin.
static int g_MSTC_SubsequentFailure = 0; // The randomly definees in bounds number of serial failures.
static bool g_MSTS_Activated = false;
////////////////////////////////////////////////////////////////

#endif //MSTC_STRESS_MEMORY

void Parametrize_MSTC( bool in_bActivate, int in_iGraceDelay, int in_iFailChances, int in_iSeqMin, int in_iSeqMax );
void Parametrize_MSTC( bool in_bActivate, int in_iGraceDelay, int in_iFailChances, int in_iSeqMin, int in_iSeqMax )
{
#ifdef MSTC_STRESS_MEMORY
	g_MSTS_Activated = false;
	if( in_iFailChances <= 0 )
		return;
	
	g_MSTC_Fail_chances = in_iFailChances;
	g_MSTC_GraceDelay = in_iGraceDelay;

	if ( in_iSeqMin <= 0 )
		in_iSeqMin = 1;//cannot be zero or negative.

	if( in_iSeqMax < in_iSeqMin )
		in_iSeqMax = in_iSeqMin;
		
	g_MSTC_SequenceFailMin = in_iSeqMin;
	g_MSTC_SequenceFailMax = in_iSeqMax;

	//must also reset the counters:
	g_MSTC_AllocCounter = 0;
	g_MSTC_SubsequentFailure = 0;
	g_MSTS_Activated = in_bActivate;
#endif
}

void AkMemPool::Init()
{
	ulNumBlocks		= 0;
	ulBlockSize		= 0;
	pcAllocAddress	= NULL;
	pcMemAddress	= NULL;
	eAttributes		= AkMalloc;
	pTlsfPool		= NULL;
	ulAlign			= 0;
	ulUsed			= 0;
	ulTotalAvailable= 0;

#ifndef AK_OPTIMIZED
	ulAllocs		= 0;
	ulFrees			= 0;
	ulPeakUsed		 = 0;
	ptcName[0]		= 0;
	bDoMonitor		= true;
#endif
}

void AkMemPool::StatsAdd( 
			void* in_pvMemAddress
			, AkUInt32 in_uBlockSize
#ifdef AK_MEMDEBUG
			, const char * pszFile
			, AkUInt32 ulLine
#endif
			)
{
	ulUsed += in_uBlockSize;
#ifndef AK_OPTIMIZED
	if(ulPeakUsed < ulUsed)
	{
		ulPeakUsed = ulUsed;
	}
	++ulAllocs;
#ifdef AK_MEMDEBUG
	g_memTracker.Add( in_pvMemAddress, in_uBlockSize, pszFile, ulLine );
#endif

#endif //AK_OPTIMIZED
}

void AkMemPool::StatsSusbtract( void* in_pvMemAddress, AkUInt32 in_uBlockSize )
{
	ulUsed -= in_uBlockSize;

#ifndef AK_OPTIMIZED
	// Update statistics here
	++ulFrees;
#ifdef AK_MEMDEBUG
	g_memTracker.Remove( in_pvMemAddress );
#endif
#endif
}

namespace AK
{

namespace MemoryMgr
{

static AkUInt32 GetPointerMemoryFootprint( void* in_ptr )
{
	return (AkUInt32)( tlsf_block_size(in_ptr) + sizeof(size_t) );
}

#ifndef AK_OPTIMIZED
static void MonitorOutOfMemory(AkOSChar * in_tszPoolName, AkIntPtr in_iFailedAllocationSize)
{
	AkOSChar wszBuffer[ AK_MAX_STRING_SIZE ];

#ifdef AK_OS_WCHAR
	static const AkOSChar* format = L"Insufficient memory in pool: %ls. Attempted alloc size: %u bytes.";
#else
	static const AkOSChar* format = "Insufficient memory in pool: %s. Attempted alloc size: %u bytes.";
#endif
	
	AK_OSPRINTF( wszBuffer, AK_MAX_STRING_SIZE, format, in_tszPoolName, (unsigned int) in_iFailedAllocationSize );

	MONITOR_ERRORMSG( wszBuffer );
}
#endif

#ifdef MSTC_STRESS_MEMORY

static bool MSTC_MustFail( AkMemPool * in_pMemPool, size_t in_ulSize ) // return true if allocation is supposed to fail.
{
	//We first let the grace delay before starting the memory issues, allowing the system to properly init.
	if( g_MSTS_Activated && g_MSTC_AllocCounter > g_MSTC_GraceDelay )
	{
		if( g_MSTC_SubsequentFailure )
		{
			--g_MSTC_SubsequentFailure;

#ifndef AK_OPTIMIZED
			if ( in_pMemPool->bDoMonitor )
			{
				MonitorOutOfMemory( in_pMemPool->ptcName, in_ulSize );
			}
#endif
			return true;
		}
		else if( (AKRANDOM::AkRandom() % g_MSTC_Fail_chances) == 0 )
		{
			AKASSERT( g_MSTC_SequenceFailMin != 0 );
			g_MSTC_SubsequentFailure = g_MSTC_SequenceFailMin - 1;
			if( g_MSTC_SequenceFailMax != g_MSTC_SequenceFailMin )
			{
				g_MSTC_SubsequentFailure += (AkUInt16)((AKRANDOM::AkRandom() % (g_MSTC_SequenceFailMax - g_MSTC_SequenceFailMin) ));
			}
#ifndef AK_OPTIMIZED
			if ( in_pMemPool->bDoMonitor )
			{
				MonitorOutOfMemory( in_pMemPool->ptcName, in_ulSize );
			}
#endif
			return true;
		}
	}
	return false;
}

#define MSTC_HANDLE( _pool, _size ) if ( MSTC_MustFail( _pool, _size ) ) return NULL

#else

#define MSTC_HANDLE( _pool, _size ) 

#endif //MSTC_STRESS_MEMORY

// "Protected"

AkInt32 s_iMaxNumPools = 0;		// how many pools we can manage at most
AkInt32 s_iNumPools = 0;		// how many pools we have
AkMemPool* s_pMemPools = NULL;	// the pools we manage

// Privates

static bool s_bInitialized = false;

// used for getting information about a given tlsf pool
struct tlsf_pool_info
{
	AkUInt32 uMaxFreeBlock;
};

// function that walks the tlsf pool for profiling
static void tlsf_pool_walker_profiling(void*, size_t size, int used, void* user)
{
	tlsf_pool_info *info = (tlsf_pool_info *)user;
	
	if( !used && size > info->uMaxFreeBlock )
	{
		info->uMaxFreeBlock = (AkUInt32)size;
	}
}

//////////////////////////////////////////////////////////////////////////////////
// MEMORY MANAGER BASE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

AKRESULT InitBase( AkInt32 in_iNumPools )
{
	// create the pool ids array
	s_pMemPools = (AkMemPool *) AK::AllocHook( sizeof( AkMemPool ) * in_iNumPools );

	if(s_pMemPools == NULL)
	{
		return AK_Fail;
	}

	for ( AkInt32 lPool = 0; lPool < in_iNumPools; lPool++ )
	{
		AkPlacementNew( s_pMemPools + lPool ) AkMemPool; 
	}

	// set the max number of pools
	s_iMaxNumPools = in_iNumPools;

	// none created for now
	s_iNumPools = 0;

	s_bInitialized = true;

	return AK_Success;
}

//////////////////////////////////////////////////////////////////////////////////
// MAIN PUBLIC INTERFACE IMPLEMENTATION
//////////////////////////////////////////////////////////////////////////////////

bool IsInitialized()
{
	return s_bInitialized;
}

//====================================================================================================
//====================================================================================================
void Term()
{
	if ( IsInitialized() )
	{
		// scan all pools
		for(AkInt32 lPoolCounter = 0 ; lPoolCounter < s_iMaxNumPools ; ++lPoolCounter)
		{
			AkMemPool & pool = s_pMemPools[ lPoolCounter ];

			// have we got blocks hanging out there ?
			if( pool.ulNumBlocks != 0 )
			{
				DestroyPool( lPoolCounter );
			}
			
			pool.~AkMemPool();
		}

		AK::FreeHook(s_pMemPools);

		s_iMaxNumPools = 0;
		s_iNumPools = 0;
		s_pMemPools = NULL;

		s_bInitialized = false;
	}
}
//====================================================================================================
//====================================================================================================
AKRESULT SetPoolName(
	AkMemPoolId     in_PoolId,
	const AkOSChar*	in_tcsPoolName
)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

#ifndef AK_OPTIMIZED
	size_t stringsize = OsStrLen( in_tcsPoolName );
	if( stringsize > AK_MAX_MEM_POOL_NAME_SIZE - 1 )
		stringsize = AK_MAX_MEM_POOL_NAME_SIZE - 1;
	memcpy( s_pMemPools[ in_PoolId ].ptcName, in_tcsPoolName, stringsize * sizeof( AkOSChar ) );
	s_pMemPools[ in_PoolId ].ptcName[ stringsize ] = 0;

	AkMonitor::Monitor_SetPoolName( in_PoolId, s_pMemPools[ in_PoolId ].ptcName );
#endif

	return AK_Success;
}

#if defined(AK_SUPPORT_WCHAR) && defined(AK_OS_WCHAR)
AKRESULT SetPoolName(
	AkMemPoolId     in_PoolId,
	const char*		in_tcsPoolName
)
{
	AKRESULT eResult = AK_Success;
	AkOSChar wideString[ AK_MAX_PATH ];	
	if(AkCharToWideChar( in_tcsPoolName, AK_MAX_PATH, wideString ) > 0)
	{
		eResult = SetPoolName(in_PoolId, wideString);
	}

	return eResult;
}
#else
AKRESULT SetPoolName(
	AkMemPoolId     in_PoolId,
	const wchar_t*		in_tcsPoolName
)
{
	AKRESULT eResult = AK_Success;
	char string[ AK_MAX_PATH ];	
	if(AkWideCharToChar( in_tcsPoolName, AK_MAX_PATH, string ) > 0)
	{
		eResult = SetPoolName(in_PoolId, string);
	}

	return eResult;
}

#endif //AK_SUPPORT_WCHAR
//====================================================================================================
//====================================================================================================
AkOSChar * GetPoolName(
	AkMemPoolId     in_PoolId
)
{
#ifndef AK_OPTIMIZED
	if ( s_pMemPools[ in_PoolId ].ulNumBlocks )
		return s_pMemPools[ in_PoolId ].ptcName;
	else
		return NULL;
#else
	return NULL;
#endif
}

//====================================================================================================
//====================================================================================================
AkMemPoolAttributes GetPoolAttributes(
	AkMemPoolId		in_poolId			///< ID of memory pool to test
	)
{
	return (AkMemPoolAttributes)s_pMemPools[ in_poolId ].eAttributes;
}

//====================================================================================================
//====================================================================================================
AKRESULT SetMonitoring(
    AkMemPoolId     in_poolId,			// ID of memory pool
    bool            in_bDoMonitor       // enables error monitoring (has no effect in AK_OPTIMIZED build)
    )
{
#ifndef AK_OPTIMIZED
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif
    // get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;
    pMemPool->bDoMonitor = in_bDoMonitor;
#endif
    return AK_Success;
}
//====================================================================================================
//====================================================================================================
#if defined (AK_MEMDEBUG)
void * dMalloc(AkMemPoolId	in_PoolId,
			   size_t	in_ulSize,
			   const char*pszFile,
			   AkUInt32	ulLine)
#else
void * Malloc(AkMemPoolId in_PoolId, 
			  size_t    in_ulSize)
#endif
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

	if(in_ulSize == 0)
		return NULL;

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;

	MSTC_HANDLE( pMemPool, in_ulSize );

	char*	pcMemAddress;

	{
		// lock the pool from this point on
		AUTOLOCKPOOL( pMemPool );

		AKASSERT( pMemPool->pTlsfPool );

		if(pMemPool->ulAlign > 4 )
		{
			pcMemAddress = (char*)tlsf_memalign( pMemPool->pTlsfPool, pMemPool->ulAlign, in_ulSize );			
		}
		else
		{
			pcMemAddress = (char*)tlsf_malloc( pMemPool->pTlsfPool, in_ulSize );
		}

		if( pcMemAddress )
		{
			pMemPool->StatsAdd( pcMemAddress, GetPointerMemoryFootprint(pcMemAddress)
#ifdef AK_MEMDEBUG
			, pszFile , ulLine
#endif
			);
		}
	}

#ifndef AK_OPTIMIZED
	if ( !pcMemAddress && pMemPool->bDoMonitor )
		MonitorOutOfMemory(pMemPool->ptcName, in_ulSize); // do this outside of autolock
#endif

#ifdef MSTC_STRESS_MEMORY
	++g_MSTC_AllocCounter;
#endif

	return pcMemAddress;
}

#if defined (AK_MEMDEBUG) // so that non-memdebug libs still link properly
void * Malloc( AkMemPoolId in_PoolId, size_t in_ulSize )
{
	return dMalloc( in_PoolId, in_ulSize, NULL, 0 );
}
#endif

//On XBOX, XMA buffers need to be aligned to 2k boundaries.  TLSF doesn't behave well with 
//such a big alignment requirement.  We use a different allocation algorithm for this case.
//This defines what is the threshold to use the alternate algorithm.
#define TLSF_BIG_ALIGNMENT 1024

//====================================================================================================
//====================================================================================================

#if defined (AK_MEMDEBUG)
void * dMalign(AkMemPoolId	in_poolId,
			   size_t	in_uSize,
			   AkUInt32	in_uAlignment,
			   const char*	pszFile,
			   AkUInt32	ulLine)
#else
void * Malign(AkMemPoolId in_poolId,			
			  size_t		in_uSize, 			
			  AkUInt32	in_uAlignment)			
#endif
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif

	if(in_uSize == 0)
	{
		AKASSERT(false);
		return NULL;
	}

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;

	MSTC_HANDLE( pMemPool, in_uSize );

	char* pcMemAddress;

	{
		// lock the pool from this point on
		AUTOLOCKPOOL( pMemPool );

		AKASSERT( pMemPool->pTlsfPool );

// Not used
#if 0
		//On XBOX, XMA buffers need to be aligned to 2k boundaries.  TLSF doesn't behave well with 
		//such a big alignment requirement.  We use a different allocation algorithm for this case.
		if (in_uAlignment >= TLSF_BIG_ALIGNMENT)
			pcMemAddress = (char*)tlsf_AllocBigAlignment(pMemPool->pTlsfPool, in_uAlignment, in_uSize);
		else
			pcMemAddress = (char*)tlsf_memalign(pMemPool->pTlsfPool, in_uAlignment, in_uSize);
#endif
		pcMemAddress = (char*)tlsf_memalign(pMemPool->pTlsfPool, in_uAlignment, in_uSize);

		if( pcMemAddress )
			pMemPool->StatsAdd( pcMemAddress, GetPointerMemoryFootprint(pcMemAddress) 
#ifdef AK_MEMDEBUG
			, pszFile , ulLine
#endif
			);
	}
	
#ifndef AK_OPTIMIZED
	if ( !pcMemAddress && pMemPool->bDoMonitor )
		MonitorOutOfMemory(pMemPool->ptcName, in_uSize); // do this outside of autolock
#endif

#ifdef MSTC_STRESS_MEMORY
	++g_MSTC_AllocCounter;
#endif

	return pcMemAddress;
}

#if defined (AK_MEMDEBUG)
void * Malign( AkMemPoolId in_poolId, size_t in_uSize, AkUInt32 in_uAlignment )
{
	return dMalign( in_poolId, in_uSize, in_uAlignment, NULL, 0 );
}
#endif

AKRESULT Falign(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
	return Free( in_PoolId, in_pvMemAddress ); // in this implementation, same as free
}

//====================================================================================================
//====================================================================================================
AKRESULT Free(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
	AKASSERT( in_pvMemAddress );
	if( in_pvMemAddress )
	{
#if defined (_DEBUG)
		AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

		// get a pointer to our pool
		AkMemPool* pMemPool = s_pMemPools + in_PoolId;

		// lock the pool from this point on
		AUTOLOCKPOOL( pMemPool );

		pMemPool->StatsSusbtract( in_pvMemAddress, GetPointerMemoryFootprint(in_pvMemAddress) );

		// Optimzation for one block in pool
		tlsf_free(pMemPool->pTlsfPool, in_pvMemAddress);
	}
	
	return AK_Success;
}

//====================================================================================================
//====================================================================================================

AKSOUNDENGINE_API void * GetBlock(
	AkMemPoolId in_poolId				///< ID of the memory pool
	)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;
	
	void * pBuffer = pMemPool->listBuffers.First();
	if ( pBuffer )
	{
		pMemPool->listBuffers.RemoveFirst();
		pMemPool->StatsAdd( pMemPool->pcMemAddress, pMemPool->ulBlockSize 
#ifdef AK_MEMDEBUG
			, "AK::MemoryMgr::GetBlock", 0
#endif 
			);
	}

	return pBuffer;
}

//====================================================================================================
//====================================================================================================
AKRESULT ReleaseBlock(AkMemPoolId in_PoolId,void* in_pvMemAddress)
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_PoolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool* pMemPool = s_pMemPools + in_PoolId;

	pMemPool->StatsSusbtract( in_pvMemAddress, pMemPool->ulBlockSize );

	AKASSERT( in_pvMemAddress );
	pMemPool->listBuffers.AddLast( reinterpret_cast<AkLinkedBuffer*>( in_pvMemAddress ) );
	
	return AK_Success;
}

//====================================================================================================
//====================================================================================================

AkUInt32 GetBlockSize( AkMemPoolId in_poolId )
{
#if defined (_DEBUG)
	AKASSERT( CheckPoolId(in_poolId) == AK_Success );
#endif

	// get a pointer to our pool
	AkMemPool* pMemPool = s_pMemPools + in_poolId;

	return pMemPool->ulBlockSize;
}

//====================================================================================================
//====================================================================================================
AkInt32 GetNumPools()
{
	return s_iNumPools;
}

//====================================================================================================
//====================================================================================================
AkInt32 GetMaxPools()
{
	return s_iMaxNumPools;
}
//====================================================================================================
// look for this id in the manager's list
//====================================================================================================
AKRESULT CheckPoolId(AkMemPoolId in_PoolId)
{
	// has this pool been created ?
	if((in_PoolId < s_iMaxNumPools)
		&& ((s_pMemPools + in_PoolId)->ulNumBlocks != 0))
	{
		// yes, we know this one
		return AK_Success;
	}

	return AK_InvalidID;
}

//====================================================================================================
//====================================================================================================
AKRESULT GetPoolStats(
	AkMemPoolId     in_PoolId,
	PoolStats&      out_stats
	)
{
	if ( in_PoolId >= s_iMaxNumPools || in_PoolId < 0 )
		return AK_Fail;

	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;
	AUTOLOCKPOOL( pMemPool );

	out_stats.uReserved = pMemPool->ulTotalAvailable;
	out_stats.uUsed     = pMemPool->ulUsed;

#ifndef AK_OPTIMIZED

	out_stats.uAllocs   = pMemPool->ulAllocs;
	out_stats.uFrees    = pMemPool->ulFrees;
	
	// Walk the heap (if the pool exists)
	tlsf_pool_info info;

	info.uMaxFreeBlock = 0;
	
	if(pMemPool->pTlsfPool)
	{
		tlsf_walk_heap(pMemPool->pTlsfPool, tlsf_pool_walker_profiling, &info);
	}
	else
	{
		info.uMaxFreeBlock = ( !pMemPool->listBuffers.IsEmpty() ) ? pMemPool->ulBlockSize : 0;
	}

	out_stats.uMaxFreeBlock = info.uMaxFreeBlock;
	out_stats.uPeakUsed = pMemPool->ulPeakUsed;

#else

	out_stats.uAllocs       = 0;
	out_stats.uFrees        = 0;
	out_stats.uMaxFreeBlock = 0;

#endif // AK_OPTIMZED
	
	return AK_Success;
}

void GetPoolMemoryUsed(
			AkMemPoolId     in_poolId,			///< ID of memory pool
			PoolMemInfo&    out_memInfo			///< Returned statistics structure
		    )
{
	// Must be implemented even in release.
	AkMemPool*	pMemPool = s_pMemPools + in_poolId;
	AUTOLOCKPOOL( pMemPool );

	out_memInfo.uUsed = pMemPool->ulUsed;
	out_memInfo.uReserved = pMemPool->ulTotalAvailable;
}

#ifndef AK_OPTIMIZED

// function that walks the tlsf pool for checking memory
static void tlsf_pool_walker_check(void* ptr, size_t, int used, void*)
{
#if defined (AK_MEMDEBUG)
	if ( used )
	{
		g_memTracker.PrintMemoryLabel(ptr);
	}
#else
	AKASSERT( used == false && "Memory leak detected" );
	// dummy method, used to walk the heap and detect inconsistencies (by crashing).

#endif
}

// Finds memory leaks
void CheckForMemUsage(AkMemPool* in_pMemPool)
{
	if( in_pMemPool )
	{
		if ( in_pMemPool->pTlsfPool )
			tlsf_walk_heap( in_pMemPool->pTlsfPool, tlsf_pool_walker_check, NULL );
		
		AKASSERT( in_pMemPool->ulAllocs == in_pMemPool->ulFrees );
		if ( in_pMemPool->ulAllocs != in_pMemPool->ulFrees )
		{
			MONITOR_ERRORMSG2( AKTEXT("Memory leak in pool: "), in_pMemPool->ptcName );
		}
	}
}

// check the integrity of a pool
void CheckPool(AkMemPoolId in_PoolId)
{
	if ( CheckPoolId( in_PoolId ) != AK_Success )
		return;

	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;
	AUTOLOCKPOOL( pMemPool );

	if( pMemPool->pTlsfPool )
	{
		// tlsf_check_heap() Returns nonzero if heap check fails.
		
		if (tlsf_check_heap(pMemPool->pTlsfPool))
		{
			AKASSERT( false );
			MONITOR_ERRORMSG2( AKTEXT("Corruption in pool: "), AK::MemoryMgr::GetPoolName( in_PoolId ) );
		}
	}
}

// check the integrity of a pointer
void CheckPointer(AkMemPoolId in_PoolId, void * in_pMemAddress)
{
	if(CheckPoolId( in_PoolId ) != AK_Success)
		return;

	AkMemPool*	pMemPool = s_pMemPools + in_PoolId;
	AUTOLOCKPOOL( pMemPool );

	if(pMemPool->pTlsfPool)
	{
		// tlsf_check_ptr() Returns nonzero if heap check fails.
		
		if (tlsf_check_ptr(pMemPool->pTlsfPool, in_pMemAddress))
		{
			AKASSERT( false );
			MONITOR_ERRORMSG2( AKTEXT("Corruption in pool: "), AK::MemoryMgr::GetPoolName( in_PoolId ) );
		}
	}
}

// check the integrity of all pools
void CheckAllPools()
{
	AkUInt32 ulNumPools = AK::MemoryMgr::GetMaxPools();

	for ( AkUInt32 ulPool = 0; ulPool < ulNumPools; ++ulPool )
	{
		CheckPool( ulPool );
	}
}

#endif // AK_OPTIMIZED

} // namespace MemoryMgr

} // namespace AK
