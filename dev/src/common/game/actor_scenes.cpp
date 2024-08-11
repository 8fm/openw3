
#include "build.h"
#include "actor.h"
#include "actorSpeech.h"
#include "storySceneComponent.h"
#include "storySceneInput.h"
#include "storySceneVoicetagMapping.h"
#include "storyScenePlayer.h"
#include "voicesetPlayer.h"
#include "storySceneSystem.h"
#include "storySceneActorMap.h"
#include "behTreeNodePlayScene.h"
#include "../engine/mimicComponent.h"
#include "itemIterator.h"
#include "storySceneDebug.h"

#ifdef DEBUG_SCENES_2
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////
// All functionality for scenes etc. This code needs to be heavily refactored
//////////////////////////////////////////////////////////////////////////

void CActor::AddStoryScene( CStorySceneController * scene )
{
	ASSERT( scene && !m_storyScenes.Exist( scene ) );
	m_storyScenes.PushBack( scene );
	if ( scene->GetInput() != NULL )
	{
		m_isInNonGameplayScene	|= ( !scene->GetInput()->IsGameplay() );
		m_isInGameplayScene		|= ( scene->GetInput()->IsGameplay() );
	}
}

void CActor::RemoveStoryScene( CStorySceneController * scene )
{
	m_storyScenes.RemoveFast( scene );

	m_isInNonGameplayScene	= false;
	m_isInGameplayScene		= false;

	for ( Uint32 i = 0; i < m_storyScenes.Size(); ++i )
	{
		if ( m_storyScenes[ i ]->GetInput() != NULL )
		{
			m_isInNonGameplayScene	|= ( !m_storyScenes[ i ]->GetInput()->IsGameplay() );
			m_isInGameplayScene		|= ( m_storyScenes[ i ]->GetInput()->IsGameplay() );
		}
	}
}

void CActor::StopAllScenes( const SceneCompletionReason& reason )
{
	RED_FATAL_ASSERT( SIsMainThread(), "CActor::StopAllScenes can be called only from main thread" );
	CStorySceneSystem* sss = GCommonGame->GetSystem< CStorySceneSystem >();
	if ( sss && !m_storyScenes.Empty() )
	{
		TDynArray< CStorySceneController* > scenes = m_storyScenes;

		for ( Uint32 i = 0; i < scenes.Size(); ++i )
		{
			if ( CStorySceneController* c = scenes[ i ] )
			{
				if ( sss->CanStopScene( c ) && ( c->CanStopByExternalSystem() || reason == SCR_ACTOR_DESPAWN ) )
				{
					sss->StopScene( c, reason );
				}
				else if ( sss->CanStopScene( c ) )
				{
					SCENE_ASSERT( 0 );
				}
				else if ( !sss->CanStopScene( c ) )
				{
					SCENE_ASSERT( 0 );
				}
			}
		}

		SCENE_ASSERT( m_storyScenes.Size() == 0 );
		m_storyScenes.Clear();
	}

	SCENE_ASSERT( !m_isInNonGameplayScene );
	SCENE_ASSERT( !m_isInGameplayScene );
	SCENE_ASSERT( !m_lockedByScene );

	//SCENE_ASSERT( !sss->HasActorInAnyPlayingScene( this ) );
}

const TDynArray< CStorySceneController* >& CActor::GetStoryScenes() const 
{ 
	return m_storyScenes; 
}

void CActor::OnCutsceneStarted()
{
	// Override CEntity func because of items
	CallEvent( CNAME( OnCutsceneStarted ) );

	for ( EntityWithItemsComponentIterator< CComponent > it( this ); it; ++it )
	{
		CComponent* component = *it;
		component->OnCutsceneStarted();
	}
}

void CActor::OnCutsceneEnded()
{
	// Override CEntity func because of items
	CallEvent( CNAME( OnCutsceneEnded ) );

	for ( EntityWithItemsComponentIterator< CComponent > it( this ); it; ++it )
	{
		CComponent* component = *it;
		component->OnCutsceneEnded();
	}
}

IActorInterface* CActor::QueryActorInterface()
{
	return static_cast< IActorInterface* >( this );
}

const IActorInterface* CActor::QueryActorInterface() const
{
	return static_cast< const IActorInterface* >( this );
}

Bool CActor::HasSceneMimic() const
{
	return HasMimic();
}

Bool CActor::SceneMimicOn()
{ 
	return MimicOn(); 
}

void CActor::SceneMimicOff()
{ 
	return MimicOff(); 
}

CEntity* CActor::GetSceneParentEntity()
{ 
	return this; 
}

Vector CActor::GetSceneHeadPosition() const
{ 
	return GetHeadPosition(); 
}

Bool CActor::WasSceneActorVisibleLastFrame() const 
{ 
	return WasVisibleLastFrame(); 
}

CName CActor::GetSceneActorVoiceTag() const
{
	return GetVoiceTag();
}

Bool CActor::MimicOn()
{
	return m_mimicComponent && m_mimicComponent->MimicHighOn();
}

void CActor::MimicOff()
{
	if ( m_mimicComponent )
	{
		m_mimicComponent->MimicHighOff();
	}
}

Bool CActor::HasMimic() const
{
	return m_mimicComponent && m_mimicComponent->HasMimicHigh();
}

Bool CActor::SetMimicVariable( const CName varName, Float value )
{
	return m_mimicComponent && m_mimicComponent->SetMimicVariable( varName, value );
}

Bool CActor::IsInNonGameplayScene() const
{
	return m_isInNonGameplayScene;
}

Int32 CActor::GetActorAnimState() const
{
	return m_actorAnimState;
}

void CActor::SetActorAnimState( EActorAnimState state )
{
	m_actorAnimState = state;
}

CMimicComponent* CActor::GetMimicComponent() const
{
	return m_mimicComponent;
}

void CActor::SetMimicComponent( CMimicComponent* comp )
{
	m_mimicComponent = comp;

	/*if ( m_mimicComponent )
	{
		for ( ComponentIterator< CMimicComponent > it ( this ); it; ++it )
		{
			CMimicComponent* m = *it;
			ASSERT( m == m_mimicComponent );
		}
	}*/
}

Bool CActor::PlayMimicAnimation( const CName& animation, const CName& slot, Float blendTime, Float offset )
{
#ifndef NO_MIMIC_ANIM
	return m_mimicComponent ? m_mimicComponent->PlayMimicAnimation( animation, slot, blendTime, offset ) : false;
#else
	return false;
#endif
}

Bool CActor::PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset )
{
#ifndef NO_MIMIC_ANIM
	return m_mimicComponent ? m_mimicComponent->PlayLipsyncAnimation( anim, offset ) : false;
#else
	return false;
#endif
}

Bool CActor::StopLipsyncAnimation()
{
#ifndef NO_MIMIC_ANIM
	return m_mimicComponent ? m_mimicComponent->StopLipsyncAnimation() : false;
#else
	return false;
#endif
}

Bool CActor::StopMimicAnimation( const CName& slot )
{
	if ( m_mimicComponent )
	{
		m_mimicComponent->StopMimicAnimation( slot );
		return true;
	}
	else
	{
		return false;
	}
}

Bool CActor::HasMimicAnimation( const CName& slot ) const
{
	if ( m_mimicComponent )
	{
		return m_mimicComponent->HasMimicAnimation( slot );
	}
	else
	{
		return false;
	}
}

Bool CActor::UpdateSpeaking( const TActorSpeechID& speechId, Float time, Float progress )
{
	if ( m_speech && m_speech == speechId )
	{
		return m_speech->SetTimeState( time, progress );
	}

	return false;
}

TActorSpeechID CActor::ProceedSpeechFromQueue()
{
	if ( m_speech )
	{
		CancelSpeech( false );
	}
	if( m_speechQueue.HasNextLine() )
	{
		RED_FATAL_ASSERT( !m_speech, "CActor::ProceedSpeechFromQueue" );
		ActorSpeechData data( 0, StringAnsi::EMPTY, false, 0 );
		m_speechQueue.NextLineData( data );
		m_speech = new CActorSpeech( this, data );
		return (TActorSpeechID)&m_speech;
	}

	return nullptr;
}

TActorSpeechID CActor::SpeakLine( const ActorSpeechData& speechData )
{
	if ( m_speech )
	{
		CancelSpeech();
	}

	RED_FATAL_ASSERT( !m_speech, "CActor::ProceedSpeechFromQueue" );
	m_speech = new CActorSpeech( this, speechData );

	return (TActorSpeechID)&m_speech;
}

void CActor::CancelSpeech( Bool cleanupQueue )
{
	if ( m_speech )
	{
		m_speech->Cancel();
		delete m_speech;
		m_speech = nullptr;
	}
	if( cleanupQueue )
	{
		m_speechQueue.Cleanup();
	}
}

Bool CActor::IsSpeaking( Uint32 stringId /*= 0 */ )
{
	if ( m_speech == NULL )
	{
		return false;
	}

	if ( stringId != 0 )
	{
		return m_speech->GetSpeechId() == stringId;
	}

	return true;
}

void CActor::ReplayCurrentSpeechLipsync()
{
	RED_ASSERT( m_speech );
	if ( m_speech )
	{
		m_speech->ReplayLipsync();
	}
}

const CActorSpeech* CActor::GetSpeech() const 
{ 
	return m_speech; 
}

void CActor::DisableLookAts()
{
	m_lookAtController.SetNoLookAts();
}

void CActor::DisableDialogsLookAts( Float speed )
{
	m_lookAtController.SetNoDialogsLookAts( speed );
}

void CActor::RemoveAllNonDialogsLookAts()
{
	m_lookAtController.RemoveAllNonDialogsLookAts();
}

Bool CActor::IsLookAtEnabled() const
{
	return m_lookAtController.HasLookAt();
}

Bool CActor::EnableLookAt( const SLookAtInfo& lookAtInfo )
{
	return m_lookAtController.AddLookAt( lookAtInfo );
}

Bool CActor::EnableDynamicLookAt( CNode* node, Float duration )
{
	CActor* actor = Cast< CActor >( node );

	if ( actor )
	{
		CAnimatedComponent* ac = actor->GetRootAnimatedComponent();
		Int32 boneIndex = actor->GetHeadBone();

		SLookAtScriptBoneInfo info;
		info.m_duration = duration;
		info.m_targetOwner = ac;
		info.m_boneIndex = boneIndex;

		return EnableLookAt( info );
	}
	else
	{
		SLookAtScriptDynamicInfo info;
		info.m_duration = duration;
		info.m_target = node;

		return EnableLookAt( info );
	}
}

Vector CActor::GetLookAtTarget() const
{
	return m_lookAtController.GetTarget();
}

Vector CActor::GetLookAtBodyPartsWeights() const
{
	return m_lookAtController.GetBodyPartWeights();
}

Vector CActor::GetLookAtCompressedData() const
{
	return m_lookAtController.GetCompressedData();
}

Vector CActor::GetEyesLookAtCompressedData() const
{
	return m_lookAtController.GetEyesCompressedData();
}

void CActor::UpdateLookAt( Float timeElapsed )
{
	PC_SCOPE( UpdateLookAt );
	m_lookAtController.Update( timeElapsed );
}

ELookAtLevel CActor::GetLookAtLevel() const
{
	if ( m_action && !m_action->CanUseLookAt() )
	{
		return LL_Null;
	}

	return m_lookAtController.GetLevel();
}

void CActor::SetLookAtLevel( ELookAtLevel level )
{
	m_lookAtController.SetLevel( level );
}

void CActor::GetLookAtDesc( CActorLookAtDesc& desc ) const
{
	m_lookAtController.GetDesc( desc );
}

void CActor::SetLookAtMode( ELookAtMode mode )
{
	m_lookAtController.SetMode( mode );
}

void CActor::ResetLookAtMode( ELookAtMode mode )
{
	m_lookAtController.ResetMode( mode );
}

void CActor::GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const
{
	m_lookAtController.GetLookAtParams( dynamicParam, staticParam, contextParam );

	if( m_isLookAtLevelForced )
	{
		contextParam.m_actorLevel = m_forcedLookAtLevel;
	}
	else if ( m_action && !m_action->CanUseLookAt() )
	{
		contextParam.m_actorLevel = LL_Null;
	}	
}

Vector CActor::GetHeadPosition() const
{
	CAnimatedComponent* animated = GetRootAnimatedComponent();
	if( m_headBoneIndex != -1 && animated )
	{
		return animated->GetBoneMatrixWorldSpace( m_headBoneIndex ).GetTranslation();
	}
	else
	{
		Vector pos = GetWorldPosition();
		pos.Z += 2.f;
		return pos;
	}
}

Int32 CActor::GetHeadBone() const
{
	return m_headBoneIndex;
}

void CActor::SetSceneLock( Bool enable, Bool isGameplayScene, Int8 priority )
{
	if ( isGameplayScene || m_lockedByScene == enable )
	{
		return;
	}
	m_lockedByScene = enable;

	if ( m_lockedByScene )
	{
		for ( EntityWithItemsComponentIterator< CComponent > it( this ); it; ++it )
		{
			CComponent* c = *it;
			c->OnCinematicStorySceneStarted();
		}
	}
	else
	{
		for ( EntityWithItemsComponentIterator< CComponent > it( this ); it; ++it )
		{
			CComponent* c = *it;
			c->OnCinematicStorySceneEnded();
		}
	}

	SPlaySceneRequestData eventData;
	if ( enable )
	{
		eventData = SPlaySceneRequestData( true, priority, isGameplayScene );
	}
	else
	{
		eventData = SPlaySceneRequestData( false );
	}
	SignalGameplayEvent( SPlaySceneRequestData::EVENT_ID, &eventData, SPlaySceneRequestData::GetStaticClass() );

	if ( enable )
	{
		SignalGameplayEvent( CNAME( AI_ForceInterruption ) );
	}
}

Bool CActor::IsLockedByScene() const 
{ 
	return m_lockedByScene || m_isInNonGameplayScene;  
}

CStorySceneComponent* CActor::GetCurrentStorySceneComponent()
{
	String componentName = TXT("voiceset_") + GetVoiceTag().AsString();
	return Cast<CStorySceneComponent>( FindComponent( componentName , false ) );
}

Bool CActor::PlayVoiceset( EArbitratorPriorities priority, const String& voiceset, Bool breakCurrentSpeach )
{
	if ( voiceset.Empty() )
	{
		return false;
	}

	if( breakCurrentSpeach )
	{
		CancelSpeech( true );
		if( m_voicesetPlayer )
		{
			delete m_voicesetPlayer;
			m_voicesetPlayer = nullptr;
		}
	}
	else if( m_voicesetPlayer )
	{
		return false;
	}

	CStorySceneComponent * ssComponent = GetCurrentStorySceneComponent();
	if ( !ssComponent )
	{
		return false;
	}

	TSoftHandle< CStoryScene > sceneHandle = ssComponent->GetStoryScene();

	m_voicesetPlayer = new CVoicesetPlayer( priority, voiceset );

	if ( m_voicesetPlayer->StartLoading( sceneHandle ) )
	{
		return true;
	}
	else
	{
		delete m_voicesetPlayer;
		m_voicesetPlayer = NULL;
		return false;
	}
}

void CActor::StopAllVoicesets( Bool cleanupQueue )
{
	CancelSpeech( cleanupQueue );
	if( m_voicesetPlayer )
	{
		delete m_voicesetPlayer;
		m_voicesetPlayer = nullptr;
	}
}

EAsyncCheckResult CActor::HasVoiceset( const String& voiceset )
{
	if ( voiceset.Empty() )
		return ASR_ReadyFalse;

	CStorySceneComponent * ssComponent = GetCurrentStorySceneComponent();
	
	if( !ssComponent )
		return ASR_ReadyFalse;
	
	String newWayInput = String::Printf( TXT( "%s_%s" ), voiceset.AsChar(), GetVoiceTag().AsString().AsChar() );

	EAsyncCheckResult result = ssComponent->HasVoiceset( newWayInput );
	if( result == ASR_InProgress || result ==  ASR_Failed || result == ASR_ReadyTrue )
	{
		return result;
	}
	else
	{
		// ssComponent->HasVoiceset( voiceset ) is fallback - "old way" of getting input
		return ssComponent->HasVoiceset( voiceset );	
	}	
}

Bool CActor::HasInteractionVoicesetScene() const
{
	return m_hasInteractionVoicesetScene;
}

void CActor::SetLookatFilterData( ELookAtLevel level, CName key )
{
	m_lookAtController.SetLookatFilterData( level, key );
}

void CActor::ActivateLookatFilter( CName key, Bool value )
{
	m_lookAtController.ActivateLookatFilter( key, value );
}

void CActor::RemoveLookatFilterData( CName key )
{
	m_lookAtController.RemoveLookatFilterData( key );
}

#ifdef DEBUG_SCENES_2
#pragma optimize("",on)
#endif
