/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "../core/depot.h"
#include "../core/debugPageHandler.h"
#include "../core/debugPageHTMLDoc.h"
#include "../core/httpResponseData.h"
#include "../core/scopedPtr.h"

#ifndef NO_DEBUG_PAGES

#include "commonGame.h"

//-----

/// page with game shortcuts
class CDebugPageGameShortcuts : public IDebugPageHandlerHTML
{
public:
	CDebugPageGameShortcuts()
		: IDebugPageHandlerHTML( "/shortcuts/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Shortcuts"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return (GGame != nullptr); }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame )
		{
			doc << "<span class=\"error\">No active game</span>";
			return true;
		}

		// shortcuts to game objects
		{
			CDebugPageHTMLInfoBlock info( doc, "Global objects" );

			info.Info("Game: ").LinkObject(GGame);

			if ( GGame->GetActiveWorld() )
				info.Info("World: ").LinkObject(GGame->GetActiveWorld());

			if ( GGame->GetActiveWorld() && GGame->GetActiveWorld()->GetDynamicLayer() )
				info.Info("Dynamic layer: ").LinkObject((CObject*) GGame->GetActiveWorld()->GetDynamicLayer());

			if ( GGame->GetPlayerEntity() )
				info.Info("Player entity: ").LinkObject(GGame->GetPlayerEntity());

			/*if ( ((CCommonGame*)GGame)->GetPlayer() )
				info.Info("Player object: ").LinkObject(((CCommonGame*)GGame)->GetPlayer());*/
		}

		return true;
	}
};

//-----

/// get save game data
class CDebugPageDownloadSaveGame : public IDebugPageHandler
{
public:
	CDebugPageDownloadSaveGame()
		: IDebugPageHandler( "/getsavegame/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Download savegame"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return false; }

	// Direct request processing
	virtual class CHTTPResponseData* OnPage( const class CBasicURL& fullURL, const class CBasicURL& relativeUrl ) override
	{
		// get savegame
		Int32 saveGameID = -1;
		if ( !relativeUrl.GetKey( "id", saveGameID ) )
		{
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_ServerError );
		}

		// get savegames
		TDynArray< SSavegameInfo > savesToLoad;
		GUserProfileManager->GetSaveFiles( savesToLoad );

		// check ID
		if ( saveGameID < 0 || saveGameID >= savesToLoad.SizeInt() )
		{
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_ServerError );
		}

		// start loading
		const auto& saveInfo = savesToLoad[ saveGameID ];
		const auto ret = GUserProfileManager->InitGameLoading( saveInfo );
		if ( ret != ELoadGameResult::LOAD_Initializing && ret != ELoadGameResult::LOAD_ReadyToLoad )
		{
			CDebugPageHTMLResponse response( fullURL );
			{
				CDebugPageHTMLDocument doc( response, GetTitle() );
				doc << "<span class=\"error\">Failed to access the save game data</span>";
			}
			return response.GetData();
		}

		// wait for the save game
		int timeout = 50;
		while ( timeout > 0 )
		{
			// update profile
			GUserProfileManager->Update();

			// is ready to get the save?
			if ( GUserProfileManager->GetLoadGameProgress() == ELoadGameResult::LOAD_ReadyToLoad )
				break;

			// wait a little bit
			Red::Threads::SleepOnCurrentThread(100);
		}

		// still not ready
		if ( timeout == 0 )
		{
			CDebugPageHTMLResponse response( fullURL );
			{
				CDebugPageHTMLDocument doc( response, GetTitle() );
				doc << "<span class=\"error\">Unable to access saved game</span>";
			}
			return response.GetData();
		}

		// create RAW file reader
		IFile* reader = GUserProfileManager->CreateSaveFileReader( /*raw*/ true );
		if ( !reader )
		{
			return new CHTTPResponseData( CHTTPResponseData::eResultCode_ServerError );
		}

		// file to big
		const Uint32 size = (Uint32) reader->GetSize();

		// load file data
		DataBlobPtr data( new CDataBlob( size ) );
		reader->Serialize( data->GetData(), data->GetDataSize() );

		// done loading
		delete reader;
		GUserProfileManager->FinalizeGameLoading();

		// send it as octet stream
		return new CHTTPResponseData( "application/octet-stream", data );
	}
};

//-----

/// page with game shortcuts
class CDebugPageGameSaveGame : public IDebugPageHandlerHTML
{
public:
	CDebugPageGameSaveGame()
		: IDebugPageHandlerHTML( "/savegames/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Savegames"; }
	virtual StringAnsi GetCategory() const override { return "Game"; }
	virtual Bool HasIndexPage() const override { return (GGame != nullptr); }

	// information about savegame
	class SaveGameInfo : public CDebugPageHTMLTable::IRow
	{
	public:
		Red::System::DateTime		m_timestamp;
		StringAnsi					m_name;
		StringAnsi					m_type;
		Uint32						m_id;

		enum EColumn
		{
			eColumn_Id=1,
			eColumn_Timestamp=2,
			eColumn_Type=3,
			eColumn_Name=4,
			eColumn_Actions=5,
		};

	public:
		static StringAnsi GetSaveGameType( const SSavegameInfo& info )
		{
			if ( info.IsQMSave() ) return "QMSave";
			if ( info.IsACPSave() ) return "APCSave";
			if ( info.IsQuickSave() ) return "Quick";
			if ( info.IsManualSave() ) return "Manual";
			if ( info.IsAutoSave() ) return "Auto";
			if ( info.IsCheckPoint() ) return "Checkpoint";
			return "Unknown";
		}

		SaveGameInfo( const SSavegameInfo& info, const Uint32 id )
			: m_id( id )
			, m_timestamp( info.m_timeStamp )
			, m_name( UNICODE_TO_ANSI( info.GetDisplayName().AsChar() ) )
			, m_type( GetSaveGameType( info ) )
		{
		}

		virtual Bool OnCompare( const IRow* other, const Int32 columnID ) override
		{
			const auto* b = ((const SaveGameInfo*)other);

			switch ( columnID )
			{
				case eColumn_Timestamp: return m_timestamp < b->m_timestamp;
				case eColumn_Name: return m_name < b->m_name;
				case eColumn_Type: return m_type < b->m_type;
			}

			return m_id < b->m_id;
		}

		virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc, const Int32 columnID ) override
		{
			switch ( columnID )
			{
				case eColumn_Id: doc.Writef( "%d", m_id ); return;
				case eColumn_Timestamp: doc.Write( UNICODE_TO_ANSI( ToString( m_timestamp ).AsChar() ) ); return;
				case eColumn_Type: doc.Write( m_type.AsChar() ); return;
				case eColumn_Name: doc.Write( m_name.AsChar() ); return;
				case eColumn_Actions:
				{
					AnsiChar saveName[128];
					Red::SNPrintF( saveName, ARRAY_COUNT(saveName), "%sSave_[%02d_%02d_%04d  %02d_%02d_%02d]_%d", 
						m_type.AsChar(),
						m_timestamp.GetDay()+1, m_timestamp.GetMonth()+1, m_timestamp.GetYear(),
						m_timestamp.GetHour(), m_timestamp.GetMinute(), m_timestamp.GetSecond(), 
						m_id );

					doc.Link("/getsavegame/%s.sav?id=%d", saveName, m_id ).Write("Download");
					//doc.Write( "&nbsp/&nbsp" );
					//doc.Link("/dumpsavegame/%s.txt?id=%d", saveName, m_id ).Write("Dump");
					//doc.Write( "&nbsp/&nbsp" );
					//doc.Link("/savegames/?load=%d", m_id ).Write("Load");
					return;
				}
			}
		}
	};

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// no layers
		if ( !GGame )
		{
			doc << "<span class=\"error\">No active game</span>";
			return true;
		}

		// get savegames
		TDynArray< SSavegameInfo > savesToLoad;
		GUserProfileManager->GetSaveFiles( savesToLoad );

		// savegames
		{

			doc.Writef( "<p>Found %d savegame(s)</p>", savesToLoad.Size() );

			{
				CDebugPageHTMLInfoBlock info( doc, "Save games" );

				CDebugPageHTMLTable table( doc, "saves" );
				table.AddColumn( "Order", 60, true );
				table.AddColumn( "Timestamp", 200, true );
				table.AddColumn( "Type", 80, true );
				table.AddColumn( "Description", 300, true );
				table.AddColumn( "Actions", 240, false );

				for ( Uint32 i=0; i<savesToLoad.Size(); ++i )
				{
					const SSavegameInfo& saveInfo = savesToLoad[i];
					table.AddRow( new SaveGameInfo( saveInfo, i ) );
				}

				table.Render( 900, "generic", doc.GetURL() );
			}
		}

		return true;
	}
};

//-----

void InitGameShortcutsDebugPages()
{
	new CDebugPageGameShortcuts();
	new CDebugPageGameSaveGame();
	new CDebugPageDownloadSaveGame();
}

//-----


#endif