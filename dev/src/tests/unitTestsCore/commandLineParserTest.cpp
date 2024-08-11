/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/commandLineParser.h"

typedef THashMap< String, CCommandLineParser::Params > TOptionMap;

void TestCommandLine( const CCommandLineParser& parser, const TOptionMap& testData )
{
	EXPECT_EQ( testData.Size(), parser.GetNumberOfOptions() );

	for( auto iter = testData.Begin(); iter != testData.End(); ++iter )
	{
		EXPECT_TRUE( parser.HasOption( iter.Key() ) );

		const CCommandLineParser::Params& expected = iter.Value();
		const CCommandLineParser::Params& actual = parser.GetValues( iter.Key() );

		EXPECT_EQ( expected.Size(), actual.Size() );

		for( Uint32 i = 0; i < expected.Size(); ++i )
		{
			EXPECT_STREQ( expected[ i ].AsChar(), actual[ i ].AsChar() );
		}
	}
}

TEST( CommandLineParser, empty )
{
	CCommandLineParser parser( String::EMPTY );

	EXPECT_EQ( 0, parser.GetNumberOfOptions() );
}

TEST( CommandLineParser, posix_style_1_0 )
{
	CCommandLineParser parser( TXT( "-one" ) );

	TOptionMap map;

	map.GetRef( TXT( "one" ) );

	TestCommandLine( parser, map );
}

TEST( CommandLineParser, posix_style_1_1 )
{
	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "-one=paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "-one     \t=paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "-one=          \t   paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "-one\t                   =          \t   paramA" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, posix_style_2_1_0 )
{
	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );
	map.GetRef( TXT( "two" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "-one=paramA -two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "-one     \t=paramA           -two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "-one=          \t   paramA \t-two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "-one\t                   =          \t   paramA\t-two" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, posix_style_2_1quotes )
{
#define CLPT_PARAM TXT( "this is a very long param with -=_:/\\ multiple symbols" )
#define CLPT_PARAMQ TXT( "\"" ) CLPT_PARAM TXT( "\"" )

	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( CLPT_PARAM );
	map.GetRef( TXT( "two" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "-one=" ) CLPT_PARAMQ TXT( " -two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "-one               \t=" ) CLPT_PARAMQ TXT( "       -two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "-one=          \t" ) CLPT_PARAMQ TXT( "  -two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "-one     \t       =         \t  " ) CLPT_PARAMQ TXT( " -two" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, posix_style_2_4_2 )
{
#define CLPT_PARAM TXT( "this is a very long param with -=_:/\\ multiple symbols" )
#define CLPT_PARAMQ TXT( "\"" ) CLPT_PARAM TXT( "\"" )

	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );
	params.PushBack( TXT( "paramB" ) );
	params.PushBack( CLPT_PARAM );
	params.PushBack( TXT( "paramD" ) );
	CCommandLineParser::Params& params2 = map.GetRef( TXT( "two" ) );
	params2.PushBack( TXT( "paramA" ) );
	params2.PushBack( TXT( "paramB" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "-one=" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " -two=paramA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "-one         \t\t     =" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " -two                \t\t\t=paramA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "-one=                   \t\t\t" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " -two=             \t\t\tparamA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "-one              \t=        \t\t     " ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " -two        \t= \t       paramA paramB" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, windows_style_1_0 )
{
	CCommandLineParser parser( TXT( "/one" ) );

	TOptionMap map;

	map.GetRef( TXT( "one" ) );

	TestCommandLine( parser, map );
}

TEST( CommandLineParser, windows_style_1_1 )
{
	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "/one=paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "/one     \t=paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "/one=          \t   paramA" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "/one\t                   =          \t   paramA" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, windows_style_2_1_0 )
{
	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );
	map.GetRef( TXT( "two" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "/one=paramA /two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "/one     \t=paramA           /two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "/one=          \t   paramA \t/two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "/one\t                   =          \t   paramA\t/two" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, windows_style_2_1quotes )
{
#define CLPT_PARAM TXT( "this is a very long param with -=_:/\\ multiple symbols" )
#define CLPT_PARAMQ TXT( "\"" ) CLPT_PARAM TXT( "\"" )

	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( CLPT_PARAM );
	map.GetRef( TXT( "two" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "/one=" ) CLPT_PARAMQ TXT( " /two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "/one               \t=" ) CLPT_PARAMQ TXT( "       /two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "/one=          \t" ) CLPT_PARAMQ TXT( "  /two" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "/one     \t       =         \t  " ) CLPT_PARAMQ TXT( " /two" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, windows_style_2_4_2 )
{
#define CLPT_PARAM TXT( "this is a very long param with -=_:/\\ multiple symbols" )
#define CLPT_PARAMQ TXT( "\"" ) CLPT_PARAM TXT( "\"" )

	TOptionMap map;
	CCommandLineParser::Params& params = map.GetRef( TXT( "one" ) );
	params.PushBack( TXT( "paramA" ) );
	params.PushBack( TXT( "paramB" ) );
	params.PushBack( CLPT_PARAM );
	params.PushBack( TXT( "paramD" ) );
	CCommandLineParser::Params& params2 = map.GetRef( TXT( "two" ) );
	params2.PushBack( TXT( "paramA" ) );
	params2.PushBack( TXT( "paramB" ) );

	// No whitespace
	{
		CCommandLineParser parser( TXT( "/one=" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " /two=paramA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before the assignment
	{
		CCommandLineParser parser( TXT( "/one         \t\t     =" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " /two                \t\t\t=paramA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace after the assignment
	{
		CCommandLineParser parser( TXT( "/one=                   \t\t\t" ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " /two=             \t\t\tparamA paramB" ) );

		TestCommandLine( parser, map );
	}

	// Whitespace before and after the assignment
	{
		CCommandLineParser parser( TXT( "/one              \t=        \t\t     " ) TXT( "paramA paramB " ) CLPT_PARAMQ TXT( " paramD" ) TXT( " /two        \t= \t       paramA paramB" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, skip_executable )
{
	{
		CCommandLineParser parser( TXT( "C:\\some\\path\\..\\.\\..\\To\\An\\executable\\file.exe -optionA -optionB=stuff" ) );

		TOptionMap map;
		map.GetRef( TXT( "optionA" ) );
		CCommandLineParser::Params& params = map.GetRef( TXT( "optionB" ) );
		params.PushBack( TXT( "stuff" ) );

		TestCommandLine( parser, map );
	}

	{
		CCommandLineParser parser( TXT( "\"C:\\some\\path\\..\\.\\..\\To\\An\\executable\\file.exe\" -optionA -optionB=stuff" ) );

		TOptionMap map;
		map.GetRef( TXT( "optionA" ) );
		CCommandLineParser::Params& params = map.GetRef( TXT( "optionB" ) );
		params.PushBack( TXT( "stuff" ) );

		TestCommandLine( parser, map );
	}
}

TEST( CommandLineParser, easy_access_parameters )
{
	{
		CCommandLineParser parser( TXT( "-one=1 -two=2 -three=three" ) );

		Uint32 a;
		EXPECT_TRUE( parser.GetFirstParam( TXT( "one" ), a ) );
		EXPECT_EQ( 1, a );

		Uint32 b;
		EXPECT_TRUE( parser.GetFirstParam( TXT( "two" ), b ) );
		EXPECT_EQ( 2, b );

		String c;
		EXPECT_TRUE( parser.GetFirstParam( TXT( "three" ), c ) );
		EXPECT_STREQ( TXT( "three" ), c.AsChar() );
	}
}
