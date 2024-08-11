/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"

class CVoiceoverConversionCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CVoiceoverConversionCommandlet, ICommandlet, 0 );

public:
	CVoiceoverConversionCommandlet();

	virtual const Char* GetOneLiner() const;

	virtual bool Execute( const CommandletOptions& options );

	virtual void PrintHelp() const;

protected:

	bool ConvertVoiceovers( String wwisePath, String speechResourceDirectory, String platform, String language, bool rebuild );

};

BEGIN_CLASS_RTTI( CVoiceoverConversionCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CVoiceoverConversionCommandlet );

RED_DEFINE_STATIC_NAME( voconvert )

CVoiceoverConversionCommandlet::CVoiceoverConversionCommandlet()
{
	m_commandletName = CNAME( voconvert );
}

bool CVoiceoverConversionCommandlet::Execute( const CommandletOptions& options )
{
	auto arguments = options.GetFreeArguments();
	if( arguments.Size() < 4 )
	{
		ERR_WCC( TXT( "Error: not enough arguments!" ) );
		return false;
	}

	String wwiseDirectory = arguments[ 0 ].ToLower(); //FIXME: this could be an optional argument (we don't support them atm...).
	String speechResourceDirectory = arguments[ 1 ].ToLower();
	String platform = arguments[ 2 ].ToLower();
	bool rebuild = false;

	if( options.HasOption( TXT( "rebuild" ) ) )
	{
		rebuild = true;
	}
	
	for ( Uint32 i = 3; i < arguments.Size(); ++i )
	{
		String language = arguments[ i ].ToLower();

		if( !ConvertVoiceovers( wwiseDirectory, speechResourceDirectory, platform, language, rebuild ) )
		{
			return false;
		}
	}
	return true;
}

const Char* CVoiceoverConversionCommandlet::GetOneLiner() const
{
	return TXT( "Convert voiceover files from .wav to .ogg" );
}

void CVoiceoverConversionCommandlet::PrintHelp() const
{
	LOG_WCC( TXT( "Use: " ) );
	LOG_WCC( TXT( "wcc voconvert wwise_bin_dir voiceivers_source_dir platform languages (pc en ...)" ) );
}

bool CVoiceoverConversionCommandlet::ConvertVoiceovers( String wwisePath, String speechResourceDirectory, String platform, String language, bool rebuild )
{
	String localSpeechFolder;
	GDepot->GetAbsolutePath( localSpeechFolder );
	localSpeechFolder += TXT( "speech\\" );

	String sourceFilesFolder;
	sourceFilesFolder = speechResourceDirectory;
	sourceFilesFolder += TXT( "\\" );

	String extSourceListTemplateFilePath = localSpeechFolder;
	extSourceListTemplateFilePath += TXT( "\\wwise\\ExtSourceList.wsources"  );

	CSystemFile extSourceTemplateListFile;
	extSourceTemplateListFile.CreateReader( extSourceListTemplateFilePath.AsChar() );

	size_t fileSize = static_cast< size_t >( extSourceTemplateListFile.GetSize() );
	RED_ASSERT( (Uint64)fileSize == extSourceTemplateListFile.GetSize(), TXT("Unexpectedly large file '%s'"), extSourceTemplateListFile.GetFileName() );
	char* buffer = ( char* ) malloc( fileSize );
	extSourceTemplateListFile.Read( buffer, fileSize );

	char* carret = buffer + extSourceTemplateListFile.GetSize();
	TDynArray< char > tempBuffer;
	Bool filling = false;
	TDynArray< String > files;

	carret = buffer + extSourceTemplateListFile.GetSize();
	while( carret > buffer )
	{
		if( *carret == 60 )
		{
			break;
		}
		--carret;
	}

	String targetExtension;
	// Hack, but what can you do...
	if( platform == TXT( "xboxone" ) )
	{
		targetExtension = TXT( "xma");
	}
	else if( platform == TXT( "windows" ) )
	{
		targetExtension = TXT( "ogg");
	}
	else if( platform == TXT( "ps4" ) )
	{
		targetExtension = TXT( "atr");
	}
	else
	{
		RED_HALT( "Platform not supported" );
		return false;
	}

	sourceFilesFolder += language;
	sourceFilesFolder += TXT( "\\audio\\*.wav" );

	for ( CSystemFindFile findFile( sourceFilesFolder.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String fileName = speechResourceDirectory;
			fileName += TXT( "\\" );
			fileName += language;
			fileName += TXT( "\\audio\\" );
			fileName += findFile.GetFileName();

			auto targetFileName = fileName;
			targetFileName.Replace( TXT( "wav" ), targetExtension );

			if( !rebuild && 
				GFileManager->FileExist( targetFileName ) &&
				GFileManager->GetFileTime( targetFileName ) > GFileManager->GetFileTime( fileName ) )
			{
				continue;
			}

			files.PushBack( fileName );
		}
	}

	if( files.Size() == 0)
	{
		LOG_WCC( TXT( "No files to convert..." ) );
		return true;
	}

	String extSourceListFilePath = localSpeechFolder;
	extSourceListFilePath += TXT( "wwise\\ExtSourceListGenerated.wsources" );

	CSystemFile extSourceListFile;
	extSourceListFile.CreateWriter( extSourceListFilePath.AsChar() );
	extSourceListFile.Write( buffer, carret - buffer );

	while( carret > buffer )
	{
		if( *carret == 34 )
		{
			if( filling )
			{
				filling = false;
				tempBuffer.PushBack( 0 );
				String fileNameWithPath = ANSI_TO_UNICODE( tempBuffer.Begin() );
				for( Uint32 i = 0; i != files.Size(); ++i )
				{
					size_t index = 0;
					if( fileNameWithPath.FindSubstring( files[ i ].AsChar(), index ) )
					{
						files.RemoveAtFast( i );

						break;

					}
				}
				tempBuffer.Clear();
			}
			else
			{
				filling = true;
			}
		}
		else if( filling )
		{
			tempBuffer.Insert( 0, *carret );
		}
		--carret;
	}

	for( Uint32 i = 0; i != files.Size(); ++i )
	{
		if( i )
		{
			char temp2[] = { 13, 10, 32, 32, 32, 32 };
			extSourceListFile.Write( temp2, 6 );
		}

		String string = TXT( "<Source Path=\"" );
		string += files[ i ];

		string += TXT( "\" />" );
		StringAnsi temp = UNICODE_TO_ANSI( string.AsChar() );
		extSourceListFile.Write( temp.AsChar(), string.Size() - 1 );
	}

	char newline[] = { 13, 10 };
	extSourceListFile.Write( newline, 2 );

	extSourceListFile.Write( "</ExternalSourcesList>", 22 );

	extSourceListFile.Close();
	extSourceTemplateListFile.Close();

	String depotPath;
	GDepot->GetAbsolutePath( depotPath );

	String querry = TXT( "\"" );
	querry += wwisePath;
	querry += TXT( "\\WwiseCLI.exe\" ") + depotPath;
	querry += TXT( "speech\\wwise\\wwise.wproj -ConvertExternalSources ");
	querry += platform;

	int result = system( UNICODE_TO_ANSI( querry.AsChar() ) );
	if( result != 0 && result != 2 )
	{
		ERR_WCC( TXT( "Error: invlid code was returned from the \"%s\" command" ), querry.AsChar() );
		return false;
	}
	String tempFilesFolder = depotPath; 
	tempFilesFolder += TXT( "speech\\" );
	tempFilesFolder += TXT( "*.wem" );

	for ( CSystemFindFile findFile( tempFilesFolder.AsChar() ); findFile; ++findFile )
	{
		if ( !findFile.IsDirectory() )
		{
			String newFile = speechResourceDirectory;
			newFile += TXT( "\\" );
			newFile += language;
			newFile += TXT( "\\audio\\" );
			newFile += findFile.GetFileName();
			newFile.Resize( newFile.Size() - 3 );

			newFile += targetExtension;
			String newFileName = newFile;

			String currentFile = localSpeechFolder;
			currentFile += TXT( "\\" );
			currentFile += findFile.GetFileName();
			GSystemIO.MoveFile( currentFile.AsChar(), newFile.AsChar() );
		}
	}

	return true;
}
