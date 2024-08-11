/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef UNITTEST_FRAMEWORK_ATTORNEY_H_
#define UNITTEST_FRAMEWORK_ATTORNEY_H_

class CObject;
class CClass;

namespace Red
{
namespace UnitTest
{
	class Attorney
	{
	public:

		static void BindClass( CObject * object, CClass * correctClass );
	};
}
}

#define RED_DECLARE_UNIT_TEST_CLASS( className ) \
	void* operator new( size_t size ){ void * result = RED_MEMORY_ALLOCATE_ALIGNED( CurrentMemoryPool::Type, CurrentMemoryClass, size, __alignof( className ) ); Red::UnitTest::Attorney::BindClass( static_cast< CObject *>( result ), className::GetStaticClass() ); return result; } \
	void operator delete( void* mem, size_t ) { RED_MEMORY_FREE( CurrentMemoryPool::Type, CurrentMemoryClass, mem ); } \
	virtual ~className() { SetFlag( OF_Finalized ); }

#endif
