/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../../common/core/filePath.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/core/dependencyLoader.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/objectDiscardList.h"
#include "../../common/core/dataError.h"

#include "../../common/game/storyScene.h"
#include "../../common/game/storySceneLine.h"
#include "../../common/game/storySceneSection.h"
#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/localizationManager.h"

#include "../../common/gpuApiUtils/gpuApiMemory.h"
#include "../../common/core/depot.h"
#include "../../common/engine/localizationManager.h"

#include "cookDataBase.h"
#include "cookDataBaseHelper.h"

class CDialogSceneLinesCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CDialogSceneLinesCommandlet, ICommandlet, 0 );

public:
	CDialogSceneLinesCommandlet();
	~CDialogSceneLinesCommandlet();

	virtual const Char* GetOneLiner() const { return TXT("Save Dialog Lines"); }
	virtual bool Execute( const CommandletOptions& options );
	virtual void PrintHelp() const;

	void DoExportStorySceneLine( const String & outp, const String & lang, const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine /*= false */ );
	void PreprocessTextForAnnosoftLipsyncSDK( const String& text, String& result );
};

BEGIN_CLASS_RTTI( CDialogSceneLinesCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDialogSceneLinesCommandlet );

CDialogSceneLinesCommandlet::CDialogSceneLinesCommandlet()
{
	m_commandletName = CName( TXT("get_txts") );
}

CDialogSceneLinesCommandlet::~CDialogSceneLinesCommandlet()
{
}

//   get_txts -outdir=z:\lipsync\ -lang=FR

bool CDialogSceneLinesCommandlet::Execute( const CommandletOptions& options )
{
	TDynArray< CDiskFile* > filesToProcess;
	if ( options.HasOption( TXT("db") ) )
	{
		CCookerDataBase		dbf;
		String cookDBPath;
		options.GetSingleOptionValue( TXT("db"), cookDBPath );
		if ( !dbf.LoadFromFile( cookDBPath ) )
		{
			ERR_WCC( TXT("Failed to load cooked DB from file '%ls'"), cookDBPath.AsChar() );
			return false;
		}
		// get the files using the entries of cook.db
		TDynArray<String> classesToProcess;
		classesToProcess.PushBack( TXT("w2scene") );
		dbf.QueryResources( [&]( const CCookerDataBase& db, const CCookerResourceEntry& entry ) 
			{
				const String depotPath = ANSI_TO_UNICODE( entry.GetFilePath().AsChar() );
				CDiskFile* file = GDepot->FindFile( depotPath );
				if ( file != nullptr )
				{
					filesToProcess.PushBack( file );
				}
				else
				{
					ERR_WCC( TXT("Cooked file '%ls' not found in depot - no validation will be done"), depotPath.AsChar() );
				}
				return true;
			}
		,
		CookDataBaseHelper::PerExtensionFilter( classesToProcess )
		);
	}
	else
	{
		GDepot->CollectFiles( filesToProcess, TXT( ".w2scene" ), true, false );
	}
	// files are collected
	// output path
	String outPath;
	if ( !options.GetSingleOptionValue( TXT("outdir"), outPath ) )
	{
		ERR_WCC( TXT("Expected output path to be specified") );
		return false;
	}
	// langeuge
	String langString;
	if ( !options.GetSingleOptionValue( TXT("lang"), langString ) )
	{
		ERR_WCC( TXT("Expected langeuge") );
		return false;
	}
	// locale to langeuge
	SLocalizationManager::GetInstance().SetCurrentLocale( langString );
	LOG_WCC( TXT("Found %d scene files"), filesToProcess.Size() );
	if ( filesToProcess.Empty() )
	{
		WARN_WCC( TXT("No dialog files found") );
		return true;
	}
	{
		const Int32 num = filesToProcess.Size();
		for ( Int32 i=0; i<num; ++i )
		{
			CDiskFile* file = filesToProcess[i];
			Bool wasLoaded = file->IsLoaded();
			if( file->Load() )
			{
				CResource* res = file->GetResource();
				if( res && res->IsA< CStoryScene >() )
				{
					if ( CStoryScene* scene = Cast< CStoryScene >( res ) )
					{
						Uint32 numSections = scene->GetNumberOfSections();
						for( Uint32 i = 0; i < numSections; ++i )
						{
							Bool sectionValid = true;
							const CStorySceneSection* section = scene->GetSection( i );
							if( section )
							{
								TDynArray< CAbstractStorySceneLine* > lines;
								section->GetLines( lines );
								for( auto it = lines.Begin(), end = lines.End(); it != end; ++it )
								{
									CStorySceneLine* line = Cast< CStorySceneLine >( *it );
									if( line )
									{
										DoExportStorySceneLine( outPath, langString, line, scene, section, false );
									}
								}
							}
						}
					}
				}
			}
			if ( !wasLoaded )
			{
				file->Unload();
			}
		}
	}
	return true;
}


void CDialogSceneLinesCommandlet::DoExportStorySceneLine( const String & outp, const String & lang, const CStorySceneLine* sceneLine, CStoryScene* storyScene, const CStorySceneSection* section, Bool isLastLine /*= false */ )
{
#ifndef NO_EDITOR 
	Uint32 stringId = sceneLine->GetLocalizedContent()->GetIndex();
	LanguagePack* languagePack = SLocalizationManager::GetInstance().GetLanguagePackSync( stringId, true, String::EMPTY );
	if ( languagePack == NULL )
	{
		return;
	}
	{
		String text		= sceneLine->GetLocalizedContent()->GetString();
		String fileName	= sceneLine->GetVoiceFileName();
		StringAnsi fileNameAnsi = UNICODE_TO_ANSI( fileName.AsChar() );
		StringAnsi langAnsi = UNICODE_TO_ANSI( lang.AsChar() );
		StringAnsi path = UNICODE_TO_ANSI( outp.AsChar() );
		StringAnsi txtfile =	path + langAnsi + "\\text\\" + fileNameAnsi + ".txt";
		String preprocessedText;
		PreprocessTextForAnnosoftLipsyncSDK( text, preprocessedText );
		GFileManager->SaveStringToFileWithUTF8( ANSI_TO_UNICODE( txtfile.AsChar() ), preprocessedText );
	}
	SLocalizationManager::GetInstance().ReleaseLanguagePack( stringId );
#endif
}
void CDialogSceneLinesCommandlet::PreprocessTextForAnnosoftLipsyncSDK( const String& text, String& result )
{
	result = text;
	String src;
	String dst;

	// Replace each HORIZONTAL ELLIPSIS (U+2026) with three full stops.
	src = L"\u2026";
	dst = L"...";
	result.ReplaceAll( src, dst, false );

	// Replace each LATIN SMALL LIGATURE OE (U+0153) with oe.
	src = L"\u0153";
	dst = L"oe";
	result.ReplaceAll( src, dst, false );

	// Replace each RIGHT SINGLE QUOTATION MARK (U+2019) with plain old apostrophe (U+0027).
	src = L"\u2019";
	dst = L"'";
	result.ReplaceAll( src, dst, false );
}

void CDialogSceneLinesCommandlet::PrintHelp() const
{
	LOG_WCC( TXT("Usage:") );
	LOG_WCC( TXT("  get_txts [-db=<cook.db file>] -outdir=<dir> [-lang=<string>]") );
	LOG_WCC( TXT("Exclusive options:") );
	LOG_WCC( TXT("  -db=<cook.db>      - Validate files from given cook") );
}