#include "build.h"
#include "reactionSceneActor.h"

#include "storySceneComponent.h"
#include "storySceneInput.h"
#include "behTreeReactionManager.h"
#include "commonGame.h"

#include "../core/tokenizer.h"

IMPLEMENT_ENGINE_CLASS( CReactionSceneActorComponent  );

RED_DEFINE_STATIC_NAME( ChatsLocked )

Bool CReactionSceneActorComponent::IsInvoker()
{
	CReactionScene* scene = m_scene.Get();
	return scene && this == scene->GetInvoker();
}

void CReactionSceneActorComponent::LockChats()
{
	m_sceneRole = CNAME( ChatsLocked );
}

void CReactionSceneActorComponent::UnlockChats()
{
	m_sceneRole = CName::NONE;
}

void CReactionSceneActorComponent::LeaveScene()
{	
	if( m_scene.Get() )
	{
		CReactionScene* scene = m_scene.Get();
		scene->LeaveScene( this );
		scene = nullptr;
	}
	if( m_sceneRole != CNAME( ChatsLocked ) )
	{
		m_sceneRole = CName::NONE;
	}	
}

void CReactionSceneActorComponent::FinishRole( Bool interupted )
{
	CReactionScene* scene = m_scene.Get();
	
	if( scene )
	{
		scene->MarkAsFinished( m_sceneRole, interupted );
	}
	
	if( m_sceneRole != CNAME( ChatsLocked ) )
	{
		m_sceneRole = CName::NONE;
	}	

	m_cooldown = GCommonGame->GetEngineTime() + m_cooldownInterval;
}

Bool CReactionSceneActorComponent::IfSceneInterupted()
{
	CReactionScene* scene = m_scene.Get();

	if( scene )
	{
		return scene->IsSceneInterupted();
	}
	return false;
}

void CReactionSceneActorComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	// HACK for Sarah G.
	// NPC will not chat immediatelly after spawn
	m_cooldown				= GCommonGame->GetEngineTime() + m_cooldownInterval;
	m_voicesetName			= String::EMPTY;
	m_voicesetGroup			= CName::NONE;
	m_voicesetGroupCached	= false;
}

void CReactionSceneActorComponent::OnDetached( CWorld* world ) 
{
	TBaseClass::OnDetached( world );
	FinishRole( true );
}

bool CReactionSceneActorComponent::IfActorHasSceneInput( String& requestedInput )
{
	LazyInitAvailableSceneInputs();	
	return m_availableSceneInputs.Exist( requestedInput );	
}

bool CReactionSceneActorComponent::IfActorHasSceneInput( TDynArray< String >& requestedInput )
{
	LazyInitAvailableSceneInputs();
	for( Int32 i=0; i<requestedInput.SizeInt(); ++i )
	{
		if( !m_availableSceneInputs.Exist( requestedInput[ i ] ) )
		{
			return false;
		}
	}
	return  true;
}

CName CReactionSceneActorComponent::GetVoicesetGroup()
{
	if( m_voicesetGroupCached )
		return m_voicesetGroup;

	const String& vsetName = GetVoicesetName();
	m_voicesetGroup = GCommonGame->GetBehTreeReactionManager()->FindGroupForVoiceset( vsetName );
	m_voicesetGroupCached = true;
	return m_voicesetGroup;
}

const String& CReactionSceneActorComponent::GetVoicesetName()
{
	if( m_voicesetName != String::EMPTY )
		return m_voicesetName;

	CActor* ownerActor = Cast< CActor >( GetEntity() );
	if( !ownerActor )
		return String::EMPTY;

	CStorySceneComponent* storySceneComponent = ownerActor->GetCurrentStorySceneComponent();
	if( !storySceneComponent )
		return String::EMPTY;

	String path = storySceneComponent->GetStoryScene().GetPath();
	size_t indexOfSlash = 0;
	size_t indexOfDot = 0;

	if( path.FindCharacter( '\\',indexOfSlash, true ) && path.FindCharacter( '.',indexOfDot, true ) )
	{
		indexOfSlash += 1;
		m_voicesetName = path.MidString( indexOfSlash, indexOfDot - indexOfSlash );
	}
	return m_voicesetName;
}

void CReactionSceneActorComponent::LazyInitAvailableSceneInputs()
{
	if( m_availableSceneInputsFetchingState == ASIFS_Fetched )
	{
		return;
	}

	if( m_availableSceneInputsFetchingState == ASIFS_Fetching )
	{
		if( m_storyScene.GetAsync() == BaseSoftHandle::ALR_Loaded )
		{
			FetchAvailableSceneInputs();
		}
		return;
	}

	

	CActor* ownerActor = Cast< CActor >( GetEntity() );
	if( !ownerActor )
		return;

	CStorySceneComponent* storySceneComponent = ownerActor->GetCurrentStorySceneComponent();
	if( !storySceneComponent )
		return;

	m_storyScene = storySceneComponent->GetStoryScene();
	m_availableSceneInputsFetchingState = ASIFS_Fetching;
	BaseSoftHandle::EAsyncLoadingResult ret = m_storyScene.GetAsync();

	if( ret == BaseSoftHandle::ALR_Loaded )
	{
		FetchAvailableSceneInputs();
	}
}

void CReactionSceneActorComponent::FetchAvailableSceneInputs()
{
	if( m_availableSceneInputsFetchingState == ASIFS_Fetched )
		return;
	CStoryScene* scene = m_storyScene.Get();
	ASSERT( scene );

	TDynArray< CStorySceneInput* > inputs;
	if ( scene )
	{
		scene->CollectControlParts< CStorySceneInput >( inputs );
	}

	m_availableSceneInputsFetchingState = ASIFS_Fetched;

	for( Int32 i=0; i<inputs.SizeInt(); ++i )
	{
		m_availableSceneInputs.PushBack( inputs[ i ]->GetName() );
	}
}