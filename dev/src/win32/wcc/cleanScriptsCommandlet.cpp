/**
* Copyright c 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/bundlePreamble.h"
#include "../../common/core/bundlePreambleParser.h"
#include "../../common/core/bundleFileReaderDecompression.h"
#include "../../common/core/asyncLoadToken.h"
#include "../../common/core/debugBundleHeaderParser.h"
#include "../../common/core/asyncIO.h"
#include "../../common/engine/scriptCompilationHelper.h"
#include "../../common/core/scriptingSystem.h"
#include "../../common/core/contentManager.h"

class CCleanScriptsCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CCleanScriptsCommandlet, ICommandlet, 0 );

public:
	CCleanScriptsCommandlet();
	~CCleanScriptsCommandlet();

	// ICommandlet interface
	virtual const Char* GetOneLiner() const override { return TXT("Prepare scripts for publishing"); }
	virtual bool Execute( const CommandletOptions& options ) override;
	virtual void PrintHelp() const override;

private:
	String	m_inPath;
	String	m_outPath;
	String	m_legalNotice;

	TDynArray< String > m_swearWord;

	Bool CleanSingleFile( const String& scriptDir );
	void FillOutCode( const String& scriptCode, String& outCode );
	void FindSwearWords( const String& fileName, const String& code ) const;
	Bool VerifyNewScripts();
};

BEGIN_CLASS_RTTI( CCleanScriptsCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CCleanScriptsCommandlet );

CCleanScriptsCommandlet::CCleanScriptsCommandlet()
{
	m_commandletName = CName( TXT("cleanScripts") );
}

CCleanScriptsCommandlet::~CCleanScriptsCommandlet()
{
}

bool CCleanScriptsCommandlet::Execute( const CommandletOptions& options )
{
	// get in path
	if ( !options.GetSingleOptionValue( TXT("dir"), m_inPath ) )
	{
		ERR_WCC( TXT("Missing input directory") );
		return false;
	}

	// get out path
	if ( !options.GetSingleOptionValue( TXT("outdir"), m_outPath ) )
	{
		ERR_WCC( TXT("Missing output directory") );
		return false;
	}
	
	{ // conform directory name
		if ( !m_inPath.EndsWith( TXT("\\") ) )
		{
			m_inPath += TXT("\\");
		}

		if ( !m_outPath.EndsWith( TXT("\\") ) )
		{
			m_outPath += TXT("\\");
		}
	}

	{ // legal notice
		String legalNoticePath;
		if ( options.GetSingleOptionValue( TXT("legal"), legalNoticePath ) )
		{
			if ( GFileManager->LoadFileToString( legalNoticePath, m_legalNotice, true ) )
			{
				m_legalNotice.Append( TXT( "\r\n" ), 2 );
			}
		}

		if( m_legalNotice.Empty() )
		{
			m_legalNotice = TXT( "/*\r\nCopyright © CD Projekt RED 2015\r\n*/\r\n" );
		}
	}

	{ // swear words
		String swearWordsPath;
		if ( options.GetSingleOptionValue( TXT("swear"), swearWordsPath ) )
		{
			String swearWords;
			if ( GFileManager->LoadFileToString( swearWordsPath, swearWords, true ) )
			{
				m_swearWord = swearWords.ToLower().Split( TXT(","), true );
			}
		}
	}

	// find scripts
	TDynArray< String > scriptFiles;
	GFileManager->FindFiles( m_inPath, TXT("*.ws"), scriptFiles, true );
	if ( scriptFiles.Empty() )
	{
		ERR_WCC( TXT( "No scripts found in specified directory '%ls'" ), m_inPath.AsChar() );
		return false;
	}

	// process scripts
	for ( const String& path : scriptFiles )
	{
		if ( !CleanSingleFile( path ) )
		{
			ERR_WCC( TXT("Error processing script file '%ls'"), path.AsChar() );
			return false;
		}
	}

	// verify after resave
	if( options.HasOption( TXT( "verify" ) ) && !VerifyNewScripts() )
	{
		return false;
	}

	return true;
}

void CCleanScriptsCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Usage:" ) );
	LOG_WCC( TXT( "  cleanScripts -dir=<path> -outdir=<path> -legal=<dir>" ) );
	LOG_WCC( TXT( "Parameters:" ) );
	LOG_WCC( TXT( "  -dir=<dir>     - directory with original scripts" ) );
	LOG_WCC( TXT( "  -outdir=<dir>  - absolute path to output directory" ) );
	LOG_WCC( TXT( "  -legal=<dir>   - (optional) legal notice file" ) );
	LOG_WCC( TXT( "  -swear=<dir>   - (optional) file with swear words to log out, separated by commas" ) );
	LOG_WCC( TXT( "    e.g. hack,dupa,something" ) );
	LOG_WCC( TXT( "  -verify        - (optional) verify scripts after resaving" ) );
}

Bool CCleanScriptsCommandlet::CleanSingleFile( const String& scriptDir )
{
	String relativePath = scriptDir.StringAfter( m_inPath );
	String outPath = m_outPath + relativePath;

	String scriptCode;
	if ( !GFileManager->LoadFileToString( scriptDir, scriptCode, true ) )
	{
		return false;
	}

	if( scriptCode.Empty() )
	{
		return true; // file is empty, ignore it
	}

	String outCode;
	outCode.Reserve( scriptCode.Size() );
	FillOutCode( scriptCode, outCode );

	{
		Red::TScopedPtr< IFile > file( GFileManager->CreateFileWriter( outPath, FOF_Buffered | FOF_AbsolutePath ) );
		if ( !file || !GFileManager->SaveStringToFileWithUTF16( *file, m_legalNotice + outCode ) )
		{
			return false;
		}
	}

	FindSwearWords( relativePath, outCode );

	return true;
}

void CCleanScriptsCommandlet::FillOutCode( const String& scriptCode, String& outCode )
{
	Bool oneLineComment = false;
	Bool multiLineComment = false;
	Bool stringStarted = false;

	for( Uint32 i = 0; i < scriptCode.Size() - 1; ++i )
	{
		if( multiLineComment )
		{
			if( ( scriptCode[ i ] == Char( '*' ) ) && ( scriptCode[ i + 1 ] == Char( '/' ) ) )
			{
				i += 1;
				multiLineComment = false;
			}

			continue;
		}

		if( oneLineComment )
		{
			if( ( scriptCode[ i ] == Char( '\r' ) ) || ( scriptCode[ i ] == Char( '\n' ) ) )
			{
				oneLineComment = false;
			}
			else
			{
				continue;
			}
		}

		if( ( scriptCode[ i ] == Char( '/' ) ) && !stringStarted )
		{
			if( scriptCode[ i + 1 ] == Char( '*' ) )
			{
				i += 1;
				multiLineComment = true;
				continue;
			}
			else if( scriptCode[ i + 1 ] == Char( '/' ) )
			{
				i += 1;
				oneLineComment = true;
				continue;
			}
		}
		else if( scriptCode[ i ] == Char( '"' ) )
		{
			stringStarted = !stringStarted;
		}

		outCode.PushBack( scriptCode[ i ] );
	}

	if( !scriptCode.EndsWith( TXT( "//" ) ) && !scriptCode.EndsWith( TXT( "*/" ) ) )
	{
		outCode.PushBack( scriptCode[ scriptCode.Size() - 1 ] );
	}
}

void CCleanScriptsCommandlet::FindSwearWords( const String& fileName, const String& code ) const
{
	size_t unused;
	String lowerCode = code.ToLower();
	for ( const String& swearWord : m_swearWord )
	{
		if( lowerCode.FindSubstring( swearWord, unused, true, 0 ) )
		{
			LOG_WCC( TXT( "Swear word [ %ls ] in file '%ls'" ), swearWord.AsChar(), fileName.AsChar() );
		}
	}
}

Bool CCleanScriptsCommandlet::VerifyNewScripts()
{
	CScriptCompilationMessages errorCollector;
	CScriptCompiler compiler( &errorCollector );
	
	TDynArray< String > scriptFiles;
	GContentManager->EnumScriptFiles( scriptFiles );

	if( !compiler.CompileFiles( /*m_outPath,*/ scriptFiles, true ) )
	{
		ERR_WCC( TXT( "Scripts verification failed" ) );
		return false;
	}

	return true;
}
