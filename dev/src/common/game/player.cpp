/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "actionAreaComponent.h"
#include "interestPointComponent.h"
#include "reactionsManager.h"
#include "actorsManager.h"
#include "definitionsManager.h"
#include "characterStats.h"
#include "interactionsManager.h"
#include "interactionComponent.h"
#include "playerStateBase.h"

#include "../engine/behaviorGraphStack.h"
#include "../../common/core/gatheredResource.h"

#include "expManager.h"
#include "questsSystem.h"
#include "../core/gameSave.h"
#include "../engine/renderFrame.h"
#include "../engine/behaviorGraphContext.h"

IMPLEMENT_ENGINE_CLASS( CPlayer );

RED_DEFINE_STATIC_NAME( Death );
RED_DEFINE_STATIC_NAME( Jump );
RED_DEFINE_STATIC_NAME( Climb );
RED_DEFINE_STATIC_NAME( OnStateChanged );
RED_DEFINE_STATIC_NAME( OnEnable );
RED_DEFINE_STATIC_NAME( GI_WalkFlag );
RED_DEFINE_STATIC_NAME( GI_WalkSwitch );
RED_DEFINE_STATIC_NAME( OnSwimmingStarted );

CGatheredResource resPlayerCombatSlotEnitity( TXT("characters\\templates\\witcher\\combat_slots.w2ent"), RGF_Startup );
CGatheredResource resKnowledgeList( TXT("gameplay\\globals\\knowledge_list.csv"), RGF_Startup );

const Float CPlayer::NOISE_THRESHOLD_SPEED		= 0.8f;
const Float CPlayer::INTERESTPOINT_UPDATE_INTERVAL	= 0.25f;

const String CPlayer::EXPLORATION_INTERACTION_COMPONENT_NAME(TXT("ExplorationInteraction"));

const Uint32 CPlayer::NUM_QUICK_SLOTS	= 4;

CPlayer::CPlayer()	
	: m_presenceInterestPoint( NULL )
	, m_slowMovementInterestPoint( NULL )
	, m_fastMovementInterestPoint( NULL )
	, m_weaponDrawnInterestPoint( NULL )
	, m_weaponDrawMomentInterestPoint( NULL )
	, m_visionInterestPoint( NULL )
	, m_npcVoicesetCooldown( 15.0f )
	, m_npcVoicesetCooldownTimer( 0.0f )
	, m_timeToInterestPointUpdate( GEngine->GetRandomNumberGenerator().Get< Float >( INTERESTPOINT_UPDATE_INTERVAL ) )
	, m_lockProcessButtonInteraction( 0 )
	, m_isMovable( true )
	, m_enemyUpscaling( false )
	, m_explorationComponent( NULL )
{	
	
	m_isInteractionActivator = true;
	m_actorGroups = PEAT_Player;

	// Load config
	LoadObjectConfig( TXT("User") );
	SetForceNoLOD( true );
}

CPlayer::~CPlayer()
{
}

CPlayerStateBase* CPlayer::GetCurrentState() const
{
	CScriptableState* currentState = TBaseClass::GetCurrentState();
	return Cast< CPlayerStateBase >( currentState );
}

Bool CPlayer::CanProcessButtonInteractions() const
{
	return m_lockProcessButtonInteraction == 0;
}

void CPlayer::LockButtonInteractions( Int32 channel )
{
	m_lockProcessButtonInteraction |= channel;
}

void CPlayer::UnlockButtonInteractions( Int32 channel )
{
	m_lockProcessButtonInteraction &= ~channel; 
}

//////////////////////////////////////////////////////////////////////////

void CPlayer::OnParentAttachmentBroken( IAttachment* attachment )
{
    TBaseClass::OnParentAttachmentBroken( attachment );

    // Save crooked pos
    EulerAngles crookedEuler = m_localToWorld.ToEulerAnglesFull();
    Matrix crookMatrix = m_localToWorld;

    // Align localToMorld matrix to global Z
    // Player should have no tilt here
    if( crookedEuler.Roll != 0.0f || crookedEuler.Pitch != 0.0f )
    {
        m_localToWorld.V[0] = Vector::Cross( m_localToWorld.V[1], Vector::EZ );
        m_localToWorld.V[1] = Vector::Cross( Vector::EZ, m_localToWorld.V[0] );
        m_localToWorld.V[2] = Vector::EZ;
        
        CMovingAgentComponent* mov = GetMovingAgentComponent();

        if( mov == nullptr )
            return;

        // Calculate diff matrix
        crookMatrix = m_localToWorld.FullInverted() * crookMatrix;

        mov->SetAdditionalOffsetToConsumeMS( crookMatrix.GetTranslation(), crookMatrix.ToEulerAnglesFull(), 1.0f );
    }
}

//////////////////////////////////////////////////////////////////////////

void CPlayer::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );	

	// Get player interaction
	m_explorationComponent = FindComponent< CInteractionComponent >( EXPLORATION_INTERACTION_COMPONENT_NAME );
	if ( !m_explorationComponent )
	{
		ERR_GAME( TXT("Player has no %s interaction component!"), EXPLORATION_INTERACTION_COMPONENT_NAME.AsChar() );
	}
}

void CPlayer::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// cleanup
	m_voiceSetsToChooseFrom.Clear();
}

void CPlayer::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

#ifdef USE_PHYSX
	// Player has physical movement by default
	GetMovingAgentComponent()->SetPhysicalRepresentationRequest( CMovingAgentComponent::Req_On, CMovingAgentComponent::LS_Initial );
#endif
	// This is important to not fall down when player goes outside navigation data and physics is not yet streamed-in
	m_freezeOnFailedPhysicalRequest = true;
}

void CPlayer::GenerateDebugFragments( CRenderFrame* frame )
{
	TBaseClass::GenerateDebugFragments( frame );

	const CRenderFrameInfo & frameInfo = frame->GetFrameInfo();

	// debug code for collisions
#if 0
	if ( frameInfo.IsShowFlagOn( SHOW_Collision ) )
	{
		Vector position = GetWorldPosition();
		Vector direction;
		GGame->GetCamera()->GetCameraDirection( direction );

		position.Z += 1.8f;
		position += direction * 2.f;

		frame->AddDebugSphere( position, 0.45f, Matrix::IDENTITY, Color::LIGHT_BLUE, false );
		frame->AddDebugSphere( position, 0.05f, Matrix::IDENTITY, Color::BLUE, false );

		SPhysicsOverlapInfo overlaps[ 16 ];
		STATIC_GAME_ONLY CPhysicsEngine::CollisionTypeMask collisionMask = GPhysicEngine->GetCollisionGroupMask( CNAME( Camera ) );
		Uint32 numResults = 0;
		const ETraceReturnValue retVal = GetMovingAgentComponent()->GetPhysicsWorld()->SphereOverlapWithMultipleResults( position, 0.45f, collisionMask, overlaps, numResults, 16 );
		for ( Uint32 i = 0; i < numResults; ++i )
		{
			frame->AddDebugSphere( overlaps[ i ].m_position, 0.01f, Matrix::IDENTITY, Color::WHITE, false );
		}

		position += direction * 2.f;
		frame->AddDebugSphere( position, 0.45f, Matrix::IDENTITY, Color::LIGHT_BLUE, false );
		frame->AddDebugSphere( position, 0.05f, Matrix::IDENTITY, Color::BLUE, false );

		frame->AddDebugSphere( position + ( direction * 2.f ), 0.45f, Matrix::IDENTITY, Color::LIGHT_BLUE, false );
		frame->AddDebugSphere( position + ( direction * 2.f ), 0.05f, Matrix::IDENTITY, Color::BLUE, false );

		SPhysicsContactInfo contacts[ 16 ];
		Uint32 numResults2 = 0;
		const ETraceReturnValue retVal2 = GetMovingAgentComponent()->GetPhysicsWorld()->SphereSweepTestWithMultipleResults( position, position + ( direction * 2.f ), 0.45f, collisionMask, contacts, numResults2,  16 );
		for ( Uint32 i = 0; i < numResults2; ++i )
		{
			frame->AddDebugSphere( contacts[ i ].m_position, 0.01f, Matrix::IDENTITY, Color::WHITE, false );
		}

	}
#endif // debug code for collisions
}

RED_DEFINE_STATIC_NAME( GetIsInCombat );
void CPlayer::OnTick( Float timeDelta )
{
	TBaseClass::OnTick( timeDelta );

	PC_SCOPE_PIX( PlayerTick );

	if( IsInGame() )
	{
		// Update the interest point
		UpdateInterestPoint( timeDelta );

		// 'Speak the nearby NPCs mind' ;)
		UpdateVoicesets( timeDelta );

		BestActionAreaToFront();

		// try triggering the nearest automatic exploration
		if ( !m_activeExplorations.Empty() )
		{
			CActionAreaComponent *activeExploration = m_activeExplorations[0].Get();
			if ( activeExploration->CanBeAutoTriggered( this ) )
			{
				// if we're in the exploration's range, trigger it
				activeExploration->TriggerExploration( this );
			}
		}



#ifdef EXPLORATION_DEBUG

		CGameWorld* world = Cast < CGameWorld > ( GetLayer()->GetWorld() );

		static const Box bound( Vector( -5.f, -5.f, -2.f ), Vector( 5.f, 5.f, 2.f ) );
		static const INodeFilter* filters[] = { &CGameplayStorage::filterIsNotPlayer, &CGameplayStorage::filterHasVehicle };
		TDynArray< TPointerWrapper< CGameplayEntity > > horsesPtr;
		GCommonGame->GetGameplayStorage()->GetClosestToEntity( *this, horsesPtr, bound, 1, filters, 2 );

		const CGameplayEntity* horse = NULL;
		
		if( horsesPtr.Size() )
		{
			horse = horsesPtr[0].Get();
		}

		SExplorationQueryToken token;
		if ( horse )
		{
			world->GetExpManager()->QueryExplorationFromObjectSync( token, this, horse );
		}
		else
		{
			world->GetExpManager()->QueryExplorationSync( token, this );
		}

		/*static Bool isLookingAtExploration = false;
		static const IExploration* previousExp = NULL;*/

		if( token.GetExploration() )
		{
			Vector p1, p2;
			token.GetExploration()->GetEdgeWS( p1, p2 );

			const Vector& entPos = GetWorldPositionRef();
			const Vector pointOnEdge = entPos.NearestPointOnEdge( p1, p2 );

			/*if( previousExp != token.GetExploration() )
			{
				isLookingAtExploration = false;
				previousExp = token.GetExploration();
				DisableLookAts();
			}

			if( !isLookingAtExploration )
			{
				isLookingAtExploration = true;
				SLookAtScriptStaticInfo info;
				info.m_target = pointOnEdge;
				info.m_duration = -1.f;
				EnableLookAt( info );
			}*/

			static const CName debug( TXT("Expdbg1") );
			GetVisualDebug()->AddArrow( debug, entPos, pointOnEdge, 1.f, 0.2f, 0.2f, Color::GREEN, true, true, 0.2f );
		}
		/*else if( isLookingAtExploration )
		{
			isLookingAtExploration = false;
			previousExp = NULL;
			DisableLookAts();
		}*/
#endif
	}
}	

void CPlayer::UpdateWeaponInterestPoint()
{
	if ( m_weaponDrawnInterestPoint )
	{
		CInventoryComponent * comp = m_inventoryComponent.Get();
		if ( comp && comp->IsWeaponHeld() && !SItemEntityManager::GetInstance().HasActorAnyLatentAction( this ) )
		{
			// generate an interest point telling that the actor wields a weapon
			CReactionsManager* mgr = GCommonGame->GetReactionsManager();
			if ( mgr )
			{
				mgr->BroadcastInterestPoint( m_weaponDrawnInterestPoint, THandle< CNode >( this ), 2.0f );
			}
		}
	}
}

void CPlayer::OnDrawWeapon()
{
	if( m_weaponDrawMomentInterestPoint )
	{
		CReactionsManager* mgr = GCommonGame->GetReactionsManager();
		if ( mgr )
		{
			mgr->BroadcastInterestPoint( m_weaponDrawMomentInterestPoint, THandle< CNode >( this ), 2.0f );
		}
	}
}

RED_DEFINE_STATIC_NAME( EPlayerInteractionLock );
RED_DEFINE_STATIC_NAME( PIL_Cutscene );
struct CPlayerLockInteractionUtil
{
	static Int32 cutsceneChannel;	
	static Int32 GetLockInteractionChannelForCS()
	{
		if( cutsceneChannel == 0 )
		{
			CName enumName = CNAME(EPlayerInteractionLock);
			CName fieldName = CNAME( PIL_Cutscene );
			SRTTI::GetInstance().FindEnum( enumName )->FindValue( fieldName, cutsceneChannel );
		}
		return cutsceneChannel;
	}	
};
Int32 CPlayerLockInteractionUtil::cutsceneChannel = 0;	

void CPlayer::OnCutsceneStarted()
{
	TBaseClass::OnCutsceneStarted();
	LockButtonInteractions( CPlayerLockInteractionUtil::GetLockInteractionChannelForCS() );
}

void CPlayer::OnCutsceneEnded()
{
	TBaseClass::OnCutsceneEnded();
	UnlockButtonInteractions( CPlayerLockInteractionUtil::GetLockInteractionChannelForCS() );
}

Bool CPlayer::SetPlayerMovable( Bool movable, Bool enableSteerAgent )
{
	m_isMovable = true;
	return true;
}

Bool CPlayer::GetPlayerMovable() const
{
	return m_isMovable;
}

void CPlayer::UpdateInterestPoint( Float timeDelta )
{
	m_timeToInterestPointUpdate -= timeDelta; 
	if( m_timeToInterestPointUpdate <= 0.0f )
	{
		THandle< CNode > thisHandle( this );

		CReactionsManager* mgr = GCommonGame->GetReactionsManager();
		CAnimatedComponent* ac = GetMovingAgentComponent();
		if( ac && mgr )
		{
			if( m_presenceInterestPoint )
			{
				mgr->BroadcastInterestPoint( m_presenceInterestPoint, thisHandle, 2.0f );
			}

			Float moveSpeedRel = ac->GetRelativeMoveSpeed();
			if ( moveSpeedRel > 0.0f )
			{	
				if( moveSpeedRel > NOISE_THRESHOLD_SPEED )
				{
					if( m_fastMovementInterestPoint )
					{						
						mgr->BroadcastInterestPoint( m_fastMovementInterestPoint, thisHandle, 2.0f );
					}					
				}
				else
				{

					if( m_slowMovementInterestPoint )
					{
						mgr->BroadcastInterestPoint( m_slowMovementInterestPoint, thisHandle, 2.0f );
					}					
				}
			}
		}

		UpdateWeaponInterestPoint();

		m_timeToInterestPointUpdate = INTERESTPOINT_UPDATE_INTERVAL;
	}
}

void CPlayer::ActivateActionArea( CActionAreaComponent * area )
{
	// Add area to list
	ASSERT( area );
	if ( m_activeExplorations.Exist( area ) )
	{
		return;
	}
	m_activeExplorations.PushBack( area );

	// Show exploration interaction
	if ( m_activeExplorations.Size() == 1 )
	{
		// Get player interaction
		if ( !m_explorationComponent )
		{
			return;
		}

		// Update caption ( HACKy )
		ASSERT( !m_explorationComponent->IsEnabled() );
		m_explorationComponent->SetFriendlyName( area->GetActionName() );

		// Enable to being executed by player
		m_explorationComponent->SetEnabled( true );

		area->CallEvent( CNAME(OnEnable), true );
	}
}

void CPlayer::DeactivateActionArea( CActionAreaComponent * area )
{
	// TODO: rethink the update label shit, this is a HACK....
	// We cannot Activate/Deactivate interaction manually on our wish...

	// Remove from list
	ASSERT( area );
	if ( !m_activeExplorations.Exist( area ) )
	{
		return;
	}
	m_activeExplorations.RemoveFast( area );

	// Deactivate interaction
	if ( m_activeExplorations.Empty() )
	{
		// Get player interaction
		if ( !m_explorationComponent )
		{
			return;
		}

		// Disable
		ASSERT( m_explorationComponent->IsEnabled() );
		m_explorationComponent->SetEnabled( false );

		area->CallEvent( CNAME(OnEnable), false );
	}
}

void CPlayer::BestActionAreaToFront()
{
	if ( m_activeExplorations.Size() < 2 )
		return;

	const Vector playerPos = GetWorldPosition();
	const Float  playerYaw = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraRotation().Yaw;

	CActionAreaComponent* firstExploration;
	TDynArray< THandle< CActionAreaComponent > >::iterator explorIter = m_activeExplorations.Begin();
	
	// Find first not-NULL exploration
	do 
	{
		firstExploration = explorIter->Get();
		if( firstExploration == NULL )
		{
			m_activeExplorations.EraseFast( explorIter );
		}
	} while( firstExploration == NULL && explorIter != m_activeExplorations.End() );
	
	if( firstExploration == NULL )
	{
		return;
	}

	// Get exploration position
	Vector EA_pos   = firstExploration->GetClosestActionPosition( playerPos, true );
	// Get exploration front shift and add it to exploration position
	Vector EA_front = firstExploration->GetLocalToWorld().TransformVector( Vector::EY * 0.5f );
	EA_pos -= EA_front * 0.5f;
	// Get vector to exploration area
	Vector vecToEA = EA_pos - playerPos;

	Uint32  bestEA_Index    = 0;
	Float bestEA_Dist_Sqr = vecToEA.SquareMag3();
	Float bestEA_Yaw      = DistanceBetweenAnglesAbs( playerYaw, vecToEA.ToEulerAngles().Yaw );

	for ( Uint32 i = 1; i < m_activeExplorations.Size(); ++i )
	{
		CActionAreaComponent* exploration = m_activeExplorations[ i ].Get();
		if( exploration == NULL )
		{
			continue;
		}

		// Get exploration position
		Vector EA_pos = exploration->GetClosestActionPosition( playerPos, true );
		// Get exploration front shift and add it to exploration position
		Vector EA_front = exploration->GetLocalToWorld().TransformVector( Vector::EY * 0.5f );
		EA_pos -= EA_front * 0.5f;
		// Get vector to exploration area
		vecToEA = EA_pos - playerPos;

		Float EA_Yaw = DistanceBetweenAnglesAbs( playerYaw, vecToEA.ToEulerAngles().Yaw );

		if ( DistanceBetweenAnglesAbs( EA_Yaw, bestEA_Yaw ) > 10 )
		{
			if ( EA_Yaw < bestEA_Yaw )
			{
				bestEA_Index    = i;
				bestEA_Yaw      = EA_Yaw;
				bestEA_Dist_Sqr = vecToEA.SquareMag3();
			}
			continue;
		}

		Float EA_dist_Sqr = vecToEA.SquareMag3();
		if ( EA_dist_Sqr < bestEA_Dist_Sqr )
		{
			bestEA_Index    = i;
			bestEA_Yaw      = EA_Yaw;
			bestEA_Dist_Sqr = EA_dist_Sqr;
		}
	}

	if ( bestEA_Index > 0 )
	{
		THandle< CActionAreaComponent > oldBestEA	= m_activeExplorations[ 0 ];
		m_activeExplorations[ 0 ]					= m_activeExplorations[ bestEA_Index ];
		m_activeExplorations[ bestEA_Index ]		= oldBestEA;
		
		if ( m_explorationComponent )
		{
			CActionAreaComponent* component = m_activeExplorations[ 0 ].Get();
			m_explorationComponent->SetFriendlyName( component->GetActionName() );
		}
	}
}

Bool CPlayer::IsExplorationInteraction( const CInteractionComponent* interaction ) const
{
	return interaction != NULL && interaction == m_explorationComponent;
}

Bool CPlayer::HasAnyActiveExploration() const
{
	return m_activeExplorations.Size() > 0;
}

const Matrix& CPlayer::GetActiveExplorationTranform() const
{
	if ( HasAnyActiveExploration() )
	{
		const CActionAreaComponent* comp = m_activeExplorations[ 0 ].Get();
		
		if ( comp )
		{
			return comp->GetLocalToWorld();
		}
		else
		{
			ASSERT( comp );
		}
	}

	return Matrix::IDENTITY;
}

Bool CPlayer::OnExplorationStarted()
{
	Bool ret = TBaseClass::OnExplorationStarted();
	if ( ret )
	{
		CMovingAgentComponent* mac = GetMovingAgentComponent();
		if ( mac )
		{
            mac->ForceEntityRepresentation( true, CMovingAgentComponent::LS_Default );

			if ( mac->GetBehaviorStack() )
			{
				mac->GetBehaviorStack()->Lock( true );
			}
		}
	}

	return ret;
}

void CPlayer::OnExplorationEnded()
{
	TBaseClass::OnExplorationEnded();

	CMovingAgentComponent* mac = GetMovingAgentComponent();
	if ( mac )
	{
		if ( mac->GetBehaviorStack() )
		{
			mac->GetBehaviorStack()->Lock( false );
		}

		mac->ForceEntityRepresentation( false, CMovingAgentComponent::LS_Default );
	}
}

void CPlayer::OnProcessInteractionExecute( class CInteractionComponent* interaction )
{
	// Pass to base class
	TBaseClass::OnProcessInteractionExecute( interaction );

	// Start exploration triggered by the player
	if ( IsExplorationInteraction( interaction ) )
	{
		if( m_activeExplorations.Size() > 0 )
		{
			CActionAreaComponent* exploration = m_activeExplorations[ 0 ].Get();
			if( exploration != NULL )
			{
				exploration->TriggerExploration( this );
			}
		}
	}
}

void CPlayer::OnGrabItem( SItemUniqueId itemId, CName slot )
{
	TBaseClass::OnGrabItem( itemId, slot );
}

void CPlayer::OnPutItem( SItemUniqueId itemId, Bool emptyHand )
{
	TBaseClass::OnPutItem( itemId, emptyHand );
}

void CPlayer::OnMountItem( SItemUniqueId itemId, Bool wasHeld )
{
	TBaseClass::OnMountItem( itemId, wasHeld );

	CInventoryComponent *inv = GetInventoryComponent();
	m_itemVisibilityController.OnMountItem( inv, itemId );
}

void CPlayer::OnUnmountItem( SItemUniqueId itemId )
{
	TBaseClass::OnUnmountItem( itemId );

	CInventoryComponent *inv = GetInventoryComponent();
	m_itemVisibilityController.OnUnmountItem( inv, itemId );
}

void CPlayer::EquipItem( SItemUniqueId itemId, Bool ignoreMount )
{
	CallFunction( this, CNAME( OnEquipItemRequested ), itemId, ignoreMount );
}

void CPlayer::UnequipItem( SItemUniqueId itemId )
{
	CallFunction( this, CNAME( OnUnequipItemRequested ), itemId);
}

void CPlayer::PlayVoicesetForNPC( CNewNPC* npc, const String& voiceSet )
{
	// check if enough time passed since the last time a voiceset's been played
	if ( m_npcVoicesetCooldownTimer > 0.f )
	{
		return;
	}

	m_voiceSetsToChooseFrom.PushBackUnique( VoiceSetDef( npc, voiceSet ) );
}

void CPlayer::UpdateVoicesets( Float timeElapsed )
{
	m_npcVoicesetCooldownTimer -= timeElapsed;
	if ( m_npcVoicesetCooldownTimer > 0 || IsInNonGameplayScene() )
	{
		return;
	}
	
	VoiceSetDef* bestVoiceSet = NULL;
	Float voiceSetMaxValue = 0.001f;	// value of 0 means that the speaker is completely
									    // outside the scope of text visibility, and should be filtered out
	for ( TDynArray< VoiceSetDef >::iterator it = m_voiceSetsToChooseFrom.Begin();
		it != m_voiceSetsToChooseFrom.End(); ++it )
	{
		CNewNPC* iteratedNpc = it->npc.Get();
		if ( iteratedNpc == NULL )
		{
			continue;
		}
		Float val = EstimateVoiceSetSpeaker( iteratedNpc );
		if ( val > voiceSetMaxValue )
		{
			bestVoiceSet = &(*it);
			voiceSetMaxValue = val;
		}
	}

	
	if ( bestVoiceSet )
	{
		CNewNPC* bestVoicesetNpc = bestVoiceSet->npc.Get();
		if ( bestVoicesetNpc != NULL )
		{
			bestVoicesetNpc->PlayVoiceset( BTAP_AboveIdle, bestVoiceSet->voiceSet );
		}
		m_npcVoicesetCooldownTimer = m_npcVoicesetCooldown;
	}
	else
	{
		m_npcVoicesetCooldownTimer = 0;
	}
	m_voiceSetsToChooseFrom.Clear();
}

Float CPlayer::EstimateVoiceSetSpeaker( CNewNPC* npc ) const
{
	static Float MAX_DIST = 10.0f;
	static Float MAX_ANGLE = 45.0f;

	// the closest npc the player's looking at directly will get the highest score
	Vector dir = npc->GetWorldPosition() - GetWorldPosition();
	Float distVal = Clamp< Float >( MAX_DIST - dir.Mag2(), 0, MAX_DIST ) / MAX_DIST;

	Float angleDist = EulerAngles::AngleDistance( dir.ToEulerAngles().Yaw, GetWorldYaw() );
	angleDist = Clamp< Float >( MAX_ANGLE - angleDist, 0, MAX_ANGLE ) / MAX_ANGLE;

	// also - the more the npc is looking in the direction of the player, 
	// the greater chance it has to speak its line
	Float npcLookDist = EulerAngles::AngleDistance( (-dir).ToEulerAngles().Yaw, npc->GetWorldYaw() );
	npcLookDist = Clamp< Float >( MAX_ANGLE - npcLookDist, 0, MAX_ANGLE ) / MAX_ANGLE;
	
	Float value = distVal * angleDist * npcLookDist;
	return value;
}

// ------------------------------------------------------------------------
// Game saves
// ------------------------------------------------------------------------
void CPlayer::OnLoadGameplayState( IGameLoader* loader )
{	
	TBaseClass::OnLoadGameplayState( loader );

	CGameSaverBlock block0( loader, CNAME( player ) );
}

void CPlayer::Hack_SetSwordsHiddenInGame( Bool state, Float distanceToCamera, Float cameraYaw )
{
	CInventoryComponent* inv = GetInventoryComponent();

	if ( !state )
	{
		m_itemVisibilityController.SetItemsHiddenInGame( inv, false );
		return;
	}

	Float swordsDistMin = GGame->GetGameplayConfig().m_cameraHidePlayerSwordsDistMin;
	Float swordsDistMax = GGame->GetGameplayConfig().m_cameraHidePlayerSwordsDistMax;

	if ( distanceToCamera < swordsDistMin )
	{
		
		Float angleDiff = MAbs( EulerAngles::AngleDistance( GetWorldYaw(), cameraYaw ) );

		Float swordsAngleMin = GGame->GetGameplayConfig().m_cameraHidePlayerSwordsAngleMin;
		Float swordsAngleMax = GGame->GetGameplayConfig().m_cameraHidePlayerSwordsAngleMax;

		if ( angleDiff < swordsAngleMin )
		{
			m_itemVisibilityController.SetItemsHiddenInGame( inv, true );
		}
		else if ( angleDiff > swordsAngleMax )
		{
			m_itemVisibilityController.SetItemsHiddenInGame( inv, false );
		}
	}
	else if ( distanceToCamera > swordsDistMax )
	{
		m_itemVisibilityController.SetItemsHiddenInGame( inv, false );
	}
}

Bool CPlayer::HACK_ForceGetBonePosWS( Uint32 boneIndex, Vector& bonePosWS ) const
{
	if ( CAnimatedComponent* ac = GetRootAnimatedComponent() )
	{
		if ( const SBehaviorSampleContext* sampleContext = ac->GetBehaviorGraphSampleContext() )
		{
			const SBehaviorGraphOutput& pose = sampleContext->GetSampledPose();

			if ( boneIndex < pose.m_numBones )
			{
				const AnimQsTransform boneTransformWS = pose.GetBoneWorldTransform( ac, boneIndex );
				bonePosWS = reinterpret_cast< const Vector& >( boneTransformWS.Translation );
				bonePosWS.W = 1.0f;
				return true;
			}
		}
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////



void CPlayer::funcLockButtonInteractions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32 , channel, -1 );
	FINISH_PARAMETERS;
	LockButtonInteractions( channel );
}

void CPlayer::funcUnlockButtonInteractions( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32 , channel, -1 );
	FINISH_PARAMETERS;
	UnlockButtonInteractions( channel );
}

void CPlayer::funcGetActiveExplorationEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	CEntity* entity = NULL;

	if( !m_activeExplorations.Empty() )
	{
		entity = m_activeExplorations[0].Get()->GetEntity();
	}

	RETURN_OBJECT( entity );
}
void CPlayer::funcSetEnemyUpscaling( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, enemyUpscaling, false );
	FINISH_PARAMETERS;

	if ( enemyUpscaling != m_enemyUpscaling )
	{
		m_enemyUpscaling = enemyUpscaling;

		struct Functor
		{
			Functor( Bool b )
				: m_enemyUpscaling( b ) {}
			enum { SORT_OUTPUT = false };

			Bool operator()( const CActorsManagerMemberData& memberData ) const
			{
				if ( CNewNPC* npc = Cast< CNewNPC >( memberData.Get() ) )
				{
					npc->OnLevelUpscalingChanged( m_enemyUpscaling );
				}
				return true;
			}
			Bool m_enemyUpscaling;
		} functor( enemyUpscaling );

		// notice all npc's about upscaling
		GCommonGame->GetActorsManager()->TQuery( functor, nullptr, 0 );
	}
}
void CPlayer::funcGetEnemyUpscaling( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( m_enemyUpscaling );
}