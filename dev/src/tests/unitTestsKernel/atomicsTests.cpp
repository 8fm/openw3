#include "build.h"

#include "../../common/redThreads/redThreadsAtomic.h"

#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#include <limits>

#ifdef RED_COMPILER_MSC
	RED_DISABLE_WARNING_MSC( 4146 ) // unary minus operator applied to unsigned type, result still unsigned
#endif

using namespace Red::System;

TEST( AtomicBool, LoadStore )
{
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( false );
		EXPECT_EQ( false, ab.GetValue() );
	}
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( true );
		EXPECT_EQ( true, ab.GetValue() );
	}
}
TEST( AtomicBool, Exchange )
{
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( false );
		Bool oldValue = ab.Exchange( true );
		EXPECT_EQ( false, oldValue );
		EXPECT_EQ( true , ab.GetValue() );
	}
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( true );
		Bool oldValue = ab.Exchange( false );
		EXPECT_EQ( true, oldValue );
		EXPECT_EQ( false , ab.GetValue() );
	}
}

TEST( AtomicBool, CompareExchange_Swap )
{
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( false );
		Bool oldValue = ab.CompareExchange( true, false );
		EXPECT_EQ( false, oldValue );
		EXPECT_EQ( true , ab.GetValue() );
	}
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( true );
		Bool oldValue = ab.CompareExchange( false, true );
		EXPECT_EQ( true, oldValue );
		EXPECT_EQ( false , ab.GetValue() );
	}
}
TEST( AtomicBool, CompareExchange_NoSwap )
{
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( false );
		Bool oldValue = ab.CompareExchange( true, true );
		EXPECT_EQ( false, oldValue );
		EXPECT_EQ( false, ab.GetValue() );
	}
	{
		Red::Threads::CAtomic< Red::Threads::Bool > ab;
		ab.SetValue( true );
		Bool oldValue = ab.CompareExchange( false, false );
		EXPECT_EQ( true, oldValue );
		EXPECT_EQ( true, ab.GetValue() );
	}
}

template< typename T > 
class Integer : public ::testing::Test
{};

TYPED_TEST_CASE_P( Integer );

TYPED_TEST_P( Integer, StoreLoad_Min ) 
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits< TypeParam >::min() );
	EXPECT_EQ( std::numeric_limits< TypeParam >::min(), atm.GetValue() );
}

TYPED_TEST_P( Integer, StoreLoad_Max )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits< TypeParam >::max() );
	EXPECT_EQ( std::numeric_limits< TypeParam >::max(), atm.GetValue() );
}


TYPED_TEST_P( Integer, StoreLoad_Zero )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam() );
	EXPECT_EQ( TypeParam(), atm.GetValue() );
}

TYPED_TEST_P( Integer, StoreLoad_MinusOne )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(-1) );
	EXPECT_EQ( TypeParam(-1), atm.GetValue() );
}


TYPED_TEST_P( Integer, StoreLoad_Other )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(127) );
	EXPECT_EQ( TypeParam(127), atm.GetValue() );
}

TYPED_TEST_P( Integer, Increment_FromZero )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(0) );
	TypeParam newValue = atm.Increment();
	EXPECT_EQ( TypeParam(1), newValue );
	EXPECT_EQ( TypeParam(1), atm.GetValue() );
}

TYPED_TEST_P( Integer, Increment_FromMinusOne_OverflowIfUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(-1) );
	TypeParam newValue = atm.Increment();
	EXPECT_EQ( TypeParam(0), newValue );
	EXPECT_EQ( TypeParam(0), atm.GetValue() );
}

TYPED_TEST_P( Integer, Increment_FromMin )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits<TypeParam>::min() );
	TypeParam newValue = atm.Increment();
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::min() + TypeParam(1);
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Increment_FromMax_Overflow )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits<TypeParam>::max() );
	TypeParam newValue = atm.Increment();
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::min();
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Increment_FromOther )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(127) );
	TypeParam newValue = atm.Increment();
	const TypeParam expectedValue = TypeParam(128);
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Decrement_FromZero_UnderflowIfUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const bool neg = std::numeric_limits<TypeParam>::min() < TypeParam(0);
	const TypeParam expectedValue = neg ? TypeParam(-1) : std::numeric_limits<TypeParam>::max();
	atm.SetValue( TypeParam(0) );
	TypeParam newValue = atm.Decrement();
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Decrement_FromMinusOne )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(-1) );
	TypeParam newValue = atm.Decrement();
	const TypeParam expectedValue = TypeParam(-1) - TypeParam(1);
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Decrement_FromMin_Underflow )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits<TypeParam>::min() );
	TypeParam newValue = atm.Decrement();
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::max();
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Decrement_FromMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( std::numeric_limits<TypeParam>::max() );
	TypeParam newValue = atm.Decrement();
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::max() - TypeParam(1);
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Decrement_FromOther )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(127) );
	TypeParam newValue = atm.Decrement();
	const TypeParam expectedValue = TypeParam(126);
	EXPECT_EQ( expectedValue, newValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_ZeroPlusMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam addValue = std::numeric_limits<TypeParam>::max();
	const TypeParam expectedValue = addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_ZeroPlusMin )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam addValue = std::numeric_limits<TypeParam>::min();
	const TypeParam expectedValue = addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_ZeroPlusZero )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam addValue = TypeParam(0);
	const TypeParam expectedValue = addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_ZeroPlusOther )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam addValue = TypeParam(5);
	const TypeParam expectedValue = addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_ZeroMinusOther_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam addValue = TypeParam(-5);
	const TypeParam expectedValue = addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MaxMinusMax_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits< TypeParam >::max();
	const TypeParam addValue = TypeParam(-std::numeric_limits< TypeParam >::max());
	const TypeParam expectedValue = TypeParam(0);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_TRUE( expectedValue < startValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MinMinusMin_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits< TypeParam >::min();
	const TypeParam addValue = TypeParam(-std::numeric_limits< TypeParam >::min());
	const TypeParam expectedValue = TypeParam(0);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MaxMinusOther_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits< TypeParam >::max();
	const TypeParam addValue = TypeParam(-5);
	const TypeParam expectedValue = std::numeric_limits< TypeParam >::max() + TypeParam(-5);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_TRUE( expectedValue < startValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MinPlusMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::min();
	const TypeParam addValue = std::numeric_limits<TypeParam>::max();
	const TypeParam expectedValue = startValue + addValue;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MaxPlusMin )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::max();
	const TypeParam addValue = std::numeric_limits<TypeParam>::min();
	const bool neg = ( addValue < TypeParam(0) );
	const TypeParam expectedValue = neg ? ( startValue + ( addValue + TypeParam(1) ) - TypeParam(1) ) : (startValue + addValue);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MaxPlusMax_Overflow_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::max();
	const TypeParam addValue = std::numeric_limits<TypeParam>::max();
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::min() + addValue - TypeParam(1);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_MinPlusMin_UnderflowIfSigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::min();
	const TypeParam addValue = std::numeric_limits<TypeParam>::min();
	const TypeParam expectedValue = 0;
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_OtherPlusOther )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(5);
	const TypeParam addValue = TypeParam(6);
	const TypeParam expectedValue = TypeParam(11);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}


TYPED_TEST_P( Integer, ExchangeAdd_OtherMinusOther_ValidIfSignedOrUnsigned )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(6);
	const TypeParam addValue = TypeParam(-5);
	const TypeParam expectedValue = TypeParam(1);
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_OtherAddOther_OverflowMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::max() - TypeParam(5);
	const TypeParam addValue = TypeParam(6);
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::min();
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, ExchangeAdd_OtherMinusOther_UnderflowMin )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::min() + TypeParam(5);
	const TypeParam addValue = TypeParam(-6);
	const TypeParam expectedValue = std::numeric_limits<TypeParam>::max();
	atm.SetValue( startValue );
	TypeParam oldValue = atm.ExchangeAdd( addValue );
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( expectedValue, atm.GetValue() );
}

TYPED_TEST_P( Integer, Exchange_Swap_SameValue )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam value = TypeParam(127);
	atm.SetValue( TypeParam(value) );
	TypeParam oldValue = atm.Exchange( value );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( value, oldValue );
	EXPECT_EQ( value, newValue );
}

TYPED_TEST_P( Integer, Exchange_Swap_FromMinToMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::min();
	const TypeParam exchange = std::numeric_limits<TypeParam>::max();
	atm.SetValue( TypeParam(startValue) );
	TypeParam oldValue = atm.Exchange( exchange );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( exchange, newValue );
}
TYPED_TEST_P( Integer, Exchange_Swap_FromMaxToMin )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::max();
	const TypeParam exchange = std::numeric_limits<TypeParam>::min();
	atm.SetValue( TypeParam(startValue) );
	TypeParam oldValue = atm.Exchange( exchange );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( exchange, newValue );
}

TYPED_TEST_P( Integer, Exchange_Swap_FromZeroToMax )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(0);
	const TypeParam exchange = std::numeric_limits<TypeParam>::max();
	atm.SetValue( TypeParam(startValue) );
	TypeParam oldValue = atm.Exchange( exchange );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( exchange, newValue );
}

TYPED_TEST_P( Integer, Exchange_Swap_FromMaxToZero )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = std::numeric_limits<TypeParam>::max();
	const TypeParam exchange = TypeParam(0);
	atm.SetValue( TypeParam(startValue) );
	TypeParam oldValue = atm.Exchange( exchange );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( exchange, newValue );
}

TYPED_TEST_P( Integer, Exchange_Swap_Other )
{
	Red::Threads::CAtomic< TypeParam > atm;
	const TypeParam startValue = TypeParam(127);
	const TypeParam exchange = TypeParam(4);
	atm.SetValue( TypeParam(startValue) );
	TypeParam oldValue = atm.Exchange( exchange );
	TypeParam newValue = atm.GetValue();
	EXPECT_EQ( startValue, oldValue );
	EXPECT_EQ( exchange, newValue );
}

TYPED_TEST_P( Integer, CompareExchange_Swap )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(10) );
	TypeParam oldValue = atm.CompareExchange( TypeParam(5), TypeParam(10) );
	EXPECT_EQ( TypeParam(10), oldValue );
	EXPECT_EQ( TypeParam(5), atm.GetValue() );
}

TYPED_TEST_P( Integer, CompareExchange_NoSwap )
{
	Red::Threads::CAtomic< TypeParam > atm;
	atm.SetValue( TypeParam(10) );
	TypeParam oldValue = atm.CompareExchange( TypeParam(5), TypeParam(123) );
	EXPECT_EQ( TypeParam(10), oldValue );
	EXPECT_EQ( TypeParam(10), atm.GetValue() );
}

REGISTER_TYPED_TEST_CASE_P( Integer, 
						   StoreLoad_Min, 
						   StoreLoad_Max, 
						   StoreLoad_Zero, 
						   StoreLoad_MinusOne, 
						   StoreLoad_Other,
						   Increment_FromZero,
						   Increment_FromMinusOne_OverflowIfUnsigned,
						   Increment_FromMin,
						   Increment_FromMax_Overflow,
						   Increment_FromOther,
						   Decrement_FromZero_UnderflowIfUnsigned,
						   Decrement_FromMinusOne,
						   Decrement_FromMin_Underflow,
						   Decrement_FromMax,
						   Decrement_FromOther,
						   ExchangeAdd_ZeroPlusMax,
						   ExchangeAdd_ZeroPlusMin,
						   ExchangeAdd_ZeroPlusZero,
						   ExchangeAdd_ZeroPlusOther,
						   ExchangeAdd_ZeroMinusOther_ValidIfSignedOrUnsigned,
						   ExchangeAdd_MaxMinusMax_ValidIfSignedOrUnsigned,
						   ExchangeAdd_MinMinusMin_ValidIfSignedOrUnsigned,
						   ExchangeAdd_MaxMinusOther_ValidIfSignedOrUnsigned,
						   ExchangeAdd_MinPlusMax,
						   ExchangeAdd_MaxPlusMin,
						   ExchangeAdd_MaxPlusMax_Overflow_ValidIfSignedOrUnsigned,
						   ExchangeAdd_MinPlusMin_UnderflowIfSigned,
						   ExchangeAdd_OtherPlusOther,
						   ExchangeAdd_OtherMinusOther_ValidIfSignedOrUnsigned,
						   ExchangeAdd_OtherAddOther_OverflowMax,
						   ExchangeAdd_OtherMinusOther_UnderflowMin,
						   Exchange_Swap_SameValue,
						   Exchange_Swap_FromMinToMax,
						   Exchange_Swap_FromMaxToMin,
						   Exchange_Swap_FromZeroToMax,
						   Exchange_Swap_FromMaxToZero,
						   Exchange_Swap_Other,
						   CompareExchange_Swap,
						   CompareExchange_NoSwap );


#ifdef RED_ARCH_X64
typedef ::testing::Types< Red::Threads::Int32, Red::Threads::Uint32, Red::Threads::Int64, Red::Threads::Uint64> IntegerTypes;
#else
typedef ::testing::Types< Red::Threads::Int32, Red::Threads::Uint32> IntegerTypes;
#endif

INSTANTIATE_TYPED_TEST_CASE_P( Atomic, Integer, IntegerTypes);

TEST( Atomic, AtomicPtr_StoreLoad )
{
	{
		unsigned char ch = 'a';
		Red::Threads::CAtomic<unsigned char*> ap;
		ap.SetValue( &ch );
		EXPECT_EQ( &ch, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::CAtomic<Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		EXPECT_EQ( &i32, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::CAtomic<Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		EXPECT_EQ( &u32, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::CAtomic<Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		EXPECT_EQ( &i64, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::CAtomic<Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		EXPECT_EQ( &u64, ap.GetValue() );
	}
#endif
}

TEST( Atomic, AtomicPtr_Exchange )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<unsigned char*> ap;
		ap.SetValue( &ch );
		unsigned char* oldVal = ap.Exchange( &ch2 );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch2, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		Red::Threads::Int32* oldVal = ap.Exchange( &i32_2 );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		Red::Threads::Uint32* oldVal = ap.Exchange( &u32_2 );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32_2, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		Red::Threads::Int64* oldVal = ap.Exchange( &i64_2 );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		Red::Threads::Uint64* oldVal = ap.Exchange( &u64_2 );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64_2, ap.GetValue() );
	}
#endif
}

TEST( Atomic, AtomicPtr_CompareExchange_Swap )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<unsigned char*> ap;
		ap.SetValue( &ch );
		unsigned char* oldVal = ap.CompareExchange( &ch2, &ch );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch2, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		Red::Threads::Int32* oldVal = ap.CompareExchange( &i32_2, &i32 );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		Red::Threads::Uint32* oldVal = ap.CompareExchange( &u32_2, &u32 );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32_2, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		Red::Threads::Int64* oldVal = ap.CompareExchange( &i64_2, &i64 );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		Red::Threads::Uint64* oldVal = ap.CompareExchange( &u64_2, &u64 );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64_2, ap.GetValue() );
	}
#endif
}

TEST( Atomic, AtomicPtr_CompareExchange_NoSwap )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<unsigned char*> ap;
		ap.SetValue( &ch );
		unsigned char* oldVal = ap.CompareExchange( &ch2, nullptr );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		Red::Threads::Int32* oldVal = ap.CompareExchange( &i32_2, nullptr );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		Red::Threads::Uint32* oldVal = ap.CompareExchange( &u32_2, nullptr );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		Red::Threads::Int64* oldVal = ap.CompareExchange( &i64_2, nullptr );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		Red::Threads::Uint64* oldVal = ap.CompareExchange( &u64_2, nullptr );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64, ap.GetValue() );
	}
#endif
}


//////////////////////////////////////////////////////////////////////////
// FIXME: Get rid of copy-paste, just for "const". Might also want "volatile" etc

TEST( Atomic, Const_AtomicPtr_StoreLoad )
{
	{
		unsigned char ch = 'a';
		Red::Threads::CAtomic<const unsigned char*> ap;
		ap.SetValue( &ch );
		EXPECT_EQ( &ch, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::CAtomic<const Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		EXPECT_EQ( &i32, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::CAtomic<const Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		EXPECT_EQ( &u32, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::CAtomic<const Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		EXPECT_EQ( &i64, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::CAtomic<const Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		EXPECT_EQ( &u64, ap.GetValue() );
	}
#endif
}

TEST( Atomic, Const_AtomicPtr_Exchange )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<const unsigned char*> ap;
		ap.SetValue( &ch );
		const unsigned char* oldVal = ap.Exchange( &ch2 );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch2, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		const Red::Threads::Int32* oldVal = ap.Exchange( &i32_2 );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		const Red::Threads::Uint32* oldVal = ap.Exchange( &u32_2 );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32_2, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		const Red::Threads::Int64* oldVal = ap.Exchange( &i64_2 );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		const Red::Threads::Uint64* oldVal = ap.Exchange( &u64_2 );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64_2, ap.GetValue() );
	}
#endif
}

TEST( Atomic, Const_AtomicPtr_CompareExchange_Swap )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<const unsigned char*> ap;
		ap.SetValue( &ch );
		const unsigned char* oldVal = ap.CompareExchange( &ch2, &ch );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch2, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		const Red::Threads::Int32* oldVal = ap.CompareExchange( &i32_2, &i32 );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		const Red::Threads::Uint32* oldVal = ap.CompareExchange( &u32_2, &u32 );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32_2, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		const Red::Threads::Int64* oldVal = ap.CompareExchange( &i64_2, &i64 );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64_2, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		const Red::Threads::Uint64* oldVal = ap.CompareExchange( &u64_2, &u64 );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64_2, ap.GetValue() );
	}
#endif
}

TEST( Atomic, Const_AtomicPtr_CompareExchange_NoSwap )
{
	{
		unsigned char ch = 'a';
		unsigned char ch2 = 'b';

		Red::Threads::CAtomic<const unsigned char*> ap;
		ap.SetValue( &ch );
		const unsigned char* oldVal = ap.CompareExchange( &ch2, nullptr );
		EXPECT_EQ( &ch, oldVal );
		EXPECT_EQ( &ch, ap.GetValue() );
	}
	{
		Red::Threads::Int32 i32 = 3;
		Red::Threads::Int32 i32_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int32*> ap;
		ap.SetValue( &i32 );
		const Red::Threads::Int32* oldVal = ap.CompareExchange( &i32_2, nullptr );
		EXPECT_EQ( &i32, oldVal );
		EXPECT_EQ( &i32, ap.GetValue() );
	}
	{
		Red::Threads::Uint32 u32 = 6u;
		Red::Threads::Uint32 u32_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint32*> ap;
		ap.SetValue( &u32 );
		const Red::Threads::Uint32* oldVal = ap.CompareExchange( &u32_2, nullptr );
		EXPECT_EQ( &u32, oldVal );
		EXPECT_EQ( &u32, ap.GetValue() );
	}
#ifdef RED_ARCH_X64
	{
		Red::Threads::Int64 i64 = 3;
		Red::Threads::Int64 i64_2 = 6;
		Red::Threads::CAtomic<const Red::Threads::Int64*> ap;
		ap.SetValue( &i64 );
		const Red::Threads::Int64* oldVal = ap.CompareExchange( &i64_2, nullptr );
		EXPECT_EQ( &i64, oldVal );
		EXPECT_EQ( &i64, ap.GetValue() );
	}
	{
		Red::Threads::Uint64 u64 = 6u;
		Red::Threads::Uint64 u64_2 = 12u;
		Red::Threads::CAtomic<const Red::Threads::Uint64*> ap;
		ap.SetValue( &u64 );
		const Red::Threads::Uint64* oldVal = ap.CompareExchange( &u64_2, nullptr );
		EXPECT_EQ( &u64, oldVal );
		EXPECT_EQ( &u64, ap.GetValue() );
	}
#endif
}
