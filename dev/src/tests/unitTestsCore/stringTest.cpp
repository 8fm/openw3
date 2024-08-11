#include "build.h"

#include "../../common/core/String.h"

TEST( String, Create )
{
	{
		String a( TXT( "A" ) );
		String b( a );

		EXPECT_TRUE( a == b );
	}

	{
		String a( TXT( "A" ) );
		String b = a;

		EXPECT_TRUE( a == b );
	}

	{
		const Char* a = TXT( "A" );
		String b = a;

		EXPECT_TRUE( b == a );
	}

	{
		const Char* a = TXT( "A" );
		String b;
		b.Set( a );

		EXPECT_TRUE( b == a );
	}

	{
		const Char* a = TXT( "A" );
		String b;
		b.Set( a, 1 );

		EXPECT_TRUE( b == a );
	}
}

TEST( String, LeftString )
{
	{
		String test( TXT( "AXXX" ) );
		EXPECT_TRUE( test.LeftString( 1 ) == TXT( "A" ) );
	}

	{
		String test( TXT( "ABXX" ) );
		EXPECT_TRUE( test.LeftString( 2 ) == TXT( "AB" ) );
	}

	{
		String test( TXT( "ABCD" ) );
		EXPECT_TRUE( test.LeftString( 4 ) == TXT( "ABCD" ) );
	}
}

TEST( String, RightString )
{
	{
		String test( TXT( "XXXA" ) );
		EXPECT_TRUE( test.RightString( 1 ) == TXT( "A" ) );
	}

	{
		String test( TXT( "XXAB" ) );
		EXPECT_TRUE( test.RightString( 2 ) == TXT( "AB" ) );
	}

	{
		String test( TXT( "ABCD" ) );
		EXPECT_TRUE( test.RightString( 4 ) == TXT( "ABCD" ) );
	}
}

TEST( String, MidString )
{
	{
		String test( TXT( "AXXX" ) );
		EXPECT_TRUE( test.MidString( 0, 1 ) == TXT( "A" ) );
	}

	{
		String test( TXT( "ABXX" ) );
		EXPECT_TRUE( test.MidString( 0, 2 ) == TXT( "AB" ) );
	}

	{
		String test( TXT( "ABCX" ) );
		EXPECT_TRUE( test.MidString( 0, 3 ) == TXT( "ABC" ) );
	}

	{
		String test( TXT( "ABCD" ) );
		EXPECT_TRUE( test.MidString( 0, 4 ) == TXT( "ABCD" ) );
	}

	{
		String test( TXT( "XAXX" ) );
		EXPECT_TRUE( test.MidString( 1, 1 ) == TXT( "A" ) );
	}

	{
		String test( TXT( "XABX" ) );
		EXPECT_TRUE( test.MidString( 1, 2 ) == TXT( "AB" ) );
	}

	{
		String test( TXT( "XABC" ) );
		EXPECT_TRUE( test.MidString( 1, 3 ) == TXT( "ABC" ) );
	}

	{
		String test( TXT( "XXXA" ) );
		EXPECT_TRUE( test.MidString( 3, 1 ) == TXT( "A" ) );
	}
}

TEST( String, FindSubstring )
{
	{
		String test( TXT( "AXXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index ) && index == 0 );
	}

	{
		String test( TXT( "AXXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index, true ) && index == 0 );
	}

	{
		String test( TXT( "AXXX" ) );
		size_t index;
		EXPECT_TRUE( !test.FindSubstring( TXT( "A" ), index, false, 1 ) && index == 0 );
	}

	{
		String test( TXT( "XAXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index ) && index == 1 );
	}

	{
		String test( TXT( "XXXA" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index ) && index == 3 );
	}

	{
		String test( TXT( "XABC" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "ABC" ), index ) && index == 1 );
	}

	{
		String test( TXT( "ABCD" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "ABCD" ), index ) && index == 0 );
	}

	{
		String test( TXT( "XAAX" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index ) && index == 1 );
	}

	{
		String test( TXT( "XAAX" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index, true ) && index == 2 );
	}

	{
		String test( TXT( "AXXA" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index ) && index == 0 );
	}

	{
		String test( TXT( "AXXA" ) );
		size_t index;
		EXPECT_TRUE( test.FindSubstring( TXT( "A" ), index, true )  && index == 3 );
	}
}

TEST( String, FindCharacter )
{
	// Left
	{
		String test( TXT( "AXXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index ) && index == 0 );
	}

	{
		String test( TXT( "XAXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index ) && index == 1 );
	}

	{
		String test( TXT( "XXXA" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index ) && index == 3 );
	}

	{
		String test( TXT( "XXXX" ) );
		size_t index;
		EXPECT_TRUE( !test.FindCharacter( L'A', index ) && index == 0 );
	}

	// Right
	{
		String test( TXT( "AXXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index, true ) && index == 0 );
	}

	{
		String test( TXT( "XAXX" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index, true ) && index == 1 );
	}

	{
		String test( TXT( "XXXA" ) );
		size_t index;
		EXPECT_TRUE( test.FindCharacter( L'A', index, true ) && index == 3 );
	}

	{
		String test( TXT( "XXXX" ) );
		size_t index;
		EXPECT_TRUE( !test.FindCharacter( L'A', index, true ) && index == 0 );
	}
}

TEST( String, GetTokens )
{
	// Check Count
	{
		String test( TXT( "A" ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', false, tokens ) == 1 );
	}
		
	{
		String test( TXT( "A,B" ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', false, tokens ) == 2 );
	}

	{
		String test( TXT( "A,B," ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', false, tokens ) == 2 );
	}

	// Unique
	{
		String test( TXT( "A,A" ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', false, tokens ) == 2 );
	}

	{
		String test( TXT( "A,A" ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', true, tokens ) == 1 );
	}

	// Check Tokens
	{
		String test( TXT( "A,BC,DEF,GHIJ" ) );
		TDynArray< String > tokens;

		EXPECT_TRUE( test.GetTokens( L',', false, tokens ) == 4 );
		EXPECT_TRUE( tokens[ 0 ] == TXT( "A" ) );
		EXPECT_TRUE( tokens[ 1 ] == TXT( "BC" ) );
		EXPECT_TRUE( tokens[ 2 ] == TXT( "DEF" ) );
		EXPECT_TRUE( tokens[ 3 ] == TXT( "GHIJ" ) );
	}
}

TEST( String, MatchAny )
{
	// No filters
	{
		String test( TXT( "XXXX" ) );
		TDynArray< String > filters;

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	// Positions
	{
		String test( TXT( "AXXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	{
		String test( TXT( "XAXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	{
		String test( TXT( "XXXA" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	// Multiple instances
	{
		String test( TXT( "AAAA" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	// Multiple filters
	{
		String test( TXT( "XXXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == false );
	}

	{
		String test( TXT( "XAXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	{
		String test( TXT( "XXBX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}

	{
		String test( TXT( "XABX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAny( filters ) == true );
	}
}

TEST( String, MatchAll )
{
	// No filters
	{
		String test( TXT( "XXXX" ) );
		TDynArray< String > filters;

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	// Positions
	{
		String test( TXT( "AXXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	{
		String test( TXT( "XAXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	{
		String test( TXT( "XXXA" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	// Multiple instances
	{
		String test( TXT( "AAAA" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	// Multiple filters
	{
		String test( TXT( "XXXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == false );
	}

	{
		String test( TXT( "XAXX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == false );
	}

	{
		String test( TXT( "XXBX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == false );
	}

	{
		String test( TXT( "XABX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	{
		String test( TXT( "AXXB" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "A" ) );
		filters.PushBack( TXT( "B" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	{
		String test( TXT( "ABXXCD" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "AB" ) );
		filters.PushBack( TXT( "CD" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == true );
	}

	{
		String test( TXT( "AXBXCXDX" ) );
		TDynArray< String > filters;
		filters.PushBack( TXT( "AB" ) );
		filters.PushBack( TXT( "CD" ) );

		EXPECT_TRUE( test.MatchAll( filters ) == false );
	}
}

TEST( String, Replace )
{
	// Nothing to replace
	{
		String test( TXT( "XXXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == false );
	}

	// Edges
	{
		String test( TXT( "AXXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BXXX" ) );
	}

	{
		String test( TXT( "XAXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "XBXX" ) );
	}

	{
		String test( TXT( "XXXA" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "XXXB" ) );
	}

	// Multiple instances
	{
		String test( TXT( "AAXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BAXX" ) );
	}

	{
		String test( TXT( "AXXA" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BXXA" ) );
	}

	{
		String test( TXT( "AAAA" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BAAA" ) );
	}

	{
		String test( TXT( "ABXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BBXX" ) );
	}

	{
		String test( TXT( "BAXX" ) );

		EXPECT_TRUE( test.Replace( TXT( "A" ), TXT( "B" ) ) == true );
		EXPECT_TRUE( test == TXT( "BBXX" ) );
	}
}

TEST( String, ReplaceAll )
{
	{
		String test( TXT( "A" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "B" ) );
	}

	{
		String test( TXT( "AA" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "BB" ) );
	}

	{
		String test( TXT( "AAA" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "BBB" ) );
	}

	{
		String test( TXT( "AXXX" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "BXXX" ) );
	}

	{
		String test( TXT( "XAXX" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "XBXX" ) );
	}

	{
		String test( TXT( "AXXA" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "BXXB" ) );
	}

	{
		String test( TXT( "XAAX" ) );
		test.ReplaceAll( TXT( "A" ), TXT( "B" ) );

		EXPECT_TRUE( test == TXT( "XBBX" ) );
	}

	{
		String test( TXT( "XAAX" ) );
		test.ReplaceAll( TXT( "AA" ), TXT( "BB" ) );

		EXPECT_TRUE( test == TXT( "XBBX" ) );
	}

	{
		String test( TXT( "AAXX" ) );
		test.ReplaceAll( TXT( "AA" ), TXT( "BB" ) );

		EXPECT_TRUE( test == TXT( "BBXX" ) );
	}

	{
		String test( TXT( "AXXA" ) );
		test.ReplaceAll( TXT( "AA" ), TXT( "BB" ) );

		EXPECT_TRUE( test == TXT( "AXXA" ) );
	}
}
