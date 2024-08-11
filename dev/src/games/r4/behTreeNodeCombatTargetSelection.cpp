#include "build.h"
#include "behTreeNodeCombatTargetSelection.h"
#include "r4BehTreeInstance.h"
#include "../../common/core/priqueue.h"
#include "../../games/r4/combatDataComponent.h"
#include "../../games/r4/damageData.h"
#include "../../games/r4/r4BehTreeInstance.h"
#include "../../common/core/dataError.h"
#include "../../common/game/vehicle.h"
#include "../../common/engine/utils.h"

IMPLEMENT_RTTI_ENUM( ECombatTargetSelectionSkipTarget );
IMPLEMENT_ENGINE_CLASS( CBehTreeValECombatTargetSelectionSkipTarget );

RED_DEFINE_STATIC_NAME( IsMonster );
RED_DEFINE_STATIC_NAME( GetThreatLevel );

#define PROFILE_COMBAT_TARGET_SELECTION

////////////////////////////////////////////////////////////////////////

namespace
{
	struct SIsMonsterEvaluator
	{
		Bool operator()( const CGameplayEntity* entity ) const
		{
			Bool ret = false;
			CallFunctionRet( const_cast< CGameplayEntity* >( entity ), CNAME( IsMonster ), ret );
			return ret;
		}
	} IsMonsterEvaluator;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTargetSelectionDefinition
////////////////////////////////////////////////////////////////////////

IBehTreeNodeInstance* CBehTreeNodeCombatTargetSelectionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeCombatTargetSelectionInstance
////////////////////////////////////////////////////////////////////////

const Float CBehTreeNodeCombatTargetSelectionInstance::UNREACHABILITY_DURATION = 5.0f;

CBehTreeNodeCombatTargetSelectionInstance::CBehTreeNodeCombatTargetSelectionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: Super( def, owner, context, parent )
	, m_testMaxFrequency( 2.0f )
	, m_nextTestDelay( -1.0f )
	, m_targetOnlyPlayer( def.m_targetOnlyPlayer.GetVal( context ) )
	, m_hostileActorWeight( def.m_hostileActorWeight.GetVal( context ) )
	, m_currentTargetWeight( def.m_currentTargetWeight.GetVal( context ) )
	, m_hitterWeight( def.m_hitterWeight.GetVal( context ) )
	, m_maxWeightedDistance( def.m_maxWeightedDistance.GetVal( context ) )
	, m_distanceWeight( def.m_distanceWeight.GetVal( context ) )
	, m_playerWeightProbability( def.m_playerWeightProbability.GetVal( context ) )
	, m_playerWeight( def.m_playerWeight.GetVal( context ) )
	, m_monsterWeight( def.m_monsterWeight.GetVal( context ) )
	, m_skipVehicle( def.m_skipVehicle.GetVal( context ) )
	, m_skipVehicleProbability( def.m_skipVehicleProbability.GetVal( context ) )
	, m_skipUnreachable( def.m_skipUnreachable.GetVal( context ) )
	, m_skipUnreachableProbability( def.m_skipUnreachableProbability.GetVal( context ) )
	, m_skipNotThreatening( def.m_skipNotThreatening.GetVal( context ) )
	, m_skipNotThreateningProbability( def.m_skipNotThreateningProbability.GetVal( context ) )
	, m_hitters( def.m_rememberedHits.GetVal( context ) )
	, m_wasHit( false )
{
	SBehTreeEvenListeningData eventListenerData;
	eventListenerData.m_eventName = CNAME( BeingHit );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( ForceTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( UnforceTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( NoticedObjectReevaluation );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	context.AddEventListener( eventListenerData, this );

	if ( owner->GetNPC() == nullptr )
	{
		DATA_HALT( DES_Minor, CResourceObtainer::GetResource( owner->GetActor() ), TXT( "BehTree" ), TXT( "CombatTargetSelection node works only for CNewNPC entities" ) );
	}
}

void CBehTreeNodeCombatTargetSelectionInstance::Update()
{
	Complete( BTTO_SUCCESS );
}

Bool CBehTreeNodeCombatTargetSelectionInstance::Activate()
{
	m_owner->SetCombatTarget( m_nextTarget.Get() );
	m_wasHit = false;
	m_nextTestDelay = GetOwner()->GetLocalTime() + m_testMaxFrequency;
	m_nextTarget = nullptr;
	return Super::Activate();
}

Bool CBehTreeNodeCombatTargetSelectionInstance::OnListenedEvent( CBehTreeEvent& e )
{
	CName eventName = e.m_eventName;
	if ( eventName == CNAME( BeingHit ) )
	{
		if ( CDamageData* damageData = e.m_gameplayEventData.Get< CDamageData >() )
		{
			CGameplayEntity* entity = damageData->m_attacker.Get();
			if ( !damageData->m_isDoTDamage && entity != nullptr && entity->IsA< CActor >() )
			{
				AddHitter( static_cast< CActor* >( entity ) );
				return true;
			}
		}
		return false;
			
	}
	else if ( eventName == CNAME( ForceTarget ) )
	{
		SGameplayEventParamObject* forcedTarget = e.m_gameplayEventData.Get< SGameplayEventParamObject >();
		if ( forcedTarget )
		{
			m_forceTarget = ::Cast< CActor >( forcedTarget->m_value.Get() );
			m_nextTestDelay = 0.f;
		}
		return true;
	}
	else if ( eventName == CNAME( UnforceTarget ) )
	{
		m_forceTarget = nullptr;
		m_nextTestDelay = 0.f;
		return true;
	}
	else if ( e.m_eventName == CNAME( NoticedObjectReevaluation ) )
	{
		//GetOwner()->SetCombatTarget( nullptr );
		m_nextTestDelay = 0.f;
		return true;
	}
	return false;
}

void CBehTreeNodeCombatTargetSelectionInstance::OnDestruction()
{
	SBehTreeEvenListeningData eventListenerData;
	eventListenerData.m_eventName = CNAME( BeingHit );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( ForceTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( UnforceTarget );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	eventListenerData.m_eventName = CNAME( NoticedObjectReevaluation );
	eventListenerData.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;
	m_owner->RemoveEventListener( eventListenerData, this );

	Super::OnDestruction();
}

Bool CBehTreeNodeCombatTargetSelectionInstance::IsAvailable()
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_IsAvailable );
#endif

	CActor* actor = GetOwner()->GetActor();
	ASSERT( actor != nullptr );
	Bool newTargetFound = false;

	CActor* forceTarget = m_forceTarget.Get();

	if ( forceTarget != nullptr && forceTarget->IsAlive() )
	{
		if ( forceTarget != GetOwner()->GetCombatTarget().Get() )
		{
			m_nextTarget = forceTarget;
			newTargetFound = true;
		}
	}
	else 
	{
		Float currentTime = GetOwner()->GetLocalTime();
		CActor* currTarget = GetOwner()->GetCombatTarget().Get();
		Bool isReachable = true;
		if ( currTarget != nullptr && ( m_skipUnreachable == CTSST_SKIP_ALWAYS || m_skipUnreachable == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS ) )
		{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
			PC_SCOPE_PIX( CombatTargetSelection_IsCurrentTargetReachable );
#endif
			isReachable = CR4BehTreeInstance::Get( m_owner )->IsCombatTargetReachable();
			if ( !isReachable )
			{
				MarkAsUnreachable( currTarget );
			}
		}
		if ( currTarget == nullptr || !isReachable || !currTarget->IsAlive() || !currTarget->IsDangerous( actor ) || m_wasHit || m_nextTestDelay < currentTime )
		{
			CActor* newTarget = FindTarget();
			if ( newTarget != currTarget )
			{
				// m_wasHit and m_nextTestDelay are not set until target is changed
				m_nextTarget = newTarget;
				newTargetFound = true;
			}
			else
			{
				m_wasHit = false;
				m_nextTestDelay = currentTime + m_testMaxFrequency;
			}
		}
	}

	if( !newTargetFound )
	{
		DebugNotifyAvailableFail();
	}

	return newTargetFound;
}

CActor* CBehTreeNodeCombatTargetSelectionInstance::FindTarget()
{
	// The following structure holds information about
	// the best chosen target and other "potential" ones.
	// If the best target cannot be used (caused it is already
	// being attacked by "more important" attackers) the second
	// best is checked and then third and so on...
	struct SPotentialTargetsPool
	{
		typedef CCombatDataComponent::AttackersPoolEntry APEntry;
		TPriQueue< APEntry >	m_potentialTargets;
		CActor*					m_bestTarget;
		Float					m_maxScore;

		SPotentialTargetsPool()
			: m_bestTarget( nullptr )
			, m_maxScore( 0.0f )
		{}

		void Add( CActor* target, Float score )
		{
			if ( score > 0.0f )
			{
				m_potentialTargets.Push( APEntry( target, static_cast< Uint32 >( score ) ) );
			}
			if ( score > m_maxScore )
			{
				m_maxScore = score;
				m_bestTarget = target;
			}
		}

		CActor* ChooseBestPossibleTarget( CActor* attacker, Uint32& maxPotentialTargetsToCheck )
		{
			Uint32 potentialTargetsToCheck = Min( m_potentialTargets.Size(), maxPotentialTargetsToCheck );
			while ( potentialTargetsToCheck-- )
			{
				APEntry entry = m_potentialTargets.Top();
				m_potentialTargets.Pop();
				CActor* target = entry.m_actor.Get();
				CCombatDataComponent* combatData = CCombatDataPtr( target ).Get();
				// We check here if a target can be used (its attacker pool is not full
				// or current attacker's priority let it to preempt the worst attacker).
				if ( combatData != nullptr && combatData->AddToAttackersPool( attacker, entry.m_priority ) )
				{
					return target;
				}
			}
			return m_bestTarget;
		}
	};

#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_FindTarget );
#endif

	CNewNPC* npc = GetOwner()->GetNPC();
	if ( npc == nullptr )
	{
		return nullptr;
	}
	if ( m_targetOnlyPlayer )
	{
		CPlayer* player = GCommonGame->GetPlayer();
		if ( player->IsAlive() && npc->IsDangerous( player ) && npc->HasActorBeenNoticed( player ) )
		{
			return player;
		}
	}

	UpdateUnreachableTargets();

	Uint32 index = 0;
	SPotentialTargetsPool defaultPool;
	SPotentialTargetsPool reservePool;
	SPotentialTargetsPool* currentPool = nullptr;
	while ( CActor* newTarget = npc->GetNoticedObject( index++ ) )
	{
		if ( newTarget->IsAlive() )
		{
			currentPool = &defaultPool;
			// If the target is a vehicle
			if ( CheckForVehicle( newTarget ) )
			{
				if ( m_skipVehicle == CTSST_SKIP_ALWAYS )
				{
					continue;
				}
				// If vehicle should be skipped only if there is another target (that is not vehicle)
				// AND probability condition is fulfilled, we add the target to reserve pool.
				else if ( m_skipVehicle == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS && 
					      GEngine->GetRandomNumberGenerator().Get< Int32 >( 100 ) < m_skipVehicleProbability )
				{
					currentPool = &reservePool;
				}
			}
			if ( currentPool == &defaultPool && CheckForUnreachable( newTarget ) )
			{
				if ( m_skipUnreachable == CTSST_SKIP_ALWAYS )
				{
					continue;
				}
				// If unreachable should be skipped only if there is another target (that is reachable)
				// AND probability condition is fulfilled, we add the target to reserve pool.
				else if ( m_skipUnreachable == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS && 
					GEngine->GetRandomNumberGenerator().Get< Int32 >( 100 ) < m_skipUnreachableProbability )
				{
					currentPool = &reservePool;
				}
			}
			if ( currentPool == &defaultPool && CheckForNotThreatening( newTarget ) )
			{
				if ( m_skipNotThreatening == CTSST_SKIP_ALWAYS )
				{
					continue;
				}
				// If not threatening should be skipped only if there is another target (that is threatening)
				// AND probability condition is fulfilled, we add the target to reserve pool.
				else if ( m_skipNotThreatening == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS && 
					GEngine->GetRandomNumberGenerator().Get< Int32 >( 100 ) < m_skipNotThreateningProbability )
				{
					currentPool = &reservePool;
				}
			}
				 
			Float score = EvaluatePotentialTarget( newTarget );
			currentPool->Add( newTarget, score );
		}
	}
	
	{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
		PC_SCOPE_PIX( CombatTargetSelection_ChooseBestPossibleTarget );
#endif

		Uint32 maxPotentialTargetsToCheck = 3;
		CActor* bestTarget = defaultPool.ChooseBestPossibleTarget( npc, maxPotentialTargetsToCheck );
		if ( bestTarget == nullptr )
		{
			bestTarget = reservePool.ChooseBestPossibleTarget( npc, maxPotentialTargetsToCheck );
		}

		ASSERT( bestTarget == nullptr || npc->GetAttitudeGroup() != bestTarget->GetAttitudeGroup() );
		return bestTarget;
	}
}

Float CBehTreeNodeCombatTargetSelectionInstance::EvaluatePotentialTarget( CActor* target )
{
	CNewNPC* npc = GetOwner()->GetNPC();
	ASSERT( npc != nullptr );

#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_EvaluatePotentialTarget );
#endif

	if ( !target->IsAlive() || !npc->IsDangerous( target ) )
	{
		return 0;
	}

	Float sum = m_hostileActorWeight;

	// If was previously targeted and is still alive and kicking
	if ( target == GetOwner()->GetCombatTarget().Get() && target->IsAlive() )
	{
		sum += m_currentTargetWeight;
	}

	// Evaluate hitter weight
	sum += GetHitterWeight( target );

	// Evaluate distance

	Float dist = npc->GetWorldPosition().DistanceTo2D( target->GetWorldPosition() );
	if ( dist < m_maxWeightedDistance )
	{
		sum += m_distanceWeight * ( 1.0f - ( dist / m_maxWeightedDistance ) );
	}

	// Player weight
	if ( target->IsPlayer() && GEngine->GetRandomNumberGenerator().Get< Int32 >( 100 ) < m_playerWeightProbability )
	{
		sum += m_playerWeight;
	}

	// Monster weight
	if ( target->GetInfoCache().Get( target, GICT_Custom1, IsMonsterEvaluator ) )
	{
		sum += m_monsterWeight;
	}

	// Final evaluation score
	return sum;
}

Bool CBehTreeNodeCombatTargetSelectionInstance::CheckForVehicle( CActor* target ) const
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_CheckForVehicle );
#endif

	if ( m_skipVehicle == CTSST_SKIP_ALWAYS || m_skipVehicle == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS )
	{
		return target->FindComponent< CVehicleComponent >() != nullptr;
	}
	return false;
}

void CBehTreeNodeCombatTargetSelectionInstance::MarkAsUnreachable( CActor* target )
{
	m_unreachableTargets[ target ] = GetOwner()->GetLocalTime();
}

void CBehTreeNodeCombatTargetSelectionInstance::UpdateUnreachableTargets()
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_UpdateUnreachableTargets );
#endif

	TStaticArray< const CActor*, 3 > targetsToRemove;
	Float removeAllBeforeTime = GetOwner()->GetLocalTime() - UNREACHABILITY_DURATION;
	for ( const auto& it : m_unreachableTargets )
	{
		if ( it.m_second <= removeAllBeforeTime )
		{
			targetsToRemove.PushBack( it.m_first );
			if ( targetsToRemove.Full() )
			{
				break;
			}
		}
	}
	for ( Uint32 i = 0; i < targetsToRemove.Size(); i++ )
	{
		m_unreachableTargets.Erase( targetsToRemove[ i ] );
	}
}

Bool CBehTreeNodeCombatTargetSelectionInstance::CheckForUnreachable( CActor* target ) const
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_CheckForUnreachable );
#endif

	if ( m_skipUnreachable == CTSST_SKIP_ALWAYS || m_skipUnreachable == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS )
	{
		return m_unreachableTargets.KeyExist( target );
	}
	return false;
}

Bool CBehTreeNodeCombatTargetSelectionInstance::CheckForNotThreatening( CActor* target ) const
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_CheckForNotThreatening );
#endif

	if ( m_skipNotThreatening == CTSST_SKIP_ALWAYS || m_skipNotThreatening == CTSST_SKIP_IF_THERE_ARE_OTHER_TARGETS )
	{
		Int32 threatLevel = 1; // by default target is threatening
		CallFunctionRet( target, CNAME( GetThreatLevel ), threatLevel );
		return threatLevel == 0;
	}
	return false;
}

void CBehTreeNodeCombatTargetSelectionInstance::AddHitter( CActor* hitter )
{
	Uint32 index = m_hitters.Size() - 1;
	while ( index > 0 )
	{
		m_hitters[ index ] = m_hitters[ index - 1 ];
		index--;
	}
	m_hitters[0] = hitter;
	m_wasHit = true;
}

Float CBehTreeNodeCombatTargetSelectionInstance::GetHitterWeight( CActor* hitter )
{
#ifdef PROFILE_COMBAT_TARGET_SELECTION
	PC_SCOPE_PIX( CombatTargetSelection_GetHitterWeight );
#endif

	Uint32 size = m_hitters.Size();
	for ( Uint32 i = 0; i < size; i++ )
	{
		if ( m_hitters[i] == hitter )
		{
			return Float( size - i ) / size * m_hitterWeight;
		}
	}
	return 0.0f;
}
