/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#ifndef NO_DEBUG_PAGES

#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"
#include "../core/depot.h"
#include "../core/diskFile.h"
#include "../core/fileSys.h"
#include "../core/httpResponseData.h"
#include "videoPlayer.h"

#include "localizationManager.h"
#include "game.h"
#include "videoDebugHud.h"

static void FillUsmFiles( CDirectory* dir, TDynArray< CDiskFile* >& outFiles );
static void GetLooseSubsFilePaths( const CDiskFile* usmFile, Bool altSubs, TDynArray< String >& outLooseSubFilePaths );
static void GetLooseSubsFilePaths( const CDiskFile* usmFile, Bool altSubs, TDynArray< const CDiskFile* >& outLooseSubFilePaths );

const AnsiChar* const LOOSE_SUBS = "loose";
const AnsiChar* const EMBEDDED_SUBS = "embedded";
const AnsiChar* const MAIN_TRACK = "main";
const AnsiChar* const ALT_TRACK = "alt";

/// video list debug page
class CDebugPageVideos : public IDebugPageHandlerHTML
{
public:
	CDebugPageVideos()
		: IDebugPageHandlerHTML( "/videos/" )
	{
		GVideoDebugHud = new CVideoDebugHud;
	}

	~CDebugPageVideos()
	{
		delete GVideoDebugHud;
		GVideoDebugHud = nullptr;
	}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Videos"; }
	virtual StringAnsi GetCategory() const override { return "Engine"; }
	virtual Bool HasIndexPage() const override { return GGame && GGame->GetVideoPlayer() != nullptr; }

	class VideoInfo: public CDebugPageHTMLTable::IRow
	{
	public:
		enum EColumn
		{
			eColumn_IconMain=1,
			eColumn_IconAlt=2,
			eColumn_SubsType=3,
			eColumn_Name=4,
			eColumn_FullPath=5,
		};

		VideoInfo( const CDiskFile* file )
		{
			if ( file )
			{
				const StringAnsi tmp = UNICODE_TO_ANSI( file->GetDepotPath().AsChar() );
				m_path = tmp;
				m_path.ReplaceAll( '\\', '/' );
				m_shortName = UNICODE_TO_ANSI( CFilePath( file->GetDepotPath() ).GetFileNameWithExt().AsChar() );
			}

			GetLooseSubsFilePaths( file, false, m_subsFilePaths );
			GetLooseSubsFilePaths( file, true, m_altSubsFilePaths );
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const VideoInfo* otherVideoInfo = static_cast<const VideoInfo*>( other );

			Bool cmp = false;
			switch (columnID)
			{
			case eColumn_Name:
				{
					cmp = m_shortName < otherVideoInfo->m_shortName;
				}
				break;
			case eColumn_SubsType:
				{
					cmp = HasLooseSubs() < otherVideoInfo->HasLooseSubs();
				}
				break;
			case eColumn_FullPath: /* fall through */
			default:
				{
					cmp = m_path < otherVideoInfo->m_path;
				}
				break;
			}

			return cmp;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
			case eColumn_IconMain:
				{
					const AnsiChar* const subs = HasLooseSubs() ? LOOSE_SUBS : EMBEDDED_SUBS;
					doc.LinkScript( "img_play", "playvideo('#<host>#', '%hs', '%hs', '%hs');", m_path.AsChar(), MAIN_TRACK, subs );
				}
				break;
			case eColumn_IconAlt:
				{
					const AnsiChar* const subs = HasLooseSubs() ? LOOSE_SUBS : EMBEDDED_SUBS;
					if ( !m_altSubsFilePaths.Empty() )
					{
						doc.LinkScript( "img_play", "playvideo('#<host>#', '%hs', '%hs', '%hs');", m_path.AsChar(), ALT_TRACK, subs );
					}
					else
					{
						doc.Write("");
					}
				}
				break;
			case eColumn_SubsType:
				{
					if ( HasLooseSubs() )
					{
						doc.Link( "/subs/%hs", m_path.AsChar() ).Write( "loose" );
					}
					else
					{
						doc.Writef( "%hs", "embedded" );
					}
				}
				break;
 			case eColumn_Name:
				{
					doc.Writef( "<font color='%hs'>%hs</font>", ( HasProblems() ? "#ff0000" : "#000000" ), m_shortName.AsChar() );
				}
				break;
			case eColumn_FullPath:
				{
					doc.Link( "/file/%hs", m_path.AsChar() ).Write( m_path.AsChar() );
				}
				break;
			default:
				break;
			}
		}

		Bool HasLooseSubs() const { return !m_altSubsFilePaths.Empty() || !m_subsFilePaths.Empty(); }
		Bool HasProblems() const { return m_subsFilePaths.Size() != m_altSubsFilePaths.Size(); }

	private:
		StringAnsi				m_path;
		StringAnsi				m_shortName;
		TDynArray< String >		m_subsFilePaths;
		TDynArray< String >		m_altSubsFilePaths;
	};

	//! Async command
	virtual CHTTPResponseData* OnCommand( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl, const StringAnsi& content ) override
	{
		// parse the params
		const CBasicURL params = CBasicURL::ParseParams( CBasicURL::Decode( content ) );

		Int32 testValue=-1;
		StringAnsi testText;
	
		if ( params.GetKey( "__video", testText ) )
		{
			String videoToPlay = ANSI_TO_UNICODE( testText.AsChar() );
			videoToPlay.ReplaceAll( TXT('/'), TXT('\\') );

			LOG_ENGINE(TXT("Video: %ls"), videoToPlay.AsChar() );

			Uint8 extraVideoParamFlags = 0;

			StringAnsi track;
			if ( !params.GetKey( "__track", track ) )
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}

			LOG_ENGINE(TXT("Track: %hs"), track.AsChar() );

			StringAnsi subs;
			if ( !params.GetKey( "__subs", subs ) )
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}

			LOG_ENGINE(TXT("Subs: %hs"), subs.AsChar() );

			if ( track.EqualsNC( MAIN_TRACK ) )
			{
				extraVideoParamFlags |= 0;
			}
			else if ( track.EqualsNC( ALT_TRACK ) )
			{
				extraVideoParamFlags |= eVideoParamFlag_AlternateTrack;
			}
			else
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}

			if ( subs.EqualsNC( EMBEDDED_SUBS ) )
			{
				extraVideoParamFlags |= 0;
			}
			else if ( subs.EqualsNC( LOOSE_SUBS ) )
			{
				extraVideoParamFlags |= eVideoParamFlag_ExternalSubs;
			}
			else
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}

			LOG_ENGINE(TXT("Extra video flags: 0x%08X"), (Uint32)extraVideoParamFlags);

			if ( !GVideoDebugHud->PlayVideo( videoToPlay, extraVideoParamFlags ) )
			{
				return new CHTTPResponseData( CHTTPResponseData::eResultCode_BadRequest );
			}

			// serviced
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_OK );
		}
		else if ( params.HasKey( "__stop") )
		{
			GVideoDebugHud->StopVideo();

			// serviced
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_OK );
		}

		// not handled
		return nullptr;
	}

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no video player
		if ( !GGame || !GGame->GetVideoPlayer() )
		{
			doc << "<span class=\"error\">No video player</span>";
			return true;
		}

		CDirectory* moviesDir = GDepot->FindLocalDirectory( TXT("movies") );
		if ( !moviesDir )
		{
			doc << "<span class=\"error\">Movies directory not found</span>";
			return true;
		}

		TDynArray< CDiskFile* > usmFiles;
		FillUsmFiles( moviesDir, usmFiles );

		Sort( usmFiles.Begin(), usmFiles.End(), [](CDiskFile* a, CDiskFile* b) { return a->GetDepotPath() < b->GetDepotPath(); } );

		if ( usmFiles.Empty() )
		{
			doc << "<span class=\"error\">No USMs found under movies directory</span>";
			return true;
		}

		// show global video stats
		{
			CDebugPageHTMLInfoBlock info( doc, "Videos Info" );

			info.Info("Number of videos: ").Writef("%u", usmFiles.Size());
		}

		// stop current video
		{
			// special actions
			{
 				CDebugPageHTMLInfoBlock info( doc, "Actions" );
 				info.Info("").LinkScript( "", "stopvideo('#<host>#');" ).Write("Stop debug video");
			}
		}

		// show video table
		{
			CDebugPageHTMLInfoBlock info( doc, "Videos List" );

			CDebugPageHTMLTable table( doc, "videos" );
			table.AddColumn( "Main", 20, false );
			table.AddColumn( "Alt", 20, false );
			table.AddColumn( "Subs", 20, true );
			table.AddColumn( "Name", 160, true );
			table.AddColumn( "Path", 580, true );

			for ( CDiskFile* usmFile : usmFiles )
			{
				table.AddRow( new VideoInfo( usmFile ) );
			}

			table.Render(800, "generic", doc.GetURL());
		}

		return true;
	}
};

class CDebugPageVideoLooseSubs : public IDebugPageHandlerHTML
{
public:
	CDebugPageVideoLooseSubs()
		: IDebugPageHandlerHTML( "/subs/" )
	{
	}

	~CDebugPageVideoLooseSubs()
	{
	}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Video Loose Subs"; }
	virtual StringAnsi GetCategory() const override { return "Engine"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// get conformed path
		StringAnsi temp;
		const StringAnsi& safeDepotPath = CFilePath::ConformPath( relativeURL.GetPath(), temp );

		// get the file
		CDiskFile* usmFile = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( safeDepotPath.AsChar() ), 0 );
		if ( !usmFile )
		{
			// display a message instead of 404
			doc << "<span class=\"error\">File not found in depot</span>";
			return true;
		}

		if ( !usmFile->GetFileName().EndsWith(TXT(".usm")) )
		{
			// display a message instead of 404
			doc << "<span class=\"error\">File not found in depot</span>";
			return true;
		}

		TDynArray< const CDiskFile* > looseSubFilePaths;
		GetLooseSubsFilePaths( usmFile, false, looseSubFilePaths );

		TDynArray< const CDiskFile* > looseAltSubFilePaths;
		GetLooseSubsFilePaths( usmFile, true, looseAltSubFilePaths );

		{
			CDebugPageHTMLInfoBlock info( doc, "USM" );
			doc.Link( "/file/%hs", relativeURL.GetPath().AsChar() ).Write( safeDepotPath.AsChar() );
		}

		// show global video stats
		{
			CDebugPageHTMLInfoBlock info( doc, "Subs Info" );

			info.Info("Number of subs: ").Writef("%u", looseSubFilePaths.Size());
			info.Info("Number of altsubs: ").Writef("%u", looseAltSubFilePaths.Size());
		}

		// show video subs table
		{
			CDebugPageHTMLInfoBlock info( doc, "Subs" );

			CDebugPageHTMLTable table( doc, "mainsubs" );
			table.AddColumn( "Files", 0.f, true );
			for ( const CDiskFile* usmFile : looseSubFilePaths )
			{
				table.AddRow( table.CreateRowData( usmFile ) );
			}

			table.Render(800, "generic", doc.GetURL());
		}

		// show video altsubs table
		{
			CDebugPageHTMLInfoBlock info( doc, "Alt Subs" );

			CDebugPageHTMLTable table( doc, "altsubs" );
			table.AddColumn( "Files", 0.f, true );
			for ( const CDiskFile* usmFile : looseAltSubFilePaths )
			{
				table.AddRow( table.CreateRowData( usmFile ) );
			}

			table.Render(800, "generic", doc.GetURL());
		}

		return true;
	}
};


static void FillUsmFiles( CDirectory* dir, TDynArray< CDiskFile* >& outFiles )
{
	for ( CDiskFile* usmFile : dir->GetFiles() )
	{
		const String ext = StringHelpers::GetFileExtension( usmFile->GetFileName() );
		if (ext.EqualsNC(TXT("usm")))
		{
			outFiles.PushBack( usmFile );
		}
	}

	for ( CDirectory* subdir : dir->GetDirectories() )
	{
		FillUsmFiles( subdir, outFiles );
	}
}

static void GetLooseSubsFilePaths( const CDiskFile* usmFile, Bool altSubs, TDynArray< String >& outLooseSubFilePaths )
{
	// FIXME: Find file wildcard matching should be updated to use better matching!
	TDynArray< String > subsFiles;
	const CDirectory* dir = usmFile->GetDirectory();
	const CDirectory* subDir = dir->FindLocalDirectory( altSubs ? TXT("altsubs") : TXT("subs") );
	if ( ! subDir )
	{
		return;
	}

	const CFilePath filePath( usmFile->GetFileName() );
	const String match = filePath.GetFileName() + TXT("_*.subs");
	for ( const CDiskFile* subFile : subDir->GetFiles() )
	{
		if ( StringHelpers::WildcardMatch( subFile->GetFileName().AsChar(), match.AsChar() ) )
		{
			outLooseSubFilePaths.PushBack( subFile->GetAbsolutePath() );
		}
	}

	// #hack: if only TR subs, then not a real "loose file" video, but using these subs to override the text
	// using EN timing info...
	if ( outLooseSubFilePaths.Size() == 1 && outLooseSubFilePaths[0].EndsWith(TXT("_tr.subs")) )
	{
		outLooseSubFilePaths.Clear();
	}
}

static void GetLooseSubsFilePaths( const CDiskFile* usmFile, Bool altSubs, TDynArray< const CDiskFile* >& outLooseSubFilePaths )
{
	// FIXME: Find file wildcard matching should be updated to use better matching!
	TDynArray< String > subsFiles;
	const CDirectory* dir = usmFile->GetDirectory();
	const CDirectory* subDir = dir->FindLocalDirectory( altSubs ? TXT("altsubs") : TXT("subs") );
	if ( ! subDir )
	{
		return;
	}

	const CFilePath filePath( usmFile->GetFileName() );
	const String match = filePath.GetFileName() + TXT("_*.subs");
	for ( const CDiskFile* subFile : subDir->GetFiles() )
	{
		if ( StringHelpers::WildcardMatch( subFile->GetFileName().AsChar(), match.AsChar() ) )
		{
			outLooseSubFilePaths.PushBack( subFile );
		}
	}

	// #hack: if only TR subs, then not a real "loose file" video, but using these subs to override the text
	// using EN timing info...
	if ( outLooseSubFilePaths.Size() == 1 && outLooseSubFilePaths[0]->GetDepotPath().EndsWith(TXT("_tr.subs")) )
	{
		outLooseSubFilePaths.Clear();
	}
}

//-----

void InitVideoDebugPages()
{
	new CDebugPageVideos;
	new CDebugPageVideoLooseSubs;
}

//-----

#endif
