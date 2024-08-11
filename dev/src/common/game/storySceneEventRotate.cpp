/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEvent.h"
#include "storySceneEventRotate.h"
#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "storySceneDirector.h"

#ifdef DEBUG_SCENES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CStorySceneEventRotate );

const CName CStorySceneEventRotate::BEH_STEP_EVENT( TXT( "Rotate" ) );
const Char* CStorySceneEventRotate::BEH_STEP_ANGLE( TXT( "rotateAngle" ) );

CStorySceneEventRotate::CStorySceneEventRotate()
	: CStorySceneEvent()
	, m_actor( CName::NONE )
	, m_angle( 0.0f )
	, m_toCamera( false )
	, m_instant( false )
	, m_absoluteAngle( false )
{

}

CStorySceneEventRotate::CStorySceneEventRotate( const String& eventName,
		CStorySceneElement* sceneElement, Float startTime, const CName& actor,
		const String& trackName )
	: CStorySceneEvent( eventName, sceneElement, startTime, trackName )
	, m_actor( actor )
	, m_angle( 0.0f )
	, m_toCamera( false )
	, m_instant( false )
	, m_absoluteAngle( false )
{

}

CStorySceneEventRotate* CStorySceneEventRotate::Clone() const
{
	return new CStorySceneEventRotate( *this );
}

void CStorySceneEventRotate::SetActor( const CName& actor )
{
	m_actor = actor;
}


void CStorySceneEventRotate::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	/*TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if( scenePlayer == NULL || scenePlayer->GetSceneDirector() == NULL )
	{
		return;
	}

	CActor* actor = scenePlayer->GetMappedActor( m_actor, false );
	if( actor == NULL )
	{
		SCENE_WARN( TXT( "Cannot find actor '%ls' for story event '%ls'" ), m_actor.AsString().AsChar(), m_eventName.AsChar() );
		return;
	}

	CCamera* camera = scenePlayer->GetSceneDirector()->GetActiveCamera();
	if( camera == NULL )
	{
		SCENE_ERROR( TXT( "Error getting camera" ) );
		return;
	}

	Float rotationAngle;

	if( m_toCamera )
	{
		// Rotate actor to face camera

		// Extract camera rotation
		CAnimatedComponent* ac = camera->GetRootAnimatedComponent();
		if( ac == NULL )
		{
			return;
		}

		const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
		if( provider == NULL )
		{
			return;
		}

		Int32 boneIndex = provider->FindBoneByName( TXT( "Camera_Node") );
		if( boneIndex < 0 )
		{
			return;
		}

		Matrix m = ac->GetBoneMatrixWorldSpace( (Uint32) boneIndex );

		Float cameraRotation = m.GetYaw();
		rotationAngle = 180.0f - cameraRotation - actor->GetWorldYaw();
	}
	else
	{
		rotationAngle = Clamp( m_angle, -180.0f, 180.0f );
	}

	EulerAngles initialActorRotation = actor->GetRotation();
	EngineTransform initialActorTransform = scenePlayer->GetSceneDirector()->GetActorPlacement( actor, true );

	if ( m_absoluteAngle == true && m_toCamera == false )
	{
		initialActorRotation = initialActorTransform.GetRotation();
	}


	if( m_instant )
	{
		EulerAngles desiredRotation = initialActorRotation - EulerAngles( 0.0f, 0.0f, rotationAngle );
		actor->Teleport( initialActorTransform.GetPosition(), desiredRotation );
		
		//actor->SetRotation( initialActorRotation - EulerAngles( 0.0f, 0.0f, rotationAngle ) );
	}
	else
	{
		rotationAngle = ( actor->GetRotation() - initialActorRotation ).Yaw + rotationAngle;

		// Set behavior variables
		Bool ok = actor->SetBehaviorVariable( BEH_STEP_ANGLE, rotationAngle / 180.0f ); // convert to -1 to 1 range (clockwise)
		ASSERT( ok );

		if( ! actor->RaiseBehaviorEvent( BEH_STEP_EVENT ) )
		{
			SCENE_WARN( TXT( "Raising behaviour event '%ls'" ), BEH_STEP_EVENT.AsString().AsChar() );
		}
	}*/
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
