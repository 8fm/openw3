/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include  "build.h"
#include "../../common/core/uniquePtr.h"

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

	class MyDerivedClass : public MyClass
	{};

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

	TEST( TUniquePtr, can_access_raw_pointer_via_arrow_operator )
	{
		MyClass * ptr = new MyClass();
		TUniquePtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, myTest->GetValue() );
	}

	TEST( TUniquePtr, can_access_reference_dereference_operator )
	{
		MyClass * ptr = new MyClass();
		TUniquePtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, (*myTest).GetValue() );
	}

	TEST( TUniquePtr, can_access_raw_pointer_via_Get )
	{
		MyClass * ptr = new MyClass();
		TUniquePtr< MyClass > myTest( ptr );

		ptr->SetValue( 10 );

		EXPECT_EQ( 10, myTest.Get()->GetValue() );
	}

	TEST( TUniquePtr, bool_opetator_return_false_by_default )
	{
		TUniquePtr< Int32 > myTest;

		EXPECT_FALSE( myTest );
	}

	TEST( TUniquePtr, bool_opetator_return_true_when_pointee_is_valid )
	{
		TUniquePtr< Int32 > myTest( new Int32 );

		EXPECT_TRUE( myTest );
	}

	TEST( TUniquePtr, not_opetator_return_true_by_default )
	{
		TUniquePtr< Int32 > myTest;

		EXPECT_TRUE( !myTest );
	}

	TEST( TUniquePtr, not_opetator_return_false_when_pointee_is_valid )
	{
		TUniquePtr< Int32 > myTest( new Int32 );

		EXPECT_FALSE( !myTest );
	}

	TEST( TUniquePtr, Swap_swap_internal_pointer )
	{
		TUniquePtr< Int32 > myFirst( new Int32() );
		TUniquePtr< Int32 > mySecond( new Int32() );

		*myFirst = 1;
		*mySecond = 2;

		myFirst.Swap( mySecond );

		EXPECT_EQ( 2, *myFirst );
		EXPECT_EQ( 1, *mySecond );
	}

	TEST( TUniquePtr, Reset_delete_current_pointer )
	{
		TUniquePtr< Int32 > myTest( new Int32() );
		myTest.Reset();
		EXPECT_EQ( myTest.Get(), nullptr );
	}

	TEST( TUniquePtr, Reset_delete_current_pointer_and_swap_with_provided_pointer )
	{
		Int32 * value = new Int32( 10 );
		TUniquePtr< Int32 > myTest( new Int32( 5 ) );
		myTest.Reset( value);
		EXPECT_EQ( myTest.Get(), value );
	}

	TEST( TUniquePtr, Destructor_Call_destroy_operator_from_Policy )
	{
		MyClass myClass;
		myClass.SetValue(1);

		{
			TUniquePtr< MyClass, MyClassDestructionPolicy< MyClass > > myTest(&myClass);
		}

		EXPECT_EQ(0, myClass.GetValue());
	}

	TEST( TUniquePtr, Destructor_call_destroy_from_policy_function )
	{
		MyClass myClass;
		myClass.SetValue(1);

		{
			TUniquePtr< MyClass, void(*)(MyClass*) > myTest(&myClass, CustomDeleteFunction);
		}


		EXPECT_EQ( 0, myClass.GetValue() );
	}

	TEST( TUniquePtr, Arrow_operator_will_assert_if_pointer_is_null )
	{
		TUniquePtr< Int32 > myTest;
		EXPECT_DEATH_IF_SUPPORTED( myTest.operator->(), "" );
	}

	TEST( TUniquePtr, Dereference_operator_will_assert_if_pointer_is_null )
	{
		TUniquePtr< Int32 > myTest;
		EXPECT_DEATH_IF_SUPPORTED( myTest.operator*(), "" );
	}

	TEST( TUniquePtr, Move_constructor_release_pointer_from_original_and_move_it_to_copy )
	{
		MyClass * raw = new MyClass;
		TUniquePtr< MyClass > ptr( raw );
		TUniquePtr< MyClass > copy( std::move( ptr ) );

		EXPECT_EQ( raw, copy.Get() );
		EXPECT_FALSE( ptr );
	}

	TEST( TUniquePtr, Move_assignment_operator_release_pointer_from_original_and_move_it_to_copy )
	{
		MyClass * raw = new MyClass;
		TUniquePtr< MyClass > ptr( raw );
		TUniquePtr< MyClass > copy;
		copy = std::move( ptr );

		EXPECT_EQ( raw, copy.Get() );
		EXPECT_FALSE( ptr );
	}

	TEST( TUniquePtr, Move_assignment_operator_support_self_assignment )
	{
		MyClass * raw = new MyClass;
		TUniquePtr< MyClass > ptr( raw );
		ptr = std::move( ptr );

		EXPECT_EQ( raw, ptr.Get() );
	}

	TEST( TUniquePtr, Move_constructor_release_pointer_from_original_and_move_it_to_copy_convertible )
	{
		MyDerivedClass * raw = new MyDerivedClass;
		TUniquePtr< MyDerivedClass > ptr( raw );
		TUniquePtr< MyClass > copy( std::move( ptr ) );

		EXPECT_EQ( raw, copy.Get() );
		EXPECT_FALSE( ptr );
	}

	TEST( TUniquePtr, Move_assignment_operator_release_pointer_from_original_and_move_it_to_copy_convertible )
	{
		MyDerivedClass * raw = new MyDerivedClass;
		TUniquePtr< MyDerivedClass > ptr( raw );
		TUniquePtr< MyClass > copy;
		copy = std::move( ptr );

		EXPECT_EQ( raw, copy.Get() );
		EXPECT_FALSE( ptr );
	}

	TEST( TUniquePtr, Release_return_ownership_of_pointer )
	{
		MyClass * raw = new MyClass;
		TUniquePtr< MyClass > ptr( raw );
		MyClass * owner = ptr.Release();
	
		EXPECT_EQ( raw, owner );
		EXPECT_FALSE( ptr );
		delete owner;
	}

	TEST( TUniquePtr, Release_return_null_if_does_not_own_any_pointer )
	{
		TUniquePtr< MyClass > ptr;
		EXPECT_EQ( nullptr, ptr.Release() );
	}

	TEST( TUniquePtr, TUniquePtr_can_be_use_with_TDynArray )
	{
		MyClass * raw1 = new MyClass;
		MyClass * raw2 = new MyClass;

		TUniquePtr< MyClass > ptr( raw1 );

		TDynArray< TUniquePtr< MyClass > > container;
		
		container.PushBack( std::move( ptr ) );
		container.PushBack( TUniquePtr< MyClass >( raw2 ) );

		EXPECT_EQ( container[0].Get(), raw1 );
		EXPECT_EQ( container[1].Get(), raw2 );
	}
}
