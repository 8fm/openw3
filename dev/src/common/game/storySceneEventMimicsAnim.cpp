/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventMimicsAnim.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "../engine/mimicComponent.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventMimicsAnim )

CStorySceneEventMimicsAnim::CStorySceneEventMimicsAnim()
	: CStorySceneEventAnimClip()
	, m_fullEyesWeight( true )
{

}

CStorySceneEventMimicsAnim::CStorySceneEventMimicsAnim( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const String& trackName )
	: CStorySceneEventAnimClip( eventName, sceneElement, startTime, actor, trackName )
	, m_fullEyesWeight( true )
{
}

CStorySceneEventMimicsAnim* CStorySceneEventMimicsAnim::Clone() const
{
	return new CStorySceneEventMimicsAnim( *this );
}

void CStorySceneEventMimicsAnim::Interface_SetDragAndDropMimicsAnimation( const TDynArray< CName >& animData, Float animDuration ) 
{
#ifndef NO_EDITOR
	SetAnimationState( animData ); 
	RefreshDuration( animDuration ); 
#endif
}

const CAnimatedComponent* CStorySceneEventMimicsAnim::GetAnimatedComponentForActor( const CStoryScenePlayer* scenePlayer ) const
{
	ASSERT( scenePlayer != NULL );

	const CActor* actor = Cast< CActor >( scenePlayer->GetSceneActorEntity( m_actor ) );

	if ( actor != NULL )
	{
		return actor->GetMimicComponent();
	}

	return NULL;
}

void CStorySceneEventMimicsAnim::OnAddExtraDataToEvent( StorySceneEventsCollector::MimicsAnimation& event ) const
{
	event.m_fullEyesWeight = m_fullEyesWeight;
}

#ifndef NO_EDITOR

void CStorySceneEventMimicsAnim::SetAnimationState( const TDynArray< CName >& data )
{
	SCENE_ASSERT( data.Size() == 3 );

	m_filterOption = data[ 0 ];
	m_friendlyName = data[ 1 ].AsString();
	m_animationName = data[ 2 ];

	m_eventName = m_friendlyName;

	if ( m_animationName.AsString() == TXT( "constant_brows_up_accent_face" ) )
	{
		m_blendIn = 0.2f;
		m_blendOut = 0.3f;
		m_weight = 0.5f;
	}
	else if ( m_animationName.AsString() == TXT( "constant_brows_down_accent_face" ) )
	{
		m_blendIn = 0.2f;
		m_blendOut = 0.3f;
		m_weight = 0.5f;
	}
	else if ( m_animationName.AsString() == TXT( "head_down_head_accent_face" ) )
	{
		m_blendIn = 0.25f;
		m_blendOut = 0.3f;
	}
}

void CStorySceneEventMimicsAnim::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	m_eventName = !m_friendlyName.Empty() ? m_friendlyName.AsChar() : m_animationName.AsString();

	if ( propertyName == CNAME(friendlyName) )
	{
		//m_animationName = GCommonGame->GetSystem< CStorySceneSystem >()->GetAnimationList().FindMimicsAnimationByFriendlyName( m_friendlyName );
		//OnPreviewPropertyChanged( previewPlayer, CNAME( animationName ) );
	}
}

#endif

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
