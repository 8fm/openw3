/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "options.h"

#include <direct.h>
#include <sys/stat.h>
#include <errno.h>

#include "../../common/redSystem/ttyWriter.h"
#include "../../common/redSystem/logFile.h"

#include "../../common/core/commandLineParser.h"
#include "../../common/core/fileSys.h"

// Command Line Parameters
#define CLP_DEFINITIONFILE		"definition"
#define CLP_DEPOTPATH			"depotpath"
#define CLP_OUTPUTDIR			"outputdir"
#define CLP_COOKEDPATH			"cookedpath"

#define CLP_SINGLETHREADED		"singlethreaded"
#define CLP_THREADCOUNT			"numthreads"

#define CLP_VERBOSE				"verbose"
#define CLP_LOGTOCONSOLE		"logtoconsole"
#define CLP_LOGTOFILE			"logtofile"
#define CLP_PROFILE				"profile"
#define CLP_ITERATIONS			"iterations"
#define CLP_FORCECOMPRESSION	"forcecompression"
#define CLP_AUTOCACHE			"autocache"
#define CLP_BUILDAUTOCACHE		"buildautocache"
#define CLP_INCREMENTALCACHE	"incrementalcache"
#define CLP_MINIMUMRATIO		"mincompressionratio"

namespace Bundler {

namespace Utility {

RED_INLINE int RedStat( const AnsiChar* path, struct _stat* buffer ) { return _stat( path, buffer ); }
RED_INLINE int RedStat( const UniChar* path, struct _stat* buffer ) { return _wstat( path, buffer ); }
RED_INLINE int RedMkDir( const AnsiChar* path ) { return _mkdir( path ); }
RED_INLINE int RedMkDir( const UniChar* path ) { return _wmkdir( path ); }

//////////////////////////////////////////////////////////////////////////
// Utility functions
//////////////////////////////////////////////////////////////////////////
void CreateDirectory( const AnsiChar* root, const AnsiChar* path )
{
	const size_t rootLength					= Red::System::StringLength( root );
	const size_t pathLengthWithTerminator	= Red::System::StringLength( path ) + 1;

	const size_t lengthWithTerminator = ( rootLength + pathLengthWithTerminator );
	AnsiChar* pathToMake = static_cast< AnsiChar* >( RED_ALLOCA( sizeof( AnsiChar ) * lengthWithTerminator ) );

	Red::System::StringCopy( pathToMake, root, lengthWithTerminator );
	size_t remainingSpace = lengthWithTerminator - rootLength;

	const AnsiChar* readPosition	= path;
	AnsiChar* appendPosition		= pathToMake + rootLength;

	const AnsiChar dividers[] = { DIRECTORY_SEPARATOR_LITERAL, ALTERNATIVE_DIRECTORY_SEPARATOR_LITERAL, '\0' };

	const AnsiChar* directoryEnd = nullptr;
	while( directoryEnd = Red::System::StringSearchSet( readPosition, dividers ) )
	{
		size_t directoryLength = directoryEnd - readPosition;
		Red::System::StringCopy( appendPosition, readPosition, remainingSpace, directoryLength );
		remainingSpace -= directoryLength;

		RedMkDir( pathToMake );

		readPosition	+= directoryLength + 1;
		appendPosition	+= directoryLength;

		Red::System::StringCopy( appendPosition, DIRECTORY_SEPARATOR_LITERAL_STRING, remainingSpace );
		--remainingSpace;
		++appendPosition;
	}
}

Bool ValidateDirectory( StringAnsi& directory )
{
	Uint32 length = directory.GetLength();
	Uint32 size = length + 1;

	AnsiChar* pathWithoutTrailingSlash = static_cast< AnsiChar* >( RED_ALLOCA( ( size ) * sizeof( AnsiChar ) ) );
	Red::System::StringCopy( pathWithoutTrailingSlash, directory.AsChar(), size );

	Bool isDirSep = CFileManager::IsDirectorySeparator( pathWithoutTrailingSlash[ length - 1 ] );
	if( isDirSep )
	{
		pathWithoutTrailingSlash[ length - 1 ] = '\0';
	}
	else
	{
		// Add a trailing slash to the original
		directory += DIRECTORY_SEPARATOR_LITERAL_STRING;
	}

	struct _stat buffer;

	if( RedStat( pathWithoutTrailingSlash, &buffer ) != 0 )
	{
		return false;
	}

	return true;
}

} // namespace Utility {

//////////////////////////////////////////////////////////////////////////
// Command line parameters
//////////////////////////////////////////////////////////////////////////
COptions::COptions()
:	m_numTestIterations( 1 )
,	m_numThreads( 0 )
,	m_singleThreaded( false )
,	m_silent( true )
,	m_profile( false )
,	m_ignoreSizeLimit( false )
,	m_compressionOverride( false )
,	m_compressionOverrideType( Red::Core::Bundle::CT_Default )
,	m_useAutoCache( false )
,	m_buildAutoCache( false )

,	m_ttyDevice( nullptr )
,	m_fileDevice( nullptr )

,	m_hasError( false )
,	m_minimumCompressionRatio( 0.95f )
{

}

COptions::~COptions()
{
	if( m_fileDevice )
	{
		delete m_fileDevice;
	}

	if( m_ttyDevice )
	{
		delete m_ttyDevice;
	}
}

void COptions::Parse( Int32 argc, const AnsiChar* argv[] )
{
	m_hasError = false;
	m_error.Clear();

	CCommandLineParser parser( argc, argv );

	// Required Parameters
	if( !parser.GetFirstParam( MACRO_TXT( CLP_DEFINITIONFILE ), m_definitionFilename ) )
	{
		m_hasError = true;
		m_error += "-" CLP_DEFINITIONFILE " not specified\n";
	}

	if( !parser.GetFirstParam( MACRO_TXT( CLP_DEPOTPATH ), m_depotPath ) )
	{
		m_hasError = true;
		m_error += "-" CLP_DEPOTPATH " not specified\n";
	}

	if( !parser.GetFirstParam( MACRO_TXT( CLP_OUTPUTDIR ), m_outputDir ) )
	{
		m_hasError = true;
		m_error += "-" CLP_OUTPUTDIR " not specified\n";
	}

	if( !parser.GetFirstParam( MACRO_TXT( CLP_COOKEDPATH ), m_cookedPath ) )
	{
		m_hasError = true;
		m_error += "-" CLP_COOKEDPATH " not specified\n";
	}

	// Optional parameters
	m_singleThreaded	= parser.HasOption( MACRO_TXT( CLP_SINGLETHREADED ) );
	m_silent			=!parser.HasOption( MACRO_TXT( CLP_VERBOSE ) );
	m_profile			= parser.HasOption( MACRO_TXT( CLP_PROFILE ) );
	m_buildAutoCache	= parser.GetFirstParam( MACRO_TXT( CLP_BUILDAUTOCACHE ), m_autoCachePath );
	parser.GetFirstParam( MACRO_TXT( CLP_INCREMENTALCACHE ), m_baseAutoCachePath );

	if( !m_buildAutoCache )
	{
		m_useAutoCache = parser.GetFirstParam( MACRO_TXT( CLP_AUTOCACHE ), m_autoCachePath );
	}

	if( parser.HasOption( MACRO_TXT( CLP_THREADCOUNT ) ) )
	{
		StringAnsi countParam;
		parser.GetFirstParam( MACRO_TXT( CLP_THREADCOUNT ), countParam );

		if( countParam == "auto" )
		{
			m_numThreads = 0;
		}
		else if( !parser.GetFirstParam( MACRO_TXT( CLP_THREADCOUNT ), m_numThreads ) )
		{
			m_hasError = true;

			m_error += "Invalid number of threads specified\n";
			m_error += "-" CLP_THREADCOUNT " requires a positive whole number, or \"auto\"\n";
		}
	}

	if( parser.HasOption( MACRO_TXT( CLP_ITERATIONS ) ) )
	{
		if( !parser.GetFirstParam( MACRO_TXT( CLP_ITERATIONS ), m_numTestIterations ) )
		{
			m_hasError = true;

			m_error += "-" CLP_ITERATIONS " requires a positive whole number\n";
		}
	}

	if( parser.HasOption( MACRO_TXT( CLP_FORCECOMPRESSION ) ) )
	{
		StringAnsi compressionType;

		if( parser.GetFirstParam( MACRO_TXT( CLP_FORCECOMPRESSION ), compressionType ) )
		{
			m_compressionOverride = true;

			if( compressionType == "auto" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_Auto;
			}
			else if( compressionType == "zlib" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_Zlib;
			}
			else if( compressionType == "snappy" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_Snappy;
			}
			else if( compressionType == "doboz" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_Doboz;
			}
			else if( compressionType == "lz4" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_LZ4;
			}
			else if( compressionType == "lz4hc" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_LZ4HC;
			}
			else if( compressionType == "none" )
			{
				m_compressionOverrideType = Red::Core::Bundle::CT_Uncompressed;
			}
			else
			{
				m_hasError = true;

				m_error += "Unrecognised compression type: ";
				m_error += compressionType;
				m_error += "\n";
			}
		}
		else
		{
			m_hasError = true;

			m_error += "-" CLP_FORCECOMPRESSION " requires a compression type to be specified\n";
		}
	}

	// Logging
	if( parser.HasOption( MACRO_TXT( CLP_LOGTOCONSOLE ) ) )
	{
		m_ttyDevice = new Red::System::Log::TTYWriter;
	}

	if( parser.HasOption( MACRO_TXT( CLP_LOGTOFILE ) ) )
	{
		String logFilePath;

		if( parser.GetFirstParam( MACRO_TXT( CLP_LOGTOFILE ), logFilePath ) )
		{
			m_fileDevice = new Red::System::Log::File( logFilePath.AsChar(), true );
		}
		else
		{
			m_hasError = true;

			m_error += "-" CLP_LOGTOFILE " requires a file path to be specified\n";
		}
	}

	if( parser.HasOption( MACRO_TXT( CLP_MINIMUMRATIO ) ) )
	{
		if( !parser.GetFirstParam( MACRO_TXT( CLP_MINIMUMRATIO ), m_minimumCompressionRatio ) 
			|| m_minimumCompressionRatio > 1.0f 
			|| m_minimumCompressionRatio < 0.0f )
		{
			m_hasError = true;

			m_error += "Invalid minimum compression ratio specified\n";
			m_error += "-" CLP_MINIMUMRATIO " requires a positive value between 0 and 1 \n";
		}
	}
}

void COptions::ValidatePaths()
{
	RED_FATAL_ASSERT( !m_hasError, "Cannot continue if errors have already been encountered" );

	if( !Utility::ValidateDirectory( m_depotPath )  )
	{
		m_hasError = true;

		m_error += "Could not find directory: ";
		m_error += m_depotPath;
		m_error += "\n";
	}

	if( !Utility::ValidateDirectory( m_cookedPath )  )
	{
		m_hasError = true;

		m_error += "Could not find directory: ";
		m_error += m_cookedPath;
		m_error += "\n";
	}

	if( !Utility::ValidateDirectory( m_outputDir )  )
	{
		if( Utility::RedMkDir( m_outputDir.AsChar() ) != 0 )
		{
			if( errno == ENOENT )
			{
				m_hasError = true;

				m_error += "Could not create directory (Parent directory not found): ";
				m_error += m_outputDir;
				m_error += "\n";
			}
		}
	}
}

Bool COptions::HasErrors() const
{
	return m_hasError;
}

void COptions::PrintErrors() const
{
	puts( m_error.AsChar() );
	puts( "\n" );
}

void COptions::PrintCommandLine() const
{
	// Output some usage text and exit
	printf( "\nOptions\n" );
	printf( " - Required:\n" );
	printf( "     -%hs <path/to/directory/>\n", CLP_DEPOTPATH );
	printf( "     -%hs <path/to/file.json>\n", CLP_DEFINITIONFILE );
	printf( "     -%hs <path/to/directory/>\n", CLP_OUTPUTDIR );
	printf( "     -%hs <path/to/directory/>\n", CLP_COOKEDPATH );

	printf( " - Optional:\n" );
	printf( "     -%hs [Default = off]\n", CLP_SINGLETHREADED );
	printf( "     -%hs <number of threads to use> [Default = \"auto\"]\n", CLP_THREADCOUNT );
	printf( "     -%hs [Default = off]\n", CLP_VERBOSE );
	printf( "     -%hs [Default = off]\n", CLP_LOGTOCONSOLE );
	printf( "     -%hs <path/to/file.log> [Default = off]\n", CLP_LOGTOFILE );
	printf( "     -%hs [Default = off]\n", CLP_PROFILE );
	printf( "     -%hs <count> [Default = 1] (This is how many times each file will be tested to figure out the best compressor to use)\n", CLP_ITERATIONS );
	printf( "     -%hs [auto/zlib/snappy/doboz/lz4/lz4hc] (Override the settings in the bundle definition)\n", CLP_FORCECOMPRESSION );
	printf( "     -%hs <path/to/auto.cache> (Cache the decisions made for resources compressed using auto)\n", CLP_AUTOCACHE );
	printf( "     -%hs <path/to/auto.cache> (Build the auto compression cache and exit)\n", CLP_BUILDAUTOCACHE );
	printf( "     -%hs <path/to/base/auto.cache> (Build the auto compression cache based on previous cache)\n", CLP_INCREMENTALCACHE );
	printf( "     -%hs <minimum compression ratio, do not compress if higher than that> [default = 0.95]", CLP_MINIMUMRATIO );
}

} // namespace Bundler {
