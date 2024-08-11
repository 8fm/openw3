/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "storySceneEventLookat.h"

#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/skeletalAnimationContainer.h"
#include "../engine/mimicComponent.h"

#include "storyScenePlayer.h"
#include "sceneLog.h"
#include "storySceneSection.h"
#include "lookAtTypes.h"
#include "storySceneEventsCollector_events.h"
#include "storySceneUtils.h"
#include "storySceneEvent.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

RED_DEFINE_STATIC_NAME( lookatOn )
RED_DEFINE_STATIC_NAME( lookAtTarget )
RED_DEFINE_STATIC_NAME( blink_normal_face )

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( SStorySceneEventLookAtBlinkSettings );

SStorySceneEventLookAtBlinkSettings::SStorySceneEventLookAtBlinkSettings()
	: m_canCloseEyes( true )
	, m_forceCloseEyes( false )
	, m_animationName( CNAME( blink_normal_face ) )
	, m_startOffset( 0.02f )
	, m_durationPercent( 0.7f )
	, m_horizontalAngleDeg( 40.f )
{

}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventLookAtDuration );

const Vector CStorySceneEventLookAtDuration::DEFAULT_STATIC_POINT( Vector( 0.f, 2.f, 1.75f ) );

CStorySceneEventLookAtDuration::CStorySceneEventLookAtDuration()
	: m_useTwoTargets( false )
	, m_bodyEnabled( true )
	, m_bodyInstant( false )
	, m_bodyWeight( 1.f )
	, m_bodyStaticPointWS( DEFAULT_STATIC_POINT )
	, m_eyesInstant( false )
	, m_eyesWeight( 1.f )
	, m_eyesStaticPointWS( DEFAULT_STATIC_POINT )
	, m_eyesLookAtConvergenceWeight( 0.22f )
	, m_eyesLookAtIsAdditive( true )
	, m_type( DLT_Dynamic )
	, m_level( LL_Body )
	, m_sceneRange( 180.f )
	, m_gameplayRange( 120.f )
	, m_limitDeact( false )
	//, m_useBodyWeightCurve2( false )
	, m_bodyTransitionWeight( 1.f )
	, m_eyesTransitionFactor( 2.f )
	, m_resetCloth( DRCDT_None )
	, m_oldLookAtEyesSpeed( 0.f )
	, m_oldLookAtEyesDampScale( 1.f )
	, m_usesNewTransition(false) // old events have false
{

}

CStorySceneEventLookAtDuration::CStorySceneEventLookAtDuration( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.0f, trackName )
	, m_actor( actor )
	, m_useTwoTargets( false )
	, m_bodyTarget( target )
	, m_bodyEnabled( enabled )
	, m_bodyInstant( false )
	, m_bodyWeight( 1.f )
	, m_bodyStaticPointWS( DEFAULT_STATIC_POINT )
	, m_eyesTarget( target )
	, m_eyesEnabled( enabled )
	, m_eyesInstant( false )
	, m_eyesWeight( 1.f )
	, m_eyesStaticPointWS( DEFAULT_STATIC_POINT )
	, m_eyesLookAtConvergenceWeight( 0.22f )
	, m_eyesLookAtIsAdditive( true )
	, m_type( DLT_Dynamic )
	, m_level( LL_Body )
	, m_sceneRange( 180.f )
	, m_gameplayRange( 120.f )
	, m_limitDeact( false )
	//, m_useBodyWeightCurve2( false )
	, m_bodyTransitionWeight( 1.f )
	, m_eyesTransitionFactor( 2.f )
	, m_resetCloth( DRCDT_None )
	, m_oldLookAtEyesSpeed( 0.f )
	, m_oldLookAtEyesDampScale( 1.f )
	, m_usesNewTransition(true) // new events have true
{
	InitializeWeightCurve();
}

/*
Cctor.

Compiler generated cctor would also copy instance vars - we don't want that.
*/
CStorySceneEventLookAtDuration::CStorySceneEventLookAtDuration( const CStorySceneEventLookAtDuration& other )
	: CStorySceneEventDuration( other )
	, m_actor( other.m_actor )
	, m_useTwoTargets( other.m_useTwoTargets )
	, m_bodyTarget( other.m_bodyTarget )
	, m_bodyEnabled( other.m_bodyEnabled )
	, m_bodyInstant( other.m_bodyInstant )
	, m_bodyWeight( other.m_bodyWeight )
	, m_bodyStaticPointWS( other.m_bodyStaticPointWS )
	, m_type( other.m_type )
	, m_level( other.m_level )
	//, m_useBodyWeightCurve2( other.m_useBodyWeightCurve2 )
	//, m_bodyWeightCurve( other.m_bodyWeightCurve )
	, m_bodyTransitionWeight( other.m_bodyTransitionWeight )
	, m_eyesTarget( other.m_eyesTarget )
	, m_eyesEnabled( other.m_eyesEnabled )
	, m_eyesInstant( other.m_eyesInstant )
	, m_eyesWeight( other.m_eyesWeight )
	, m_eyesTransitionFactor( other.m_eyesTransitionFactor )
	, m_eyesStaticPointWS( other.m_eyesStaticPointWS )
	, m_eyesLookAtConvergenceWeight( other.m_eyesLookAtConvergenceWeight )
	, m_eyesLookAtIsAdditive( other.m_eyesLookAtIsAdditive )
	, m_sceneRange( other.m_sceneRange )
	, m_gameplayRange( other.m_gameplayRange )
	, m_limitDeact( other.m_limitDeact )
	, m_oldLookAtEyesSpeed( other.m_oldLookAtEyesSpeed )
	, m_oldLookAtEyesDampScale( other.m_oldLookAtEyesDampScale )
	, m_resetCloth( other.m_resetCloth )
	, m_blinkSettings( other.m_blinkSettings )
	, m_usesNewTransition( other.m_usesNewTransition )
{}

CStorySceneEventLookAtDuration* CStorySceneEventLookAtDuration::Clone() const
{
	return new CStorySceneEventLookAtDuration( *this );
}

void CStorySceneEventLookAtDuration::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( m_type == DLT_Static )
	{
		m_type = DLT_Dynamic;
	}
}

void CStorySceneEventLookAtDuration::DoLookAt( const CStorySceneInstanceBuffer& instanceBuffer, StorySceneEventsCollector::ActorLookAt& event, const SStorySceneEventTimeInfo& timeInfo, Float weight, const CStoryScenePlayer* scenePlayer ) const
{
	if ( m_actor )
	{
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_eventTimeAbs = timeInfo.m_timeAbs;
		const Float curveVal = weight; // ? Clamp( m_bodyWeightCurve.GetFloatValue( timeInfo.m_progress ), 0.f, 1.f ) : weight;

		event.m_curveValue = curveVal;
		event.m_duration = GetInstanceDuration( instanceBuffer );
		event.m_useTwoTargets = m_useTwoTargets;

		event.m_bodyTarget = m_bodyTarget;
		event.m_bodyEnabled = m_bodyEnabled;
		event.m_bodyInstant = m_bodyInstant;
		event.m_bodyWeight = m_bodyEnabled ? m_bodyWeight : 1.f - m_bodyWeight;
		event.m_bodyUseWeightCurve = false; //m_useBodyWeightCurve2;
		event.m_bodyTransitionWeight = m_bodyTransitionWeight;
		event.m_bodyStaticPointWS = m_bodyStaticPointWS;

		event.m_eyesTarget = m_eyesTarget;
		event.m_eyesEnabled = m_eyesEnabled;
		event.m_eyesInstant = m_eyesInstant;
		event.m_eyesWeight = weight * m_eyesWeight;
		event.m_eyesStaticPointWS = m_eyesStaticPointWS;
		event.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
		event.m_eyesLookAtIsAdditive = m_eyesLookAtIsAdditive;
		event.m_eyesTransitionFactor = m_eyesTransitionFactor;

		event.m_type = m_type;
		event.m_level = m_level;

		event.m_sceneRange = m_sceneRange;
		event.m_gameplayRange = m_gameplayRange;
		event.m_limitDeact = m_limitDeact;

		event.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

		event.m_oldLookAtEyesSpeed = m_oldLookAtEyesSpeed;

		event.m_id = GetGUID();
		event.m_blinkHorAngleDeg = UseBlink() ? Clamp( m_blinkSettings.m_horizontalAngleDeg, 0.f, 360.f ) : 0.f;

		event.m_useDeformationMS = m_usesNewTransition;
	}
}

void CStorySceneEventLookAtDuration::DoOldEyesLookAt( CStoryScenePlayer* scenePlayer, Bool instant, CStorySceneEventsCollector& collector ) const
{
	CActor* actor = m_actor ? scenePlayer->GetMappedActor( m_actor ) : nullptr;
	if ( !actor || !scenePlayer || !scenePlayer->GetSceneDirector() )
	{
		return;
	}
	CActor* target = m_bodyTarget ? scenePlayer->GetMappedActor( m_bodyTarget ) : nullptr;

	//////////////////////////////////////////////////////////////////////////
	// For debug
	String desc = TXT("Scene");
	if( GetSceneElement() && GetSceneElement()->GetSection() && GetSceneElement()->GetSection()->GetScene() )
	{
		desc = String::Printf( TXT("Scene: %s, Section: %s, Time %.2f"), GetSceneElement()->GetSection()->GetScene()->GetFriendlyName().AsChar(), GetSceneElement()->GetSection()->GetName().AsChar(), m_startPosition );
	}
	//////////////////////////////////////////////////////////////////////////

	Bool isInGameplay = scenePlayer->IsInGameplay();

	const CStorySceneSection* currentSection = scenePlayer->GetCurrentSection();
	if ( currentSection && currentSection->IsGameplay() && !currentSection->CanHaveLookats() )
	{
		return;
	}

	const Float range = 180.f;
	EDialogLookAtType type = m_type;

	Float headRotationRatio = 1.f;
	Bool eyesLookAtIsAdditive = true;

	StorySceneEventsCollector::ActorLookAtTick lookatData( this, m_actor );
	lookatData.m_disableSpeed = m_oldLookAtEyesSpeed;
	lookatData.m_enable = m_bodyEnabled;
	lookatData.m_type = type;
	lookatData.m_isSetByNewLookat = true;
	lookatData.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();

	if ( !m_bodyEnabled )
	{
		collector.AddEvent( lookatData );
	}
	else
	{
		if ( type == DLT_Dynamic && target != NULL )
		{
			const CAnimatedComponent* ac =  target->GetRootAnimatedComponent();
			Int32 bone = target->GetHeadBone();			
			ASSERT( ac && bone != -1 );
			if ( ac && bone != -1 )
			{
				SLookAtDialogBoneInfo& info = lookatData.m_infoA;
				info.m_boneIndex = bone;
				info.m_targetOwner = ac;
				info.m_level = LL_Eyes;
				info.m_speedOverride = m_oldLookAtEyesSpeed;
				info.m_instant = instant;
				info.m_range = range;
				info.m_autoLimitDeact = m_limitDeact;
				info.m_desc = desc;
				info.m_headRotationRatio = headRotationRatio;
				info.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
				info.m_eyesLookAtIsAdditive = m_eyesLookAtIsAdditive;
				info.m_eyesLookAtDampScale = m_oldLookAtEyesDampScale;
			}
			collector.AddEvent( lookatData );
		}
		else 
		{
			if ( type == DLT_Static && target != NULL )
			{	
				SLookAtDialogDynamicInfo& info = lookatData.m_infoC;
				info.m_level = LL_Eyes;
				info.m_speedOverride = m_oldLookAtEyesSpeed;
				info.m_instant = instant;
				info.m_range = range;
				info.m_autoLimitDeact = m_limitDeact;
				info.m_desc = desc;
				info.m_headRotationRatio = headRotationRatio;
				info.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
				info.m_eyesLookAtIsAdditive = eyesLookAtIsAdditive;
				info.m_eyesLookAtDampScale = m_oldLookAtEyesDampScale;
				lookatData.m_targetId = m_bodyTarget;
				collector.AddEvent( lookatData );

			}							
			else if ( type == DLT_StaticPoint )
			{
				SLookAtDialogStaticInfo& info = lookatData.m_infoB;
				info.m_level = LL_Eyes;
				info.m_speedOverride = m_oldLookAtEyesSpeed;
				info.m_instant = instant;
				info.m_range = range;
				info.m_autoLimitDeact = m_limitDeact;
				info.m_desc = desc;
				info.m_headRotationRatio = headRotationRatio;
				info.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
				info.m_eyesLookAtIsAdditive = eyesLookAtIsAdditive;
				info.m_eyesLookAtDampScale = m_oldLookAtEyesDampScale;
				lookatData.m_staticPoint = m_bodyStaticPointWS;
				collector.AddEvent( lookatData );
			}
		}
	}
}

void CStorySceneEventLookAtDuration::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_blinkAnimationDuration;
}

void CStorySceneEventLookAtDuration::OnInit( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer ) const
{
	TBaseClass::OnInit( data, scenePlayer );

	const CSkeletalAnimationSetEntry* anim = FindMimicAnimation( scenePlayer, m_blinkSettings.m_animationName );

	data[ i_blinkAnimationDuration ] = anim && anim->GetAnimation() ? anim->GetAnimation()->GetDuration() : 0.f;
}

void CStorySceneEventLookAtDuration::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( IsInstant() )
	{
		StorySceneEventsCollector::ActorLookAt event( this, m_actor );

		DoLookAt( data, event, timeInfo, 1.f, scenePlayer );

		collector.AddEvent( event );
	}
	else if ( m_resetCloth != DRCDT_None )
	{
		StorySceneEventsCollector::ActorResetClothAndDangles cEvt( this, m_actor );
		cEvt.m_eventTimeAbs = timeInfo.m_timeAbs;
		cEvt.m_eventTimeLocal = timeInfo.m_timeLocal;
		cEvt.m_forceRelaxedState = m_resetCloth == DRCDT_ResetAndRelax;
	}

	{
		if ( CActor* actor = const_cast< CStoryScenePlayer* >( scenePlayer )->GetMappedActor( m_actor ) )
		{
			actor->DisableDialogsLookAts( 0.f );
		}

		DoOldEyesLookAt( scenePlayer, IsInstant(), collector );
	}
}

void CStorySceneEventLookAtDuration::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );
	
	if ( !IsInstant() )
	{
		{
			StorySceneEventsCollector::ActorLookAt event( this, m_actor );

			DoLookAt( data, event, timeInfo, timeInfo.m_progress, scenePlayer );

			collector.AddEvent( event );
		}

		{
			SAnimationState animationState;
			if ( ShouldPlayBlink( data, timeInfo, animationState ) )
			{
				StorySceneEventsCollector::MimicsAnimation event( this, m_actor );

				event.m_eventTimeAbs = timeInfo.m_timeAbs;
				event.m_eventTimeLocal = timeInfo.m_timeLocal;
				event.m_weight = 1.f;
				event.m_animationState = animationState;
				event.m_ID = GetGUID();

				collector.AddEvent( event );
			}
		}
	}
}

void CStorySceneEventLookAtDuration::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	if ( !IsInstant() )
	{
		StorySceneEventsCollector::ActorLookAt event( this, m_actor );

		DoLookAt( data, event, timeInfo, 1.f, scenePlayer );

		collector.AddEvent( event );
	}
	{
		StorySceneEventsCollector::MimicsAnimation event( this, m_actor );

		collector.RemoveEvent( event );
	}
}

Bool CStorySceneEventLookAtDuration::ShouldPlayBlink( CStorySceneInstanceBuffer& data, const SStorySceneEventTimeInfo& timeInfo, SAnimationState& outAnimState ) const
{
	if ( UseBlink() )
	{
		const Float animDuration = data[ i_blinkAnimationDuration ];
		if ( animDuration > 0.f )
		{
			const Float evtDuration = GetInstanceDuration( data );
			if ( ( animDuration > 0.f && evtDuration > animDuration + m_blinkSettings.m_startOffset ) || m_blinkSettings.m_forceCloseEyes )
			{
				const Float animTime = timeInfo.m_timeLocal - m_blinkSettings.m_startOffset;
				const Float duration = evtDuration - m_blinkSettings.m_startOffset;

				SCENE_ASSERT( duration > 0.f );

				if ( duration > 0.f && animTime > 0.f )
				{
					const Float animScale = animDuration / ( duration * Clamp( m_blinkSettings.m_durationPercent, 0.f, 1.f ) );
					const Float finalAnimTime = animTime * animScale;
					if ( finalAnimTime <= animDuration )
					{
						outAnimState.m_currTime = finalAnimTime;
						outAnimState.m_prevTime = outAnimState.m_currTime;
						outAnimState.m_animation = m_blinkSettings.m_animationName;

						return true;
					}
				}
			}
		}
	}

	return false;
}

const CSkeletalAnimationSetEntry* CStorySceneEventLookAtDuration::FindMimicAnimation( const CStoryScenePlayer* scenePlayer, const CName& animName ) const
{
	if ( const CActor* actor = Cast< const CActor >( scenePlayer->GetSceneActorEntity( m_actor ) ) )
	{
		if ( const CMimicComponent* m = actor->GetMimicComponent() )
		{
			const CSkeletalAnimationSetEntry* anim = m->GetAnimationContainer() ? m->GetAnimationContainer()->FindAnimation( animName ) : nullptr;
			if ( anim && anim->GetAnimation() )
			{
				return anim;
			}
		}
	}

	return nullptr;
}

#ifndef NO_EDITOR
void CStorySceneEventLookAtDuration::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	if ( propertyName == TXT("type") && m_type == DLT_StaticPoint )
	{
		CalcStaticPointInSS( previewPlayer );
	}
	//else if ( propertyName == TXT("useBodyWeightCurve") && m_useBodyWeightCurve2 )
	//{
	//	InitializeWeightCurve();
	//}
	else if ( propertyName == TXT("duration") && m_resetCloth == DRCDT_None )
	{
		m_resetCloth = DRCDT_Reset;
	}
}

void CStorySceneEventLookAtDuration::CalcStaticPointInSS( const CStoryScenePlayer* previewPlayer )
{
	if ( Vector::Near3( m_bodyStaticPointWS, DEFAULT_STATIC_POINT ) )
	{
		const Matrix mat = previewPlayer->GetActorPosition( m_actor );

		const EngineTransform sceneWS = previewPlayer->GetSceneDirector()->GetCurrentScenePlacement();
		Matrix sceneW2L;
		sceneWS.CalcWorldToLocal( sceneW2L );

		const Matrix finalMat = Matrix::Mul( sceneW2L, mat );

		m_bodyStaticPointWS = finalMat.TransformPoint( m_bodyStaticPointWS );
	}
}

#endif

void CStorySceneEventLookAtDuration::InitializeWeightCurve()
{
	/*m_bodyWeightCurve.Clear();
	m_bodyWeightCurve.AddPoint( 0.f,	0.f,	Vector( -0.1f,		0.f,		0.1391f,	0.0023f ),	CST_Bezier );
	m_bodyWeightCurve.AddPoint( 0.142f, 0.058f, Vector( -0.024f,	-0.0239f,	0.053f,		0.057f ),	CST_Bezier );
	m_bodyWeightCurve.AddPoint( 0.33f,	0.47f,	Vector( -0.086f,	-0.174f,	0.0865f,	0.174f ),	CST_BezierSmoothSymertric );
	m_bodyWeightCurve.AddPoint( 1.f,	1.f,	Vector( -0.1f,		0.f,		0.1f,		0.f ),		CST_Interpolate );*/
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventLookAt );

CStorySceneEventLookAt::CStorySceneEventLookAt()
	: m_enabled( true )
   , m_speed( 0.f )
   , m_level( LL_Body )
   , m_type( DLT_Dynamic )
   , m_instant( false )
   , m_range( 180.f )
   , m_gameplayRange( 120.0f )
   , m_limitDeact( false )
   , m_staticPoint( Vector( 0.f, 2.f, 1.75f ) )
   , m_headRotationRatio(1.f)
   , m_eyesLookAtConvergenceWeight( 0.22f )
   , m_eyesLookAtDampScale( 1.f )
   , m_eyesLookAtIsAdditive( true )
   , m_resetCloth( DRCDT_None )
{

}

CStorySceneEventLookAt::CStorySceneEventLookAt( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName )
   : CStorySceneEvent( eventName, sceneElement, startTime, trackName )
   , m_actor( actor )
   , m_target( target )
   , m_enabled( enabled )
   , m_speed( 0.f )
   , m_level( LL_Head )
   , m_type( DLT_Dynamic )
   , m_instant( false )
   , m_range( 180.f )
   , m_gameplayRange( 120.0f )
   , m_limitDeact( false )
   , m_staticPoint( Vector( 0.f, 2.f, 1.75f ) )
   , m_headRotationRatio(1.f)
   , m_eyesLookAtConvergenceWeight( 0.22f )
   , m_eyesLookAtDampScale( 1.f )
   , m_eyesLookAtIsAdditive( true )
   , m_resetCloth( DRCDT_None )
{

}

CStorySceneEventLookAt* CStorySceneEventLookAt::Clone() const
{
	return new CStorySceneEventLookAt( *this );
}

void CStorySceneEventLookAt::OnPostLoad()
{
	TBaseClass::OnPostLoad();

	if ( m_type == DLT_Static )
	{
		m_type = DLT_Dynamic;
	}
}

void CStorySceneEventLookAt::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	DoLookAt( scenePlayer, m_instant, collector );

	// CStorySceneEventExitActor jest tez look at

	if ( m_resetCloth != DRCDT_None )
	{
		StorySceneEventsCollector::ActorResetClothAndDangles cEvt( this, m_actor );
		cEvt.m_eventTimeAbs = timeInfo.m_timeAbs;
		cEvt.m_eventTimeLocal = timeInfo.m_timeLocal;
		cEvt.m_forceRelaxedState = m_resetCloth == DRCDT_ResetAndRelax;

		collector.AddEvent( cEvt );
	}
}

void CStorySceneEventLookAt::DoLookAt( CStoryScenePlayer* scenePlayer, Bool instant, CStorySceneEventsCollector& collector ) const
{

	CActor* actor = m_actor ? scenePlayer->GetMappedActor( m_actor ) : nullptr;
	if ( !actor || !scenePlayer || !scenePlayer->GetSceneDirector() )
	{
		return;
	}
	CActor* target = m_target ? scenePlayer->GetMappedActor( m_target ) : nullptr;

	//////////////////////////////////////////////////////////////////////////
	// For debug
	String desc = TXT("Scene");
	if( GetSceneElement() && GetSceneElement()->GetSection() && GetSceneElement()->GetSection()->GetScene() )
	{
		desc = String::Printf( TXT("Scene: %s, Section: %s, Time %.2f"), GetSceneElement()->GetSection()->GetScene()->GetFriendlyName().AsChar(), GetSceneElement()->GetSection()->GetName().AsChar(), m_startPosition );
	}
	//////////////////////////////////////////////////////////////////////////

	Bool isInGameplay = scenePlayer->IsInGameplay();

	const CStorySceneSection* currentSection = scenePlayer->GetCurrentSection();
	if ( currentSection && currentSection->IsGameplay() && !currentSection->CanHaveLookats() )
	{
		return;
	}

	Float range = isInGameplay ? m_gameplayRange : m_range;
	EDialogLookAtType type = isInGameplay ? DLT_Dynamic : m_type;

	StorySceneEventsCollector::ActorLookAtTick lookatData( this, m_actor );
	lookatData.m_disableSpeed = m_speed;
	lookatData.m_enable = m_enabled;
	lookatData.m_type = type;
	if ( !m_enabled )
	{
		collector.AddEvent( lookatData );
	}
	else
	{
		if ( type == DLT_Dynamic && target != NULL )
		{
			const CAnimatedComponent* ac =  target->GetRootAnimatedComponent();
			Int32 bone = target->GetHeadBone();			
			ASSERT( ac && bone != -1 );
			if ( ac && bone != -1 )
			{
				SLookAtDialogBoneInfo& info = lookatData.m_infoA;
				info.m_boneIndex = bone;
				info.m_targetOwner = ac;
				info.m_level = m_level;
				info.m_speedOverride = m_speed;
				info.m_instant = instant;
				info.m_range = range;
				info.m_autoLimitDeact = m_limitDeact;
				info.m_desc = desc;
				info.m_headRotationRatio = m_headRotationRatio;
				info.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
				info.m_eyesLookAtIsAdditive = m_eyesLookAtIsAdditive;
				info.m_eyesLookAtDampScale = m_eyesLookAtDampScale;			
			}
			collector.AddEvent( lookatData );
		}
		else 
		{
			SLookAtDialogStaticInfo& info = lookatData.m_infoB;
			info.m_level = m_level;
			info.m_speedOverride = m_speed;
			info.m_instant = instant;
			info.m_range = range;
			info.m_autoLimitDeact = m_limitDeact;
			info.m_desc = desc;
			info.m_headRotationRatio = m_headRotationRatio;
			info.m_eyesLookAtConvergenceWeight = m_eyesLookAtConvergenceWeight;
			info.m_eyesLookAtIsAdditive = m_eyesLookAtIsAdditive;
			info.m_eyesLookAtDampScale = m_eyesLookAtDampScale;
			
			if ( type == DLT_Static && target != NULL )
			{				
				lookatData.m_targetId = m_target;
				collector.AddEvent( lookatData );
			}							
			else if ( type == DLT_StaticPoint )
			{				
				lookatData.m_staticPoint = m_staticPoint;
				collector.AddEvent( lookatData );
			}
		}
	}
}

#ifndef NO_EDITOR

void CStorySceneEventLookAt::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	if ( propertyName == TXT("instant") )
	{
		if ( m_instant )
		{
			if ( m_resetCloth == DRCDT_None )
			{
				m_resetCloth = DRCDT_Reset;
			}
		}
		else
		{
			m_resetCloth = DRCDT_None;
		}
	}
}

#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CStorySceneEventGameplayLookAt );

const Vector CStorySceneEventGameplayLookAt::DEFAULT_STATIC_POINT( Vector( 0.f, 2.f, 1.75f ) );

CStorySceneEventGameplayLookAt::CStorySceneEventGameplayLookAt()
	: m_enabled( true )
	, m_instant( false )
	, m_weight( 1.f )
	, m_staticPoint( DEFAULT_STATIC_POINT )
	, m_type( DLT_Dynamic )
	, m_useWeightCurve( true )
	, m_behaviorVarWeight( CNAME( lookatOn ) )
	, m_behaviorVarTarget( CNAME( lookAtTarget ) )
{

}

CStorySceneEventGameplayLookAt::CStorySceneEventGameplayLookAt( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const CName& actor, const CName& target, Bool enabled, const String& trackName )
	: CStorySceneEventDuration( eventName, sceneElement, startTime, 1.0f, trackName )
	, m_actor( actor )
	, m_target( target )
	, m_enabled( enabled )
	, m_instant( false )
	, m_weight( 1.f )
	, m_staticPoint( DEFAULT_STATIC_POINT )
	, m_type( DLT_Dynamic )
	, m_useWeightCurve( true )
	, m_behaviorVarWeight( CNAME( lookatOn ) )
	, m_behaviorVarTarget( CNAME( lookAtTarget ) )
{
	InitializeWeightCurve();
}

CStorySceneEventGameplayLookAt* CStorySceneEventGameplayLookAt::Clone() const
{
	return new CStorySceneEventGameplayLookAt( *this );
}

void CStorySceneEventGameplayLookAt::DoLookAt( const CStorySceneInstanceBuffer& instanceBuffer, StorySceneEventsCollector::ActorGameplayLookAt& event, const SStorySceneEventTimeInfo& timeInfo, const CStoryScenePlayer* scenePlayer ) const
{
	if ( m_actor )
	{
		event.m_eventTimeLocal = timeInfo.m_timeLocal;
		event.m_eventTimeAbs = timeInfo.m_timeAbs;

		const Float curveVal = m_useWeightCurve ? Clamp( m_weightCurve.GetFloatValue( timeInfo.m_progress ), 0.f, 1.f ) : BehaviorUtils::BezierInterpolation( timeInfo.m_progress );
		event.m_curveValue = curveVal;
		event.m_duration = GetInstanceDuration( instanceBuffer );
		event.m_weight = m_weight;

		event.m_target = m_target;
		event.m_enabled = m_enabled;
		event.m_instant = m_instant;
		event.m_staticPointSS = m_staticPoint;
		event.m_type = m_type;
		event.m_sceneTransformWS = scenePlayer->GetSceneDirector()->GetCurrentScenePlacement();
		event.m_behaviorVarWeight = m_behaviorVarWeight;
		event.m_behaviorVarTarget = m_behaviorVarTarget;
	}
}

void CStorySceneEventGameplayLookAt::OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnStart( data, scenePlayer, collector, timeInfo );

	if ( CActor* actor = const_cast< CStoryScenePlayer* >( scenePlayer )->GetMappedActor( m_actor ) )
	{
		actor->DisableLookAts();
	}

	if ( IsInstant() )
	{
		StorySceneEventsCollector::ActorGameplayLookAt event( this, m_actor );

		DoLookAt( data, event, timeInfo, scenePlayer );

		collector.AddEvent( event );
	}
}

void CStorySceneEventGameplayLookAt::OnProcess( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnProcess( data, scenePlayer, collector, timeInfo );

	if ( !IsInstant() )
	{
		StorySceneEventsCollector::ActorGameplayLookAt event( this, m_actor );

		DoLookAt( data, event, timeInfo, scenePlayer );

		collector.AddEvent( event );
	}
}

void CStorySceneEventGameplayLookAt::OnEnd( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const
{
	TBaseClass::OnEnd( data, scenePlayer, collector, timeInfo );

	if ( !IsInstant() )
	{
		StorySceneEventsCollector::ActorGameplayLookAt event( this, m_actor );

		DoLookAt( data, event, timeInfo, scenePlayer );

		collector.AddEvent( event );
	}
}

#ifndef NO_EDITOR
void CStorySceneEventGameplayLookAt::OnPreviewPropertyChanged( const CStoryScenePlayer* previewPlayer, const CName& propertyName )
{
	TBaseClass::OnPreviewPropertyChanged( previewPlayer, propertyName );

	if ( propertyName == TXT("type") && m_type == DLT_StaticPoint )
	{
		CalcStaticPointInSS( previewPlayer );
	}
	else if ( propertyName == TXT("useWeightCurve") && m_useWeightCurve )
	{
		InitializeWeightCurve();
	}
}

void CStorySceneEventGameplayLookAt::CalcStaticPointInSS( const CStoryScenePlayer* previewPlayer )
{
	if ( Vector::Near3( m_staticPoint, DEFAULT_STATIC_POINT ) )
	{
		const Matrix mat = previewPlayer->GetActorPosition( m_actor );

		const EngineTransform sceneWS = previewPlayer->GetSceneDirector()->GetCurrentScenePlacement();
		Matrix sceneW2L;
		sceneWS.CalcWorldToLocal( sceneW2L );

		const Matrix finalMat = Matrix::Mul( sceneW2L, mat );

		m_staticPoint = finalMat.TransformPoint( m_staticPoint );
	}
}

#endif

void CStorySceneEventGameplayLookAt::InitializeWeightCurve()
{
	m_weightCurve.Clear();
	m_weightCurve.AddPoint( 0.f,	0.f,	Vector( -0.1f,		0.f,		0.1391f,	0.0023f ),	CST_Bezier );
	m_weightCurve.AddPoint( 0.142f, 0.058f, Vector( -0.024f,	-0.0239f,	0.053f,		0.057f ),	CST_Bezier );
	m_weightCurve.AddPoint( 0.33f,	0.47f,	Vector( -0.086f,	-0.174f,	0.0865f,	0.174f ),	CST_BezierSmoothSymertric );
	m_weightCurve.AddPoint( 1.f,	1.f,	Vector( -0.1f,		0.f,		0.1f,		0.f ),		CST_Interpolate );
}

#ifdef DEBUG_SCENES
#pragma optimize("",on)
#endif
