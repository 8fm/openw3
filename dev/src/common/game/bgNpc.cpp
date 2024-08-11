
#include "build.h"
#include "bgNpc.h"
#include "bgNpcRoot.h"
#include "bgNpcMesh.h"
#include "bgNpcItem.h"
#include "storySceneParam.h"
#include "../engine/behaviorGraphStack.h"
#include "../engine/gameTimeManager.h"
#include "../engine/animationManager.h"

IMPLEMENT_ENGINE_CLASS( SBgNpcJobTree );

IMPLEMENT_ENGINE_CLASS( CBgNpc );

CBgNpc::CBgNpc()
	: m_voicesetPlayer( NULL )
	, m_chatPlayer( NULL )
	, m_isInTick( false )
	, m_headBoneIndex( -1 )
	, m_lookAtController( NULL )
	, m_jobPlayer( NULL )
	, m_jobObject( NULL )
	, m_state( NS_None )
	, m_collisionCapsule( true )
	, m_physicsBodyWrapper( NULL )
	, m_isInAsyncTick( false )
	, m_originalTemplete( NULL )
	, m_currentJobNumber( 0 )
	, m_requestedJobTreeChange( false )
	, m_bucketId()
#ifndef NO_EDITOR
	, m_initInEditor( false )
#endif
{
}

CBgNpc::~CBgNpc()
{
	RED_ASSERT( !IsInTick(), TXT("CBgNpc is still in tick list. Was OnDetached() called ?") );
}

int BgNpcJobTreeCompare( const void* jobA, const void* jobB )
{
	// sort by hours, then minutes, then seconds, EXCLUDE DAYS IN SORTING PROCESS
	SBgNpcJobTree* bgNpcJobA = (SBgNpcJobTree*)jobA;
	SBgNpcJobTree* bgNpcJobB = (SBgNpcJobTree*)jobB;

	Uint32 timeA = bgNpcJobA->m_fireTime.Hours();
	Uint32 timeB = bgNpcJobB->m_fireTime.Hours();

	// compare hours
	if ( timeA == timeB )
	{
		// hours equal, compare minutes
		timeA = bgNpcJobA->m_fireTime.Minutes();
		timeB = bgNpcJobB->m_fireTime.Minutes();

		if ( timeA == timeB )
		{
			// minutes equal, compare seconds
			timeA = bgNpcJobA->m_fireTime.Seconds();
			timeB = bgNpcJobB->m_fireTime.Seconds();

			if ( timeA == timeB )
			{
				// seconds equal, so whole time equal
				return 0;
			}
			else
			{
				return timeA < timeB ? -1 : 1;
			}
		}
		else
		{
			return timeA < timeB ? -1 : 1;
		}
	}
	else
	{
		return timeA < timeB ? -1 : 1;
	}
}

void CBgNpc::OnPropertyPostChange( IProperty* property )
{
	if ( property->GetName() == TXT( "jobTrees" ) )
	{
		// sort the job trees by time
		if ( m_jobTrees.Size() > 0 )
		{
			qsort( m_jobTrees.TypedData(), m_jobTrees.Size(), sizeof( SBgNpcJobTree ), BgNpcJobTreeCompare );
		}
	}
}

void CBgNpc::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		if ( m_voicesetPlayer )
		{
			file << *m_voicesetPlayer;
		}

		if ( m_chatPlayer )
		{
			file << *m_chatPlayer;
		}

		//...
	}
}

void CBgNpc::GetLocalizedStrings( TDynArray< LocalizedStringEntry >& localizedStrings ) /*const*/
{
	LocalizedStringEntry stringEntry( &m_displayName, TXT( "Entity display name" ), m_template.Get() );
	localizedStrings.PushBack( stringEntry );
}

#ifndef NO_EDITOR_ENTITY_VALIDATION

Bool CBgNpc::OnValidate( TDynArray< String >& log ) const
{
	if ( !TBaseClass::OnValidate( log ) )
	{
		return false;
	}

	m_displayName.Load();

	if ( m_displayName.GetString().Empty() )
	{
		log.PushBack( TXT( "Select m_displayName" ) );
		return false;
	}

	if ( m_jobTrees.Size() > 0 )
	{
		for ( Uint32 i = 0; i < m_jobTrees.Size(); ++i )
		{
			CJobTree* tree = m_jobTrees[ i ].m_jobTree.Get();
			if ( !tree )
			{
				log.PushBack( TXT( "One of elements in job trees array is null" ) );
				return false;
			}
			
			if ( m_jobTrees[i].m_category == CName::NONE )
			{
				log.PushBack( TXT( "One of elements in job trees array has empty category" ) );
				return false;
			}

			if ( !tree->GetRootNode() )
			{
				log.PushBack( TXT( "One of elements in job trees array is empty" ) );
				return false;
			}

			TDynArray< CName > cats;
			tree->GetRootNode()->CollectValidCategories( cats );

			if ( !cats.Exist( m_category ) )
			{
				log.PushBack( String::Printf( TXT("One of elements in job trees array doesn't have valid category '%ls'"), m_category.AsString().AsChar() ) );
				return false;
			}
		}
	}

	if ( m_jobTree.Get() )
	{
		if ( m_category == CName::NONE )
		{
			log.PushBack( TXT("Category for job tree is empty") );
			return false;
		}

		CJobTree* tree = m_jobTree.Get();
		ASSERT( tree );

		if ( !tree->GetRootNode() )
		{
			log.PushBack( TXT("Job tree is empty") );
			return false;
		}

		TDynArray< CName > cats;
		tree->GetRootNode()->CollectValidCategories( cats );

		if ( !cats.Exist( m_category ) )
		{
			log.PushBack( String::Printf( TXT("Job tree doesn't have valid category '%ls'"), m_category.AsString().AsChar() ) );
			return false;
		}
	}

	return true;
}

#endif

//////////////////////////////////////////////////////////////////////////
// Logic

void CBgNpc::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	ASSERT( !m_isInAsyncTick );

#ifndef NO_EDITOR
	// Editor preview mode
	if ( GIsEditor && !GGame->IsActive() )
	{
		m_initInEditor = true;
	}
	AddToTick();
#endif

	ASSERT( GetRootAnimatedComponent() && GetRootAnimatedComponent()->IsA< CBgRootComponent >() );

	if ( m_collisionCapsule )
	{
		CreateCollisionCapsule( world );
	}	
}

void CBgNpc::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	GAnimationManager->AddAsync( this );

	if ( CanWork() )
	{
		SetState( NS_Working );
	}
	else
	{
		SetState( NS_Idle );
	}
}

void CBgNpc::OnDetached( CWorld* world )
{
	while ( m_isInAsyncTick )
	{
		// Waiting...
	}

	if ( m_collisionCapsule )
	{
		DestroyCollisionCapsule( world );
	}

	GAnimationManager->RemoveAsync( this );

	DisableAllLookAts();

	if ( IsPlayingVoiceset() )
	{
		StopPlayingVoiceset();
	}

	if ( IsPlayingChat() )
	{
		StopPlayingChat();
	}

	SetState( NS_None );

	if ( m_jobPlayer )
	{
		ASSERT( m_jobPlayer );

		delete m_jobPlayer;
		m_jobPlayer = NULL;
	}

	if ( m_jobObject )
	{
		ASSERT( m_jobObject );

		delete m_jobObject;
		m_jobObject = NULL;
	}

	if ( IsInTick() )
	{
		RemoveFromTick();
	}

	ASSERT( !m_isInAsyncTick );

	TBaseClass::OnDetached( world );
}

void CBgNpc::Activate()
{
	ASSERT( ::SIsMainThread() );
	ASSERT( !m_isInAsyncTick );
}

void CBgNpc::Deactivate()
{
	ASSERT( ::SIsMainThread() );
	ASSERT( !m_isInAsyncTick );
}

Bool CBgNpc::IsInTick() const
{
	return m_isInTick;
}

void CBgNpc::AddToTick()
{
	ASSERT( !IsInTick() );

	GAnimationManager->AddSync( this );

	m_isInTick = true;
}

void CBgNpc::RemoveFromTick()
{
	ASSERT( IsInTick() );

	GAnimationManager->RemoveSync( this );

	m_isInTick = false;
}

Bool CBgNpc::SetState( EBgNpcState state )
{
	ASSERT( ::SIsMainThread() );
	ASSERT( !m_isInAsyncTick );

	Bool canLeaveState = true;

	switch ( m_state )
	{
	case NS_None:
		{
			break;
		}

	case NS_Idle:
		{
			m_state = NS_None;
			break;
		}

	case NS_Working:
		{
			canLeaveState = StopStateWorking();
			break;
		}
	}

	if ( !canLeaveState )
	{
		return false;
	}

	ASSERT( m_state == NS_None );

	Bool changedState = true;

	switch ( state )
	{
	case NS_None:
		{
			break;
		}

	case NS_Idle:
		{
			m_state = NS_Idle;
			break;
		}

	case NS_Working:
		{
			changedState = StartStateWorking();
			break;
		}
	}

	if ( changedState )
	{
		ASSERT( m_state == state );
	}

	return changedState;
}

Bool CBgNpc::UpdateState( Float dt )
{
	switch ( m_state )
	{
	case NS_None:
		{
			ASSERT( 0 );
			return true;
		}

	case NS_Idle:
		{
			return true;
		}

	case NS_Working:
		{
			if ( m_jobPlayer )
			{
				if ( m_bucketId.ShouldUpdateThisTick() && m_jobTrees.Size() > 0 )
				{
					ChangeStateWorking();
				}

				ASSERT( m_jobObject );

				m_jobPlayer->Update( dt );

				return true;
			}
			else
			{
				ASSERT( m_jobPlayer );
				ASSERT( m_jobObject );
			}

			return true;
		}

	default:
		{
			ASSERT( 0 );

			return false;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Updates

CBgRootComponent* CBgNpc::GetRoot() const
{
	return m_components.Size() > 0 ? Cast< CBgRootComponent >( m_components[ 0 ] ) : NULL;
}

EAsyncAnimPriority CBgNpc::GetPriority() const
{
	return AAP_High;
}

Box CBgNpc::GetBox() const
{
	return Box( GetWorldPositionRef() - Vector( 0.5f, 0.5f, 0.f ), GetWorldPositionRef() + Vector( 0.5f, 0.5f, 2.f ) );
}

void CBgNpc::DoAsyncTick( Float dt )
{
	// It is not solution, only temp
	if ( IsDetaching() || !IsAttached() )
	{
		return;
	}

	m_isInAsyncTick = true;

	UpdateState( dt );

	CBgRootComponent* root = GetRoot();
	if ( root )
	{
		root->AsyncUpdate( dt );
	}

	for ( ComponentIterator< CBgMeshComponent > it( this ); it; ++it )
	{
		CBgMeshComponent* comp = *it;

		if ( comp->UsesAutoUpdateTransform() )
		{
			comp->ForceUpdateTransformNodeAndCommitChanges();
		}
	}

	m_isInAsyncTick = false;
}

void CBgNpc::DoSyncTick( Float timeDelta )
{
	Bool removeFromTick = true;

	ASSERT( !m_isInAsyncTick );

#ifndef NO_EDITOR
	if ( m_initInEditor )
	{
		GAnimationManager->AddAsync( this );

		if ( CanWork() )
		{
			SetState( NS_Working );
		}
		else
		{
			SetState( NS_Idle );
		}

		m_initInEditor = false;
	}
#endif

	CBgRootComponent* root = GetRoot();
	if ( root )
	{
		root->FireEvents();
	}

	if ( m_voicesetPlayer )
	{
		if ( !m_voicesetPlayer->Update( timeDelta ) )
		{
			StopPlayingVoiceset();
		}
		else
		{
			removeFromTick = false;
		}
	}

	if ( m_chatPlayer )
	{
		if ( !m_chatPlayer->Update( timeDelta ) )
		{
			StopPlayingChat();
		}
		else
		{
			removeFromTick = false;
		}
	}

	if ( m_lookAtController )
	{
		if ( !m_lookAtController->Update( timeDelta ) )
		{
			DisableAllLookAts();
		}
		else
		{
			removeFromTick = false;
		}
	}

	removeFromTick = false;

	if ( removeFromTick )
	{
		RemoveFromTick();
	}

	ASSERT( !m_isInAsyncTick );
}

//////////////////////////////////////////////////////////////////////////
// Work

Bool CBgNpc::CanWork() const
{
	if ( !GetRootAnimatedComponent() )
	{
		return false;
	}

	if ( m_jobTrees.Size() > 0 )
	{
		for ( Uint32 i = 0; i < m_jobTrees.Size(); ++i )
		{
			if ( m_jobTrees[i].m_jobTree.GetPath().Empty() || m_jobTrees[i].m_category == CName::NONE )
			{
				return false;
			}
		}
		return true;
	}
	return !m_jobTree.GetPath().Empty() && m_category != CName::NONE && GetRootAnimatedComponent();
}

Bool CBgNpc::StartWorking()
{
	return SetState( NS_Working );
}

Bool CBgNpc::StopWorking()
{
	return SetState( NS_Idle );
}

Bool CBgNpc::IsWorking() const
{
	return m_state == NS_Working;
}

Bool CBgNpc::StartStateWorking()
{
	ASSERT( m_state != NS_Working );

	CAnimatedComponent* root = GetRootAnimatedComponent();
	
	if ( m_jobObject )
	{
		ASSERT( m_jobObject );
		delete m_jobObject;
		m_jobObject = NULL;
	}

	if ( m_jobPlayer )
	{
		ASSERT( m_jobPlayer );
		delete m_jobPlayer;
		m_jobPlayer = NULL;
	}

	m_jobObject = new CBgJobTreeObject();

	if ( m_jobObject->Initialize( root ) )
	{
		if ( m_jobTrees.Size() > 0 && m_jobTrees[ m_currentJobNumber ].m_jobTree.Get() )
		{
			m_jobPlayer = new CJobTreePlayer( m_jobTrees[ m_currentJobNumber ].m_jobTree, m_jobTrees[ m_currentJobNumber ].m_category, m_jobObject );
		}
		else
		{
			m_jobPlayer = new CJobTreePlayer( m_jobTree, m_category, m_jobObject );
		}

		if ( !IsInTick() )
		{
			AddToTick();
		}

		m_state = NS_Working;

		return true;
	}
	else
	{
		delete m_jobObject;
		m_jobObject = NULL;
	}

	return false;
}

Bool CBgNpc::StopStateWorking()
{
	ASSERT( m_state == NS_Working );

	if ( m_jobObject )
	{
		delete m_jobObject;
		m_jobObject = NULL;
	}
	else
	{
		ASSERT( m_jobObject );
	}

	if ( m_jobPlayer )
	{
		delete m_jobPlayer;
		m_jobPlayer = NULL;
	}
	else
	{
		ASSERT( m_jobPlayer );
	}

	m_state = NS_None;

	return true;
}

Bool CBgNpc::ChangeStateWorking()
{
	if ( !m_jobPlayer )
	{
		return false;
	}

	ASSERT( GGame && GGame->GetTimeManager() );

	Uint32 jobIndexForCurrentTime = SelectAppropriateJobIndex( GGame->GetTimeManager()->GetTime() );
	if ( jobIndexForCurrentTime != m_currentJobNumber || m_requestedJobTreeChange )
	{
		m_jobPlayer->RequestEnd();
		m_requestedJobTreeChange = true;
		if ( m_jobPlayer->HasEnded() )
		{
			StopStateWorking();
			m_currentJobNumber = jobIndexForCurrentTime;
			StartStateWorking();
			m_requestedJobTreeChange = false;
		}
	}	
	return true;
}

Uint32 CBgNpc::SelectAppropriateJobIndex( const GameTime& time )
{
	// jobs are sorted by time, so choose first that fits
	for ( Int32 i = m_jobTrees.Size() - 1; i >= 0; --i )
	{
		if ( time.IsAfter( m_jobTrees[ i ].m_fireTime ) )
		{
			return i;
		}
	}

	// this is a situation in which time is counted from 0 hour
	// and first job starts from 1 or 2 or 3, then choose last job from day
	return m_jobTrees.Size() - 1;
}

//////////////////////////////////////////////////////////////////////////
// Items

void CBgNpc::MountItem( const CName& category )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemCategory() == category )
		{
			comp->SetItemState( IS_MOUNT );
		}
	}
}

void CBgNpc::HoldItem( const CName& category )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemCategory() == category )
		{
			comp->SetItemState( IS_HOLD );
		}
	}
}

void CBgNpc::UnmountItem( const CName& category )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemCategory() == category )
		{
			comp->SetItemState( IS_HIDDEN );
		}
	}
}

void CBgNpc::MountItemByName( const CName& itemName )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemName() == itemName )
		{
			comp->SetItemState( IS_MOUNT );
		}
	}
}

void CBgNpc::HoldItemByName( const CName& itemName )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemName() == itemName )
		{
			comp->SetItemState( IS_HOLD );
		}
	}
}

void CBgNpc::UnmountItemByName( const CName& itemName )
{
	ASSERT( ::SIsMainThread() );

	for ( ComponentIterator< CBgNpcItemComponent > it( this ); it; ++it )
	{
		CBgNpcItemComponent* comp = *it;
		if ( comp->GetItemName() == itemName )
		{
			comp->SetItemState( IS_HIDDEN );
		}
	}
}

void CBgNpc::FireJobEvent( const CName& evtName )
{
	if ( m_jobPlayer )
	{
		CName itemName = m_jobPlayer->GetCurrentActionsItemName();
		if ( itemName != CName::NONE )
		{
			if ( evtName == CNAME( take_item ) )
			{
				MountItemByName( itemName );
			}
			else if ( evtName == CNAME( leave_item ) )
			{
				HoldItemByName( itemName );
			}
			else
			{
				ASSERT( 0 );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Scenes

Bool CBgNpc::IsSpeaking() const
{
	return m_voicesetPlayer != NULL || m_chatPlayer != NULL;
}

Bool CBgNpc::IsPlayingVoiceset() const
{
	return m_voicesetPlayer != NULL;
}

Bool CBgNpc::CanPlayVoiceset() const
{
	return !IsSpeaking();
}

Bool CBgNpc::PlayDefaultVoiceset()
{
	return PlayVoiceset( TXT("greeting_reply") );
}

Bool CBgNpc::PlayVoiceset( const String& voiceset )
{
	ASSERT( ::SIsMainThread() );

	if ( CanPlayVoiceset() )
	{
		// Find scene
		TSoftHandle< CStoryScene > scene;
		CName voiceTag;
		if ( FindScene( voiceset, scene, voiceTag ) )
		{
			// Add to tick
			if ( !IsInTick() )
			{
				AddToTick();
			}

			// Create and run voiceset player
			m_voicesetPlayer = new CBgNpcVoicesetPlayer( this, scene, voiceset, voiceTag );

			return true;
		}
	}
	else
	{
		ASSERT( CanPlayVoiceset() );
	}

	return false;
}

void CBgNpc::StopPlayingVoiceset()
{
	ASSERT( ::SIsMainThread() );

	ASSERT( m_voicesetPlayer );

	if ( m_voicesetPlayer )
	{
		delete m_voicesetPlayer;
		m_voicesetPlayer = NULL;
	}
}

Bool CBgNpc::FindScene( const String& voiceset, TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const
{
	if ( m_voiceset && m_voiceset->GetRandomVoiceset( scene, voiceTag ) )
	{
		return true;
	}

	CEntityTemplate* templ = GetEntityTemplate();
	if ( templ )
	{
		CVoicesetParam* param = templ->FindParameter< CVoicesetParam >( true );

		// Temp
		if ( param && param->GetRandomVoiceset( scene, voiceTag ) )
		{
			return true;
		}

		/*if ( param && param->FindVoiceset( voiceset, scene, voiceTag ) )
		{
			return true;
		}*/
	}

	return NULL;
}

Bool CBgNpc::CanPlayChat() const
{
	return !IsSpeaking();
}

Bool CBgNpc::PlayChat( const SExtractedSceneLineData& data )
{
	if ( CanPlayChat() )
	{
		m_chatPlayer = new CBgNpcChatPlayer( this, data );
		return true;
	}
	else
	{
		ASSERT( CanPlayChat() );
	}

	return false;
}

void CBgNpc::StopPlayingChat()
{
	if ( m_chatPlayer )
	{
		delete m_chatPlayer;
		m_chatPlayer = NULL;
	}
}

Bool CBgNpc::IsPlayingChat() const
{
	return m_chatPlayer != NULL;
}

//////////////////////////////////////////////////////////////////////////
// Look ats

Bool CBgNpc::LookAt( const SLookAtInfo& lookAtInfo )
{
	ASSERT( ::SIsMainThread() );

	if ( !m_lookAtController )
	{
		m_lookAtController = new CLookAtController();
	}

	if ( !IsInTick() )
	{
		AddToTick();
	}

	return m_lookAtController->AddLookAt( lookAtInfo );
}

void CBgNpc::DisableAllLookAts()
{
	ASSERT( ::SIsMainThread() );

	if ( m_lookAtController )
	{
		delete m_lookAtController;
		m_lookAtController = NULL;
	}
}

void CBgNpc::GetLookAtParams( const CLookAtDynamicParam*& dynamicParam, const CLookAtStaticParam*& staticParam, CLookAtContextParam& contextParam ) const
{
	if ( m_lookAtController )
	{
		m_lookAtController->GetLookAtParams( dynamicParam, staticParam, contextParam );
	}
	else
	{
		dynamicParam = NULL;
		staticParam = NULL;
	}
}

void CBgNpc::SetLookAtLevel( ELookAtLevel level )
{
	if ( m_lookAtController )
	{
		m_lookAtController->SetLevel( level );
	}
}

void CBgNpc::OnProcessBehaviorPose( const CAnimatedComponent* poseOwner, const SBehaviorGraphOutput& pose )
{
	TBaseClass::OnProcessBehaviorPose( poseOwner, pose );

	if ( m_lookAtController && poseOwner == GetRootAnimatedComponent() )
	{
		// Reset look at level after behavior sampling
		m_lookAtController->SetLevel( LL_Body );
	}
}

//////////////////////////////////////////////////////////////////////////
// ISceneActorInterface

CBehaviorGraphStack* CBgNpc::GetBehaviorStack()
{
	return GetRootAnimatedComponent() ? GetRootAnimatedComponent()->GetBehaviorStack() : NULL;
}

const CBehaviorGraphStack* CBgNpc::GetBehaviorStack() const
{
	return GetRootAnimatedComponent() ? GetRootAnimatedComponent()->GetBehaviorStack() : NULL;
}

Bool CBgNpc::HasMimicAnimation( const CName& slotName ) const
{
	const CBehaviorGraphStack* stack = GetBehaviorStack();
	return stack ? stack->HasSlotAnimation( slotName ) : false;
}

Bool CBgNpc::PlayLipsyncAnimation( CSkeletalAnimationSetEntry* anim, Float offset )
{
	CBehaviorGraphStack* stack = GetBehaviorStack();
	if ( stack )
	{
		return stack->PlaySlotAnimation( CNAME( MIMIC_SLOT ), anim, NULL );
	}
	return false;
}

Bool CBgNpc::StopLipsyncAnimation()
{
	CBehaviorGraphStack* stack = GetBehaviorStack();
	if ( stack )
	{
		return stack->StopSlotAnimation( CNAME( MIMIC_SLOT ) );
	}
	return false;
}

Bool CBgNpc::PlayMimicAnimation( const CName& animation, const CName& slotName, Float blendTime, Float offset )
{
	CBehaviorGraphStack* stack = GetBehaviorStack();
	if ( stack )
	{
		return stack->PlaySlotAnimation( slotName, animation, NULL );
	}
	return false;
}

Bool CBgNpc::StopMimicAnimation( const CName& slotName )
{
	CBehaviorGraphStack* stack = GetBehaviorStack();
	if ( stack )
	{
		return stack->StopSlotAnimation( slotName );
	}
	return false;
}

Vector CBgNpc::GetSceneHeadPosition() const
{
	CAnimatedComponent* animated = GetRootAnimatedComponent();
	if ( animated && m_headBoneIndex != -1 )
	{		
		return animated->GetBoneMatrixWorldSpace( m_headBoneIndex ).GetTranslation() + Vector( 0.f, 0.f, 0.4f );
	}
	else
	{
		Vector pos = GetWorldPosition();
		pos.Z += 2.f;
		return pos;
	}
}

Int32 CBgNpc::GetSceneHeadBone() const
{
	return m_headBoneIndex;
}

String CBgNpc::GetDisplayName() const
{
	m_displayName.Load();

	// Try to use localized display name 
	String displayName = m_displayName.GetString();

	if ( displayName.Empty() )
	{
		if ( m_fakeDisplayName.Empty() )
		{
			CBgNpc* npc = const_cast< CBgNpc* >( this );

			if ( npc->m_fakeDisplayName.Empty() )
			{
				npc->m_fakeDisplayName = TXT("Sam");
			}
		}

		return m_fakeDisplayName;
	}
	else
	{
		return displayName;
	}
}

Vector CBgNpc::GetAimPosition() const
{
	return GetSceneHeadPosition() - Vector( 0.0f, 0.0f, 0.6f );
}

Vector CBgNpc::GetBarPosition() const
{ 
	return GetSceneHeadPosition() - Vector(0.0f, 0.0f, 0.2f);
}

//////////////////////////////////////////////////////////////////////////
// Collision

void CBgNpc::CreateCollisionCapsule( CWorld* world )
{
}

void CBgNpc::DestroyCollisionCapsule( CWorld* world )
{
#ifdef USE_HAVOK
	if ( m_physicsBodyWrapper )
	{
		delete m_physicsBodyWrapper;
		m_physicsBodyWrapper = NULL;
	}
#endif
}

//////////////////////////////////////////////////////////////////////////
// Editor

#ifndef NO_EDITOR

void CBgNpc::CopyFrom( const CNewNPC* npc )
{
	m_headBoneIndex = npc->GetHeadBone();


	//TODO
	//m_colorVariant = npc->GetColorVariant();

	m_originalTemplete = npc->GetEntityTemplate();
}

#endif

CName CBgNpc::GetSceneActorVoiceTag() const
{
	return CName::NONE;
}
