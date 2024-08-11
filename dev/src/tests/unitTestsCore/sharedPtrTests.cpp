/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/sharedPtr.h"
#include "../../common/core/intrusivePtr.h"
#include "../../common/core/atomicSharedPtr.h"

namespace Red
{
namespace
{
	class MyBaseClass
	{
	public:
		static bool DestructorCalled;

		MyBaseClass()
		{
			DestructorCalled = false;
		}

		~MyBaseClass()
		{
			DestructorCalled = true;
		}
	};

	bool MyBaseClass::DestructorCalled = false;

	class MySharedPtrTestClass : public MyBaseClass
	{};

	class MyDerivedSharedPtrTestClass : public MySharedPtrTestClass
	{};

	class MyIntrusivePtrTestClass : public MyBaseClass
	{
	public:
		MyIntrusivePtrTestClass()
			: m_refCount( 1 )
		{}

		void AddRef() { ++m_refCount; }
		Int32 Release() { return --m_refCount; }

		Int32 GetRefCount() const{ return m_refCount; }

	private:

		Int32 m_refCount;
	};

	class MyDerivedIntrusivePtrTestClass : public MyIntrusivePtrTestClass
	{};

	template< typename T >
	Int32 GetRefCount( T & ptr )
	{
		return ptr.GetRefCount();
	}

	template< typename T >
	Int32 GetRefCount( TIntrusivePtr< T > & ptr )
	{
		return ptr ? ptr->GetRefCount() : 0;
	}

	struct TSharedPtrTraits
	{
		typedef MySharedPtrTestClass PtrType;
		typedef MyDerivedSharedPtrTestClass ConversionPtrType;
		
		typedef TSharedPtr< PtrType > SharedPtr;
		typedef TSharedPtr< ConversionPtrType > ConvertionSharedPtr;
	};

	struct TAtomicSharedPtrTraits
	{
		typedef MySharedPtrTestClass PtrType;
		typedef MyDerivedSharedPtrTestClass ConversionPtrType;

		typedef TAtomicSharedPtr< PtrType > SharedPtr;
		typedef TAtomicSharedPtr< ConversionPtrType > ConvertionSharedPtr;
	};

	struct TIntrusivePtrTraits
	{
		typedef MyIntrusivePtrTestClass PtrType;
		typedef MyDerivedIntrusivePtrTestClass ConversionPtrType;

		typedef TIntrusivePtr< PtrType > SharedPtr;
		typedef TIntrusivePtr< ConversionPtrType > ConvertionSharedPtr;
	};

	template< typename T >
	class SharedPtrFixutre : public ::testing::Test
	{
	public:

	};
}

	TYPED_TEST_CASE_P( SharedPtrFixutre );

	TYPED_TEST_P( SharedPtrFixutre, Get_function_return_null_by_default )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr sharedPtr;
		
		EXPECT_EQ( nullptr, sharedPtr.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, Get_return_provided_pointer )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * rawPtr = new PtrType;
		SharedPtr sharedPtr( rawPtr );

		EXPECT_EQ( rawPtr, sharedPtr.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, Constructor_can_take_raw_pointer_that_are_convertible )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new ConversionPtrType );
		EXPECT_TRUE( ptr );
	}

	TYPED_TEST_P( SharedPtrFixutre, bool_operator_return_false_by_default )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr sharedPtr;
		EXPECT_FALSE( sharedPtr );
	}

	TYPED_TEST_P( SharedPtrFixutre, bool_operator_return_true_if_pointee_is_not_null )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr pointer( new PtrType );
		EXPECT_TRUE( pointer );
	}

	TYPED_TEST_P( SharedPtrFixutre, not_operator_return_true_by_default )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr pointer;
		EXPECT_TRUE( !pointer );
	}

	TYPED_TEST_P( SharedPtrFixutre, not_operator_return_false_if_pointer_is_not_null )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr pointer( new PtrType );
		EXPECT_FALSE( !pointer );
	}

	TYPED_TEST_P( SharedPtrFixutre, pointer_is_destroy_if_only_one_reference_and_is_out_of_scope )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		{
			SharedPtr ptr( new PtrType );
		}

		EXPECT_TRUE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, pointer_is_not_destroy_if_one_reference_is_not_out_of_scope )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		{
			SharedPtr copy( ptr );
		}

		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, pointer_is_not_destroy_if_one_reference_is_not_out_of_scope_assignment_operator )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		{
			SharedPtr copy = ptr;
		}

		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, Swap_exchange_internals )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * raw1 = new PtrType();
		PtrType * raw2 = new PtrType();

		SharedPtr ptr1( raw1 );
		SharedPtr ptr2( raw2 );
		ptr1.Swap( ptr2 );

		EXPECT_EQ( raw1, ptr2.Get() );
		EXPECT_EQ( raw2, ptr1.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, GetRefCount_return_correct_amount_of_reference )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		SharedPtr copy1( ptr );
		SharedPtr copy2 = ptr;
		
		{
			SharedPtr dummy1 = ptr;
			SharedPtr dummy2 = ptr;
		}

		EXPECT_EQ( 3, GetRefCount( ptr ) );
	}

	TYPED_TEST_P( SharedPtrFixutre, Reset_delete_pointer_if_no_more_reference )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		ptr.Reset();

		EXPECT_TRUE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, Get_return_null_after_Reset_called )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		ptr.Reset();

		EXPECT_EQ( nullptr, ptr.Get() );
	}
	
	TYPED_TEST_P( SharedPtrFixutre, Reset_decrement_ref_count_by_one ) 
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		SharedPtr copy1( ptr );
		SharedPtr copy2 = ptr;

		copy1.Reset();

		EXPECT_EQ( 2, GetRefCount( ptr ) );
	}

	TYPED_TEST_P( SharedPtrFixutre, Reset_with_raw_pointer_swap_pointer_and_delete_old_one )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * newPtr = new PtrType;
		SharedPtr ptr( new PtrType );
		ptr.Reset( newPtr );

		EXPECT_TRUE( MyBaseClass::DestructorCalled );
		EXPECT_EQ( ptr.Get(), newPtr );
	}

	TYPED_TEST_P( SharedPtrFixutre, Reset_with_raw_pointer_do_not_delete_if_there_is_still_reference_on_old_pointer )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		SharedPtr copy1( ptr );
		ptr.Reset( new PtrType );
		
		EXPECT_FALSE( MyBaseClass::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, Reset_allow_convertible_raw_pointer )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr;
		ptr.Reset( new ConversionPtrType );
	}

	TYPED_TEST_P( SharedPtrFixutre, Copy_constructor_allow_convertible_TSharedPtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr( new ConversionPtrType );
		SharedPtr copy( ptr );
		EXPECT_TRUE( ptr );
		EXPECT_EQ( 2, GetRefCount( copy ) );
	}

	TYPED_TEST_P( SharedPtrFixutre, Copy_assignment_allow_convertible_TSharedPtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr( new ConversionPtrType );
		SharedPtr copy = ptr;
		EXPECT_TRUE( ptr );
		EXPECT_EQ( 2, GetRefCount( copy ) );
	}

	TYPED_TEST_P( SharedPtrFixutre, Arrow_operator_return_pointer )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * rawPtr = new PtrType;
		SharedPtr ptr( rawPtr );

		EXPECT_EQ( rawPtr, ptr.operator->() );
	}

	TYPED_TEST_P( SharedPtrFixutre, Arrow_operator_assert_if_pointer_is_null )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr;
		EXPECT_DEATH_IF_SUPPORTED( ptr.operator->(), "" );
	}

	TYPED_TEST_P( SharedPtrFixutre, Dereference_operator_return_pointer_reference )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * rawPtr = new PtrType;
		SharedPtr ptr( rawPtr );

		EXPECT_EQ( rawPtr, &*ptr );
	}

	TYPED_TEST_P( SharedPtrFixutre, Dereference_operator_assert_if_null )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr;

		EXPECT_DEATH_IF_SUPPORTED( *ptr, "" );
	}

	TYPED_TEST_P( SharedPtrFixutre, Equal_operator_return_true_if_both_pointer_are_equal )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr( new ConversionPtrType );
		SharedPtr copy( ptr );

		EXPECT_TRUE( ptr == copy );
	}

	TYPED_TEST_P( SharedPtrFixutre, Equal_operator_return_false_if_both_pointer_are_not_equal )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr1( new ConversionPtrType );
		SharedPtr ptr2( new PtrType );

		EXPECT_FALSE( ptr1 == ptr2 );
	}

	TYPED_TEST_P( SharedPtrFixutre, Not_Equal_operator_return_false_if_both_pointer_are_equal )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr( new ConversionPtrType );
		SharedPtr copy( ptr );

		EXPECT_FALSE( ptr != copy );
	}

	TYPED_TEST_P( SharedPtrFixutre, Not_Equal_operator_return_true_if_both_pointer_are_not_equal )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConvertionSharedPtr ptr1( new ConversionPtrType );
		SharedPtr ptr2( new PtrType );

		EXPECT_TRUE( ptr1 != ptr2 );
	}

	TYPED_TEST_P( SharedPtrFixutre, Less_operator_return_true_if_left_pointer_address_is_less_than_right_pointer_address )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * left = new PtrType;
		PtrType * right = new PtrType;
		if( left > right )
		{
			::Swap( left, right );
		}

		SharedPtr ptr1( left );
		SharedPtr ptr2( right );

		EXPECT_TRUE( ptr1 < ptr2 );
	}

	TYPED_TEST_P( SharedPtrFixutre, Less_operator_return_false_if_left_pointer_address_is_same_than_right_pointer_address )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr1( new PtrType );
		SharedPtr ptr2( ptr1 );

		EXPECT_FALSE( ptr1 < ptr2 );
	}

	TYPED_TEST_P( SharedPtrFixutre, Less_operator_return_false_if_left_pointer_address_is_more_than_right_pointer_address )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * left = new PtrType;
		PtrType * right = new PtrType;
		if( left < right )
		{
			::Swap( left, right );
		}

		SharedPtr ptr1( left );
		SharedPtr ptr2( right );

		EXPECT_FALSE( ptr1 < ptr2 );
	}

	TYPED_TEST_P( SharedPtrFixutre, Self_assignement_do_not_call_destructor_and_do_not_fuck_up_refcount )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( new PtrType );
		ptr = ptr;
		EXPECT_EQ( GetRefCount( ptr ), 1 );
		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_assgined_from_default_constructed_SharedPtr_do_not_increment_ref_Count )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr1;
		SharedPtr ptr2 = ptr1;

		EXPECT_EQ( 0, GetRefCount( ptr1 ) );
		EXPECT_EQ( 0, GetRefCount( ptr2 ) );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_move_constructor_handle_rvalue )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( std::move( SharedPtr( new PtrType ) ) );
		EXPECT_EQ( 1, GetRefCount( ptr ));
		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_move_constructor_handle_convertible_rvalue )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr( std::move( ConvertionSharedPtr( new ConversionPtrType ) ) );
		EXPECT_EQ( 1, GetRefCount( ptr ));
		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_move_assigment_operator_handle_rvalue )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr;
		ptr = SharedPtr( new PtrType );
		EXPECT_EQ( 1, GetRefCount( ptr ));
		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_move_assigment_operator_handle_convertible_rvalue )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		SharedPtr ptr;
		ptr = ConvertionSharedPtr( new ConversionPtrType );
		EXPECT_EQ( 1, GetRefCount( ptr ));
		EXPECT_FALSE( PtrType::DestructorCalled );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_can_be_constructed_from_UniquePtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * rawPtr = new PtrType;
		Red::TUniquePtr< PtrType > pointer( rawPtr );
		SharedPtr sharedPtr( std::move( pointer ) );

		EXPECT_FALSE( pointer );
		EXPECT_EQ( rawPtr, sharedPtr.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_can_be_constructed_from_convertible_UniquePtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConversionPtrType * rawPtr = new ConversionPtrType;
		Red::TUniquePtr< ConversionPtrType > pointer( rawPtr );
		SharedPtr sharedPtr( std::move( pointer ) );

		EXPECT_FALSE( pointer );
		EXPECT_EQ( rawPtr, sharedPtr.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_can_be_assigned_from_UniquePtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		PtrType * rawPtr = new PtrType;
		Red::TUniquePtr< PtrType > pointer( rawPtr );
		SharedPtr sharedPtr;
		sharedPtr = std::move( pointer );

		EXPECT_FALSE( pointer );
		EXPECT_EQ( rawPtr, sharedPtr.Get() );
	}

	TYPED_TEST_P( SharedPtrFixutre, SharedPtr_can_be_assigned_from_convertible_UniquePtr )
	{
		typedef typename TypeParam::PtrType PtrType;
		typedef typename TypeParam::ConversionPtrType ConversionPtrType;
		typedef typename TypeParam::SharedPtr SharedPtr;
		typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

		ConversionPtrType * rawPtr = new ConversionPtrType;
		Red::TUniquePtr< ConversionPtrType > pointer( rawPtr );
		SharedPtr sharedPtr;
		sharedPtr = std::move( pointer );

		EXPECT_FALSE( pointer );
		EXPECT_EQ( rawPtr, sharedPtr.Get() );
	}

	REGISTER_TYPED_TEST_CASE_P(
		SharedPtrFixutre,
		Get_function_return_null_by_default,
		Get_return_provided_pointer,
		Constructor_can_take_raw_pointer_that_are_convertible,
		bool_operator_return_false_by_default,
		bool_operator_return_true_if_pointee_is_not_null,
		not_operator_return_true_by_default,
		not_operator_return_false_if_pointer_is_not_null,
		pointer_is_destroy_if_only_one_reference_and_is_out_of_scope,
		pointer_is_not_destroy_if_one_reference_is_not_out_of_scope,
		pointer_is_not_destroy_if_one_reference_is_not_out_of_scope_assignment_operator,
		Swap_exchange_internals,
		GetRefCount_return_correct_amount_of_reference,
		Reset_delete_pointer_if_no_more_reference,
		Get_return_null_after_Reset_called,
		Reset_decrement_ref_count_by_one,
		Reset_with_raw_pointer_swap_pointer_and_delete_old_one,
		Reset_with_raw_pointer_do_not_delete_if_there_is_still_reference_on_old_pointer,
		Reset_allow_convertible_raw_pointer,
		Copy_constructor_allow_convertible_TSharedPtr,
		Copy_assignment_allow_convertible_TSharedPtr,
		Arrow_operator_return_pointer,
		Arrow_operator_assert_if_pointer_is_null,
		Dereference_operator_return_pointer_reference,
		Dereference_operator_assert_if_null,
		Equal_operator_return_true_if_both_pointer_are_equal,
		Equal_operator_return_false_if_both_pointer_are_not_equal,
		Not_Equal_operator_return_false_if_both_pointer_are_equal,
		Not_Equal_operator_return_true_if_both_pointer_are_not_equal,
		Less_operator_return_true_if_left_pointer_address_is_less_than_right_pointer_address,
		Less_operator_return_false_if_left_pointer_address_is_same_than_right_pointer_address,
		Less_operator_return_false_if_left_pointer_address_is_more_than_right_pointer_address,
		Self_assignement_do_not_call_destructor_and_do_not_fuck_up_refcount,
		SharedPtr_assgined_from_default_constructed_SharedPtr_do_not_increment_ref_Count,
		SharedPtr_move_constructor_handle_rvalue,
		SharedPtr_move_constructor_handle_convertible_rvalue,
		SharedPtr_move_assigment_operator_handle_rvalue,
		SharedPtr_move_assigment_operator_handle_convertible_rvalue,
		SharedPtr_can_be_constructed_from_UniquePtr,
		SharedPtr_can_be_constructed_from_convertible_UniquePtr,
		SharedPtr_can_be_assigned_from_UniquePtr,
		SharedPtr_can_be_assigned_from_convertible_UniquePtr
		);

	typedef ::testing::Types< 
		TSharedPtrTraits,
		TIntrusivePtrTraits,
		TAtomicSharedPtrTraits
	> SharedPtrTypes;

	INSTANTIATE_TYPED_TEST_CASE_P( SmartObject, SharedPtrFixutre, SharedPtrTypes);
}
