/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneSystem.h"
#include "..\core\debugPageHandler.h"
#include "..\core\debugPageHTMLDoc.h"
#include "..\core\objectMap.h"
#include "storyScenePlayer.h"
#include "storySceneAbstractLine.h"
#include "..\core\handleMap.h"
#include "questScenePlayer.h"
#include "questSceneBlock.h"
#include "storySceneInput.h"
#include "storySceneLine.h"
#include "storySceneSection.h"
#include "storySceneItems.h"

//----

#ifndef NO_DEBUG_PAGES

namespace Helper
{
	static void GetSceneFiles( CDirectory* dir, TDynArray< CDiskFile* >& files )
	{
		const String sceneExt( ResourceExtension<CStoryScene>() );

		// get all scenes in this directory
		for ( auto* ptr : dir->GetFiles() )
		{
			if ( ptr->GetFileName().EndsWith( sceneExt ) )
			{
				files.PushBack( ptr );
			}
		}

		// recurse
		for ( auto* ptr : dir->GetDirectories() )
			GetSceneFiles( ptr, files );
	}
}

//----

class CDebugHTMLRowStorySceneLoaded : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugHTMLRowStorySceneLoaded( THandle< CStoryScenePlayer > sa )
		: m_scene( sa )
	{
	}

	virtual Bool OnCompare( const IRowData* other ) const
	{
		const auto otherScene = ((const CDebugHTMLRowStorySceneLoaded*) other)->m_scene;
		return m_scene->GetStoryScene()->GetDepotPath() < otherScene->GetStoryScene()->GetDepotPath();
	}

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
	{
		CFilePath filePath( m_scene->GetStoryScene()->GetDepotPath() );
		doc.Link( "/scene/?id=%d", m_scene->GetObjectIndex() ).Writef( "%ls", filePath.GetFileName().AsChar() );
	}


private:
	THandle< CStoryScenePlayer >		m_scene;
};

class CDebugHTMLRowStorySceneNotLoaded : public CDebugPageHTMLTable::IRowData
{
public:
	CDebugHTMLRowStorySceneNotLoaded( const CDiskFile* file )
		: m_file( file )
	{
	}

	virtual Bool OnCompare( const IRowData* other ) const
	{
		const auto otherScene = ((const CDebugHTMLRowStorySceneNotLoaded*) other)->m_file;
		return m_file->GetDepotPath() < otherScene->GetDepotPath();
	}

	virtual void OnGenerateHTML( CDebugPageHTMLDocument& doc ) const
	{
		CFilePath filePath( m_file->GetDepotPath() );
		doc.Link( "/sceneloader/?path=%ls", m_file->GetDepotPath().AsChar() ).Writef( "%ls", filePath.GetFileName().AsChar() );
	}


private:
	const CDiskFile*		m_file;
};

/// playing scenes
class CDebugPageSceneLister : public IDebugPageHandlerHTML
{
public:
	CDebugPageSceneLister()
		: IDebugPageHandlerHTML( "/scenes/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Active scenes"; }
	virtual StringAnsi GetCategory() const override { return "Scenes"; }
	virtual Bool HasIndexPage() const override { return true; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		{
			CDebugPageHTMLInfoBlock info( doc, "Active scenes" );

			CDebugPageHTMLTable table( doc, "scenes" );
			table.AddColumn( "Gameplay", 60, true );
			table.AddColumn( "Scene", 400, true );
			table.AddColumn( "Section", 200, true );
			table.AddColumn( "Actors", 300, true );

			// get active scene players
			const auto& scenePlrs = GCommonGame->GetSystem< CStorySceneSystem >()->GetScenePlayers();
			for ( const auto player : scenePlrs )
			{			
				if ( player )
				{
					const auto& actors = player->GetSceneActors();

					StringAnsi actorString = "";
					for ( auto it = actors.Begin(); it != actors.End(); ++it )
					{
						if ( !actorString.Empty() )
							actorString += "; ";

						actorString += it->m_first.AsAnsiChar();
					}

					table.AddRow( table.CreateRowData( player->IsGameplay() ? "Gameplay" : "Normal" ),
							new CDebugHTMLRowStorySceneLoaded( player ),
							table.CreateRowData( player->GetCurrentSection() ? player->GetCurrentSection()->GetName() : TXT("None") ),
							table.CreateRowData( actorString ) );
				}
			}

			table.Render( 950, "generic", fullURL );
		}

		// valid page generated
		return true;
	}
};

//----

/// scene loader and player
class CDebugPageSceneStarter : public IDebugPageHandlerHTML
{
public:
	CDebugPageSceneStarter()
		: IDebugPageHandlerHTML( "/allscenes/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Scene loader"; }
	virtual StringAnsi GetCategory() const override { return "Scenes"; }
	virtual Bool HasIndexPage() const override { return true; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get all scenes from the depot (NOTE: results cached)
		static TDynArray< CDiskFile* > scenePaths;
		if ( scenePaths.Empty() )
		{
			Helper::GetSceneFiles( GDepot, scenePaths );
		}

		// show table with streaming locks
		{
			CDebugPageHTMLInfoBlock info( doc, "All loadable scenes" );

			CDebugPageHTMLTable table( doc, "scenes" );
			table.AddColumn( "Scene name", 500, true );

			for ( const auto* ptr : scenePaths )
			{
				table.AddRow( new CDebugHTMLRowStorySceneNotLoaded( ptr ) );
			}

			table.Render( 800, "generic", fullURL );
		}

		// done
		return true;
	}
};

//----

/// scene debug information
class CDebugPageSceneDebug : public IDebugPageHandlerHTML
{
public:
	CDebugPageSceneDebug()
		: IDebugPageHandlerHTML( "/scene/" )
	{}

	// The "global" test scene
	static CQuestScenePlayer* st_testPlayer;
	static TSoftHandle< CStoryScene > st_testPlayerSceneHandle;

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Scene debugger"; }
	virtual StringAnsi GetCategory() const override { return "Scenes"; }
	virtual Bool HasIndexPage() const override { return false; }

	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// Load/Start scene
		if ( relativeURL.HasKey( "play" ) )
		{
			// get scene path
			StringAnsi path;
			if ( !relativeURL.GetKey( "play", path ) )
				return false;

			// get scene input name
			StringAnsi sceneInput;
			if ( !relativeURL.GetKey( "input", sceneInput ) )
				return false;

			// load scene
			THandle< CStoryScene > scene = ::LoadResource< CStoryScene >( ANSI_TO_UNICODE( path.AsChar() ) );
			if ( !scene )
			{
				doc << "<span class=\"error\">Unable to load specified scene file</span>";
				return true;
			}

			// stop current scene
			if ( st_testPlayer )
			{
				st_testPlayer->Stop();
				st_testPlayer->Finish();

				delete st_testPlayer;
				st_testPlayer = nullptr;
			}

			// create new fake player
			st_testPlayerSceneHandle = scene->GetDepotPath();
			st_testPlayer = new CQuestScenePlayer( st_testPlayerSceneHandle, SSFM_SpawnAndForcePosition, ANSI_TO_UNICODE( sceneInput.AsChar() ), false, true );
			st_testPlayer->Play();
			st_testPlayer->Execute();
			st_testPlayer->Execute();
			st_testPlayer->Execute();

			// scene should be started, click to navigate to list of active scenes
			doc << "Scene started but it may take some time to load.<br>Click below to navigate to list of active scenes.<br><br>";
			doc.Link( "/scenes/" ).Write( "Active scenes" );
			return true;
		}

		// get object index
		Int32 objectID = -1;
		if ( !relativeURL.GetKey( "id", objectID ) )
			return false;

		// get object from object map that has given index
		CObject* object = nullptr;
		{
			CObjectsMap::ObjectIndexer indexer( GObjectsMap );
			object = indexer.GetObject( objectID );
			if ( !object )
			{
				doc << "<span class=\"error\">Invalid object</span>";
				return true;
			}
		}

		// invalid scene ?
		CStoryScenePlayer* player = Cast< CStoryScenePlayer >( object );
		if ( !player )
		{
			doc << "<span class=\"error\">Invalid scene (no scene player found)</span>";
			return true;
		}

		// generic scene debug
		{
			CDebugPageHTMLInfoBlock info( doc, "Scene player" );

			if ( player && player->GetStoryScene() && player->GetStoryScene()->GetFile() )
				info.Info( "Scene file: ").LinkFile( UNICODE_TO_ANSI( player->GetStoryScene()->GetFile()->GetDepotPath().AsChar() ) );

			info.Info( "Scene object: ").LinkObject( player->GetStoryScene() );
			info.Info( "Scene player: ").LinkObject( player );
			//info.Info( "Scene controller : ").LinkObject( player->GetSceneController() );
			info.Info( "Scene camera: ").LinkObject( (const CObject*) player->GetSceneCamera() );
			//info.Info( "Scene director: ").LinkObject( (CObject*) player->GetSceneDirector() );
		}

		// generic scene debug
		auto section = player->GetCurrentSection();
		if ( section )
		{
			{
				CDebugPageHTMLInfoBlock info( doc, "Current section" );
				info.Info( "Name: ").Write( UNICODE_TO_ANSI( section->GetName().AsChar() ) );
				info.Info( "Object: ").LinkObject( section );
				info.Info( "Time in: ").Writef( "%1.2fs", player->GetCurrentSectionTime() );
			}

			{
				CDebugPageHTMLInfoBlock info( doc, "Section streaming" );
				info.Info( "Streaming area tag: ").Write( section->GetStreamingAreaTag().AsAnsiChar() );
				info.Info( "Allowed camera jump: ").Writef( "%1.2fm", section->GetStreamingCameraAllowedJumpDistance() );
				info.Info( "Streaming lock: ").Write( section->GetStreamingLock() ? "Yes" : "No" );
				info.Info( "Use camera position: ").Write( section->GetStreamingUseCameraPosition() ? "Yes" : "No" );
			}

			{
				TDynArray< CAbstractStorySceneLine* > lines;
				section->GetLines( lines );

				if ( !lines.Empty() )
				{
					CDebugPageHTMLInfoBlock info( doc, "Section lines" );

					CDebugPageHTMLTable table( doc, "lines" );
					table.AddColumn( "ID", 80, true );
					table.AddColumn( "Voicetag", 150, true );
					table.AddColumn( "StringID", 80, true );
					table.AddColumn( "Text", 800, true );
					table.AddColumn( "SoundEvent", 100, true );

					for ( auto* ptr : lines)
					{
						auto* realLine = Cast< CStorySceneLine >( ptr );

						table.AddRow( 
							table.CreateRowData( ptr->GetElementID() ),
							table.CreateRowData( ptr->GetVoiceTag().AsAnsiChar() ),
							table.CreateRowData( (Int64)(ptr->GetLocalizedContent() ? ptr->GetLocalizedContent()->GetIndex() : 0) ),
							table.CreateRowData( ptr->GetLocalizedContent() ? ptr->GetLocalizedContent()->GetString() : String::EMPTY ),
							table.CreateRowData( realLine ? realLine->GetSoundEventName().AsChar() : StringAnsi::EMPTY ) );
					}

					table.Render( 800, "generic", fullURL );
				}
			}
		}

		// actors :)
		TDynArray< CName > sectionVoicetags;
		if ( section )
		{
			section->GetVoiceTags( sectionVoicetags );
		}

		const auto actors = player->GetSceneActors();
		for ( auto it = actors.Begin(); it != actors.End(); ++it )
		{
			const CName actorTag = it->m_first;
			CDebugPageHTMLInfoBlock info( doc, "Actor: %hs", actorTag.AsAnsiChar() );

			const auto entity = it->m_second;
			info.Info( "Entity: ").LinkObject( entity.Get() );
			info.Info( "Template: ").LinkObject( entity ? entity->GetEntityTemplate() : nullptr );

			const auto actor = Cast< CActor >( entity );
			if ( actor )
			{
				info.Info( "Voicetag: ").Write( actor->GetVoiceTag().AsAnsiChar() );
				info.Info( "In current section: ").Write( sectionVoicetags.Exist(actor->GetVoiceTag()) ? "Yes" : "No" );
			}
		}

		// done
		return true;
	}
};

//----

/// scene loader (display lists of inputs/sections)
class CDebugPageSceneLoader : public IDebugPageHandlerHTML
{
public:
	CDebugPageSceneLoader()
		: IDebugPageHandlerHTML( "/sceneloader/" )
	{}

private:
	// IDebugPageHandler interface
	virtual StringAnsi GetTitle() const override { return "Scene loader"; }
	virtual StringAnsi GetCategory() const override { return "Scenes"; }
	virtual Bool HasIndexPage() const override { return false; }


	//! Process command, commands are always passed via URLs, return NULL if not handled
	virtual bool OnFillPage( const class CBasicURL& fullURL, const class CBasicURL& relativeURL, class CDebugPageHTMLDocument& doc ) override
	{
		// check presence
		if ( !GGame || !GGame->GetActiveWorld() )
		{
			doc << "<span class=\"error\">No world is loaded</span>";
			return true;
		}

		// get scene path
		StringAnsi path;
		if ( !relativeURL.GetKey( "path", path ) )
			return false;

		// load scene
		THandle< CStoryScene > scene = ::LoadResource< CStoryScene >( ANSI_TO_UNICODE( path.AsChar() ) );
		if ( !scene )
		{
			doc << "<span class=\"error\">Unable to load specified scene file</span>";
			return true;
		}

		// general scene information
		{
			CDebugPageHTMLInfoBlock info( doc, "Scene information" );
			info.Info( "File: ").LinkFile( path );
			info.Info( "Object: ").LinkObject( scene );
			info.Info( "Name: ").Writef( "%ls", scene->GetSceneName().AsChar() );
		}

		// scene actors
		const auto actors = scene->GetSceneActorsDefinitions();
		for ( const auto* ptr : actors )
		{
			CDebugPageHTMLInfoBlock info( doc, "Scene actor" );
			info.Info( "Tags: ").Writef( "%ls", ptr->m_actorTags.ToString().AsChar() );
			info.Info( "Template: ").Writef( "%ls", ptr->m_entityTemplate.GetPath().AsChar() );

			StringAnsi appearanceFilter;
			for ( const auto tag : ptr->m_appearanceFilter )
			{
				if ( !appearanceFilter.Empty() )
					appearanceFilter += "; ";

				appearanceFilter += tag.AsAnsiChar();
			}
			info.Info( "Appearance filter: ").Write( appearanceFilter.AsChar() );
			info.Info( "Alias: ").Writef( "%ls", ptr->m_alias.AsChar() );

			info.Info( "Don't search by voicetag: ").Write( ptr->m_dontSearchByVoicetag ? "Yes" : "No" );
			info.Info( "Use hires shadows: ").Write( ptr->m_useHiresShadows ? "Yes" : "No" );
			info.Info( "Force spawn: ").Write( ptr->m_forceSpawn ? "Yes" : "No" );
			info.Info( "Use mimic: ").Write( ptr->m_useMimic ? "Yes" : "No" );
		}

		// display scene inputs
		TDynArray< CStorySceneInput* > sceneInputs;
		scene->CollectControlParts< CStorySceneInput >( sceneInputs );
		if ( !sceneInputs.Empty() )
		{
			CDebugPageHTMLInfoBlock info( doc, "Play from input" );

			for ( const auto* ptr : sceneInputs )
			{
				const auto inputName = ptr->GetName();
				if ( ptr->IsGameplay() )
				{
					info.Info( "%ls: ", inputName.AsChar() ).Write( "Gameplay only" );
				}
				else
				{
					info.Info( "%ls: ", inputName.AsChar() ).Link( "/scene/?play=%hs&input=%ls", path.AsChar(), inputName.AsChar() ).Write( "Play!" );
				}
			}
		}


		// valid page generated
		return true;
	}
};

CQuestScenePlayer* CDebugPageSceneDebug::st_testPlayer;
TSoftHandle< CStoryScene > CDebugPageSceneDebug::st_testPlayerSceneHandle;

void InitSceneDebugPages()
{
	new CDebugPageSceneLister(); // autoregister
	new CDebugPageSceneStarter(); // autoregister
	new CDebugPageSceneDebug(); // autoregister
	new CDebugPageSceneLoader(); // autoregister
}

void TickSceneDebugPages()
{
	if ( CDebugPageSceneDebug::st_testPlayer )
	{
		CDebugPageSceneDebug::st_testPlayer->Execute();
	}
}

#endif

//----
