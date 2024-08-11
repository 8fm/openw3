#include "build.h"

#define ENABLE_ARRAY_BUFFER_OVERRUN_PROTECTION

#include "../../common/redContainers/array.h"
#include "../../common/redContainers/bufferAllocatorStatic.h"
#include "../../common/redContainers/bufferAllocatorDynamic.h"
#include "../../common/redContainers/sortedArray.h"
#include "../../common/redContainers/pair.h"
#include "../../common/redContainers/arrayMultiMap.h"
#include "../../common/redContainers/bitField.h"
#include "../../common/redContainers/heap.h"
#include "../../common/redContainers/list.h"
#include "../../common/redContainers/doubleList.h"
#include "../../common/redContainers/queue.h"
#include "../../common/redContainers/arraySet.h"
#include "../../common/redMemoryFramework/redMemoryFramework.h"

/////////////////////////////////////////////////////////////////////////////////////////
// Test memory manager for dynamic buffer 
Red::MemoryFramework::MemoryManager g_containerMemory( 0, 0 );
enum ContainerMemPools
{
	Pool_ContainerTests
};
enum ContainerMemoryClasses
{
	Class_Default,
	Class_Ints,
	Class_Floats,
	Class_DynamicArray,
	Class_ListNodes,
};

// Set up test container pools
INTERNAL_RED_MEMORY_BEGIN_POOL_TYPES( ContainerPools )
	INTERNAL_RED_MEMORY_DECLARE_POOL( Pool_ContainerTests, Red::MemoryFramework::TLSFAllocator );
INTERNAL_RED_MEMORY_END_POOL_TYPES

//////////////////////////////////////////////////////////////////////////////////////////
// Static array wrapper
template< class ElementType, Red::System::Uint32 MaximumSize >
class TestStaticArray : public Red::Containers::Array< ElementType, Red::Containers::BufferAllocatorStatic< ElementType, MaximumSize >, 0 >
{
private:
	typedef Red::Containers::Array< ElementType, Red::Containers::BufferAllocatorStatic< ElementType, MaximumSize >, 0 > BaseClass;
public:
	TestStaticArray()	{ }
	explicit TestStaticArray( Red::System::Uint32 initialSize ) : BaseClass( initialSize ) { }
	TestStaticArray( TestStaticArray&& other ) : BaseClass( Red::Containers::ElementMoveConstruct( other ) ) { }
	TestStaticArray( const TestStaticArray& other ) : BaseClass( other ) { }
};

//////////////////////////////////////////////////////////////////////////////////////////
// Dynamic array allocator policy
template< Red::MemoryFramework::PoolLabel MemPool >
class TestDynamicArrayAllocator
{
public:
	static void* Reallocate( void* ptr, Red::System::MemSize size, Red::MemoryFramework::MemoryClass memClass )
	{
		return INTERNAL_RED_MEMORY_REALLOCATE( g_containerMemory, ContainerPools, MemPool, ptr, memClass, size );
	}
};

///////////////////////////////////////////////////////////////////////////////////////////
// Dynamic array wrapper
template< class ElementType, Red::MemoryFramework::MemoryClass MemClass = Class_DynamicArray, Red::MemoryFramework::PoolLabel MemPool = Pool_ContainerTests >
class TestDynamicArray : public Red::Containers::Array< ElementType, Red::Containers::BufferAllocatorDynamic< TestDynamicArrayAllocator< MemPool > >, MemClass >
{
private:
	typedef Red::Containers::Array< ElementType, Red::Containers::BufferAllocatorDynamic< TestDynamicArrayAllocator< MemPool > >, MemClass > BaseClass;
public:
	TestDynamicArray()	{ }
	explicit TestDynamicArray( Red::System::Uint32 initialSize ) : BaseClass( initialSize ) { }
	TestDynamicArray( TestDynamicArray&& other ) : BaseClass( Red::Containers::ElementMoveConstruct( other ) ) { }
	TestDynamicArray( const TestDynamicArray& other ) : BaseClass( other ) { }
};

// Allocator flags
#ifdef RED_PLATFORM_ORBIS
	const Red::System::Uint32 c_ContainersAllocatorFlags = Red::MemoryFramework::Allocator_DirectMemory | 
														   Red::MemoryFramework::Allocator_UseOnionBus | 
														   Red::MemoryFramework::Allocator_AccessCpuReadWrite;
#elif defined( RED_PLATFORM_DURANGO )
	const Red::System::Uint32 c_ContainersAllocatorFlags = Red::MemoryFramework::Allocator_AccessCpu | Red::MemoryFramework::Allocator_Cached | RED_MEMORY_DURANGO_ALLOCID( 1 );
#else
	const Red::System::Uint32 c_ContainersAllocatorFlags = 0;
#endif

///////////////////////////////////////////////////////////////////////////////////////////
// Base array typedef 
typedef Red::Containers::ArrayBase< Red::Containers::BufferAllocatorDynamic< TestDynamicArrayAllocator< Pool_ContainerTests > > > TestDynamicArrayBase;

class ContainersTest : public ::testing::Test
{
private:
	virtual void SetUp()
	{
		INTERNAL_RED_MEMORY_CREATE_POOL( g_containerMemory, 
										ContainerPools, 
										Pool_ContainerTests, 
										Red::MemoryFramework::TLSFAllocator::CreationParameters( 1024 * 1024 * 1, 1024 * 1024 * 16 ), 
										c_ContainersAllocatorFlags );
	}

	virtual void TearDown()
	{
		INTERNAL_RED_MEMORY_DESTROY_POOL( g_containerMemory, ContainerPools, Pool_ContainerTests );
	}
};

TEST_F( ContainersTest, StaticArrayInit )
{
	// Test default ctor
	TestStaticArray< int, 20 > s0;
	EXPECT_TRUE( s0.Size() == 0 );
	EXPECT_TRUE( s0.Capacity() == 20 );
	s0.PushBack( 1 );
	s0.PushBack( 2 );
	s0.PushBack( 3 );
	EXPECT_TRUE( s0.Size() == 3 );

	// test copy ctr
	TestStaticArray< int, 20 > s1( s0 );
	EXPECT_TRUE( s1.Size() == 3 );
	EXPECT_TRUE( s1.Capacity() == 20 );

	// test init with size
	TestStaticArray< int, 20 > s2( 10 );
	EXPECT_TRUE( s2.Size() == 10 );
	EXPECT_TRUE( s2.Capacity() == 20 );
	for(int i=0;i<10;++i)
	{
		s2[i] = i + 1;
	}
}

TEST_F( ContainersTest, StaticArrayPushPop )
{
	TestStaticArray< int, 20 > array0;
	for( int i = 0; i < 10; ++i )
	{
		array0.PushBack( i + 1 );
		EXPECT_TRUE( array0[i] == i + 1 );
	}
	EXPECT_TRUE( array0.Size() == 10 );

	TestStaticArray< int, 10 > array1;
	for( int i = 0; i < 10; ++i )
	{
		array1.PushBack( 10 + i + 1 );
		EXPECT_TRUE( array1[i] == 10 + i + 1 );
	}
	EXPECT_TRUE( array1.Size() == 10 );

	array0.PushBack( array1 );
	EXPECT_TRUE( array0.Size() == 20 );
	for( int i=0; i < 20; ++i )
	{
		EXPECT_TRUE( array0[i] == i + 1 );
	}

	while( array0.Size() > 0 )
	{
		int val = array0.PopBack();
		EXPECT_TRUE( val == (int)array0.Size() + 1 );
	}
}

TEST_F( ContainersTest, StaticSizeManipulation )
{
	TestStaticArray< int, 100 > theArray( 5 );
	EXPECT_TRUE( theArray.Size() == 5 );

	theArray.Reserve( 10 );
	EXPECT_TRUE( theArray.Size() == 5 );
	EXPECT_TRUE( theArray.Capacity() >= 10 );

	theArray.Resize( 2 );
	EXPECT_TRUE( theArray.Size() == 2 );

	theArray.ClearFast();
	EXPECT_TRUE( theArray.Size() == 0 );

	theArray.ResizeFast( 15 );
	EXPECT_TRUE( theArray.Size() == 15 );

	theArray.Clear();
	EXPECT_TRUE( theArray.Size() == 0 );

	theArray.Resize( 20 );
	theArray.Rewind( 15 );
	EXPECT_TRUE( theArray.Size() == 15 );
}

TEST_F( ContainersTest, DynamicArrayInit )
{
	// Test default ctor
	TestDynamicArray< int > s0;
	EXPECT_TRUE( s0.Size() == 0 );
	EXPECT_TRUE( s0.Capacity() == 0 );
	s0.PushBack( 1 );
	s0.PushBack( 2 );
	s0.PushBack( 3 );
	EXPECT_TRUE( s0.Size() == 3 );

	// test copy ctr
	TestDynamicArray< int > s1( s0 );
	EXPECT_TRUE( s1.Size() == 3 );
	EXPECT_TRUE( s0 == s1 );

	// test init with size
	TestDynamicArray< int > s2( 10 );
	EXPECT_TRUE( s2.Size() == 10 );
	for(int i=0;i<10;++i)
	{
		s2[i] = i + 1;
	}

	// test move ctor
	TestDynamicArray< int > s3( Red::Containers::ElementMoveConstruct( s2 ) );
	EXPECT_TRUE( s3.Size() == 10 );
	EXPECT_TRUE( s2.Size() == 0 );

	// Test different mem classes
	TestDynamicArray< int, Class_Ints > intsArray;
	intsArray.PushBack( 5 );
	EXPECT_TRUE( intsArray.Size() == 1 );

	TestDynamicArray< float, Class_Floats > floatsArray;
	floatsArray.PushBack( 3.2f );
	floatsArray.PushBack( 6.4f );
	EXPECT_TRUE( floatsArray.Size() == 2 );
}

TEST_F( ContainersTest, ArrayOps )
{
	TestDynamicArray< int, Class_Ints > dynIntArray0(5);
	for(int i=0;i<5;++i)
		dynIntArray0[i] = i;

	TestDynamicArray< int, Class_DynamicArray > dynIntArray1;
	dynIntArray1.PushBack( dynIntArray0 );
	EXPECT_TRUE( dynIntArray0 == dynIntArray1 );
	EXPECT_TRUE( !( dynIntArray0 != dynIntArray1 ) );

	TestStaticArray< int, 20 > testStatic;
	testStatic.PushBack( dynIntArray0 );
	EXPECT_TRUE( testStatic == dynIntArray0 );

	for(int i=0;i<5;++i)
		testStatic[i] = testStatic[i] + 1;
		
	EXPECT_TRUE( testStatic > dynIntArray0 );
	EXPECT_TRUE( testStatic >= dynIntArray0 );

	EXPECT_TRUE( dynIntArray1 < testStatic );
	EXPECT_TRUE( dynIntArray1 <= testStatic );
}

TEST_F( ContainersTest, Iterators )
{
	typedef TestDynamicArray< float, Class_Floats > TestFloatArray;
	TestFloatArray testArray;
	for(int i = 0; i < 100; ++i )
	{
		testArray.PushBack( i * 2.0f );
	}
		
	int c = 0;
	for( TestFloatArray::iterator it = testArray.Begin();
			it != testArray.End();
			++it, ++c )
	{
		EXPECT_TRUE( ( *it == (c * 2.0f) ) );
	}

	c = 0;
	for( TestFloatArray::const_iterator it = testArray.Begin();
		it != testArray.End();
		++it, ++c )
	{
		EXPECT_TRUE( *it == (c * 2.0f) );
	}

	testArray.Erase( testArray.Begin() );
	EXPECT_TRUE( testArray.Size() == 99 );

	testArray.Erase( testArray.End() - 1 );
	EXPECT_TRUE( testArray.Size() == 98 );
	EXPECT_TRUE( testArray[ testArray.Size() - 1 ] == ( 98 * 2.0f ) );

	float lastValue = testArray[ testArray.Size() - 1 ];
	testArray.EraseFast( testArray.Begin() );
	EXPECT_TRUE( testArray.Size() == 97 );
	EXPECT_TRUE( *testArray.Begin() == lastValue );
}

enum TestArrayType
{
	eArrayStatic,
	eArrayDynamic,
	eArrayStaticReserved,
	eArrayDynamicReserved,
	eArrayStaticCopied,
	eArrayDynamicCopied,
	eArrayMax
};

class ConstructorElementStats
{
public:
	void Reset()
	{
		for( int i=0; i < eArrayMax; ++i )
		{
			s_totalElements[i] = s_totalConstructed[i] = s_totalDestructed[i] = 0;
		}
	}

	static Red::System::Int32 s_totalElements[ eArrayMax ];
	static Red::System::Uint32 s_totalConstructed[ eArrayMax ];
	static Red::System::Uint32 s_totalDestructed[ eArrayMax ];
};
Red::System::Int32 ConstructorElementStats::s_totalElements[ eArrayMax ] = {0};
Red::System::Uint32 ConstructorElementStats::s_totalConstructed[ eArrayMax ] = {0};
Red::System::Uint32 ConstructorElementStats::s_totalDestructed[ eArrayMax ] = {0};

template< TestArrayType arrType >
class ConstructionCounterElement
{
public:
	ConstructionCounterElement()
	{
		ConstructorElementStats::s_totalElements[ arrType ]++;
		ConstructorElementStats::s_totalConstructed[ arrType ]++;
	}

	ConstructionCounterElement( const ConstructionCounterElement& )
	{
		ConstructorElementStats::s_totalElements[ arrType ]++;
		ConstructorElementStats::s_totalConstructed[ arrType ]++;
	}

	ConstructionCounterElement( ConstructionCounterElement&& )
	{
		ConstructorElementStats::s_totalElements[ arrType ]++;
		ConstructorElementStats::s_totalConstructed[ arrType ]++;
	}

	~ConstructionCounterElement()
	{
		ConstructorElementStats::s_totalElements[ arrType ]--;
		ConstructorElementStats::s_totalDestructed[ arrType ]++;
	}
};

TEST_F( ContainersTest, ConstructorDestructor )
{
	////////////////////////////////////////////////////////////////////////////////////////
	// Test adding / removing elements
	TestDynamicArray< ConstructionCounterElement< eArrayDynamic > > dynArray0;
	{
		dynArray0.PushBack( ConstructionCounterElement< eArrayDynamic >() );
		dynArray0.PushBack( ConstructionCounterElement< eArrayDynamic >() );
		dynArray0.PushBack( ConstructionCounterElement< eArrayDynamic >() );
		EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayDynamic ] == 3 );
		dynArray0.Resize(0);
	}
	EXPECT_TRUE( ConstructorElementStats::s_totalConstructed[ eArrayDynamic ] == ConstructorElementStats::s_totalDestructed[ eArrayDynamic ] );

	TestStaticArray< ConstructionCounterElement< eArrayStatic >, 10 > staticArray0;
	{
		staticArray0.PushBack( ConstructionCounterElement< eArrayStatic >() );
		staticArray0.PushBack( ConstructionCounterElement< eArrayStatic >() );
		staticArray0.PushBack( ConstructionCounterElement< eArrayStatic >() );
		EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayStatic ]== 3 );
		staticArray0.Resize(0);
	}
	EXPECT_TRUE( ConstructorElementStats::s_totalConstructed[ eArrayStatic ] == ConstructorElementStats::s_totalDestructed[ eArrayStatic ] );

	////////////////////////////////////////////////////////////////////////////////////////
	// Test reserving elements
	TestDynamicArray< ConstructionCounterElement< eArrayDynamicReserved > > dynArray1( 5 );
	EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayDynamicReserved ] == 5 );

	TestStaticArray< ConstructionCounterElement< eArrayStaticReserved >, 10 > staticArray1( 5 );
	EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayStaticReserved ] == 5 );

	////////////////////////////////////////////////////////////////////////////////////////
	// Test copying elements / arrays
	TestDynamicArray< ConstructionCounterElement< eArrayDynamicCopied > > dynArray2;
	dynArray2.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	dynArray2.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	dynArray2.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	TestDynamicArray< ConstructionCounterElement< eArrayDynamicCopied > > dynArray3;
	dynArray3.PushBack( dynArray2 );
	EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayDynamicCopied ] == 6 );

	TestStaticArray< ConstructionCounterElement< eArrayStaticCopied >, 10 > staticArray2;
	staticArray2.PushBack( ConstructionCounterElement< eArrayStaticCopied >() );
	staticArray2.PushBack( ConstructionCounterElement< eArrayStaticCopied >() );
	staticArray2.PushBack( ConstructionCounterElement< eArrayStaticCopied >() );
	TestStaticArray< ConstructionCounterElement< eArrayStaticCopied >, 15 > staticArray3;
	staticArray3.PushBack( staticArray2 );
	EXPECT_TRUE( ConstructorElementStats::s_totalElements[ eArrayStaticCopied ] == 6 );
}

TEST_F( ContainersTest, WrappedArray )
{
	TestDynamicArray< ConstructionCounterElement< eArrayDynamicCopied > > someArray;
	TestDynamicArrayBase* wrappedArray = (TestDynamicArrayBase*)&someArray;
	someArray.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	someArray.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	someArray.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	someArray.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	someArray.PushBack( ConstructionCounterElement< eArrayDynamicCopied >() );
	RED_UNUSED( wrappedArray );
}

TEST_F( ContainersTest, SortedArray )
{
	class SortIntsLowToHigh
	{
	public:
		bool operator() ( const int& i0, const int& i1 ) const
		{
			return i0 < i1;
		}
	};

	class SortIntsHighToLow
	{
	public:
		bool operator() ( const int& i0, const int& i1 ) const
		{
			return i0 > i1;
		}
	};

	typedef TestDynamicArray< int > TestIntArray;
	Red::Containers::SortedArray< TestIntArray, SortIntsLowToHigh > sortedArrayLowToHigh;
	sortedArrayLowToHigh.Insert( 1 );
	sortedArrayLowToHigh.Insert( 29 );
	sortedArrayLowToHigh.Insert( 4 );
	sortedArrayLowToHigh.Insert( 2 );		
	sortedArrayLowToHigh.Insert( 8 );
	sortedArrayLowToHigh.Insert( 32 );
	sortedArrayLowToHigh.Insert( 8 );
	sortedArrayLowToHigh.Insert( 15 );
	sortedArrayLowToHigh.Insert( 19 );
	sortedArrayLowToHigh.Insert( 4 );
	sortedArrayLowToHigh.Insert( 24 );
	int prevValue = 0;
	for( auto it = sortedArrayLowToHigh.Begin(); it != sortedArrayLowToHigh.End(); ++it )
	{
		EXPECT_TRUE( *it >= prevValue );
		prevValue = *it;
	}
	EXPECT_TRUE( sortedArrayLowToHigh.Find( 8 ) != sortedArrayLowToHigh.End() );
	EXPECT_TRUE( sortedArrayLowToHigh.Find( 35 ) == sortedArrayLowToHigh.End() );

	Red::Containers::SortedArray< TestIntArray, SortIntsHighToLow > sortedArrayHighToLow;
	sortedArrayHighToLow.Insert( 1 );
	sortedArrayHighToLow.Insert( 29 );
	sortedArrayHighToLow.Insert( 4 );
	sortedArrayHighToLow.Insert( 2 );		
	sortedArrayHighToLow.Insert( 8 );
	sortedArrayHighToLow.Insert( 32 );
	sortedArrayHighToLow.Insert( 8 );
	sortedArrayHighToLow.Insert( 15 );
	sortedArrayHighToLow.Insert( 19 );
	sortedArrayHighToLow.Insert( 4 );
	sortedArrayHighToLow.Insert( 24 );
	for( auto it = sortedArrayHighToLow.Begin(); it != sortedArrayHighToLow.End(); ++it )
	{
		if( ( it + 1 ) < sortedArrayHighToLow.End() )
		{
			EXPECT_TRUE( *it >= *( it + 1 ) );
		}
	}
	EXPECT_TRUE( sortedArrayHighToLow.Find( 8 ) != sortedArrayHighToLow.End() );
	EXPECT_TRUE( sortedArrayHighToLow.Find( 35 ) == sortedArrayHighToLow.End() );
}

TEST_F( ContainersTest, ArrayMultiMap )
{
	typedef Red::Containers::Pair< int, bool > ArrayMultiMapTestPair;
	typedef TestStaticArray< ArrayMultiMapTestPair, 15 > ArrayMultiMapTestArray;
	typedef Red::Containers::ArrayMultiMap< ArrayMultiMapTestArray > TestIntBoolMap;
	TestIntBoolMap testMap;
	testMap.Insert( 2, false );
	testMap.Insert( 8, false );
	testMap.Insert( 6, true );
	testMap.Insert( 20, false );
	testMap.Insert( 17, true );
	testMap.Insert( 12, false );
	testMap.Insert( 60, true );
	testMap.Insert( 12, false );
	EXPECT_TRUE( testMap.Size() == 8 );

	auto foundIt = testMap.Find( 12 );
	EXPECT_TRUE( foundIt != testMap.End() );

	auto foundItConst = testMap.Find( 60 );
	EXPECT_TRUE( foundItConst != testMap.End() );

	testMap.Erase( foundIt );
	EXPECT_TRUE( testMap.Size() == 7 );
		
	Red::System::Bool eraseResult = testMap.Erase( 8 );
	EXPECT_TRUE( eraseResult );
	EXPECT_TRUE( testMap.Size() == 6 );
}

TEST_F( ContainersTest, BitFields )
{
	typedef TestDynamicArray< Red::System::Uint32 > TestBitfieldArrayDynamic;
	typedef TestStaticArray< Red::System::Uint16, 32 > TestBitfieldStatic;

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > testBitFieldDynamic( 32 );		// ( 32 * 4 ) / sizeof( uint32 ) = 4 uints
	testBitFieldDynamic.Set( 0, 1 );
	testBitFieldDynamic.Set( 1, 2 );
	testBitFieldDynamic.Set( 2, 3 );
	testBitFieldDynamic.Set( 3, 4 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 0 ) == 1 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 1 ) == 2 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 2 ) == 3 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 3 ) == 4 );

	testBitFieldDynamic.Set( 4, 5 );
	testBitFieldDynamic.Set( 5, 6 );
	testBitFieldDynamic.Set( 6, 7 );
	testBitFieldDynamic.Set( 7, 8 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 4 ) == 5 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 5 ) == 6 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 6 ) == 7 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 7 ) == 8 );

	testBitFieldDynamic.Set( 8, 9);
	testBitFieldDynamic.Set( 9, 10);
	testBitFieldDynamic.Set( 10,11);
	testBitFieldDynamic.Set( 11,12);
	EXPECT_TRUE( testBitFieldDynamic.Get(  8 ) ==  9 );
	EXPECT_TRUE( testBitFieldDynamic.Get(  9 ) ==  10);
	EXPECT_TRUE( testBitFieldDynamic.Get(  10 ) == 11 );
	EXPECT_TRUE( testBitFieldDynamic.Get(  11 ) == 12 );

	testBitFieldDynamic.Set( 12,13 );
	testBitFieldDynamic.Set( 13,14 );
	testBitFieldDynamic.Set( 14,15 );
	testBitFieldDynamic.Set( 15,0 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 12 ) == 13 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 13 ) == 14 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 14 ) == 15 );
	EXPECT_TRUE( testBitFieldDynamic.Get( 15 ) == 0  );

	testBitFieldDynamic.SetAll( 0x5 );
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( testBitFieldDynamic.Get( i ) == 0x5 );
	}

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > nottedField = ~testBitFieldDynamic;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( nottedField.Get( i ) == 0xfa );
	}

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > opTestField( 32 );
	opTestField.SetAll( 0xf8 );
	testBitFieldDynamic.SetAll( 0xaa );

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > andResult = testBitFieldDynamic & opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( andResult.Get( i ) == 0xa8 );
	}

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > orResult = testBitFieldDynamic | opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( orResult.Get( i ) == 0xfa );
	}

	Red::Containers::BitField< TestBitfieldArrayDynamic, 8 > xorResult = testBitFieldDynamic ^ opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( xorResult.Get( i ) == 0x52 );
	}

	testBitFieldDynamic.SetAll( 0x8f );
	opTestField.SetAll( 0x35 );
	testBitFieldDynamic &= opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( testBitFieldDynamic.Get( i ) == 0x5 );
	}

	testBitFieldDynamic.SetAll( 0x95 );
	opTestField.SetAll( 0x64 );
	testBitFieldDynamic |= opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( testBitFieldDynamic.Get( i ) == 0xf5 );
	}

	testBitFieldDynamic.SetAll( 0xe5 );
	opTestField.SetAll( 0xc2 );
	testBitFieldDynamic ^= opTestField;
	for( Red::System::Uint32 i=0;i<32;++i )
	{
		EXPECT_TRUE( testBitFieldDynamic.Get( i ) == 0x27 );
	}
}

template < class Comparator, class Iterator >
Red::System::Bool IsHeapValid(Iterator itBegin,Iterator itEnd)
{
	Comparator compare;

	if (itBegin == itEnd)
		return true;
	Red::System::Uint32 nIndex = 0;
	for (Iterator it = itBegin; ; it++, nIndex++)
	{
		// Check if this node sons are higher by given order
		Iterator it1 = itBegin + (nIndex << 1) + 1;
		if (it1 == itEnd)
			return true;
		if (compare(*it1,*it))
			return false;
		Iterator it2 = itBegin + (nIndex << 1) + 2;
		if (it2 == itEnd)
			return true;
		if (compare(*it2,*it))
			return false;
	}
}

TEST_F( ContainersTest, Heaps )
{
	typedef TestStaticArray< Red::System::Uint32, 32 > TestHeapArrayStatic;
	Red::Containers::Heap< TestHeapArrayStatic > testHeap;
	testHeap.Push( 1 );
	testHeap.Push( 5 );
	testHeap.Push( 16 );
	testHeap.Push( 3 );
	testHeap.Push( 20 );
	testHeap.Push( 8 );
	testHeap.Push( 900 );
	testHeap.Push( 400 );
	testHeap.Push( 400 );
	testHeap.Push( 30 );
	testHeap.Push( 28 );
	testHeap.Push( 100 );
	testHeap.Push( 205 );
	testHeap.Push( 60 );
	EXPECT_TRUE( *testHeap.Begin() == 1 );
	Red::System::Bool heapValid = IsHeapValid< Red::Containers::Heap< TestHeapArrayStatic >::comparator, Red::Containers::Heap< TestHeapArrayStatic >::iterator >( testHeap.Begin(), testHeap.End() );
	EXPECT_TRUE( heapValid );
	auto smallestNode = testHeap.Begin();
	*smallestNode = 1000;		
	testHeap.UpdateElement( smallestNode );
	heapValid = IsHeapValid< Red::Containers::Heap< TestHeapArrayStatic >::comparator, Red::Containers::Heap< TestHeapArrayStatic >::iterator >( testHeap.Begin(), testHeap.End() );
	EXPECT_TRUE( heapValid );

	Red::System::Uint32 poppedValue = 0;
	Red::System::Uint32 lastPoppedValue = 0;
	while( testHeap.Begin() != testHeap.End() )
	{
		poppedValue = testHeap.Pop();
		EXPECT_TRUE( poppedValue >= lastPoppedValue );
		lastPoppedValue = poppedValue;
	}
}

class TestListAllocator
{
public:
	template< class NodeType >
	static NodeType* AllocateNode( )
	{
		void* memory = INTERNAL_RED_MEMORY_ALLOCATE( g_containerMemory, ContainerPools, Pool_ContainerTests, Class_ListNodes, sizeof( NodeType ) );
		return new( memory ) NodeType();
	}

	template< class NodeType >
	static void FreeNode( NodeType* theNode )
	{
		theNode->~NodeType();
		INTERNAL_RED_MEMORY_FREE( g_containerMemory, ContainerPools, Pool_ContainerTests, Class_ListNodes, theNode );
	}
};

TEST_F( ContainersTest, SingleList )
{
	Red::Containers::List< int, TestListAllocator > testIntList;
	testIntList.PushBack( 10 );
	EXPECT_TRUE( *testIntList.Begin() == 10 );

	testIntList.PushFront( 5 );
	EXPECT_TRUE( *testIntList.Begin() == 5 );

	testIntList.PushBack( 20 );
		
	auto it = testIntList.Begin();
	++it;
	it++;
	EXPECT_TRUE( *it == 20 );

	Red::Containers::List< int, TestListAllocator > listCopy = testIntList;
	EXPECT_TRUE( listCopy == testIntList );
	EXPECT_TRUE( *listCopy.Begin() == 5 );
	it = listCopy.Begin();
	++it;
	it++;
	EXPECT_TRUE( *it == 20 );

	EXPECT_TRUE( testIntList.Size() == 3 );
	EXPECT_TRUE( testIntList.DataSize() == 3 * sizeof( int ) );
	EXPECT_TRUE( testIntList.Empty() == false );

	testIntList.PopFront();
	EXPECT_TRUE( *testIntList.Begin() == 10 );
	EXPECT_TRUE( testIntList.Size() == 2 );

	testIntList.PopBack();
	EXPECT_TRUE( *testIntList.Begin() == 10 );
	EXPECT_TRUE( testIntList.Size() == 1 );

	testIntList.PopBack();
	EXPECT_TRUE( testIntList.Size() == 0 );

	auto emptyIt = testIntList.Begin();
	EXPECT_TRUE( emptyIt == testIntList.End() );

	testIntList.PushFront( 2 );
	testIntList.PushFront( 4 );
	testIntList.PushFront( 6 );
	testIntList.PushFront( 8 );
	testIntList.PushFront( 10 );
	auto insertPos = testIntList.Begin();
	testIntList.Insert( insertPos, 1 );
	EXPECT_TRUE( *testIntList.Begin() == 1 );
	++insertPos;
	++insertPos;
	testIntList.Insert( insertPos, 5 );
	auto checkInserted = testIntList.Begin();
	checkInserted++;
	checkInserted++;
	checkInserted++;
	EXPECT_TRUE( *checkInserted == 5 );

	EXPECT_TRUE( testIntList.Exist( 4 ) == true );
	EXPECT_TRUE( testIntList.Exist( 6 ) == true );
	EXPECT_TRUE( testIntList.Exist( 8 ) == true );
	EXPECT_TRUE( testIntList.Remove( 2 ) == true );
	EXPECT_TRUE( testIntList.Remove( 20 ) == false );
	EXPECT_TRUE( testIntList.Exist( 5 ) == true );
	testIntList.Erase( checkInserted );
	EXPECT_TRUE( testIntList.Exist( 5 ) == false );

	testIntList.Clear();
	EXPECT_TRUE( testIntList.Size() == 0 );
	EXPECT_TRUE( testIntList.Begin() == testIntList.End() );

	for( auto itTestLoop = testIntList.Begin(); itTestLoop != testIntList.End(); ++itTestLoop )
	{
	}
}

TEST_F( ContainersTest, DoubleList )
{
	Red::Containers::DoubleList< int, TestListAllocator > testIntList;
	testIntList.PushBack( 10 );
	EXPECT_TRUE( *testIntList.Begin() == 10 );

	testIntList.PushFront( 5 );
	EXPECT_TRUE( *testIntList.Begin() == 5 );

	testIntList.PushBack( 20 );

	auto it = testIntList.Begin();
	++it;
	it++;
	EXPECT_TRUE( *it == 20 );

	Red::Containers::DoubleList< int, TestListAllocator > listCopy = testIntList;
	EXPECT_TRUE( listCopy == testIntList );
	EXPECT_TRUE( *listCopy.Begin() == 5 );
	it = listCopy.Begin();
	++it;
	it++;
	EXPECT_TRUE( *it == 20 );

	EXPECT_TRUE( testIntList.Size() == 3 );
	EXPECT_TRUE( testIntList.DataSize() == 3 * sizeof( int ) );
	EXPECT_TRUE( testIntList.Empty() == false );

	testIntList.PopFront();
	EXPECT_TRUE( *testIntList.Begin() == 10 );
	EXPECT_TRUE( testIntList.Size() == 2 );

	testIntList.PopBack();
	EXPECT_TRUE( *testIntList.Begin() == 10 );
	EXPECT_TRUE( testIntList.Size() == 1 );

	testIntList.PopBack();
	EXPECT_TRUE( testIntList.Size() == 0 );

	auto emptyIt = testIntList.Begin();
	EXPECT_TRUE( emptyIt == testIntList.End() );

	testIntList.PushFront( 2 );
	testIntList.PushFront( 4 );
	testIntList.PushFront( 6 );
	testIntList.PushFront( 8 );
	testIntList.PushFront( 10 );
	auto insertPos = testIntList.Begin();
	testIntList.Insert( insertPos, 1 );
	EXPECT_TRUE( *testIntList.Begin() == 1 );
	++insertPos;
	++insertPos;
	testIntList.Insert( insertPos, 5 );
	auto checkInserted = testIntList.Begin();
	checkInserted++;
	checkInserted++;
	checkInserted++;
	EXPECT_TRUE( *checkInserted == 5 );

	EXPECT_TRUE( testIntList.Exist( 4 ) == true );
	EXPECT_TRUE( testIntList.Exist( 6 ) == true );
	EXPECT_TRUE( testIntList.Exist( 8 ) == true );
	EXPECT_TRUE( testIntList.Remove( 2 ) == true );
	EXPECT_TRUE( testIntList.Remove( 20 ) == false );
	EXPECT_TRUE( testIntList.Exist( 5 ) == true );
	testIntList.Erase( checkInserted );
	EXPECT_TRUE( testIntList.Exist( 5 ) == false );

	auto itTest = testIntList.Begin();
	itTest++;
	itTest++;
	itTest--;
	EXPECT_TRUE( *itTest == 10 );

	testIntList.Clear();
	EXPECT_TRUE( testIntList.Size() == 0 );
	EXPECT_TRUE( testIntList.Begin() == testIntList.End() );

	for( auto itTestLoop = testIntList.Begin(); itTestLoop != testIntList.End(); ++itTestLoop )
	{

	}
}

TEST_F( ContainersTest, Queues )
{
	typedef TestStaticArray< Red::System::Uint32, 32 > TestQueueArray;
	Red::Containers::Queue< TestQueueArray > testQueue;
	testQueue.Push( 1 );
	EXPECT_TRUE( testQueue.Size() == 1 );
	EXPECT_TRUE( testQueue.Capacity() >= 1 );
	EXPECT_TRUE( testQueue.Empty() == false );
	testQueue.Push( 2 );
	testQueue.Push( 3 );
	testQueue.Push( 4 );
	testQueue.Push( 5 );
	EXPECT_TRUE( testQueue.Size() == 5 );
	EXPECT_TRUE( testQueue.Front() == 1 );
	testQueue.Pop();
	EXPECT_TRUE( testQueue.Front() == 2 );
	EXPECT_TRUE( testQueue.Back() == 5 );
	testQueue.Clear();
	EXPECT_TRUE( testQueue.Empty() == true );
	testQueue.Push( 3 );
	testQueue.Push( 7 );
	EXPECT_TRUE( testQueue.Back() == 7 );

	Red::Containers::Queue< TestQueueArray > testCompare;
	testCompare.Push( 3 );
	testCompare.Push( 7 );
	EXPECT_TRUE( testQueue == testCompare );
	testCompare.Pop();
	EXPECT_TRUE( testQueue != testCompare );
}

TEST_F( ContainersTest, ArraySet )
{
	typedef TestStaticArray< Red::System::Uint32, 16 > TestSetArray;
	Red::Containers::ArraySet< TestSetArray > testSet;
	EXPECT_TRUE( testSet.Empty() );
	EXPECT_TRUE( testSet.Size() == 0 );
	testSet.Reserve( 10 );
	testSet.Insert( 1 );
	testSet.Insert( 2 );
	testSet.Insert( 3 );	
	testSet.Insert( 4 );
	testSet.Insert( 5 );
	EXPECT_TRUE( testSet.Size() == 5 );
	EXPECT_TRUE( testSet.Empty() == false );
	EXPECT_TRUE( testSet.Find( 3 ) != testSet.End() );
	EXPECT_TRUE( *testSet.Find( 3 ) == 3 );
	EXPECT_TRUE( testSet.Exist( 2 ) == true );
	EXPECT_TRUE( testSet.Remove( 2 ) );
	EXPECT_TRUE( testSet.Exist( 2 ) == false );
	EXPECT_TRUE( testSet.Insert( 5 ) == false );
	auto it = testSet.Find( 4 );
	testSet.Erase( it );
	EXPECT_TRUE( testSet.Exist( 4 ) == false );

	Red::Containers::ArraySet< TestSetArray > testSet2;
	testSet2.Insert( 1 );
	testSet2.Insert( 3 );	
	testSet2.Insert( 5 );
	EXPECT_TRUE( testSet2 == testSet );
}
