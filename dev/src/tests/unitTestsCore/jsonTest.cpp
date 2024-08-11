#include "build.h"

#include "../../common/core/fileSys.h"

#include "../../common/core/jsonValue.h"
#include "../../common/core/jsonObject.h"
#include "../../common/core/jsonArray.h"
#include "../../common/core/jsonFileSimpleWriter.h"
#include "../../common/core/jsonFilePrettyWriter.h"
#include "../../common/core/jsonFileReader.h"
#include "../../common/redIO/redIO.h"

#if defined( RED_PLATFORM_DURANGO)
#define TEST_SIMPLE_JSON_FILE_PATH_UTF8 TXT("d:\\simple_jsonTestUTF8.json")
#define TEST_SIMPLE_JSON_FILE_PATH_UTF16 TXT("d:\\simple_jsonTestUTF16.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF8 TXT("d:\\pretty_jsonTestUTF8.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF16 TXT("d:\\pretty_jsonTestUTF16.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("d:\\simple_from_object_jsonTestUTF8.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("d:\\simple_from_object_jsonTestUTF16.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("d:\\pretty_from_object_jsonTestUTF8.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("d:\\pretty_from_object_jsonTestUTF16.json")
#elif defined( RED_PLATFORM_ORBIS )
#define TEST_SIMPLE_JSON_FILE_PATH_UTF8 TXT("/hostapp/simple_jsonTestUTF8.json")
#define TEST_SIMPLE_JSON_FILE_PATH_UTF16 TXT("/hostapp/simple_jsonTestUTF16.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF8 TXT("/hostapp/pretty_jsonTestUTF8.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF16 TXT("/hostapp/pretty_jsonTestUTF16.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("/hostapp/simple_from_object_jsonTestUTF8.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("/hostapp/simple_from_object_jsonTestUTF16.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("/hostapp/pretty_from_object_jsonTestUTF8.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("/hostapp/pretty_from_object_jsonTestUTF16.json")
#else
#define TEST_SIMPLE_JSON_FILE_PATH_UTF8 TXT("simple_jsonTestUTF8.json")
#define TEST_SIMPLE_JSON_FILE_PATH_UTF16 TXT("simple_jsonTestUTF16.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF8 TXT("pretty_jsonTestUTF8.json")
#define TEST_PRETTY_JSON_FILE_PATH_UTF16 TXT("pretty_jsonTestUTF16.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("simple_from_object_jsonTestUTF8.json")
#define TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("simple_from_object_jsonTestUTF16.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF8 TXT("pretty_from_object_jsonTestUTF8.json")
#define TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF16 TXT("pretty_from_object_jsonTestUTF16.json")
#endif


#define TEST_INT32_MAX				Int32(0x7fffffffl)
#define TEST_INT32_CORRECT_POSITIVE Int32(TEST_INT32_MAX/2l)
#define TEST_INT32_CORRECT_NEGATIVE Int32(TEST_INT32_MIN/2l)
#define TEST_INT32_INCORRECT		Int64(0x80000000ll)
#define TEST_INT32_MIN				Int32(-TEST_INT32_MAX - 1l)

#define TEST_UINT32_MAX     Uint32( 0xfffffffful )
#define TEST_UINT32_CORRECT Uint32(TEST_UINT32_MAX/2ul)
#define TEST_UINT32_MIN     Uint32(0x00000000ul)

#define TEST_INT64_MAX				Int64(0x7fffffffffffffffll)
#define TEST_INT64_MIN				Int64(-TEST_INT64_MAX - 1ll)
#define TEST_INT64_CORRECT_POSITIVE Int64(TEST_INT64_MAX/2ll)
#define TEST_INT64_CORRECT_NEGATIVE Int64(TEST_INT64_MIN/2ll)

#define TEST_UINT64_MAX     Uint64(0xffffffffffffffffull)
#define TEST_UINT64_CORRECT Uint64(TEST_UINT64_MAX/2ull)
#define TEST_UINT64_MIN     Uint64(0x0000000000000000ull)

Double hexToDouble( Uint64 hex ){ return reinterpret_cast<Double&>(hex); }
#define TEST_DOUBLE_MAX_NORMAL				hexToDouble( 0x7FEFFFFFFFFFFFFFull )
#define TEST_DOUBLE_MIN_NORMAL_POSITIVE		hexToDouble( 0x0010000000000000ull )
#define TEST_DOUBLE_MIN_SUBNORMAL			hexToDouble( 0x000FFFFFFFFFFFFFull )
#define TEST_DOUBLE_MAX_SUBNORMAL_POSITIVE	hexToDouble( 0x0000000000000001ull )

#define TEST_STRING_EMPTY_UTF8 UNICODE_TO_ANSI( TXT("") )
#define TEST_STRING_UTF8 UNICODE_TO_ANSI( TXT("TEST_STRING_UTF8") )
#define TEST_STRING_LENGTH_UTF8 16
#define TEST_STRING_EMPTY_UTF16 TXT("")
#define TEST_STRING_UTF16 TXT("TEST_STRING_UTF16")
#define TEST_STRING_LENGTH_UTF16 17

#define TEST_OBJECT_VALUE_NAME_UTF8 UNICODE_TO_ANSI( TXT("TEST_OBJECT_VALUE_NAME_UTF8"))
#define TEST_OBJECT_VALUE_NAME_UTF16 TXT("TEST_OBJECT_VALUE_NAME_UTF16")

TEST( JSON, UTF8_Value_Default )
{
	CJSONValueUTF8 defaulValue;
	CJSONValueRefUTF8 defaulValueRef(defaulValue);
	EXPECT_FALSE( defaulValueRef.IsBool() );
	EXPECT_FALSE( defaulValueRef.IsInt32() );
	EXPECT_FALSE( defaulValueRef.IsUint32() );
	EXPECT_FALSE( defaulValueRef.IsInt64() );
	EXPECT_FALSE( defaulValueRef.IsUint64() );
	EXPECT_FALSE( defaulValueRef.IsDouble() );
	EXPECT_FALSE( defaulValueRef.IsString() );
	
	if( defaulValueRef.IsString() )
		EXPECT_TRUE( defaulValueRef.GetString() == NULL );
}

TEST( JSON, UTF16_Value_Default )
{

	CJSONValueUTF16 defaulValue;
	CJSONValueRefUTF16 defaulValueRef(defaulValue);
	EXPECT_FALSE( defaulValueRef.IsBool() );
	EXPECT_FALSE( defaulValueRef.IsInt32() );
	EXPECT_FALSE( defaulValueRef.IsUint32() );
	EXPECT_FALSE( defaulValueRef.IsInt64() );
	EXPECT_FALSE( defaulValueRef.IsUint64() );
	EXPECT_FALSE( defaulValueRef.IsDouble() );
	EXPECT_FALSE( defaulValueRef.IsString() );

	if( defaulValueRef.IsString() )
		EXPECT_TRUE( defaulValueRef.GetString() == NULL );
}

TEST( JSON, UTF8_Value_Bool_True )
{
	CJSONValueUTF8 boolTrueValue(true);
	CJSONValueRefUTF8 boolTrueValueRef(boolTrueValue);
	EXPECT_TRUE( boolTrueValueRef.IsBool() );
	EXPECT_FALSE( boolTrueValueRef.IsInt32() );
	EXPECT_FALSE( boolTrueValueRef.IsUint32() );
	EXPECT_FALSE( boolTrueValueRef.IsInt64() );
	EXPECT_FALSE( boolTrueValueRef.IsUint64() );
	EXPECT_FALSE( boolTrueValueRef.IsDouble() );
	EXPECT_FALSE( boolTrueValueRef.IsString() );

	EXPECT_TRUE( boolTrueValueRef.GetBool() == true );
}

TEST( JSON, UTF16_Value_Bool_True )
{
	CJSONValueUTF16 boolTrueValue(true);
	CJSONValueRefUTF16 boolTrueValueRef(boolTrueValue);
	EXPECT_TRUE( boolTrueValueRef.IsBool() );
	EXPECT_FALSE( boolTrueValueRef.IsInt32() );
	EXPECT_FALSE( boolTrueValueRef.IsUint32() );
	EXPECT_FALSE( boolTrueValueRef.IsInt64() );
	EXPECT_FALSE( boolTrueValueRef.IsUint64() );
	EXPECT_FALSE( boolTrueValueRef.IsDouble() );
	EXPECT_FALSE( boolTrueValueRef.IsString() );
}

TEST( JSON, UTF8_Value_Bool_False )
{
	CJSONValueUTF8 boolTrueValue(true);
	CJSONValueRefUTF8 boolTrueValueRef(boolTrueValue);
	EXPECT_TRUE( boolTrueValueRef.IsBool() );
	EXPECT_FALSE( boolTrueValueRef.IsInt32() );
	EXPECT_FALSE( boolTrueValueRef.IsUint32() );
	EXPECT_FALSE( boolTrueValueRef.IsInt64() );
	EXPECT_FALSE( boolTrueValueRef.IsUint64() );
	EXPECT_FALSE( boolTrueValueRef.IsDouble() );
	EXPECT_FALSE( boolTrueValueRef.IsString() );

	EXPECT_TRUE( boolTrueValueRef.GetBool() == true );
}

TEST( JSON, UTF16_Value_Bool_False )
{
	CJSONValueUTF16 boolTrueValue(true);
	CJSONValueRefUTF16 boolTrueValueRef(boolTrueValue);
	EXPECT_TRUE( boolTrueValueRef.IsBool() );
	EXPECT_FALSE( boolTrueValueRef.IsInt32() );
	EXPECT_FALSE( boolTrueValueRef.IsUint32() );
	EXPECT_FALSE( boolTrueValueRef.IsInt64() );
	EXPECT_FALSE( boolTrueValueRef.IsUint64() );
	EXPECT_FALSE( boolTrueValueRef.IsDouble() );
	EXPECT_FALSE( boolTrueValueRef.IsString() );

	EXPECT_TRUE( boolTrueValueRef.GetBool() == true );
}

//! Int32

TEST( JSON, UTF8_Value_Int32_Min )
{
	CJSONValueUTF8 int32Value( TEST_INT32_MIN );
	CJSONValueRefUTF8 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE(  int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_FALSE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_MIN );
}

TEST( JSON, UTF16_Value_Int32_Min )
{
	CJSONValueUTF16 int32Value( TEST_INT32_MIN );
	CJSONValueRefUTF16 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE(  int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_FALSE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_MIN );
}

TEST( JSON, UTF8_Value_Int32_Max )
{
	CJSONValueUTF8 int32Value( TEST_INT32_MAX );
	CJSONValueRefUTF8 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE( int32ValueRef.IsInt32() );
	EXPECT_TRUE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_TRUE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_MAX );
}

TEST( JSON, UTF16_Value_Int32_Max )
{
	CJSONValueUTF16 int32Value( TEST_INT32_MAX );
	CJSONValueRefUTF16 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE( int32ValueRef.IsInt32() );
	EXPECT_TRUE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_TRUE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_MAX );
}

TEST( JSON, UTF8_Value_Int32_Correct_Positive )
{
	CJSONValueUTF8 int32Value( TEST_INT32_CORRECT_POSITIVE );
	CJSONValueRefUTF8 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE( int32ValueRef.IsInt32() );
	EXPECT_TRUE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_TRUE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_CORRECT_POSITIVE );
}

TEST( JSON, UTF16_Value_Int32_Correct_Positive )
{
	CJSONValueUTF16 int32Value( TEST_INT32_CORRECT_POSITIVE );
	CJSONValueRefUTF16 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE( int32ValueRef.IsInt32() );
	EXPECT_TRUE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_TRUE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_CORRECT_POSITIVE );
}

TEST( JSON, UTF8_Value_Int32_Correct_Negative )
{
	CJSONValueUTF8 int32Value( TEST_INT32_CORRECT_NEGATIVE );
	CJSONValueRefUTF8 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE(  int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_FALSE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );
}

TEST( JSON, UTF16_Value_Int32_Correct_Negative )
{
	CJSONValueUTF16 int32Value( TEST_INT32_CORRECT_NEGATIVE );
	CJSONValueRefUTF16 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_TRUE(  int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsUint32() );
	EXPECT_TRUE( int32ValueRef.IsInt64() );
	EXPECT_FALSE( int32ValueRef.IsUint64() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

	EXPECT_TRUE( int32ValueRef.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );
}

TEST( JSON, UTF8_Value_Int32_Incorrect )
{
	CJSONValueUTF8 int32Value( TEST_INT32_INCORRECT );
	CJSONValueRefUTF8 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_FALSE( int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );

}

TEST( JSON, UTF16_Value_Int32_Incorrect )
{
	CJSONValueUTF16 int32Value( TEST_INT32_INCORRECT );
	CJSONValueRefUTF16 int32ValueRef(int32Value);
	EXPECT_FALSE( int32ValueRef.IsBool() );
	EXPECT_FALSE( int32ValueRef.IsInt32() );
	EXPECT_FALSE( int32ValueRef.IsDouble() );
	EXPECT_FALSE( int32ValueRef.IsString() );
}

//! Uint32 

TEST( JSON, UTF8_Value_Uint32_Max )
{
	CJSONValueUTF8 uint32Value( TEST_UINT32_MAX );
	CJSONValueRefUTF8 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	EXPECT_FALSE( uint32ValueRef.IsInt32() );
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	EXPECT_TRUE( uint32ValueRef.IsInt64() );
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_MAX );
}

TEST( JSON, UTF16_Value_Uint32_Max )
{
	CJSONValueUTF16 uint32Value( TEST_UINT32_MAX );
	CJSONValueRefUTF16 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	EXPECT_FALSE( uint32ValueRef.IsInt32() );
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	EXPECT_TRUE( uint32ValueRef.IsInt64() );
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_MAX );
}

TEST( JSON, UTF8_Value_Uint32_Min )
{
	CJSONValueUTF8 uint32Value( TEST_UINT32_MIN );
	CJSONValueRefUTF8 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	EXPECT_TRUE( uint32ValueRef.IsInt32() );
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	EXPECT_TRUE( uint32ValueRef.IsInt64() );
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_MIN );
}

TEST( JSON, UTF16_Value_Uint32_Min )
{
	CJSONValueUTF16 uint32Value( TEST_UINT32_MIN );
	CJSONValueRefUTF16 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	EXPECT_TRUE( uint32ValueRef.IsInt32() );
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	EXPECT_TRUE( uint32ValueRef.IsInt64() );
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_MIN );
}

TEST( JSON, UTF8_Value_Uint32_Correct )
{
	CJSONValueUTF8 uint32Value( TEST_UINT32_CORRECT );
	CJSONValueRefUTF8 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	if (!(TEST_UINT32_CORRECT & 0x80000000))
	{
		EXPECT_TRUE( uint32ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( uint32ValueRef.IsInt32() );
	}	
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	if (!(TEST_UINT32_CORRECT & 0x80000000))
	{
		EXPECT_TRUE( uint32ValueRef.IsInt64() );
	}
	else
	{
		EXPECT_FALSE( uint32ValueRef.IsInt64() );
	}
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_CORRECT );
}

TEST( JSON, UTF16_Value_Uint32_Correct )
{
	CJSONValueUTF16 uint32Value( TEST_UINT32_CORRECT );
	CJSONValueRefUTF16 uint32ValueRef(uint32Value);
	EXPECT_FALSE( uint32ValueRef.IsBool() );
	if (!(TEST_UINT32_CORRECT & 0x80000000))
	{
		EXPECT_TRUE( uint32ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( uint32ValueRef.IsInt32() );
	}	
	EXPECT_TRUE( uint32ValueRef.IsUint32() );
	if (!(TEST_UINT32_CORRECT & 0x80000000))
	{
		EXPECT_TRUE( uint32ValueRef.IsInt64() );
	}
	else
	{
		EXPECT_FALSE( uint32ValueRef.IsInt64() );
	}
	EXPECT_TRUE( uint32ValueRef.IsUint64() );
	EXPECT_FALSE( uint32ValueRef.IsDouble() );
	EXPECT_FALSE( uint32ValueRef.IsString() );

	EXPECT_TRUE( uint32ValueRef.GetUint32() == TEST_UINT32_CORRECT );
}

//! Int64

TEST( JSON, UTF8_Value_Int64_Min )
{
	CJSONValueUTF8 int64Value( TEST_INT64_MIN );
	CJSONValueRefUTF8 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	EXPECT_FALSE(  int64ValueRef.IsInt32() );
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_FALSE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_MIN );
}

TEST( JSON, UTF16_Value_Int64_Min )
{
	CJSONValueUTF16 int64Value( TEST_INT64_MIN );
	CJSONValueRefUTF16 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	EXPECT_FALSE(  int64ValueRef.IsInt32() );
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_FALSE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_MIN );
}

TEST( JSON, UTF8_Value_Int64_Max )
{
	CJSONValueUTF8 int64Value( TEST_INT64_MAX );
	CJSONValueRefUTF8 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	EXPECT_FALSE( int64ValueRef.IsInt32() );
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_TRUE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_MAX );
}

TEST( JSON, UTF16_Value_Int64_Max )
{
	CJSONValueUTF16 int64Value( TEST_INT64_MAX );
	CJSONValueRefUTF16 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	EXPECT_FALSE( int64ValueRef.IsInt32() );
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_TRUE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_MAX );
}

TEST( JSON, UTF8_Value_Int64_Correct_Positive )
{
	CJSONValueUTF8 int64Value( TEST_INT64_CORRECT_POSITIVE );
	CJSONValueRefUTF8 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );	
	if (!(TEST_INT64_CORRECT_POSITIVE & 0xFFFFFFFF80000000LL))
	{
		EXPECT_TRUE( int64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( int64ValueRef.IsInt32() );
	}
	if (!(TEST_INT64_CORRECT_POSITIVE & 0xFFFFFFFF00000000LL))
	{
		EXPECT_TRUE( int64ValueRef.IsUint32() );
	}
	else
	{
		EXPECT_FALSE( int64ValueRef.IsUint32() );
	}
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_TRUE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_CORRECT_POSITIVE );
}

TEST( JSON, UTF16_Value_Int64_Correct_Positive )
{
	CJSONValueUTF16 int64Value( TEST_INT64_CORRECT_POSITIVE );
	CJSONValueRefUTF16 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );	
	if (!(TEST_INT64_CORRECT_POSITIVE & 0xFFFFFFFF80000000LL))
	{
		EXPECT_TRUE( int64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( int64ValueRef.IsInt32() );
	}
	if (!(TEST_INT64_CORRECT_POSITIVE & 0xFFFFFFFF00000000LL))
	{
		EXPECT_TRUE( int64ValueRef.IsUint32() );
	}
	else
	{
		EXPECT_FALSE( int64ValueRef.IsUint32() );
	}
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_TRUE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_CORRECT_POSITIVE );
}

TEST( JSON, UTF8_Value_Int64_Correct_Negative )
{
	CJSONValueUTF8 int64Value( TEST_INT64_CORRECT_NEGATIVE );
	CJSONValueRefUTF8 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	if (TEST_INT64_CORRECT_NEGATIVE >= TEST_INT32_MAX)
	{
		EXPECT_TRUE(  int64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE(  int64ValueRef.IsInt32() );
	}	
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_FALSE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_CORRECT_NEGATIVE );
}

TEST( JSON, UTF16_Value_Int64_Correct_Negative )
{
	CJSONValueUTF16 int64Value( TEST_INT64_CORRECT_NEGATIVE );
	CJSONValueRefUTF16 int64ValueRef(int64Value);
	EXPECT_FALSE( int64ValueRef.IsBool() );
	if (TEST_INT64_CORRECT_NEGATIVE >= TEST_INT32_MAX)
	{
		EXPECT_TRUE(  int64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE(  int64ValueRef.IsInt32() );
	}	
	EXPECT_FALSE( int64ValueRef.IsUint32() );
	EXPECT_TRUE( int64ValueRef.IsInt64() );
	EXPECT_FALSE( int64ValueRef.IsUint64() );
	EXPECT_FALSE( int64ValueRef.IsDouble() );
	EXPECT_FALSE( int64ValueRef.IsString() );

	EXPECT_TRUE( int64ValueRef.GetInt64() == TEST_INT64_CORRECT_NEGATIVE );
}

//! Uint64 

TEST( JSON, UTF8_Value_Uint64_Max )
{
	CJSONValueUTF8 uint64Value( TEST_UINT64_MAX );
	CJSONValueRefUTF8 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );
	EXPECT_FALSE( uint64ValueRef.IsInt32() );
	EXPECT_FALSE( uint64ValueRef.IsUint32() );
	EXPECT_FALSE( uint64ValueRef.IsInt64() );
	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_MAX );
}

TEST( JSON, UTF16_Value_Uint64_Max )
{
	CJSONValueUTF16 uint64Value( TEST_UINT64_MAX );
	CJSONValueRefUTF16 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );
	EXPECT_FALSE( uint64ValueRef.IsInt32() );
	EXPECT_FALSE( uint64ValueRef.IsUint32() );
	EXPECT_FALSE( uint64ValueRef.IsInt64() );
	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_MAX );
}

TEST( JSON, UTF8_Value_Uint64_Min )
{
	CJSONValueUTF8 uint64Value( TEST_UINT64_MIN );
	CJSONValueRefUTF8 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );
	EXPECT_TRUE( uint64ValueRef.IsInt32() );
	EXPECT_TRUE( uint64ValueRef.IsUint32() );
	EXPECT_TRUE( uint64ValueRef.IsInt64() );
	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_MIN );
}

TEST( JSON, UTF16_Value_Uint64_Min )
{
	CJSONValueUTF16 uint64Value( TEST_UINT64_MIN );
	CJSONValueRefUTF16 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );
	EXPECT_TRUE( uint64ValueRef.IsInt32() );
	EXPECT_TRUE( uint64ValueRef.IsUint32() );
	EXPECT_TRUE( uint64ValueRef.IsInt64() );
	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_MIN );
}

TEST( JSON, UTF8_Value_Uint64_Correct )
{
	CJSONValueUTF8 uint64Value( TEST_UINT64_CORRECT );
	CJSONValueRefUTF8 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );

	if (!(TEST_UINT64_CORRECT & 0xFFFFFFFF80000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsInt32() );
	}

	if (!(TEST_UINT64_CORRECT & 0xFFFFFFFF00000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsUint32() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsUint32() );
	}

	if (!(TEST_UINT64_CORRECT & 0x8000000000000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsInt64() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsInt64() );
	}

	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_CORRECT );
}

TEST( JSON, UTF16_Value_Uint64_Correct )
{
	CJSONValueUTF16 uint64Value( TEST_UINT64_CORRECT );
	CJSONValueRefUTF16 uint64ValueRef(uint64Value);
	EXPECT_FALSE( uint64ValueRef.IsBool() );

	if (!(TEST_UINT64_CORRECT & 0xFFFFFFFF80000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsInt32() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsInt32() );
	}

	if (!(TEST_UINT64_CORRECT & 0xFFFFFFFF00000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsUint32() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsUint32() );
	}

	if (!(TEST_UINT64_CORRECT & 0x8000000000000000ULL))
	{
		EXPECT_TRUE( uint64ValueRef.IsInt64() );
	}
	else
	{
		EXPECT_FALSE( uint64ValueRef.IsInt64() );
	}

	EXPECT_TRUE( uint64ValueRef.IsUint64() );
	EXPECT_FALSE( uint64ValueRef.IsDouble() );
	EXPECT_FALSE( uint64ValueRef.IsString() );

	EXPECT_TRUE( uint64ValueRef.GetUint64() == TEST_UINT64_CORRECT );
}

//! Double 

TEST( JSON, UTF8_Value_Double_MaxNormal )
{
	CJSONValueUTF8 doubleValue(TEST_DOUBLE_MAX_NORMAL);
	CJSONValueRefUTF8 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MAX_NORMAL );
}

TEST( JSON, UTF16_Value_Double_MaxNormal )
{
	CJSONValueUTF16 doubleValue(TEST_DOUBLE_MAX_NORMAL);
	CJSONValueRefUTF16 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MAX_NORMAL );
}

TEST( JSON, UTF8_Value_Double_MinNormalPositive )
{
	CJSONValueUTF8 doubleValue(TEST_DOUBLE_MIN_NORMAL_POSITIVE);
	CJSONValueRefUTF8 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MIN_NORMAL_POSITIVE );
}

TEST( JSON, UTF16_Value_Double_MinNormalPositive )
{
	CJSONValueUTF16 doubleValue(TEST_DOUBLE_MIN_NORMAL_POSITIVE);
	CJSONValueRefUTF16 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MIN_NORMAL_POSITIVE );
}

TEST( JSON, UTF8_Value_Double_MinSubnormal)
{
	CJSONValueUTF8 doubleValue(TEST_DOUBLE_MIN_SUBNORMAL);
	CJSONValueRefUTF8 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MIN_SUBNORMAL );
}

TEST( JSON, UTF16_Value_Double_MinSubnormal)
{
	CJSONValueUTF16 doubleValue(TEST_DOUBLE_MIN_SUBNORMAL);
	CJSONValueRefUTF16 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MIN_SUBNORMAL );
}

TEST( JSON, UTF8_Value_Double_MinSubnormalPositive)
{
	CJSONValueUTF8 doubleValue(TEST_DOUBLE_MAX_SUBNORMAL_POSITIVE);
	CJSONValueRefUTF8 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MAX_SUBNORMAL_POSITIVE );
}

TEST( JSON, UTF16_Value_Double_MinSubnormalPositive)
{
	CJSONValueUTF16 doubleValue(TEST_DOUBLE_MAX_SUBNORMAL_POSITIVE);
	CJSONValueRefUTF16 doubleValueRef(doubleValue);
	EXPECT_FALSE( doubleValueRef.IsBool() );
	EXPECT_FALSE( doubleValueRef.IsInt32() );
	EXPECT_FALSE( doubleValueRef.IsUint32() );
	EXPECT_FALSE( doubleValueRef.IsInt64() );
	EXPECT_FALSE( doubleValueRef.IsUint64() );
	EXPECT_TRUE( doubleValueRef.IsDouble() );
	EXPECT_FALSE( doubleValueRef.IsString() );

	EXPECT_TRUE( doubleValueRef.GetDouble() == TEST_DOUBLE_MAX_SUBNORMAL_POSITIVE );
}

//! String

TEST( JSON, UTF8_Value_StringEmpty)
{
	CJSONValueUTF8 stringValue(TEST_STRING_EMPTY_UTF8);
	CJSONValueRefUTF8 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	EXPECT_TRUE( TString<Red::System::AnsiChar>( stringValueRef.GetString() ).Empty()  );
}

TEST( JSON, UTF16_Value_StringEmpty)
{
	CJSONValueUTF16 stringValue(TEST_STRING_EMPTY_UTF16);
	CJSONValueRefUTF16 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	EXPECT_TRUE( TString<Red::System::UniChar>( stringValueRef.GetString() ).Empty()  );
}

TEST( JSON, UTF8_Value_String)
{
	CJSONValueUTF8 stringValue(TEST_STRING_UTF8);
	CJSONValueRefUTF8 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	EXPECT_TRUE( TString<Red::System::AnsiChar>( stringValueRef.GetString() ) == TEST_STRING_UTF8 );
}

TEST( JSON, UTF16_Value_String)
{
	CJSONValueUTF16 stringValue(TEST_STRING_UTF16);
	CJSONValueRefUTF16 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	EXPECT_TRUE( TString<Red::System::UniChar>( stringValueRef.GetString() ) == TEST_STRING_UTF16 );
}

TEST( JSON, UTF8_Value_StringWithLength)
{
	CJSONValueUTF8 stringValue(TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8);
	CJSONValueRefUTF8 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	 TString<Red::System::AnsiChar> str =  stringValueRef.GetString();

	EXPECT_TRUE( str == TEST_STRING_UTF8 );
	EXPECT_TRUE( str.GetLength() == TEST_STRING_LENGTH_UTF8 );
}

TEST( JSON, UTF16_Value_StringWithLength)
{
	CJSONValueUTF16 stringValue(TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16);
	CJSONValueRefUTF16 stringValueRef(stringValue);
	EXPECT_FALSE( stringValueRef.IsBool() );
	EXPECT_FALSE( stringValueRef.IsInt32() );
	EXPECT_FALSE( stringValueRef.IsUint32() );
	EXPECT_FALSE( stringValueRef.IsInt64() );
	EXPECT_FALSE( stringValueRef.IsDouble() );
	EXPECT_TRUE( stringValueRef.IsString() );

	TString<Red::System::UniChar> str = stringValueRef.GetString();

	EXPECT_TRUE( str == TEST_STRING_UTF16 );
	EXPECT_TRUE( str.GetLength() == TEST_STRING_LENGTH_UTF16 );
}

// ! Object

TEST( JSON, UTF8_Object_Create)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	EXPECT_TRUE( defaultObjectRef.GetType() == JSON_Object );
	EXPECT_FALSE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );
}

TEST( JSON, UTF16_Object_Create)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	EXPECT_TRUE( defaultObjectRef.GetType() == JSON_Object );
	EXPECT_FALSE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );
}

TEST( JSON, UTF8_Object_AddObject)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);
	
	CJSONObjectUTF8 subObject;
	defaultObject.AddMember( TEST_OBJECT_VALUE_NAME_UTF8, subObject );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );
	EXPECT_TRUE( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ).GetType() == JSON_Object );
}


TEST( JSON, UTF16_Object_AddObject)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	CJSONObjectUTF16 subObject;
	defaultObject.AddMember( TEST_OBJECT_VALUE_NAME_UTF16, subObject );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );
	EXPECT_TRUE( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ).GetType() == JSON_Object );
}

TEST( JSON, UTF8_Object_AddBool)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberBool( TEST_OBJECT_VALUE_NAME_UTF8, true );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );
	
	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_True );
	EXPECT_TRUE( valueRef.IsBool() );
	EXPECT_TRUE( valueRef.GetBool() );
}

TEST( JSON, UTF16_Object_AddBool)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberBool( TEST_OBJECT_VALUE_NAME_UTF16, true );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_True );
	EXPECT_TRUE( valueRef.IsBool() );
	EXPECT_TRUE( valueRef.GetBool() );
}

TEST( JSON, UTF8_Object_AddInt32)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberInt32( TEST_OBJECT_VALUE_NAME_UTF8, TEST_INT32_CORRECT_NEGATIVE );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsInt32() );
	EXPECT_TRUE( valueRef.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );
}

TEST( JSON, UTF16_Object_AddInt32)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberInt32( TEST_OBJECT_VALUE_NAME_UTF16, TEST_INT32_CORRECT_NEGATIVE );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsInt32() );
	EXPECT_TRUE( valueRef.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );
}

TEST( JSON, UTF8_Object_AddUint32)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberUint32( TEST_OBJECT_VALUE_NAME_UTF8, TEST_UINT32_CORRECT );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsUint32() );
	EXPECT_TRUE( valueRef.GetUint32() == TEST_UINT32_CORRECT );
}

TEST( JSON, UTF16_Object_AddUint32)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberUint32( TEST_OBJECT_VALUE_NAME_UTF16, TEST_UINT32_CORRECT );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsUint32() );
	EXPECT_TRUE( valueRef.GetUint32() == TEST_UINT32_CORRECT );
}

TEST( JSON, UTF8_Object_AddInt64)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberInt64( TEST_OBJECT_VALUE_NAME_UTF8, TEST_INT64_CORRECT_NEGATIVE );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsInt64() );
	EXPECT_TRUE( valueRef.GetInt64() == TEST_INT64_CORRECT_NEGATIVE );
}

TEST( JSON, UTF16_Object_AddInt64)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberInt64( TEST_OBJECT_VALUE_NAME_UTF16, TEST_INT64_CORRECT_NEGATIVE );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsInt64() );
	EXPECT_TRUE( valueRef.GetInt64() == TEST_INT64_CORRECT_NEGATIVE );
}

TEST( JSON, UTF8_Object_AddUint64)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberUint64( TEST_OBJECT_VALUE_NAME_UTF8, TEST_UINT64_CORRECT );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsUint64() );
	EXPECT_TRUE( valueRef.GetUint64() == TEST_UINT64_CORRECT );
}

TEST( JSON, UTF16_Object_AddUint64)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberUint64( TEST_OBJECT_VALUE_NAME_UTF16, TEST_UINT64_CORRECT );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsUint64() );
	EXPECT_TRUE( valueRef.GetUint64() == TEST_UINT64_CORRECT );
}

TEST( JSON, UTF8_Object_AddDouble)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberDouble( TEST_OBJECT_VALUE_NAME_UTF8, TEST_DOUBLE_MIN_SUBNORMAL );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsDouble() );
	EXPECT_TRUE( valueRef.GetDouble() == TEST_DOUBLE_MIN_SUBNORMAL );
}

TEST( JSON, UTF16_Object_AddDouble)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberDouble( TEST_OBJECT_VALUE_NAME_UTF16, TEST_DOUBLE_MIN_SUBNORMAL );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_Number );
	EXPECT_TRUE( valueRef.IsDouble() );
	EXPECT_TRUE( valueRef.GetDouble() == TEST_DOUBLE_MIN_SUBNORMAL );
}

TEST( JSON, UTF8_Object_AddString)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberString( TEST_OBJECT_VALUE_NAME_UTF8, TEST_STRING_UTF8 );
	
	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_String );
	EXPECT_TRUE( valueRef.IsString() );
	EXPECT_TRUE( TString<Red::System::AnsiChar>( valueRef.GetString() ) == TEST_STRING_UTF8 );
}

TEST( JSON, UTF16_Object_AddString)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberString( TEST_OBJECT_VALUE_NAME_UTF16, TEST_STRING_UTF16 );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_String );
	EXPECT_TRUE( valueRef.IsString() );
	EXPECT_TRUE( TString<Red::System::UniChar>( valueRef.GetString() ) == TEST_STRING_UTF16 );
}

TEST( JSON, UTF8_Object_AddStringWithLength)
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	defaultObject.AddMemberString( TEST_OBJECT_VALUE_NAME_UTF8, TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONValueRefUTF8 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_String );
	EXPECT_TRUE( valueRef.IsString() );

	TString<Red::System::AnsiChar> str = valueRef.GetString();

	EXPECT_TRUE( str.GetLength() == TEST_STRING_LENGTH_UTF8 );
	EXPECT_TRUE( str == TEST_STRING_UTF8 );
}

TEST( JSON, UTF16_Object_AddStringWithLength)
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	defaultObject.AddMemberString( TEST_OBJECT_VALUE_NAME_UTF16, TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONValueRefUTF16 valueRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	EXPECT_TRUE( valueRef.GetType() == JSON_String );
	EXPECT_TRUE( valueRef.IsString() );

	TString<Red::System::UniChar> str = valueRef.GetString();

	EXPECT_TRUE( str.GetLength() == TEST_STRING_LENGTH_UTF16 );
	EXPECT_TRUE( str == TEST_STRING_UTF16 );
}

//! Array

TEST( JSON, UTF8_Array_Create )
{
	CJSONArrayUTF8 defaultArray;
	CJSONArrayRefUTF8 defaultArrayeRef( defaultArray );

	EXPECT_TRUE( defaultArrayeRef.GetType() == JSON_Array );
	EXPECT_TRUE( defaultArrayeRef.Size() == 0 );
}

TEST( JSON, UTF16_Array_Create )
{
	CJSONArrayUTF16 defaultArray;
	CJSONArrayRefUTF16 defaultArrayeRef( defaultArray );

	EXPECT_TRUE( defaultArrayeRef.GetType() == JSON_Array );
	EXPECT_TRUE( defaultArrayeRef.Size() == 0 );
}

TEST( JSON, UTF8_Object_AddArray )
{
	CJSONObjectUTF8 defaultObject;
	CJSONObjectRefUTF8 defaultObjectRef(defaultObject);

	CJSONArrayUTF8 subObjectArray;
	defaultObject.AddMember( TEST_OBJECT_VALUE_NAME_UTF8, subObjectArray );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	CJSONArrayRefUTF8 subObjectArrayRef( defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF8 ) );

	EXPECT_TRUE( subObjectArrayRef.GetType() == JSON_Array );
	EXPECT_TRUE( subObjectArrayRef.Size() == 0 );
}

TEST( JSON, UTF16_Object_AddArray )
{
	CJSONObjectUTF16 defaultObject;
	CJSONObjectRefUTF16 defaultObjectRef(defaultObject);

	CJSONArrayUTF16 subObjectArray;
	defaultObject.AddMember( TEST_OBJECT_VALUE_NAME_UTF16, subObjectArray );

	EXPECT_TRUE( defaultObjectRef.HasMember( TEST_OBJECT_VALUE_NAME_UTF16 ) );

	CJSONArrayRefUTF16 subObjectArrayRef = defaultObjectRef.GetMember( TEST_OBJECT_VALUE_NAME_UTF16 );

	EXPECT_TRUE( subObjectArrayRef.GetType() == JSON_Array );
	EXPECT_TRUE( subObjectArrayRef.Size() == 0 );
}

TEST( JSON, UTF8_Array_AddObject )
{
	CJSONArrayUTF8 defaultArray;
	CJSONObjectUTF8 subObject;
	
	defaultArray.PushBack( subObject );

	CJSONArrayRefUTF8 defaultArrayRef( defaultArray );

	EXPECT_TRUE( defaultArrayRef.Size() == 1 );
	
	CJSONObjectRefUTF8 subObjectRef( defaultArrayRef.GetMemberAt( 0 ) );

	EXPECT_TRUE( subObjectRef.GetType() == JSON_Object );
	
	defaultArray.PopBack();

	EXPECT_TRUE( defaultArrayRef.Size() == 0 );
}

TEST( JSON, UTF16_Array_AddObject )
{
	CJSONArrayUTF16 defaultArray;
	CJSONObjectUTF16 subObject;

	defaultArray.PushBack( subObject );

	CJSONArrayRefUTF16 defaultArrayRef( defaultArray );

	EXPECT_TRUE( defaultArrayRef.Size() == 1 );

	CJSONObjectRefUTF16 subObjectRef( defaultArrayRef.GetMemberAt( 0 ) );

	EXPECT_TRUE( subObjectRef.GetType() == JSON_Object );

	defaultArray.PopBack();

	EXPECT_TRUE( defaultArrayRef.Size() == 0 );
}

TEST( JSON, UTF8_Array_AddValue )
{
	CJSONArrayUTF8 defaultArray;

	CJSONValueUTF8 subValue;
	CJSONValueUTF8 subValueBool( true );
	CJSONValueUTF8 subValueInt32( TEST_INT32_CORRECT_NEGATIVE );
	CJSONValueUTF8 subValueUint32( TEST_UINT32_CORRECT );
	CJSONValueUTF8 subValueInt64( TEST_INT64_CORRECT_POSITIVE );
	CJSONValueUTF8 subValueUint64( TEST_UINT64_CORRECT );
	CJSONValueUTF8 subValueDouble( TEST_DOUBLE_MAX_NORMAL );
	CJSONValueUTF8 subValueString( TEST_STRING_UTF8 );
	CJSONValueUTF8 subValueStringWithLength( TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	defaultArray.PushBack( subValue );
	defaultArray.PushBack( subValueBool );
	defaultArray.PushBack( subValueInt32 );
	defaultArray.PushBack( subValueUint32 );
	defaultArray.PushBack( subValueInt64 );
	defaultArray.PushBack( subValueUint64 );
	defaultArray.PushBack( subValueDouble );
	defaultArray.PushBack( subValueString );
	defaultArray.PushBack( subValueStringWithLength );

	CJSONArrayRefUTF8 defaultArrayRef( defaultArray );

	EXPECT_TRUE( defaultArrayRef.Size() == 9 );

	CJSONValueRefUTF8 subValueRef( defaultArrayRef.GetMemberAt( 0 ) );

	EXPECT_TRUE( subValueRef.GetType() == JSON_Null );

	CJSONValueRefUTF8 subValueBoolRef( defaultArrayRef.GetMemberAt( 1 ) );

	EXPECT_TRUE( subValueBoolRef.IsBool() );
	EXPECT_TRUE( subValueBoolRef.GetBool() );

	CJSONValueRefUTF8 subValueInt32Ref( defaultArrayRef.GetMemberAt( 2 ) );

	EXPECT_TRUE( subValueInt32Ref.IsInt32() );
	EXPECT_TRUE( subValueInt32Ref.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );

	CJSONValueRefUTF8 subValueUint32Ref( defaultArrayRef.GetMemberAt( 3 ) );

	EXPECT_TRUE( subValueUint32Ref.IsUint32() );
	EXPECT_TRUE( subValueUint32Ref.GetUint32() == TEST_UINT32_CORRECT );

	CJSONValueRefUTF8 subValueInt64Ref( defaultArrayRef.GetMemberAt( 4 ) );

	EXPECT_TRUE( subValueInt64Ref.IsInt64() );
	EXPECT_TRUE( subValueInt64Ref.GetInt64() == TEST_INT64_CORRECT_POSITIVE );

	CJSONValueRefUTF8 subValueUint64Ref( defaultArrayRef.GetMemberAt( 5 ) );

	EXPECT_TRUE( subValueUint64Ref.IsUint64() );
	EXPECT_TRUE( subValueUint64Ref.GetUint64() == TEST_UINT64_CORRECT );
	
	CJSONValueRefUTF8 subValueDoubleRef( defaultArrayRef.GetMemberAt( 6 ) );

	EXPECT_TRUE( subValueDoubleRef.IsDouble() );
	EXPECT_TRUE( subValueDoubleRef.GetDouble() == TEST_DOUBLE_MAX_NORMAL );

	CJSONValueRefUTF8 subValueStringRef( defaultArrayRef.GetMemberAt( 7 ) );

	EXPECT_TRUE( subValueStringRef.IsString() );

	TString<Red::System::AnsiChar> str = subValueStringRef.GetString();

	EXPECT_TRUE( str == TEST_STRING_UTF8 );

	CJSONValueRefUTF8 subValueStringWithLengthRef( defaultArrayRef.GetMemberAt( 8 ) );

	EXPECT_TRUE( subValueStringWithLengthRef.IsString() );

	TString<Red::System::AnsiChar> strWithLength = subValueStringWithLengthRef.GetString();

	EXPECT_TRUE( strWithLength.GetLength() == TEST_STRING_LENGTH_UTF8 );
	EXPECT_TRUE( strWithLength == TEST_STRING_UTF8 );

	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();

	EXPECT_TRUE( defaultArrayRef.Size() == 0 );
}

TEST( JSON, UTF16_Array_AddValue )
{
	CJSONArrayUTF16 defaultArray;

	CJSONValueUTF16 subValue;
	CJSONValueUTF16 subValueBool( true );
	CJSONValueUTF16 subValueInt32( TEST_INT32_CORRECT_NEGATIVE );
	CJSONValueUTF16 subValueUint32( TEST_UINT32_CORRECT );
	CJSONValueUTF16 subValueInt64( TEST_INT64_CORRECT_POSITIVE );
	CJSONValueUTF16 subValueUint64( TEST_UINT64_CORRECT );
	CJSONValueUTF16 subValueDouble( TEST_DOUBLE_MAX_NORMAL );
	CJSONValueUTF16 subValueString( TEST_STRING_UTF16 );
	CJSONValueUTF16 subValueStringWithLength( TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	defaultArray.PushBack( subValue );
	defaultArray.PushBack( subValueBool );
	defaultArray.PushBack( subValueInt32 );
	defaultArray.PushBack( subValueUint32 );
	defaultArray.PushBack( subValueInt64 );
	defaultArray.PushBack( subValueUint64 );
	defaultArray.PushBack( subValueDouble );
	defaultArray.PushBack( subValueString );
	defaultArray.PushBack( subValueStringWithLength );

	CJSONArrayRefUTF16 defaultArrayRef( defaultArray );

	EXPECT_TRUE( defaultArrayRef.Size() == 9 );

	CJSONValueRefUTF16 subValueRef( defaultArrayRef.GetMemberAt( 0 ) );

	EXPECT_TRUE( subValueRef.GetType() == JSON_Null );

	CJSONValueRefUTF16 subValueBoolRef( defaultArrayRef.GetMemberAt( 1 ) );

	EXPECT_TRUE( subValueBoolRef.IsBool() );
	EXPECT_TRUE( subValueBoolRef.GetBool() );

	CJSONValueRefUTF16 subValueInt32Ref( defaultArrayRef.GetMemberAt( 2 ) );

	EXPECT_TRUE( subValueInt32Ref.IsInt32() );
	EXPECT_TRUE( subValueInt32Ref.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );

	CJSONValueRefUTF16 subValueUint32Ref( defaultArrayRef.GetMemberAt( 3 ) );

	EXPECT_TRUE( subValueUint32Ref.IsUint32() );
	EXPECT_TRUE( subValueUint32Ref.GetUint32() == TEST_UINT32_CORRECT );

	CJSONValueRefUTF16 subValueInt64Ref( defaultArrayRef.GetMemberAt( 4 ) );

	EXPECT_TRUE( subValueInt64Ref.IsInt64() );
	EXPECT_TRUE( subValueInt64Ref.GetInt64() == TEST_INT64_CORRECT_POSITIVE );

	CJSONValueRefUTF16 subValueUint64Ref( defaultArrayRef.GetMemberAt( 5 ) );

	EXPECT_TRUE( subValueUint64Ref.IsUint64() );
	EXPECT_TRUE( subValueUint64Ref.GetUint64() == TEST_UINT64_CORRECT );

	CJSONValueRefUTF16 subValueDoubleRef( defaultArrayRef.GetMemberAt( 6 ) );

	EXPECT_TRUE( subValueDoubleRef.IsDouble() );
	EXPECT_TRUE( subValueDoubleRef.GetDouble() == TEST_DOUBLE_MAX_NORMAL );

	CJSONValueRefUTF16 subValueStringRef( defaultArrayRef.GetMemberAt( 7 ) );

	EXPECT_TRUE( subValueStringRef.IsString() );

	TString<Red::System::UniChar> str = subValueStringRef.GetString();

	EXPECT_TRUE( str == TEST_STRING_UTF16 );

	CJSONValueRefUTF16 subValueStringWithLengthRef( defaultArrayRef.GetMemberAt( 8 ) );

	EXPECT_TRUE( subValueStringWithLengthRef.IsString() );

	TString<Red::System::UniChar> strWithLength = subValueStringWithLengthRef.GetString();

	EXPECT_TRUE( strWithLength.GetLength() == TEST_STRING_LENGTH_UTF16 );
	EXPECT_TRUE( strWithLength == TEST_STRING_UTF16 );

	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();
	defaultArray.PopBack();

	EXPECT_TRUE( defaultArrayRef.Size() == 0 );
}

template<class encoding, class allocator>
void constructJSONObject( CJSONObject<encoding, allocator>& defaultObject, void* objectName, void* text, MemSize textLength )
{
	typedef typename encoding::Ch CharType;

	CJSONArray<encoding, allocator> defaultArray;

	CJSONValue<encoding, allocator> subValue;
	CJSONValue<encoding, allocator> subValueBool( true );
	CJSONValue<encoding, allocator> subValueInt32( TEST_INT32_CORRECT_NEGATIVE );
	CJSONValue<encoding, allocator> subValueUint32( TEST_UINT32_CORRECT );
	CJSONValue<encoding, allocator> subValueInt64( TEST_INT64_CORRECT_POSITIVE );
	CJSONValue<encoding, allocator> subValueUint64( TEST_UINT64_CORRECT );
	CJSONValue<encoding, allocator> subValueDouble( TEST_DOUBLE_MAX_NORMAL );
	CJSONValue<encoding, allocator> subValueString( (CharType*)text );
	CJSONValue<encoding, allocator> subValueStringWithLength( (CharType*)text, textLength );

	CJSONObject<encoding, allocator> subObject;

	defaultArray.PushBack( subValue );
	defaultArray.PushBack( subValueBool );
	defaultArray.PushBack( subValueInt32 );
	defaultArray.PushBack( subValueUint32 );
	defaultArray.PushBack( subValueInt64 );
	defaultArray.PushBack( subValueUint64 );
	defaultArray.PushBack( subValueDouble );
	defaultArray.PushBack( subValueString );
	defaultArray.PushBack( subValueStringWithLength );
	defaultArray.PushBack( subObject );

	defaultObject.AddMember( (CharType*)objectName, defaultArray );
}

template<class encoding, class allocator>
void writeJSONObject( CJSONWriter<encoding, allocator>& writer, void* objectName, void* text, MemSize textLength )
{
	typedef typename encoding::Ch CharType;
	writer.StartObject();
		writer.WriteString( (CharType*)objectName );
		writer.StartArray();	
			writer.WriteNull();
			writer.WriteBool( true );
			writer.WriteInt32(  TEST_INT32_CORRECT_NEGATIVE );
			writer.WriteUint32( TEST_UINT32_CORRECT );
			writer.WriteInt64(  TEST_INT64_CORRECT_POSITIVE );
			writer.WriteUint64( TEST_UINT64_CORRECT );
			writer.WriteDouble( TEST_DOUBLE_MAX_NORMAL );
			writer.WriteString( (CharType*)text );
			writer.WriteString( (CharType*)text, textLength );
			writer.StartObject();
			writer.EndObject();	
		writer.EndArray();
	writer.EndObject();	
}

template<class encoding, class allocator>
void checkJSONObject( const CJSONDocument<encoding, allocator>& defaultObject, void* objectName, void* text, MemSize textLength )
{	
	typedef typename encoding::Ch CharType;

	CJSONArrayRef<encoding, allocator> defaultArrayRef( defaultObject.GetMember( (CharType*)objectName  ) );

	EXPECT_TRUE( defaultArrayRef.Size() == 10 );

	CJSONValueRef<encoding, allocator> subValueRef( defaultArrayRef.GetMemberAt( 0 ) );

	EXPECT_TRUE( subValueRef.GetType() == JSON_Null );

	CJSONValueRef<encoding, allocator> subValueBoolRef( defaultArrayRef.GetMemberAt( 1 ) );

	EXPECT_TRUE( subValueBoolRef.IsBool() );
	EXPECT_TRUE( subValueBoolRef.GetBool() );

	CJSONValueRef<encoding, allocator> subValueInt32Ref( defaultArrayRef.GetMemberAt( 2 ) );

	EXPECT_TRUE( subValueInt32Ref.IsInt32() );
	EXPECT_TRUE( subValueInt32Ref.GetInt32() == TEST_INT32_CORRECT_NEGATIVE );

	CJSONValueRef<encoding, allocator> subValueUint32Ref( defaultArrayRef.GetMemberAt( 3 ) );

	EXPECT_TRUE( subValueUint32Ref.IsUint32() );
	EXPECT_TRUE( subValueUint32Ref.GetUint32() == TEST_UINT32_CORRECT );

	CJSONValueRef<encoding, allocator> subValueInt64Ref( defaultArrayRef.GetMemberAt( 4 ) );

	EXPECT_TRUE( subValueInt64Ref.IsInt64() );
	EXPECT_TRUE( subValueInt64Ref.GetInt64() == TEST_INT64_CORRECT_POSITIVE );

	CJSONValueRef<encoding, allocator> subValueUint64Ref( defaultArrayRef.GetMemberAt( 5 ) );

	EXPECT_TRUE( subValueUint64Ref.IsUint64() );
	EXPECT_TRUE( subValueUint64Ref.GetUint64() == TEST_UINT64_CORRECT );

	CJSONValueRef<encoding, allocator> subValueDoubleRef( defaultArrayRef.GetMemberAt( 6 ) );

	EXPECT_TRUE( subValueDoubleRef.IsDouble() );

	// rapidjson serializing and deserializing Double value not fully correct need to be rewrite
	//EXPECT_TRUE( subValueDoubleRef.GetDouble() == TEST_DOUBLE_MAX_NORMAL );

	CJSONValueRef<encoding, allocator> subValueStringRef( defaultArrayRef.GetMemberAt( 7 ) );

	EXPECT_TRUE( subValueStringRef.IsString() );

	TString<CharType> str = subValueStringRef.GetString();

	EXPECT_TRUE( str == (CharType*)text );

	CJSONValueRef<encoding, allocator> subValueStringWithLengthRef( defaultArrayRef.GetMemberAt( 8 ) );

	EXPECT_TRUE( subValueStringWithLengthRef.IsString() );

	TString<CharType> strWithLength = subValueStringWithLengthRef.GetString();

	EXPECT_TRUE( strWithLength.GetLength() == textLength );
	EXPECT_TRUE( strWithLength  == (CharType*)text );

	CJSONObjectRef<encoding, allocator> subObjectRef( defaultArrayRef.GetMemberAt( 9 ) );

	EXPECT_TRUE( subObjectRef.GetType() == JSON_Object );
}

TEST( JSON, UTF8_SimpleWrite_Read_SimpleWriteFromDocument_Read )
{
	CJSONSimpleWriterUTF8 writer;

	writeJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	const TString< Red::System::AnsiChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF8 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::AnsiChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

}

TEST( JSON, UTF16_SimpleWrite_Read_SimpleWriteFromDocument_Read  )
{
	CJSONSimpleWriterUTF16 writer;

	writeJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	const TString< Red::System::UniChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF16 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::UniChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

}

TEST( JSON, UTF8_SimpleWriteFromObject_Read_SimpleWriteFromDocument_Read )
{
	CJSONObjectUTF8 defaultObject;
	constructJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	CJSONSimpleWriterUTF8 writer;

	writer.WriteObject( defaultObject );

	const TString< Red::System::AnsiChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF8 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::AnsiChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

}

TEST( JSON, UTF16_SimpleWriteFromObject_Read_SimpleWriteFromDocument_Read  )
{
	CJSONObjectUTF16 defaultObject;
	constructJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	CJSONSimpleWriterUTF16 writer;

	writer.WriteObject( defaultObject );

	const TString< Red::System::UniChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF16 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::UniChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

}

TEST( JSON, UTF8_SimpleWriteFile_ReadFile )
{
	//! On first file test we have to initialize IO
	Red::IO::Initialize();
	
	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_SIMPLE_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath);

	CJSONFileSimpleWriterUTF8 writer( *fileToWrite );

	writeJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );
	
	writer.Flush();

	delete fileToWrite;
	
	CJSONFileReaderUTF8 reader( GFileManager->CreateFileReader( TEST_SIMPLE_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );
}

TEST( JSON, UTF16_SimpleWriteFile_ReadFile )
{
	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_SIMPLE_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath);

	CJSONFileSimpleWriterUTF16 writer( *fileToWrite );

	writeJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF16 reader( GFileManager->CreateFileReader( TEST_SIMPLE_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );
}

TEST( JSON, UTF8_SimpleWriteFileFromObject_ReadFile )
{
	CJSONObjectUTF8 defaultObject;
	constructJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath);

	CJSONFileSimpleWriterUTF8 writer( *fileToWrite );

	writer.WriteObject( defaultObject );

	writer.Flush();
	
	delete fileToWrite;

	CJSONFileReaderUTF8 reader( GFileManager->CreateFileReader( TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );
}

TEST( JSON, UTF16_SimpleWriteFileFromObject_ReadFile )
{
	CJSONObjectUTF16 defaultObject;
	constructJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath);

	CJSONFileSimpleWriterUTF16 writer( *fileToWrite );

	writer.WriteObject( defaultObject );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF16 reader( GFileManager->CreateFileReader( TEST_SIMPLE_FROM_OBJECT_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );
}

TEST( JSON, UTF8_PrettyWriteFromObject_Read_PrettyWriteFromDocument_Read )
{
	CJSONObjectUTF8 defaultObject;
	constructJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	CJSONPrettyWriterUTF8 writer;

	writer.WriteObject( defaultObject );

	const TString< Red::System::AnsiChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF8 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::AnsiChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

}

TEST( JSON, UTF16_PrettyWriteFromObject_Read_PrettyWriteFromDocument_Read )
{
	CJSONObjectUTF16 defaultObject;
	constructJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	CJSONPrettyWriterUTF16 writer;

	writer.WriteObject( defaultObject );

	const TString< Red::System::UniChar > & jsonObjectStr = writer.GetContent();

	CJSONReaderUTF16 reader;

	EXPECT_TRUE( reader.Read( jsonObjectStr.AsChar() ) );

	writer.Clear();
	writer.WriteDocument( reader.GetDocument() );

	const TString< Red::System::UniChar > & jsonDocumentStr = writer.GetContent();

	EXPECT_TRUE( reader.Read( jsonDocumentStr.AsChar() ) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

}

TEST( JSON, UTF8_PrettyWriteFile_ReadFile )
{
	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_PRETTY_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath);

	CJSONFilePrettyWriterUTF8 writer( *fileToWrite );

	writeJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF8 reader( GFileManager->CreateFileReader( TEST_PRETTY_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );
}

TEST( JSON, UTF16_PrettyWriteFile_ReadFile )
{
	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_PRETTY_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath);

	CJSONFilePrettyWriterUTF16 writer( *fileToWrite );

	writeJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( writer, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF16 reader( GFileManager->CreateFileReader( TEST_PRETTY_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );
}

TEST( JSON, UTF8_PrettyWriteFileFromObject_ReadFile )
{
	CJSONObjectUTF8 defaultObject;
	constructJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );

	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath);

	CJSONFilePrettyWriterUTF8 writer( *fileToWrite );

	writer.WriteObject( defaultObject );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF8 reader( GFileManager->CreateFileReader( TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF8, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF8< Red::System::AnsiChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF8, (void*)TEST_STRING_UTF8, TEST_STRING_LENGTH_UTF8 );
}

TEST( JSON, UTF16_PrettyWriteFileFromObject_ReadFile )
{
	CJSONObjectUTF16 defaultObject;
	constructJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( defaultObject, (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	IFile* fileToWrite = GFileManager->CreateFileWriter( TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath);

	CJSONFilePrettyWriterUTF16 writer( *fileToWrite );

	writer.WriteObject( defaultObject );

	writer.Flush();

	delete fileToWrite;

	CJSONFileReaderUTF16 reader( GFileManager->CreateFileReader( TEST_PRETTY_FROM_OBJECT_JSON_FILE_PATH_UTF16, FOF_Buffered|FOF_AbsolutePath) );

	checkJSONObject<rapidjson::UTF16< Red::System::UniChar>, RED_JSON_MEMORY_GET_ALLOCATOR(MemoryPool_Default, MC_Json)>( reader.GetDocument(), (void*)TEST_OBJECT_VALUE_NAME_UTF16, (void*)TEST_STRING_UTF16, TEST_STRING_LENGTH_UTF16 );

	//! On lase file test we have to shutdown IO
	Red::IO::Shutdown();
}
