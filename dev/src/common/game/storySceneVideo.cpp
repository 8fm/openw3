/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "storySceneVideo.h"
#include "storySceneGraphSocket.h"
#include "storyScenePlayer.h"
#include "storySceneControlPartsUtil.h"
#include "guiManager.h"
#include "../engine/viewport.h"
#include "../engine/videoPlayer.h"
#include "../engine/graphConnectionRebuilder.h"
#include "../engine/renderCommands.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneVideoSection )
IMPLEMENT_ENGINE_CLASS( CStorySceneVideoBlock )
IMPLEMENT_ENGINE_CLASS( CStorySceneVideoElement )

CStorySceneVideoSection::CStorySceneVideoSection()
	: m_suppressRendering( true )
{

}

void CStorySceneVideoSection::CollectControlPartsImpl( TDynArray< CStorySceneControlPart* >& controlParts, TDynArray< const CStorySceneControlPart* >& visitedControlParts )
{
	const Bool alreadyVisited = visitedControlParts.Exist( this );
	if( !alreadyVisited )
	{
		visitedControlParts.PushBack( this );

		CStorySceneControlPart* nextContolPart = StorySceneControlPartUtils::GetControlPartFromLink( GetNextElement() );
		if ( nextContolPart != NULL )
		{
			nextContolPart->CollectControlParts( controlParts, visitedControlParts );
		}
	}
}

void CStorySceneVideoSection::OnSectionCreate()
{
	TBaseClass::OnSectionCreate();

	if ( GetNumberOfElements() == 0 )
	{
		CStorySceneVideoElement* videoElement = CreateObject< CStorySceneVideoElement >( this );

		AddSceneElement( videoElement, 0 );
		NotifyAboutElementAdded( videoElement );
	}

	ASSERT( GetNumberOfElements() == 1 );
}

//////////////////////////////////////////////////////////////////////////

#ifndef NO_EDITOR_GRAPH_SUPPORT

String CStorySceneVideoBlock::GetBlockName() const
{
	String blockName = TXT( "Video" );
	if ( m_sceneVideo != NULL )
	{
		blockName = m_sceneVideo->GetName();
	}
	return blockName;
}

void CStorySceneVideoBlock::OnRebuildSockets()
{
	GraphConnectionRebuilder rebuilder( this );
	TBaseClass::OnRebuildSockets();

	if ( m_sceneVideo )
	{
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( In ), m_sceneVideo, LSD_Input ) );

		// Create output
		CreateSocket( StorySceneGraphSocketSpawnInfo( CNAME( Out ), m_sceneVideo, LSD_Output ) );
	}
}

#endif // NO_EDITOR_GRAPH_SUPPORT

//////////////////////////////////////////////////////////////////////////

IStorySceneElementInstanceData* CStorySceneVideoElement::OnStart( CStoryScenePlayer* player ) const
{
	return new CStorySceneVideoElementInstance( this, player );
}

void CStorySceneVideoElement::OnGetSchedulableElements( TDynArray< const CStorySceneElement* >& elements ) const
{
	elements.PushBack( this );
}

//////////////////////////////////////////////////////////////////////////

static void W3Hack_AdjustSystemsForVideo( const String& fileName )
{
	extern Bool GHackStorySceneVideoElementInstance;
	extern Uint32 GW3Hack_WhiteFadeOverride;

	SCENE_LOG(TXT("Disabling world tick for video %ls"), fileName.AsChar());
	GHackStorySceneVideoElementInstance = true;

	SSceneVideo sceneVideo;
	if ( GCommonGame->GetSystem< CStorySceneSystem >()->GetSceneVideo( fileName, sceneVideo ) )
	{
		if ( sceneVideo.m_whiteScreenCtrl == SSceneVideo::eWhiteScreen_Start )
		{
			( new CRenderCommand_W3HackSetVideoClearRGB( Color::WHITE ) )->Commit();
			( new CRenderCommand_W3HackShowVideoBackground )->Commit();
			GW3Hack_WhiteFadeOverride = 0xFFFFFFFF;
		}
		else if ( sceneVideo.m_whiteScreenCtrl == SSceneVideo::eWhiteScreen_End )
		{
			( new CRenderCommand_W3HackSetVideoClearRGB( Color::BLACK ) )->Commit();
			( new CRenderCommand_W3HackHideVideoBackground )->Commit();
			GW3Hack_WhiteFadeOverride = 1; // off for next time
		}
	}
}

void CStorySceneVideoElementInstance::OnPlay()
{
	m_hasVideoStarted = false;

	CStorySceneVideoSection* videoSection = Cast< CStorySceneVideoSection >( m_element->GetSection() );
	if ( videoSection )
	{
		m_player->PlayVideo( SVideoParams( videoSection->GetVideoName(), eVideoParamFlag_Preemptive, eVideoBuffer_Default ) );
		W3Hack_AdjustSystemsForVideo( videoSection->GetVideoName() );

		const TDynArray< String >& extraVideoFiles = videoSection->GetExtraVideoFileNames();
		for ( Uint32 i = 0; i < extraVideoFiles.Size(); ++i )
		{
			m_player->PlayVideo( SVideoParams( extraVideoFiles[ i ], eVideoParamFlag_None ) );
			W3Hack_AdjustSystemsForVideo( videoSection->GetVideoName() );
		}
	}	
}

void CStorySceneVideoElementInstance::OnStop()
{
	m_player->StopVideo();
	m_player->SetSceneBlackscreen( true, TXT("StorySceneVideo OnStop") );
	m_player->CallEvent( CNAME( OnMovieEnded ) );

	OnEnded();
}

Bool CStorySceneVideoElementInstance::OnTick( Float timeDelta )
{
	if ( !m_player->IsPlayingVideo() )
	{
		m_player->SetSceneBlackscreen( true, TXT("StorySceneVideo OnTick") );
		m_player->CallEvent( CNAME( OnMovieEnded ) );

		OnEnded();

		return false;
	}
	else if ( m_player->HasValidVideo() )
	{
		if ( !m_hasVideoStarted )
		{
			m_player->SceneFadeIn( TXT("Video") );
			m_player->CallEvent( CNAME( OnMovieStarted ) );
			m_hasVideoStarted = true;

			OnStarted();
		}
		else
		{
			CheckSubtitles();
		}
	}

	return true;
}

void CStorySceneVideoElementInstance::OnStarted()
{
	CStorySceneVideoSection* videoSection = Cast< CStorySceneVideoSection >( m_element->GetSection() );
	if ( videoSection )
	{
		if ( GGame->IsActive() )
		{
			if ( videoSection->ShouldSuppressRendering() && !m_hasSuppressedRender )
			{
				( new CRenderCommand_SuppressSceneRendering( GGame->GetViewport(), true ) )->Commit();
				m_hasSuppressedRender = true;
			}
		}

		CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
		if ( guiManager )
		{
			const String& text = videoSection->GetEventDescription();
			if ( text != String::EMPTY )
			{
				guiManager->DebugTextShow( text );
			}
			guiManager->OnVideoStarted( CNAME(VideoClient_StoryScenePlayer) );
		}
	}
}

void CStorySceneVideoElementInstance::OnEnded()
{
	CStorySceneVideoSection* videoSection = Cast< CStorySceneVideoSection >( m_element->GetSection() );
	if ( videoSection )
	{
		if ( GGame->IsActive() )
		{
			if ( videoSection->ShouldSuppressRendering() && m_hasSuppressedRender )
			{
				( new CRenderCommand_SuppressSceneRendering( GGame->GetViewport(), false ) )->Commit();
				m_hasSuppressedRender = false;
			}
		}
	}

	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	if ( guiManager )
	{
		const String& text = videoSection->GetEventDescription();
		if ( text != String::EMPTY )
		{
			guiManager->DebugTextHide();
		}

		guiManager->OnVideoSubtitles( CNAME(VideoClient_StoryScenePlayer), String::EMPTY ); // clear active subtitles if movie skipped
		guiManager->OnVideoStopped( CNAME(VideoClient_StoryScenePlayer) );
	}

	extern Bool GHackStorySceneVideoElementInstance;
	GHackStorySceneVideoElementInstance = false;
	SCENE_LOG(TXT("Restored world tick for video"));
}

void CStorySceneVideoElementInstance::CheckSubtitles()
{
	CGuiManager* guiManager = GCommonGame ? GCommonGame->GetGuiManager() : NULL;
	if ( guiManager )
	{
		String subtitles;
		if ( m_player->GetVideoSubtitles( subtitles ) )
		{
			guiManager->OnVideoSubtitles( CNAME(VideoClient_StoryScenePlayer), subtitles );
		}
	}
}