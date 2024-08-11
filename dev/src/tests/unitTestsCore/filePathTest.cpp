/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include  "build.h"

#include "../../common/core/filePath.h"

#if defined( RED_PLATFORM_WIN32 ) || defined( RED_PLATFORM_WIN64 ) || defined( RED_PLATFORM_DURANGO )
#	define SEPARATOR TXT( "\\" )
#elif defined( RED_PLATFORM_ORBIS )
#	define SEPARATOR TXT( "/" )
#else
#	error Undefined platform
#endif

#define MAKEPATH_1( a )			MACRO_TXT( a )
#define MAKEPATH_2( a, b )		MACRO_TXT( a ) SEPARATOR MACRO_TXT( b )

#define MAKEPATH_X( a, b )		a SEPARATOR b
#define MAKEPATH_3( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_2( __VA_ARGS__ ) )
#define MAKEPATH_4( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_3( __VA_ARGS__ ) )
#define MAKEPATH_5( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_4( __VA_ARGS__ ) )
#define MAKEPATH_6( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_5( __VA_ARGS__ ) )
#define MAKEPATH_7( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_6( __VA_ARGS__ ) )
#define MAKEPATH_8( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_7( __VA_ARGS__ ) )
#define MAKEPATH_9( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_8( __VA_ARGS__ ) )
#define MAKEPATH_10( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_9( __VA_ARGS__ ) )
#define MAKEPATH_11( a, ... )	MAKEPATH_X( MACRO_TXT( a ), MAKEPATH_10( __VA_ARGS__ ) )
// ...
// #define MAX_OF_64(a,...)   MAX_OF_2(a,MAX_OF_63(__VA_ARGS__))

// NUM_ARGS(...) evaluates to the literal number of the passed-in arguments.
#define NUM_ARGS3(X,X64,X63,X62,X61,X60,X59,X58,X57,X56,X55,X54,X53,X52,X51,X50,X49,X48,X47,X46,X45,X44,X43,X42,X41,X40,X39,X38,X37,X36,X35,X34,X33,X32,X31,X30,X29,X28,X27,X26,X25,X24,X23,X22,X21,X20,X19,X18,X17,X16,X15,X14,X13,X12,X11,X10,X9,X8,X7,X6,X5,X4,X3,X2,X1,N,...) N
#define NUM_ARGS2( args )	NUM_ARGS3 args 
#define NUM_ARGS(...)		NUM_ARGS2((0, __VA_ARGS__ ,64,63,62,61,60,59,58,57,56,55,54,53,52,51,50,49,48,47,46,45,44,43,42,41,40,39,38,37,36,35,34,33,32,31,30,29,28,27,26,25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1,0))

#define EXPANDVA( args )	args
#define ___MAKEPATH(N, a)	MAKEPATH_ ## N a
#define __MAKEPATH(N, ... )	___MAKEPATH( N, ( __VA_ARGS__ ) )
#define _MAKEPATH(N, ...)	__MAKEPATH( EXPANDVA( N ), __VA_ARGS__ )
#define MAKEPATH(...)		_MAKEPATH( NUM_ARGS( __VA_ARGS__ ), __VA_ARGS__ )

// Expected values
template< typename TChar >
struct PathValues
{
	const TChar* filename;
	const TChar* extension;
	const TChar* filenameWithExt;
	
	const TChar* path;
	const TChar* pathTrailingSeparator;

	const TChar* toString;

	Uint32 numDirectories;
	const TChar** directories;

	Bool isRelative;
};

template< typename TChar >
void TestDirectories( TDynArray< TString< TChar > >& directories, PathValues< TChar >& values )
{
	ASSERT_EQ( values.numDirectories, directories.Size() );

	for( Uint32 i = 0; i < values.numDirectories; ++i )
	{
		EXPECT_STREQ( values.directories[ i ], directories[ i ].AsChar() );
	}
}

template< typename TChar >
void TestPath( CFilePath& path, PathValues< TChar >& values )
{
	EXPECT_STREQ( values.extension, path.GetExtension().AsChar() );

	EXPECT_STREQ( values.filename, path.GetFileName().AsChar() );
	EXPECT_STREQ( values.filenameWithExt, path.GetFileNameWithExt().AsChar() );

	EXPECT_STREQ( values.path, path.GetPathString( false ).AsChar() );
	EXPECT_STREQ( values.pathTrailingSeparator, path.GetPathString( true ).AsChar() );

	EXPECT_STREQ( values.toString, path.ToString().AsChar() );

	TDynArray< TString< TChar > > directories1;
	path.GetDirectories( directories1 );
	TestDirectories( directories1, values );

	TDynArray< TString< TChar > > directories2 = path.GetDirectories();
	TestDirectories( directories2, values );

	EXPECT_EQ( values.isRelative, path.IsRelative() );
}

TEST( FilePath, Empty )
{
	CFilePath path;

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= String::EMPTY.AsChar();

	values.toString					= String::EMPTY.AsChar();

	values.numDirectories			= 0;
	values.directories				= nullptr;

	values.isRelative				= true;

	TestPath( path, values );
}

TEST( FilePath, AbsoluteDirectory )
{
	// The empty string at the end causes the path to end with a trailing slash
	const Char* input = MAKEPATH( "C:", "this", "is", "my", "boomstick", "" );

	CFilePath path( input );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "c:", "this", "is", "my", "boomstick" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "boomstick", "" );

	values.toString					= values.path;

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "boomstick" ) };
	values.numDirectories			= 4;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= newFilename;
	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boomstick", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boomstick", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, RelativeDirectory )
{
	// The empty string at the end causes the path to end with a trailing slash
	const Char* input = MAKEPATH( "..", "..", "this", "is", "my", "boomstick", "" );

	CFilePath path( input );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "..", "..", "this", "is", "my", "boomstick" );
	values.pathTrailingSeparator	= MAKEPATH( "..", "..", "this", "is", "my", "boomstick", "" );

	values.toString					= values.path;

	static const Char* directories[]= { TXT( ".." ), TXT( ".." ), TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "boomstick" ) };
	values.numDirectories			= 6;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= newFilename;
	values.toString					= MAKEPATH( "..", "..", "this", "is", "my", "boomstick", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "..", "..", "this", "is", "my", "boomstick", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, filename )
{
	const Char* input = MAKEPATH( "boomstick" );

	CFilePath path( input );

	PathValues< Char > values;

	values.filename					= input;
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= input;

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= String::EMPTY.AsChar();

	values.toString					= input;

	values.numDirectories			= 0;
	values.directories				= nullptr;

	values.isRelative				= true;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= newFilename;
	values.toString					= newFilename;

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, filenameWithExtension )
{
	CFilePath path( MAKEPATH( "boom.stick" ) );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boom" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "boom.stick" );

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= String::EMPTY.AsChar();

	values.toString					= MAKEPATH( "boom.stick" );

	values.numDirectories			= 0;
	values.directories				= nullptr;

	values.isRelative				= true;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= MAKEPATH( "groovy.stick" );
	values.toString					= MAKEPATH( "groovy.stick" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, AbsoluteDirectoryWithFilename )
{
	// absent the trailing slash, "boomstick" becomes the filename
	const Char* fullpath		= MAKEPATH( "C:", "this", "is", "my", "boomstick" );

	CFilePath path( fullpath );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boomstick" );
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= MAKEPATH( "boomstick" );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boomstick" );;

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= MAKEPATH( "groovy" );
	values.toString					= MAKEPATH( "c:", "this", "is", "my", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "c:", "this", "is", "my", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, RelativeDirectoryWithFilename )
{
	// absent the trailing slash, "boomstick" becomes the filename
	const Char* fullpath		= MAKEPATH( "..", "this", "is", "my", "boomstick" );

	CFilePath path( fullpath );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boomstick" );
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= MAKEPATH( "boomstick" );

	values.path						= MAKEPATH( "..", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "..", "this", "is", "my", "" );

	values.toString					= fullpath;

	static const Char* directories[]= { TXT( ".." ), TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 4;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= MAKEPATH( "groovy" );
	values.toString					= MAKEPATH( "..", "this", "is", "my", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "..", "this", "is", "my", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, AbsoluteDirectoryWithFilenamePosix )
{
	// The preceeding empty string prefixes the path with a slash
	// absent the trailing slash, "boomstick" becomes the filename
	const Char* fullpath		= MAKEPATH( "", "this", "is", "my", "boomstick" );

	CFilePath path( fullpath );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boomstick" );
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= MAKEPATH( "boomstick" );

	values.path						= MAKEPATH( "", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "", "this", "is", "my", "" );

	values.toString					= fullpath;

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= newFilename;
	values.toString					= MAKEPATH( "", "this", "is", "my", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "", "this", "is", "my", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, SambaDirectory )
{
	// The preceeding empty string prefixes the path with a slash
	// The empty string at the end causes the path to end with a trailing slash
	const Char* fullpath = MAKEPATH( "", "", "this", "is", "my", "boomstick", "" );

	CFilePath path( fullpath );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "", "", "this", "is", "my", "boomstick" );
	values.pathTrailingSeparator	= MAKEPATH( "", "", "this", "is", "my", "boomstick", "" );

	values.toString					= values.path;

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "boomstick" ) };
	values.numDirectories			= 4;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "groovy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= newFilename;
	values.toString					= MAKEPATH( "", "", "this", "is", "my", "boomstick", "groovy" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "groovy.baby" );
	values.toString					= MAKEPATH( "", "", "this", "is", "my", "boomstick", "groovy.baby" );

	TestPath( path, values );
}

TEST( FilePath, FilenameWithMultipleDots )
{
	const Char* fullpath = MAKEPATH( "this", "is", "my", "b.o.o.m.stick" );

	CFilePath path( fullpath );

	PathValues< Char > values;
	
	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= MAKEPATH( "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "my", "" );

	values.toString					= fullpath;

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	// Modify the filename and re-perform tests
	const Char* newFilename = TXT( "gr.o.o.vy" );
	path.SetFileName( newFilename );

	values.filename					= newFilename;
	values.filenameWithExt			= MAKEPATH( "gr.o.o.vy.stick" );
	values.toString					= MAKEPATH( "this", "is", "my", "gr.o.o.vy.stick" );

	TestPath( path, values );

	// Modify the extension and re-perform tests
	const Char* newExtension = TXT( "baby" );
	path.SetExtension( newExtension );

	values.extension				= newExtension;
	values.filenameWithExt			= MAKEPATH( "gr.o.o.vy.baby" );
	values.toString					= MAKEPATH( "this", "is", "my", "gr.o.o.vy.baby" );

	TestPath( path, values );
}

TEST( FilePath, Normalise )
{
	const Char* fullpath = MAKEPATH( "this", "is", "my", "big", "..", "b.o.o.m.stick" );

	CFilePath path( fullpath );
	path.Normalize();

	PathValues< Char > values;

	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= MAKEPATH( "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "my", "" );

	values.toString					= MAKEPATH( "this", "is", "my", "b.o.o.m.stick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );
}

TEST( FilePath, NormaliseComplex )
{
	const Char* fullpath = MAKEPATH( "..", "this", "..", "is", ".", "my", ".", "..", "big", "..", "b.o.o.m.stick" );

	CFilePath path( fullpath );
	path.Normalize();

	PathValues< Char > values;

	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= MAKEPATH( "..", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "..", "is", "" );

	values.toString					= MAKEPATH( "..", "is", "b.o.o.m.stick" );

	static const Char* directories[]= { TXT( ".." ), TXT( "is" ) };
	values.numDirectories			= 2;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );
}

TEST( FilePath, PopDirectoryWithFilename )
{
	const Char* fullpath = MAKEPATH( "this", "is", "my", "b.o.o.m.stick" );

	CFilePath path( fullpath );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	PathValues< Char > values;

	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= MAKEPATH( "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "" );

	values.toString					= MAKEPATH( "this", "is", "b.o.o.m.stick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ) };
	values.numDirectories			= 2;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= MAKEPATH( "this" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "" );

	values.toString					= MAKEPATH( "this", "b.o.o.m.stick" );

	values.numDirectories			= 1;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.filename					= MAKEPATH( "b.o.o.m" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "b.o.o.m.stick" );

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= MAKEPATH( "" );

	values.toString					= MAKEPATH( "b.o.o.m.stick" );

	values.numDirectories			= 0;
	values.directories				= nullptr;

	TestPath( path, values );
}

TEST( FilePath, PopDirectoryWithoutFilename )
{
	const Char* fullpath = MAKEPATH( "this", "is", "my", "b.o.o.m.stick", "" );

	CFilePath path( fullpath );

	//////////////////////////////////////////////////////////////////////////
	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "this", "is", "my", "b.o.o.m.stick" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "my", "b.o.o.m.stick", "" );

	values.toString					= MAKEPATH( "this", "is", "my", "b.o.o.m.stick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "b.o.o.m.stick" ) };
	values.numDirectories			= 4;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.path						= MAKEPATH( "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "my", "" );

	values.toString					= MAKEPATH( "this", "is", "my" );

	values.numDirectories			= 3;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.path						= MAKEPATH( "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is",  "" );

	values.toString					= MAKEPATH( "this", "is" );

	values.numDirectories			= 2;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.path						= MAKEPATH( "this" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "" );

	values.toString					= MAKEPATH( "this" );

	values.numDirectories			= 1;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PopDirectory();

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= String::EMPTY.AsChar();

	values.toString					= String::EMPTY.AsChar();

	values.numDirectories			= 0;

	TestPath( path, values );
}

TEST( FilePath, PushDirectory )
{
	CFilePath path;

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( TXT( "this" ) );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "this" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "" );

	values.toString					= MAKEPATH( "this" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 1;
	values.directories				= directories;

	values.isRelative				= true;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( TXT( "is" ) );

	values.path						= MAKEPATH( "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "" );

	values.toString					= MAKEPATH( "this", "is" );

	values.numDirectories			= 2;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( TXT( "my" ) );

	values.path						= MAKEPATH( "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "this", "is", "my", "" );

	values.toString					= MAKEPATH( "this", "is", "my" );

	values.numDirectories			= 3;

	TestPath( path, values );
}

TEST( FilePath, PushDirectoryDrive )
{
	CFilePath path( MAKEPATH( "C:", "" ) );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "this" ) );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "c:", "this" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "" );

	values.toString					= MAKEPATH( "c:", "this" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 1;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "is" ) );

	values.path						= MAKEPATH( "c:", "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "" );

	values.toString					= MAKEPATH( "c:", "this", "is" );

	values.numDirectories			= 2;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "my" ) );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my" );

	values.numDirectories			= 3;

	TestPath( path, values );
}

TEST( FilePath, PushDirectoryMultiple )
{
	CFilePath path;

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "C:", "this", "is" ) );

	PathValues< Char > values;

	values.filename					= String::EMPTY.AsChar();
	values.extension				= String::EMPTY.AsChar();
	values.filenameWithExt			= String::EMPTY.AsChar();

	values.path						= MAKEPATH( "c:", "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "" );

	values.toString					= MAKEPATH( "c:", "this", "is" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "boomstick" ) };
	values.numDirectories			= 2;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "my", "boomstick" ) );

	values.path						= MAKEPATH( "c:", "this", "is", "my", "boomstick" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "boomstick", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boomstick" );

	values.numDirectories			= 4;

	TestPath( path, values );
}

TEST( FilePath, PushDirectoryFilename )
{
	CFilePath path( MAKEPATH( "boom.stick" ) );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "C:", "this", "is" ) );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boom" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "boom.stick" );

	values.path						= MAKEPATH( "c:", "this", "is" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "boom.stick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ), TXT( "boomstick" ) };
	values.numDirectories			= 2;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );

	//////////////////////////////////////////////////////////////////////////
	path.PushDirectory( MAKEPATH( "my" ) );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boom.stick" );

	values.numDirectories			= 3;

	TestPath( path, values );
}

TEST( FilePath, FilenameContainedInExtensionRenamed )
{
	// The filename should be properly substituted when it is contained in the extension string.
	const Char* input = MAKEPATH( "c:", "this", "is", "my", "boom.boomstick" );

	CFilePath path( input );
	path.SetFileName( TXT( "new" ) );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "new" );
	values.extension				= MAKEPATH( "boomstick" );
	values.filenameWithExt			= MAKEPATH( "new.boomstick" );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", "new.boomstick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );
}

TEST( FilePath, NamelessFileWithExtension )
{
	// The filename should be properly substituted when it is contained in the extension string.
	const Char* input = MAKEPATH( "c:", "this", "is", "my", ".boomstick" );

	CFilePath path( input );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "" );
	values.extension				= MAKEPATH( "boomstick" );
	values.filenameWithExt			= MAKEPATH( ".boomstick" );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", ".boomstick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );
}

TEST( FilePath, NamelessFileWithExtensionRenamed )
{
	// The filename should be properly substituted when it is contained in the extension string.
	const Char* input = MAKEPATH( "c:", "this", "is", "my", ".stick" );

	CFilePath path( input );
	path.SetFileName( TXT( "boom" ) );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boom" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "boom.stick" );

	values.path						= MAKEPATH( "c:", "this", "is", "my" );
	values.pathTrailingSeparator	= MAKEPATH( "c:", "this", "is", "my", "" );

	values.toString					= MAKEPATH( "c:", "this", "is", "my", "boom.stick" );

	static const Char* directories[]= { TXT( "this" ), TXT( "is" ), TXT( "my" ) };
	values.numDirectories			= 3;
	values.directories				= directories;

	values.isRelative				= false;

	TestPath( path, values );
}

TEST( FilePath, NamelessFileWithExtensionRenamedNoDirs )
{
	// The filename should be properly substituted when it is contained in the extension string (special case with no dirs).
	const Char* input = MAKEPATH( ".stick" );

	CFilePath path( input );
	path.SetFileName( TXT( "boom" ) );

	PathValues< Char > values;

	values.filename					= MAKEPATH( "boom" );
	values.extension				= MAKEPATH( "stick" );
	values.filenameWithExt			= MAKEPATH( "boom.stick" );

	values.path						= String::EMPTY.AsChar();
	values.pathTrailingSeparator	= String::EMPTY.AsChar();

	values.toString					= MAKEPATH( "boom.stick" );

	values.numDirectories			= 0;
	values.directories				= nullptr;

	values.isRelative				= true;

	TestPath( path, values );
}
