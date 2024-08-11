#pragma once

/************************************************************************/
/* Red Memory Framework Setup											*/
/************************************************************************/

#include "../redMemoryFramework/redMemoryFramework.h"
#include "singleton.h"
#include <functional>		// For std::function

////////////////////////////////////////////////////////////////////////////
// Debug Allocator Switch
// Enable the define below to route everything to the debug allocator.
// Note, you need a LOT of memory (40gb+)
// #define CORE_USES_DEBUG_ALLOCATOR

////////////////////////////////////////////////////////////////////////////
// Generate memory pool labels enum

#ifndef RED_USE_NEW_MEMORY_SYSTEM

#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	PoolName,
enum EMemoryPoolLabel : Uint16
{
#include "memoryPools.h"
	MemoryPool_Max
};
#undef DECLARE_MEMORY_POOL

#define RED_CONTAINER_POOL_TYPE EMemoryPoolLabel
#define RED_TYPED_CLASS_CAST (EMemoryPoolLabel)

#else

#include "../redMemory/include/redMemoryPublic.h"
#include "redMemoryPools.h"

#define RED_CONTAINER_POOL_TYPE typename
#define RED_TYPED_CLASS_CAST typename

typedef Uint16 EMemoryPoolLabel;

#endif

///////////////////////////////////////////////////////////////////////////
// Generate memory class enum
#define DECLARE_MEMORY_CLASS( ClassName )	ClassName,
enum EMemoryClass : Uint16
{
#include "memoryClasses.h"

	MC_Max
};
#undef DECLARE_MEMORY_CLASS

namespace CoreAllocatorBudgets {
	// Static pool. Used to catch any heap allocations called from static objects / globals
#ifdef RED_ARCH_X64
	const Red::System::MemSize c_StaticHeapSize	= 1024u * 1024u * 16u;
#else
	const Red::System::MemSize c_StaticHeapSize	= 1024u * 1024u * 8u;
#endif

#ifdef RED_PLATFORM_CONSOLE
	// Overflow pool. Used to catch any heap allocations that failed altogether.
	const Red::System::MemSize c_OverflowHeapSize = 1024u * 1024u * 512u;
#else
	const Red::System::MemSize c_OverflowHeapSize = 0;
#endif
	
}

///////////////////////////////////////////////////////////////////////////
// Core Memory Initialisation
//	Should be called as early as possible
namespace CoreMemory
{
	// Helper to add debug strings to memory allocations
	// Creating one of these on the stack will mark all allocations in the same scope with the string provided
	class ScopedMemoryDebugString
	{
	public:
		ScopedMemoryDebugString( const AnsiChar* dbgText );
		~ScopedMemoryDebugString();
	};

	// This is a small utility class used to dump how much memory is allocated in a particular scope
	class CScopedMemoryDump
	{
	public:
		CScopedMemoryDump( const Red::System::Char* titleText, Red::MemoryFramework::PoolLabel pool=-1 );
		~CScopedMemoryDump();

	private:
		void PopulateMetrics( Red::MemoryFramework::RuntimePoolMetrics& metrics );

		Red::MemoryFramework::PoolLabel m_poolId;			// -1 = all pools
		Red::System::Char m_reportTitle[ 256 ];
		Red::MemoryFramework::RuntimePoolMetrics m_startMetrics;
	};

	// This interface is used to pass memory pool parameters (sizes, etc). 
	// MUST be defined per-platform, and params must be set for each pool instantiated
	class CMemoryPoolParameters
	{
	public:
		CMemoryPoolParameters();
		virtual ~CMemoryPoolParameters() { }
		const Red::MemoryFramework::IAllocatorCreationParameters* GetPoolParameters( Red::MemoryFramework::PoolLabel poolLabel );
		void ForEachPoolParameter( std::function< void(Red::MemoryFramework::PoolLabel, const Red::MemoryFramework::IAllocatorCreationParameters*) > callback );

	protected:
		void SetPoolParameters( Red::MemoryFramework::PoolLabel poolLabel, const Red::MemoryFramework::IAllocatorCreationParameters* params );

	private:
		const Red::MemoryFramework::IAllocatorCreationParameters* m_poolCreationParameters[ Red::MemoryFramework::k_MaximumPools ];
	};

	enum EMemoryInitialisationResult
	{
		MemInit_OK,
		MemInit_OutOfMemory
	};
	EMemoryInitialisationResult Initialise( CMemoryPoolParameters* poolParameters );
}


///////////////////////////////////////////////////////////////////////////
// Memory Manager Singleton
// Creation policy used to pass extra parameters to the manager's constructor
template <class MemoryManagerTypename>
class CoreMemoryManagerCreationPolicy : public TCreateStatic< MemoryManagerTypename >
{
public:
	CoreMemoryManagerCreationPolicy();
	static MemoryManagerTypename* Create()
	{
		static typename TCreateStatic< MemoryManagerTypename >::MaxAlign staticT;
		return new(&staticT) MemoryManagerTypename( CoreAllocatorBudgets::c_StaticHeapSize, CoreAllocatorBudgets::c_OverflowHeapSize );
	}
};

// Note this singleton is never destroyed, since on MSVC, the CRT allocates from the manager before the game / editor statics are
// even initialised. As a result, destroying the manager with the rest of the static causes problems when the CRT library shuts down 
// and tries to free its memory (in short, MS CRT calls new() which hits the memory manager)

#ifndef RED_USE_NEW_MEMORY_SYSTEM
typedef TSingleton< Red::MemoryFramework::MemoryManager, TNoDestructionLifetime, CoreMemoryManagerCreationPolicy> SRedMemory;
#endif


#ifndef RED_USE_NEW_MEMORY_SYSTEM
///////////////////////////////////////////////////////////////////////////
// Generate memory pool type bindings
INTERNAL_RED_MEMORY_BEGIN_POOL_TYPES( CoreMemory )
	#define DECLARE_MEMORY_POOL( PoolName, PoolType, Flags )	INTERNAL_RED_MEMORY_DECLARE_POOL( PoolName, PoolType );
	#include "memoryPools.h"
	#undef DECLARE_MEMORY_POOL
INTERNAL_RED_MEMORY_END_POOL_TYPES
#endif

#include "memoryMacros.h"


///////////////////////////////////////////////////////////////////////////
// Default memory alignment calculation
// Try to align data based on the size of the data being allocated when alignment is not specified!
// Ideally we would use C++11's alignof() operator
RED_INLINE size_t CalculateDefaultAlignment( size_t size )
{
	return 8;
}

/************************************************************************/
/* Global operators new and delete                                      */
/************************************************************************/

// Operator delete on Orbis should not throw
#ifdef RED_COMPILER_MSC
#define DELETE_THROW throw()
#elif defined(RED_COMPILER_CLANG)
#define DELETE_THROW _THROW0()
#endif

/************************************************************************/
/* Default memory context												*/
/************************************************************************/

// This is a global state of "current" memory class and memory pool that should be used for allocations
// This is overriden inside classes and therefore can be used
enum CurrentMemoryContext
{
	CurrentMemoryClass = MC_AllObjects
};

#ifndef RED_USE_NEW_MEMORY_SYSTEM
template< EMemoryPoolLabel pool >
struct MemoryPoolType
{
	enum { Type = pool };
};
#else

template< typename PoolType >
struct MemoryPoolType
{
	typedef PoolType Type;
};

#endif

typedef MemoryPoolType< MemoryPool_CObjects >  CurrentMemoryPool;

#define CURRENT_MEMORY_CONTEXT (EMemoryClass)CurrentMemoryClass, CurrentMemoryPool

/************************************************************************/
/* Custom operators new and delete                                      */
/************************************************************************/

#define RED_MAX_CACHE_ALIGNMENT			64

// Default heap alignment for structs is force to 16 bytes for now. This is because the VirtualAllocator used 16 byte aligned blocks
// and too many things rely on that. We will switch this out eventually!
#define RED_DEFAULT_STRUCT_ALIGNMENT	16

// Declare a struct allocated into a specific pool, with any heap allocations aligned to a specific alignment
#define DECLARE_STRUCT_MEMORY_POOL_CONTEXT( heapAlignment )																																				\
	RED_INLINE void* operator new( size_t size )			{ return RED_MEMORY_ALLOCATE_ALIGNED( CurrentMemoryPool::Type, (EMemoryClass)CurrentMemoryClass, size, heapAlignment ); }		\
	RED_INLINE void  operator delete( void* ptr )			{ if ( ptr ) RED_MEMORY_FREE( CurrentMemoryPool::Type, (EMemoryClass)CurrentMemoryClass, ptr ); }								\
	RED_INLINE void* operator new[]( size_t size )			{ return RED_MEMORY_ALLOCATE_ALIGNED( CurrentMemoryPool::Type, (EMemoryClass)CurrentMemoryClass, size, heapAlignment ); }		\
	RED_INLINE void  operator delete[]( void* ptr )			{ if ( ptr ) RED_MEMORY_FREE( CurrentMemoryPool::Type, (EMemoryClass)CurrentMemoryClass, ptr ); }								\
	RED_INLINE void *operator new( size_t, void *ptr )		{ return ptr; }																																\
	RED_INLINE void operator delete( void *, void * )		{}																																			\
	RED_INLINE void *operator new[]( size_t, void *ptr )	{ return ptr; }																																\
	RED_INLINE void operator delete[]( void *, void* )		{}

// Declare a struct allocated into a specific pool, with any heap allocations aligned to default alignment
#define DECLARE_STRUCT_MEMORY_POOL( memoryPool, memoryClass )																				\
	public:																																	\
	enum ClassMemoryContext { CurrentMemoryClass = memoryClass };																			\
	typedef MemoryPoolType< memoryPool > CurrentMemoryPool;																					\
	DECLARE_STRUCT_MEMORY_POOL_CONTEXT( RED_DEFAULT_STRUCT_ALIGNMENT )																		\
	public:

// Declare a class allocated into a specific pool, with any heap allocations aligned to a specific alignment
#define DECLARE_STRUCT_MEMORY_POOL_ALIGNED( memoryPool, memoryClass, heapAlignment)															\
	public:																																	\
	enum ClassMemoryContext { CurrentMemoryClass = memoryClass };											\
	typedef MemoryPoolType< memoryPool > CurrentMemoryPool;																					\
	DECLARE_STRUCT_MEMORY_POOL_CONTEXT( heapAlignment )																						\
	public:

// Declare a class allocated into a specific pool, with any heap allocations aligned to a specific alignment
#define DECLARE_CLASS_MEMORY_POOL_ALIGNED( memoryPool, memoryClass, heapAlignment)															\
	public:																																	\
	enum ClassMemoryContext { CurrentMemoryClass = memoryClass };											\
	typedef MemoryPoolType< memoryPool > CurrentMemoryPool;																					\
	DECLARE_STRUCT_MEMORY_POOL_CONTEXT( heapAlignment )																						\
	private:

// Declare a struct that is allocated into the default pool, using a specific memory class
#define DECLARE_STRUCT_MEMORY_ALLOCATOR( memoryClass )																						\
	DECLARE_STRUCT_MEMORY_POOL( MemoryPool_Default, memoryClass )

// Declare a class allocated into a specific pool, with any heap allocations aligned to default alignment
#define DECLARE_CLASS_MEMORY_POOL( memoryPool, memoryClass )																				\
	public:																																	\
	DECLARE_STRUCT_MEMORY_POOL( memoryPool, memoryClass )																					\
	private:

// Declare a class that is allocated into the default pool, using a specific memory class
#define DECLARE_CLASS_MEMORY_ALLOCATOR( memoryClass )																						\
	public:																																	\
	DECLARE_STRUCT_MEMORY_ALLOCATOR( memoryClass )																							\
	private:
