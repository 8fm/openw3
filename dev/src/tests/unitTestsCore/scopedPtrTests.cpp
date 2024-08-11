/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/scopedPtr.h"

namespace Red
{
namespace
{
	class MyClass
	{
	public:
		void SetValue( Int32 value ) { m_value = value; }
		Int32 GetValue() const { return m_value; }

	private:

		Int32 m_value;
	};

	template< typename Type >
	class MyClassDestructionPolicy
	{
	public:
		void operator()(MyClass * myClass) const
		{
			if (myClass)
			{
				myClass->SetValue(0);
			}
		}
	};

	void CustomDeleteFunction( MyClass * myClass )
	{
		if (myClass)
		{
			myClass->SetValue(0);
		}
	}
}
	
	TEST( TScopedPtr, can_access_raw_pointer_via_arrow_operator )
	{
		MyClass * ptr = new MyClass();
		TScopedPtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, myTest->GetValue() );
	}

	TEST( TScopedPtr, can_access_reference_dereference_operator )
	{
		MyClass * ptr = new MyClass();
		TScopedPtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, (*myTest).GetValue() );
	}

	TEST( TScopedPtr, can_access_raw_pointer_via_Get )
	{
		MyClass * ptr = new MyClass();
		TScopedPtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, myTest.Get()->GetValue() );
	}

	TEST( TScopedPtr, bool_opetator_return_false_by_default )
	{
		TScopedPtr< Int32 > myTest;

		EXPECT_FALSE( myTest );
	}

	TEST( TScopedPtr, bool_opetator_return_true_when_pointee_is_valid )
	{
		TScopedPtr< Int32 > myTest( new Int32 );

		EXPECT_TRUE( myTest );
	}

	TEST( TScopedPtr, not_opetator_return_true_by_default )
	{
		TScopedPtr< Int32 > myTest;

		EXPECT_TRUE( !myTest );
	}

	TEST( TScopedPtr, not_opetator_return_false_when_pointee_is_valid )
	{
		TScopedPtr< Int32 > myTest( new Int32 );

		EXPECT_FALSE( !myTest );
	}

	TEST( TScopedPtr, Swap_swap_internal_pointer )
	{
		TScopedPtr< Int32 > myFirst( new Int32() );
		TScopedPtr< Int32 > mySecond( new Int32() );

		*myFirst = 1;
		*mySecond = 2;

		myFirst.Swap( mySecond );

		EXPECT_EQ( 2, *myFirst );
		EXPECT_EQ( 1, *mySecond );
	}

	TEST( TScopedPtr, Reset_delete_current_pointer )
	{
		TScopedPtr< Int32 > myTest( new Int32() );
		myTest.Reset();
		EXPECT_EQ( myTest.Get(), nullptr );
	}

	TEST( TScopedPtr, Reset_delete_current_pointer_and_swap_with_provided_pointer )
	{
		Int32 * value = new Int32( 10 );
		TScopedPtr< Int32 > myTest( new Int32( 5 ) );
		myTest.Reset( value);
		EXPECT_EQ( myTest.Get(), value );
	}

	TEST( TScopedPtr, Destructor_Call_destroy_operator_from_Policy )
	{
		MyClass myClass;
		myClass.SetValue(1);

		{
			TScopedPtr< MyClass, MyClassDestructionPolicy< MyClass > > myTest(&myClass);
		}

		EXPECT_EQ(0, myClass.GetValue());
	}

	TEST( TScopedPtr, Destructor_call_destroy_from_policy_function )
	{
		MyClass myClass;
		myClass.SetValue(1);

		{
			TScopedPtr< MyClass, void(*)(MyClass*) > myTest(&myClass, CustomDeleteFunction);
		}


		EXPECT_EQ( 0, myClass.GetValue() );
	}

	TEST( TScopedPtr, Arrow_operator_will_assert_if_pointer_is_null )
	{
		TScopedPtr< Int32 > myTest;
		EXPECT_DEATH_IF_SUPPORTED( myTest.operator->(), "" );
	}

	TEST( TScopedPtr, Dereference_operator_will_assert_if_pointer_is_null )
	{
		TScopedPtr< Int32 > myTest;
		EXPECT_DEATH_IF_SUPPORTED( myTest.operator*(), "" );
	}
}
