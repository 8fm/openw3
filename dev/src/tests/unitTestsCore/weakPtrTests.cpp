/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/weakPtr.h"
#include "../../common/core/atomicWeakPtr.h"

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

	struct TWeakPtrTraits
	{
		typedef MyClass PtrType;
		typedef MyDerivedClass ConversionPtrType;

		typedef Red::TSharedPtr< PtrType > SharedPtr;
		typedef Red::TSharedPtr< ConversionPtrType > ConvertionSharedPtr;
		typedef Red::TWeakPtr< PtrType > WeakPtr;
		typedef Red::TWeakPtr< ConversionPtrType > ConvertionWeakPtr;
	};

	struct TAtomicWeakPtrTraits
	{
		typedef MyClass PtrType;
		typedef MyDerivedClass ConversionPtrType;

		typedef Red::TAtomicSharedPtr< PtrType > SharedPtr;
		typedef Red::TAtomicSharedPtr< ConversionPtrType > ConvertionSharedPtr;

		typedef Red::TAtomicWeakPtr< PtrType > WeakPtr;
		typedef Red::TAtomicWeakPtr< ConversionPtrType > ConvertionWeakPtr;
	};

	template< typename T >
	class WeakPtrFixutre : public ::testing::Test
	{
	public:

	};
}

TYPED_TEST_CASE_P( WeakPtrFixutre );

TYPED_TEST_P( WeakPtrFixutre, Expired_return_true_by_default )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	WeakPtr ptr;
	EXPECT_TRUE( ptr.Expired() );
}

TYPED_TEST_P( WeakPtrFixutre, Expired_return_false_if_strong_reference_is_one )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr( sharedPtr );

	EXPECT_FALSE( ptr.Expired() );
}

TYPED_TEST_P( WeakPtrFixutre, Expired_return_true_after_strong_reference_went_to_0 )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr( sharedPtr );
	sharedPtr.Reset();

	EXPECT_TRUE( ptr.Expired() );
}

TYPED_TEST_P( WeakPtrFixutre, Lock_return_an_invalid_TSharedPtr_by_default )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	WeakPtr ptr;
	SharedPtr sharedPtr = ptr.Lock();

	EXPECT_TRUE( !sharedPtr );
}

TYPED_TEST_P( WeakPtrFixutre, Lock_return_an_invalid_TSharedPtr_if_strong_ref_are_0 )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr( sharedPtr );
	sharedPtr.Reset();

	SharedPtr lockResult = ptr.Lock();
	EXPECT_TRUE( !lockResult );
}

TYPED_TEST_P( WeakPtrFixutre, Lock_return_valid_TSharedPtr_if_strong_ref_are_more_than_0 )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr( sharedPtr );
	SharedPtr lockResult = ptr.Lock();
	EXPECT_TRUE( lockResult );
	EXPECT_EQ( sharedPtr.Get(), lockResult.Get() );
}

TYPED_TEST_P( WeakPtrFixutre, Lock_return_invalid_TSharedPtr_if_executed_after_Reset )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr( sharedPtr );
	ptr.Reset();
	SharedPtr lockResult = ptr.Lock();
	EXPECT_TRUE( !lockResult );
}



TYPED_TEST_P( WeakPtrFixutre, Copy_constructor_with_sharedPtr_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr1( sharedPtr );
	WeakPtr ptr2( sharedPtr );
	WeakPtr ptr3( sharedPtr );

	sharedPtr.Reset();

	EXPECT_EQ( 3, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 3, ptr2.GetWeakRefCount() );
	EXPECT_EQ( 3, ptr3.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Copy_constructor_with_weakPtr_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr1( sharedPtr );
	WeakPtr ptr2( ptr1 );
	WeakPtr ptr3( ptr1 );
	WeakPtr ptr4( ptr1 );
	
	sharedPtr.Reset();

	EXPECT_EQ( 4, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr2.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr3.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr4.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Copy_Constructor_with_convertible_type_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	ConvertionSharedPtr sharedPtr( new MyDerivedClass );
	ConvertionWeakPtr ptr1( sharedPtr );
	WeakPtr ptr2( ptr1 );

	sharedPtr.Reset();

	EXPECT_EQ( 2, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 2, ptr2.GetWeakRefCount() );
}

//////////////////////////////////////////////////////////////////////////
TYPED_TEST_P( WeakPtrFixutre, Assignment_operator_with_sharedPtr_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr1;;
	WeakPtr ptr2;
	WeakPtr ptr3;

	ptr1 = sharedPtr;
	ptr2 = sharedPtr;
	ptr3 = sharedPtr;

	sharedPtr.Reset();

	EXPECT_EQ( 3, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 3, ptr2.GetWeakRefCount() );
	EXPECT_EQ( 3, ptr3.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Assignment_operator_with_weakPtr_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr1;
	WeakPtr ptr2;
	WeakPtr ptr3;
	WeakPtr ptr4;

	ptr1 = sharedPtr;
	ptr2 = ptr1;
	ptr3 = ptr1;
	ptr4 = ptr1;

	sharedPtr.Reset();

	EXPECT_EQ( 4, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr2.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr3.GetWeakRefCount() );
	EXPECT_EQ( 4, ptr4.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Assignment_operator_with_convertible_type_increment_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	ConvertionSharedPtr sharedPtr( new MyDerivedClass );
	ConvertionWeakPtr ptr1( sharedPtr );
	WeakPtr ptr2;
	ptr2 = ptr1;

	sharedPtr.Reset();

	EXPECT_EQ( 2, ptr1.GetWeakRefCount() );
	EXPECT_EQ( 2, ptr2.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Reset_decrement_weak_ref_count )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr( new MyClass );
	WeakPtr ptr1( sharedPtr );
	sharedPtr.Reset();
	ptr1.Reset();

	EXPECT_EQ( 0, ptr1.GetWeakRefCount() );
}

TYPED_TEST_P( WeakPtrFixutre, Swap_exchange_internal )
{
	typedef typename TypeParam::PtrType PtrType;
	typedef typename TypeParam::ConversionPtrType ConversionPtrType;
	typedef typename TypeParam::SharedPtr SharedPtr;
	typedef typename TypeParam::ConvertionSharedPtr ConvertionSharedPtr;

	typedef typename TypeParam::WeakPtr WeakPtr;
	typedef typename TypeParam::ConvertionWeakPtr ConvertionWeakPtr;

	SharedPtr sharedPtr1( new MyClass );
	WeakPtr ptr1( sharedPtr1 );

	SharedPtr sharedPtr2( new MyClass );
	WeakPtr ptr2( sharedPtr2 );

	ptr1.Swap( ptr2 );

	EXPECT_EQ( sharedPtr1, ptr2.Lock() );
	EXPECT_EQ( sharedPtr2, ptr1.Lock() );
}

REGISTER_TYPED_TEST_CASE_P(
	WeakPtrFixutre,
	Expired_return_true_by_default,
	Expired_return_false_if_strong_reference_is_one,
	Expired_return_true_after_strong_reference_went_to_0,
	Lock_return_an_invalid_TSharedPtr_by_default,
	Lock_return_an_invalid_TSharedPtr_if_strong_ref_are_0,
	Lock_return_valid_TSharedPtr_if_strong_ref_are_more_than_0,
	Lock_return_invalid_TSharedPtr_if_executed_after_Reset,
	Copy_constructor_with_sharedPtr_increment_weak_ref_count,
	Copy_constructor_with_weakPtr_increment_weak_ref_count,
	Copy_Constructor_with_convertible_type_increment_weak_ref_count,
	Assignment_operator_with_sharedPtr_increment_weak_ref_count,
	Assignment_operator_with_weakPtr_increment_weak_ref_count,
	Assignment_operator_with_convertible_type_increment_weak_ref_count,
	Reset_decrement_weak_ref_count,
	Swap_exchange_internal
	);

typedef ::testing::Types< 
	TWeakPtrTraits,
	TAtomicWeakPtrTraits
> WeakPtrTypes;

INSTANTIATE_TYPED_TEST_CASE_P( SmartObject, WeakPtrFixutre, WeakPtrTypes);

