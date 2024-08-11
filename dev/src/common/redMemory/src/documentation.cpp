/**
 * Copyright © 2016 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "../include/redMemoryPublic.h"
#include "../include/utils.h"
#include "../include/fixedSizeAllocator.h"
#include "../include/tlsfAllocator.h"

using namespace red;

//////////////////////////////////////////////////////////////////////////
//
// redMemory DOCUMENTATION, REFERENCE AND HOW-TO
//
// This documentation should always compile.
// If it doesn't compile because of changes in redMemory public interface, please fix code sample and update documentation below.
//
// For more example and use case, please refer yourself to UnitTestMemory project. 
// 
// TABLE OF CONTENT
// 
// 1. RED_NEW and RED_DELETE replacement utility for new and delete operator
//	1.1 Automatic Pool resolution
//  1.2 Allocating from a Pool
//  1.3 Allocating from a specific Allocator
//
// 2. RED_NEW_ARRAY and RED_DELETE_ARRAY replacement utility for new[] and delete[]
//
// 3. RED_ALLOCATE, RED_FREE and RED_REALLOCATE utility replacement for malloc, free and realloc
//  3.1 Allocating from a specific Pool
//  3.2 Allocating from a specific Allocator
//
// 4. Allocators
//	4.1 DefaultAllocator
//	4.2 TLSFAllocator
//	4.3 FixedSizeAllocator
//	4.4 How to create an Allocator compatible with redMemory
//	4.5 Orbis Flexible Memory support
//	4.6 GPU memory support
//
// 5. Pool
//	5.1 How to create a simple Pool
//	5.2 DLL support 
//	5.3 Initializing your pool
//	5.4 OOM handling
//
// 6. Runtime Metrics
//	6.1 Per Pool Metrics
//	6.2 Extended Pool Metrics
//	6.3 Detailed Object Metrics
//
// 7. Debug Utility
//	7.1 Hook system
//	7.1 Memory Stomp Detection
//	7.2 Memory Dump
//
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
// 
// 1. RED_NEW and RED_DELETE replacement utility for new and delete operator
//
// By default, RED_NEW and RED_DELETE will use the DefaultAllocator via the Default Pool. It will correctly ctor/dtor your object.
// They behaves like normal new/delete operator.
// 
// Simple scalar can be allocate/deallocate via RED_NEW,RED_DELETE.
// Like all scalar, ctor parameter is optional. 
// IMPORTANT NOTE memory is not wiped by allocator!
void Sample_1_0_Scalar()
{
	int * scalar = RED_NEW( int );
	RED_DELETE( scalar );
}

//////////////////////////////////////////////////////////////////////////
// Simple pod object are supported. However, pod do not have a ctor. 
// IMPORTANT NOTE memory is not wiped by allocator!
//
void Sample_1_0_POD()
{
	struct POD
	{
		int param1;
		float param2;
	};

	POD * pod = RED_NEW( POD );
	RED_DELETE( pod );
}

//////////////////////////////////////////////////////////////////////////
// Object will see their ctor and dtor called when allocated/deallocated via RED_NEW and RED_DELETE
//
void Sample_1_0_Object()
{
	static bool ctorCalled = false;
	static bool dtorCalled = false;

	struct Object
	{
		Object() { ctorCalled = true; }
		~Object() { dtorCalled = true; }
	};

	Object * object = RED_NEW( Object );
	RED_DELETE( object );
}

//////////////////////////////////////////////////////////////////////////
// RED_NEW support ctor with a variable count of argument using the form RED_NEW( Foo )( param1, param2, ..., paramN );
//
void Sample_1_0_Ctor_with_params()
{
	int * scalar = RED_NEW( int )( 123 );
	
	struct Object
	{
		Object( int , float , bool ) { }
	};

	Object * object = RED_NEW( Object )( 123, 0.123f, true );

	RED_DELETE( scalar );
	RED_DELETE( object );
}

//////////////////////////////////////////////////////////////////////////
// RED_NEW will correctly align allocated object according to it's requirement.
//
void Sample_1_0_Alignment()
{
	struct Object
	{
		RED_ALIGN( 64 ) char content[ 128 ];	// Object alignment requirement is now 64!
	};

	Object * object = RED_NEW( Object );
	RED_FATAL_ASSERT( memory::IsAligned( object, 64 ), "Believe me, your object is correctly aligned!" );
	RED_DELETE( object );
}

//////////////////////////////////////////////////////////////////////////
// RED_DELETE compilation will fail with a static assert if object pointer provided refer to a polymorphic object without a virtual dtor
//
void Sample_1_0_Missing_virtual_dtor()
{
	struct Interface
	{
		virtual void Foo() {}
	};

	struct Object : Interface
	{
		virtual void Foo() override final {}
	};

	Interface * object = RED_NEW( Object );
	//RED_DELETE( object ); // This won't compile. If you don't trust me, try it yourself ! :)
	RED_UNUSED( object );
}

//////////////////////////////////////////////////////////////////////////
// 
// 1.1 Automatic Pool resolution
//
// In previous sample, RED_NEW and RED_DELETE used the Default pool provided by the memory framework.
// It is possible to tag an object with a specific pool to use when RED_NEW/RED_DELETE are used. 
// When tagged, it can't be changed at runtime. However it can be overridden, see section 1.2
//
// For example:
//
void Sample_1_1_Tagged_Object()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );
	
	struct Object
	{
		RED_USE_MEMORY_POOL( SampleMemoryPool );
	};

	Object * object = RED_NEW( Object ); // Will allocate from DefaultAllocator via SampleMemoryPool
	RED_DELETE( object ); // Will deallocate from DefaultAllocator via SampleMemoryPool
}

//////////////////////////////////////////////////////////////////////////
// 
// 1.2 Allocating from a specific Pool
//
// It is possible to provide a pool directly to the RED_NEW/RED_DELETE.
// If the object was tagged, the provided pool will always take precedence.
// 
void Sample_1_2_Specific_Pool_Type_Provided()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );

	int * scalar = RED_NEW( int, SampleMemoryPool )( 123 );
	RED_DELETE( scalar, SampleMemoryPool );

	struct Object
	{
		virtual ~Object() {}
		virtual void Foo() {}
	};

	Object * object = RED_NEW( Object, SampleMemoryPool );
	RED_DELETE( object, SampleMemoryPool );
}

//////////////////////////////////////////////////////////////////////////
// Pool concrete type is not the only way to specify the pool while using RED_NEW/RED_DELETE
// Pool stored as a variable is also valid using the same syntax.
// It is as fast as the previous example.
//
void Sample_1_2_Specific_Pool_Provided()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );
	SampleMemoryPool pool;

	int * scalar = RED_NEW( int, pool )( 123 );
	RED_DELETE( scalar, pool );

	struct Object
	{
		Object() : value( 123 ) {}
		int value;
	};

	Object * object = RED_NEW( Object, pool );
	RED_DELETE( object, pool );
}

//////////////////////////////////////////////////////////////////////////
// It is not always possible to know the actual type of the pool, or keeping the concrete pool variable.
// Fortunately, you can also provide pool interface variable.
//
void Sample_1_2_Pool_Interface_Provided()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );
	
	SampleMemoryPool pool;
	red::memory::Pool & poolInterface = pool;

	int * scalar = RED_NEW( int, poolInterface )( 123 );
	RED_DELETE( scalar, poolInterface );

	struct Object
	{
		Object( int , float , bool ) { }
		RED_ALIGN( 64 ) char content[ 128 ];
	};

	Object * object = RED_NEW( Object, poolInterface )( 0, 0.12f, true );
	RED_DELETE( object, poolInterface );
}

//////////////////////////////////////////////////////////////////////////
//
//  1.3 Allocating from a specific Allocator
//
// In some case, a local allocator is available and is not bound to any pool. 
// Using same syntax than with pool, you can easily use RED_NEW/RED_DELETE. 
// Take note than Hook system will be available but, of course, pool metrics won't.
//
void Sample_1_3_Allocator_Provided()
{
	char localBuffer[ 128 ];
	memory::StaticFixedSizeAllocatorParameter param = { localBuffer, sizeof( localBuffer ), 16, 16 };
	memory::StaticFixedSizeAllocator allocator;
	allocator.Initialize( param );

	float * scalar = RED_NEW( float, allocator )( 0.123f );
	RED_DELETE( scalar, allocator );
	
	struct Interface
	{
		Interface( int value_ ) : value( value_ ) {}
		virtual ~Interface(){}
		int value;
	};

	struct Object : Interface
	{
		Object( int value_ ) : Interface( value_ ) {}
	};

	Interface * object = RED_NEW( Object, allocator )( 123 );
	RED_DELETE( object, allocator );
}

//////////////////////////////////////////////////////////////////////////
//
// 2. RED_NEW_ARRAY and RED_DELETE_ARRAY replacement utility for new[] and delete[]
//
// Like RED_NEW/RED_DELETE replace new and delete operator, RED_NEW_ARRAY and RED_DELETE_ARRAY aim to replace new [] delete [].
// The correct form are: 
// RED_NEW_ARRAY( ObjectType, Count, (optional) Pool/Allocator ) 
// RED_DELETE_ARRAY( ObjectType, Count, (optional) Pool/Allocator )
//
// You will notice than RED_DELETE_ARRAY requires also object counts. 
// The reasoning is that in order to know how much object need their dtor called I need to store the object count in memory block.
// This result in wasted memory and could make alignment compliance waste even more memory.
// To use correctly object allocated via RED_NEW_ARRAY or new[], owner usually keep count locally. 
// Therefor, the owner can provide, in majority of case, the object count.
//
// Ex:
//
void Sample_2_0_Array()
{
	int * scalarArray = RED_NEW_ARRAY( int, 10 );
	RED_DELETE_ARRAY( scalarArray, 10 );

	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );

	struct Object
	{
		Object() : value( true ) {}
		bool value;
	};

	Object * objectArray = RED_NEW_ARRAY( Object, 10, SampleMemoryPool );
	RED_DELETE_ARRAY( objectArray, 10, SampleMemoryPool );
	
	// TAKE NOTE Like previous Sample from RED_NEW/RED_DELETE, you can use RED_NEW_ARRAY and RED_NEW_DELETE with pool variable or allocator 
}

//////////////////////////////////////////////////////////////////////////
// 3. RED_ALLOCATE, RED_FREE and RED_REALLOCATE utility replacement for malloc, free and realloc
// 
// The C allocation operator are also changed with a macro utility.
// However, there are no default version available. You must always provide a pool or allocator. 
// Be careful to make sure to not change pool or proxy between allocation and free. 
// Doing so will either result in assertion (if allocator is different) or metric corruption (if allocator is same).
//
//
// 3.1 Allocating from a specific Pool
// 
// One important note, is that previous replacement operator always had their pool or allocator specified at the end. 
// The reason why it was at the end is to make VisualAssist behave nicely with macro. It was also optional. 
// For RED_ALLOCATE, RED_FREE and RED_REALLOCATE it is always the first argument and is mandatory
//
// Ex:
//
void Sample_3_1_Allocate()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );

	const int bufferSize = 128;
	const int bufferAlignment = 16;

	void * buffer = RED_ALLOCATE( SampleMemoryPool, bufferSize );
	void * bufferAligned = RED_ALLOCATE_ALIGNED( SampleMemoryPool, bufferSize, bufferAlignment );

	RED_FREE( SampleMemoryPool, buffer );
	RED_FREE( SampleMemoryPool, bufferAligned );
}

void Sample_3_1_Reallocate()
{
	RED_MEMORY_POOL_STATIC( SampleMemoryPool, memory::DefaultAllocator );

	// providing nullptr as input memory block is same than using RED_ALLOCATE
	void * buffer = RED_REALLOCATE( SampleMemoryPool, nullptr, 128 ); 
	
	void * reallocatedBuffer = RED_REALLOCATE( SampleMemoryPool, buffer, 256 );

	// providing 0 as size is same than calling RED_FREE with provided input memory block. It will return nullptr
	void * freedBuffer = RED_REALLOCATE( SampleMemoryPool, reallocatedBuffer, 0 ); 
	RED_UNUSED( freedBuffer );
}

//////////////////////////////////////////////////////////////////////////
//  3.2 Allocating from a specific Allocator
//
// Like new operator replacement, you can also provide explicitly the allocator.
//
// Ex:
//
void Sample_3_2_Allocator()
{
	const int allocatorBufferSize = RED_KILO_BYTE( 64 );
	void * allocatorBuffer = RED_ALLOCATE( red::memory::PoolDefault, allocatorBufferSize );
	const red::memory::StaticTLSFAllocatorParameter param = { allocatorBuffer, allocatorBufferSize };
	red::memory::StaticTLSFAllocator allocator;
	allocator.Initialize( param );

	void * buffer = RED_ALLOCATE( allocator, 128 );
	void * reallocBuffer = RED_REALLOCATE( allocator, buffer, 256 );
	RED_FREE( allocator, buffer );
	RED_FREE( allocator, reallocBuffer );
}
