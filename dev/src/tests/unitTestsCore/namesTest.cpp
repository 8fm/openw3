#include "build.h"

#include "../../common/core/names.h"
#include "../../common/core/namesPool.h"

static CName testName1(TXT("Test123"));
static CName testName2(TXT("TestABC"));
 
	TEST( Names, None_Identity )
	{
		EXPECT_TRUE( CName::NONE == CName::NONE );
	}

	TEST( Names, None_Identity2 )
	{
		EXPECT_EQ( false, CName::NONE != CName::NONE );
	}

	TEST( Names, None_SelfOrder )
	{
		EXPECT_EQ( false, CName::NONE < CName::NONE );
	}

	TEST( Names, None_SelfOrder2 )
	{
		EXPECT_EQ( false, CName::NONE > CName::NONE );
	}

	TEST( Names, None_Order_None_Lowest )
	{
		EXPECT_TRUE( CName::NONE < testName1 );
	}

	TEST( Names, None_Order_None_Lowest2 )
	{
		EXPECT_EQ( false, testName1 < CName::NONE );
	}

	TEST( Names, None_Order_None_Lowest3 )
	{
		EXPECT_TRUE( testName1 > CName::NONE );
	}

	TEST( Names, None_Order_None_Lowest4 )
	{
		EXPECT_EQ( false, CName::NONE > testName1 );
	}

	TEST( Names, Name_None_DefaultCtor )
	{
		CName none;
		EXPECT_TRUE( CName::NONE == none );
	}

	TEST( Names, Name_None_WideChar )
	{
		EXPECT_TRUE( CName::NONE == CName(L"None") );
		EXPECT_TRUE( CName::NONE == L"None" );

	}

	TEST( Names, Name_None_Equals_AnsiChar )
	{
		EXPECT_TRUE( CName::NONE == CName(ANSI_TO_UNICODE("None")) );
		EXPECT_TRUE( CName::NONE == ANSI_TO_UNICODE("None") );
	}

	TEST( Names, Name_None_Equals_NoneHash )
	{
		EXPECT_TRUE( CName::NONE == CName::CreateFromHash( Red::CNameHash::Hash("None")) );
		EXPECT_TRUE( CName::NONE == CName::CreateFromHash( Red::CNameHash::Hash(UNICODE_TO_ANSI(TXT("None")))) );
	}

	TEST( Names, Name_None_NegativeTest_Equals_NoneHash )
	{
		// Create with names to avoid assert
		CName name1(TXT("NONE"));
		CName name2(TXT("none"));

		EXPECT_EQ( false, CName::NONE == CName::CreateFromHash(Red::CNameHash::Hash("NONE")) );
		EXPECT_EQ( false, CName::NONE == CName::CreateFromHash(Red::CNameHash::Hash(UNICODE_TO_ANSI(TXT("NONE")))) );
		EXPECT_EQ( false, CName::NONE == CName::CreateFromHash(Red::CNameHash::Hash("none")) );
		EXPECT_EQ( false, CName::NONE == CName::CreateFromHash(Red::CNameHash::Hash(UNICODE_TO_ANSI(TXT("none")))) );
	}

	TEST( Names, Name_None_Equals_EmptyWideChar )
	{
		EXPECT_TRUE( CName::NONE == CName(TXT("")) );
		EXPECT_TRUE( CName::NONE == TXT("") );
	}

	TEST( Names, Name_None_Equals_EmptyChar )
	{
		EXPECT_TRUE( CName::NONE == CName(ANSI_TO_UNICODE("")));
		EXPECT_TRUE( CName::NONE == ANSI_TO_UNICODE("") );
	}

	TEST( Names, Name_None_Equals_EmptyString )
	{
		EXPECT_TRUE( CName::NONE == CName(String::EMPTY));
		EXPECT_TRUE( CName::NONE == String::EMPTY.AsChar() );
	}

	TEST( Names, Name_None_EqualsEmptyString2 )
	{
		const String str(TXT(""));
		EXPECT_TRUE( CName::NONE == CName(str));
		EXPECT_TRUE( CName::NONE == str.AsChar() );
	}

	TEST( Names, Name_None_EqualsEmptyString3 )
	{
		const String str;

		EXPECT_TRUE( CName::NONE == CName(str));
		EXPECT_TRUE( CName::NONE == str.AsChar() );
	}

	TEST( Names, Name_None_Equals_NullPtr )
	{
		EXPECT_TRUE( CName::NONE == CName(nullptr));
		EXPECT_TRUE( CName::NONE == nullptr );
	}

	TEST( Names, Name_None_Equals_InvalidHash )
	{
		EXPECT_TRUE( CName::NONE == CName::CreateFromHash(Red::CNameHash()) );
		EXPECT_TRUE( CName::NONE.GetSerializationHash() == Red::CNameHash() );
		EXPECT_TRUE( CName::NONE.GetIndex() == CNamesPool::INDEX_NONE );
	}

	TEST( Names, Name_None_Empty )
	{
		EXPECT_TRUE( CName::NONE.Empty() );
	}

	TEST( Names, Name_None_NegativeTest_Case )
	{
		CName notNone1( TXT("NONE") );
		CName notNone2( TXT("none") );
		CName notNone3( TXT("NoNE") );
		CName notNone4( ANSI_TO_UNICODE("NONE") );
		CName notNone5( ANSI_TO_UNICODE("none") );
		CName notNone6( ANSI_TO_UNICODE("NoNE") );

		EXPECT_TRUE( CName::NONE != notNone1 );
		EXPECT_TRUE( CName::NONE != notNone2 );
		EXPECT_TRUE( CName::NONE != notNone3 );
		EXPECT_TRUE( CName::NONE != notNone4 );
		EXPECT_TRUE( CName::NONE != notNone5 );
		EXPECT_TRUE( CName::NONE != notNone6 );
	}

	TEST( Names, Name_None_SelfAssign )
	{
		CName none = CName::NONE;
		none = none;
		EXPECT_TRUE( CName::NONE == none );
	}

	TEST( Names, Name_None_Update )
	{
		CName none;
		EXPECT_TRUE( none == CName::NONE );
		none = testName1;
		EXPECT_TRUE( none != CName::NONE );
		EXPECT_TRUE( none == testName1 );
	}

	TEST( Names, Name_CopyCtor )
	{
		CName test(testName1);
		EXPECT_TRUE( test == testName1 );
	}

	TEST( Names, OperatorNot_None )
	{
		EXPECT_TRUE( ! CName::NONE );
	}

	TEST( Names, OperatorNot_Test1 )
	{
		EXPECT_EQ( false, !testName1 );
	}

	TEST( Names, Name_Identity )
	{
		EXPECT_TRUE( testName1 == testName1 );
		EXPECT_EQ( false, testName1 == testName2 );
	}

	TEST( Names, Name_Identity2 )
	{
		EXPECT_EQ( false, testName1 != testName1 );
		EXPECT_TRUE( testName1 != testName2 );
	}

	TEST( Names, Name_SelfOrder )
	{
		EXPECT_EQ( false, testName1 < testName1 );
		EXPECT_EQ( false, testName2 < testName2 );
	}

	TEST( Names, Name_SelfOrder2 )
	{
		EXPECT_EQ( false, testName1 > testName1 );
		EXPECT_EQ( false, testName2 > testName2 );
	}

	TEST( Names, Name_SomeOrder )
	{
		EXPECT_TRUE( ( testName1 < testName2 ) ^ ( testName2 < testName1 ) );
	}

	TEST( Names, Name_SomeOrder2 )
	{
		EXPECT_TRUE( ( testName1 > testName2 ) ^ ( testName2 > testName1 ) );
	}


	TEST( Names, NoCollision_NonAnsiVsAnsiChars )
	{
		const Red::CNameHash hash1 = Red::CNameHash::Hash( TXT("Pomog\x0119 wam.") );
		const Red::CNameHash hash2 = Red::CNameHash::Hash( TXT("Pomoge wam.") );
		EXPECT_TRUE( hash1 != hash2 );
	}

	TEST( Names, SameHash_Ansi_Vs_Uni )
	{
		const Red::CNameHash hash1 = Red::CNameHash::Hash( TXT("Hello") );
		const Red::CNameHash hash2 = Red::CNameHash::Hash( "Hello" );
		EXPECT_EQ( hash1.GetValue(), hash2.GetValue() );
	}

	TEST( Names, SameHash_Empty_Ansi_Vs_Uni )
	{
		const Red::CNameHash hash1 = Red::CNameHash::Hash( TXT("") );
		const Red::CNameHash hash2 = Red::CNameHash::Hash( "" );

		EXPECT_EQ( hash1.GetValue(), hash2.GetValue() );
	}

	TEST( Names, SameHash_PlatformCheck )
	{
		const Red::CNameHash hash1 = Red::CNameHash::Hash( TXT("Hello") );
		const Red::CNameHash hash2 = Red::CNameHash::Hash( "Hello" );
		const Red::CNameHash::TValue expectedHash = 2334234897;

		EXPECT_EQ( expectedHash, hash1.GetValue() );
		EXPECT_EQ( expectedHash, hash2.GetValue() );
	}

	TEST( Names, SameHash_PlatformCheck_Ansi_Vs_Uni )
	{
		const Red::CNameHash hash1 = Red::CNameHash::Hash( TXT("Pomog\x0119 wam.") );
		const Red::CNameHash hash2 = Red::CNameHash::Hash( TXT("Pomoge wam.") );
		const Red::CNameHash::TValue expectedHash1 = 3036239398;
		const Red::CNameHash::TValue expectedHash2 = 1802867389;

		EXPECT_EQ( expectedHash1, hash1.GetValue() );
		EXPECT_EQ( expectedHash2, hash2.GetValue() );
	}
