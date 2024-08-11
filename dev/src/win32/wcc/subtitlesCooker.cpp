#include "build.h"

#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/fileSys.h"
#include "../../common/core/filePath.h"

#include "cookerTextEncoder.h"

class CCookSubtitlesCommandlet: public ICommandlet
{
	DECLARE_ENGINE_CLASS( CCookSubtitlesCommandlet, ICommandlet, 0 );

public:
	CCookSubtitlesCommandlet();

	// Executes commandlet command
	virtual Bool Execute( const CommandletOptions& options );

	// Returns commandlet one-liner
	virtual const Char* GetOneLiner() const { return TXT( "Don't ask" ); };

	// Prints commandlet help
	virtual void PrintHelp() const 
	{
		LOG_WCC( TXT("Use: ") );
		LOG_WCC( TXT("wcc cooksubs font_path subtitles_path") );
		LOG_WCC( TXT("subtitles_path needs to be ucs-2 little endian because of our engine") );
		LOG_WCC( TXT("the output file is utf8..."));
	}
};

BEGIN_CLASS_RTTI( CCookSubtitlesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CCookSubtitlesCommandlet );

CCookSubtitlesCommandlet::CCookSubtitlesCommandlet()
{
	m_commandletName = CName( TXT("cooksubs") );
}

Bool CCookSubtitlesCommandlet::Execute( const CommandletOptions& options )
{
	auto arguments = options.GetFreeArguments();

	if( arguments.Size() != 2 )
	{
		LOG_WCC( TXT( "wcc cooksubs font_path subtitles_path" ) );
		return false;
	}

	const String fontPath = arguments[0];
	CCookerTextEncoder textEncoder;

	if ( ! textEncoder.LoadFont( fontPath ) )
	{
		ERR_WCC(TXT("Failed to load fontPath '%ls'"), fontPath.AsChar() );
		return false;
	}

	textEncoder.SelectLanguageScript( CCookerTextEncoder::SCRIPT_Arabic );

	const CFilePath fontFilePath( fontPath );
	const String fontNameKey = fontFilePath.GetFileNameWithExt().ToLower();
	if ( ! textEncoder.SelectFont( fontNameKey ) )
	{
		ERR_WCC(TXT("Failed to use font %ls"), fontNameKey.AsChar() );
		return false;
	}

	const String subtitlesPath = arguments[1];

	String subtitlesBlob;
	if ( ! GFileManager->LoadFileToString( subtitlesPath, subtitlesBlob, true ) )
	{
		ERR_WCC(TXT("Could not open '%ls' for XML parsing"), subtitlesPath.AsChar() );
		return false;
	}

	String outBlob;
	TDynArray< String > lines = subtitlesBlob.Split( TXT("\r\n") );

	for ( const String& line : lines )
	{
		String outText;
		if ( textEncoder.ShapeText( line, outText ) )
		{
			// Put text back into logical (vs presentation) order so Scaleform can reverse it more fine grained using its bidi alg
			// and not break HTML tags
			textEncoder.ReverseTextInPlace( outText );
		}

		if ( outText == line )
		{
			WARN_WCC(TXT("Line unchanged: %ls"), line.AsChar() );
		}

		outBlob += outText;
		outBlob += TXT("\r\n");
	}

	CFilePath subtitlesFilePath( subtitlesPath );
	const String outputFileName = subtitlesFilePath.GetPathString( true ) + subtitlesFilePath.GetFileName() + TXT("_new.") + subtitlesFilePath.GetExtension();
	if ( ! GFileManager->SaveStringToFileWithUTF8( outputFileName, outBlob ) )
	{
		ERR_WCC(TXT("Failed to save file '%ls'"), outputFileName.AsChar() );
		return false;
	}

	// Done
	return true;
}
