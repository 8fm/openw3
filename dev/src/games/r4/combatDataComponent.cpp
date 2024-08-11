#include "build.h"
#include "combatDataComponent.h"

#include "ticketSystem.h"
#include "ticketSystemConfiguration.h"
#include "../../common/engine/gameTimeManager.h"

IMPLEMENT_ENGINE_CLASS( CCombatDataComponent );
IMPLEMENT_ENGINE_CLASS( CCombatDataComponentParam );

//////////////////////////////////////////////////////////////////////////
// CCombatDataComponent
//////////////////////////////////////////////////////////////////////////
CCombatDataComponent::CCombatDataComponent()
	: m_attackersPoolMaxSize( 3 )
	, m_lowestPriority( NumericLimits< Uint32 >::Max() )
	, m_nextPoolCleanUp( -1.0f )
	, m_poolCleanUpInterval( 1.0f )
{
}

void CCombatDataComponent::ComputeYawDistances()
{
	const EngineTime& currentTime = GGame->GetEngineTime();
	if ( currentTime <= m_lastYawComputation )
	{
		return;
	}
	m_lastYawComputation = currentTime;

	Vector2 myPosition = GetEntity()->GetWorldPositionRef().AsVector2();

	for ( auto it = m_attackers.Begin(), end = m_attackers.End(); it != end; ++it )
	{
		CActor* attacker = it->m_attacker.Get();
		if ( attacker )
		{
			Vector2 diff = attacker->GetWorldPositionRef().AsVector2() - myPosition;
			it->m_worldSpaceYaw = EulerAngles::YawFromXY( diff.X, diff.Y );
			it->m_distance2DSq = diff.SquareMag();
		}
	}
	m_attackers.InsertionSort();											// it could possibly be insertion sort because usually there are only little changes, and not-so-much data
}

Int32 CCombatDataComponent::RegisterAttacker( CActor* actor )
{
	m_lastYawComputation = 0.f;
	CAttacker attackerData;
	attackerData.m_actionRing = -1;
	attackerData.m_attacker = actor;
	attackerData.m_desiredSeparation = 180.f;
	attackerData.m_worldSpaceYaw = 0.f;
	m_attackers.PushBack( attackerData );
	return m_attackers.Size()-1;
}

Bool CCombatDataComponent::UnregisterAttacker( CActor* actor )
{
	Int32 index = GetAttackerIndex( actor );
	if ( IsAttackerIndexValid( index ) )
	{
		m_attackers.RemoveAt( index );
		return true;
	}
	return false;
}
CTicketSourceConfiguration* CCombatDataComponent::GetCustomConfiguration( CName name )
{	
	CEntity* entity = GetEntity();
	if ( entity )
	{
		CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
		if ( entityTemplate )
		{
			CTicketConfigurationParam* conf = entityTemplate->FindGameplayParamT< CTicketConfigurationParam >();
			if ( conf )
			{
				CTicketSourceConfiguration* ticketConf = conf->GetConfiguration( name );
				return ticketConf;
			}
		}
	}
	return nullptr;
}

Uint32 CCombatDataComponent::GetAttackersCount( Int16 actionRing ) const
{
	Uint32 count = 0;
	for ( Uint32 i = 0; i < m_attackers.Size(); --i )
	{
		if ( m_attackers[ i ].m_actionRing == actionRing )
		{
			++count;
		}
	}
	return count;
}

Bool CCombatDataComponent::AddToAttackersPool( CActor* attacker, Uint32 priority )
{
	Float currentTime = ( Float ) EngineTime::GetNow();
	if ( m_nextPoolCleanUp < currentTime )
	{
		m_nextPoolCleanUp = currentTime + m_poolCleanUpInterval;
		CleanAttackersPool();
	}
	if ( m_attackersPool.Size() >= m_attackersPoolMaxSize && priority <= m_lowestPriority )
	{
		return false;
	}
	AttackersPool::iterator currentAttacker = m_attackersPool.End();
	AttackersPool::iterator worstAttacker = m_attackersPool.End();
	Uint32 lowestPriority = priority;
	Uint32 secondLowestPriority = priority;
	for ( AttackersPool::iterator it = m_attackersPool.Begin(); it != m_attackersPool.End(); ++it )
	{
		if ( it->m_actor.Get() == attacker )
		{
			currentAttacker = it;
		}
		if ( it->m_priority < lowestPriority )
		{
			secondLowestPriority = lowestPriority;
			lowestPriority = it->m_priority;
			worstAttacker = it;
		}
		else if ( it->m_priority < secondLowestPriority )
		{
			secondLowestPriority = it->m_priority;
		}
	}
	if ( m_attackersPool.Size() >= m_attackersPoolMaxSize )
	{
		if ( currentAttacker != m_attackersPool.End() )
		{
			currentAttacker->m_priority = priority;
			m_lowestPriority = lowestPriority;
		}
		else if ( worstAttacker != m_attackersPool.End() )
		{
			m_attackersPool.Erase( worstAttacker );
			m_attackersPool.PushBack( AttackersPoolEntry( attacker, priority ) );
			m_lowestPriority = secondLowestPriority;
		}
		else
		{
			return false;
		}
	}
	else
	{
		if ( currentAttacker != m_attackersPool.End() )
		{
			currentAttacker->m_priority = priority;
		}
		else
		{
			m_attackersPool.PushBack( AttackersPoolEntry( attacker, priority ) );
		}
		m_lowestPriority = lowestPriority;
	}
	return true;
}

void CCombatDataComponent::CleanAttackersPool()
{
	m_lowestPriority = NumericLimits< Uint32 >::Max();
	for ( Int32 i = m_attackersPool.Size() - 1; i >= 0; i-- )
	{
		CActor* actor = m_attackersPool[ i ].m_actor.Get();
		// remove empty actors or non-attackers
		if ( actor == nullptr || GetAttackerIndex( actor ) == -1 )
		{
			m_attackersPool.RemoveAt( i );
		}
		else if ( m_attackersPool[ i ].m_priority < m_lowestPriority )
		{
			m_lowestPriority = m_attackersPool[ i ].m_priority;
		}
	}
}

void CCombatDataComponent::LoadAttackersPoolConfig()
{
	CEntityTemplate* templ = Cast< CEntityTemplate >( GetEntity()->GetTemplate() );
	if ( templ != NULL )
	{
		CCombatDataComponentParam *param = templ->FindGameplayParamT< CCombatDataComponentParam >( false );
		if ( param != NULL )
		{
			m_attackersPoolMaxSize = param->GetAttackersPoolMaxSize();
		}
	}
}

void CCombatDataComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CCombatDataComponent_OnAttached );

	LoadAttackersPoolConfig();
}

void CCombatDataComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	Clear();
}
void CCombatDataComponent::OnFinalize()
{
	TBaseClass::OnFinalize();

	Clear();
}
void CCombatDataComponent::Clear()
{
	ClearTickets();
	for ( auto it = m_attackers.Begin(), end = m_attackers.End(); it != end; ++it )
	{
		CActor* attacker = (*it).m_attacker.Get();
		if ( attacker )
		{
			CBehTreeMachine* behTreeMachine = attacker->GetBehTreeMachine();
			CBehTreeInstance* behTreeInstance = behTreeMachine ? behTreeMachine->GetBehTreeInstance() : nullptr;
			if ( behTreeInstance )
			{
				behTreeInstance->OnCombatTargetDestroyed();
			}
		}
	}
	m_attackersPool.Clear();
}

Int32 CCombatDataComponent::GetAttackerIndex( CActor* actor )
{
	for ( Int32 index = 0, size = m_attackers.Size(); index < size; ++index )
	{
		if ( m_attackers[ index ].m_attacker.Get() == actor )
		{
			return index;
		}
	}
	return -1;
}
Int32 CCombatDataComponent::RightAttackerIndex( Int32 index )
{
	const CAttacker& attacker = m_attackers[ index ];
	Float distToleranceSq = sqrt( attacker.m_distance2DSq ) + 2.f; distToleranceSq *= distToleranceSq;
	Int32 rightIndex = index;
	do
	{
		rightIndex = (rightIndex == 0) ? m_attackers.Size()-1 : rightIndex-1;
		const CAttacker& leftAttacker =  m_attackers[ rightIndex ];
		if ( ( attacker.m_actionRing >= 0 && leftAttacker.m_actionRing == attacker.m_actionRing ) ||
			leftAttacker.m_distance2DSq <= distToleranceSq )
		{
			return rightIndex;
		}
	}
	while ( rightIndex != index );
	return index;
}

Int32 CCombatDataComponent::LeftAttackerIndex( Int32 index )
{
	const CAttacker& attacker = m_attackers[ index ];
	Float distToleranceSq = sqrt( attacker.m_distance2DSq ) + 2.f; distToleranceSq *= distToleranceSq;
	Int32 leftIndex = index;
	Int32 maxIndex = m_attackers.Size()-1;
	do
	{
		leftIndex = (leftIndex == maxIndex) ? 0 : leftIndex+1;
		const CAttacker& rightAttacker =  m_attackers[ leftIndex ];
		if ( ( attacker.m_actionRing >= 0 && rightAttacker.m_actionRing == attacker.m_actionRing ) ||
			rightAttacker.m_distance2DSq <= distToleranceSq )
		{
			return leftIndex;
		}
	}
	while ( leftIndex != index );
	return index;
}

void CCombatDataComponent::funcGetAttackersCount( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_INT( static_cast< Int32 >( m_attackers.Size() ) );
}

void CCombatDataComponent::funcGetTicketSourceOwners( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( TDynArray< THandle< CActor > >, output, TDynArray< THandle< CActor > > () );
	GET_PARAMETER( CName, ticketName, CName::NONE );
	FINISH_PARAMETERS;

	CTicketSource* ticketSource = GetTicketSource( ticketName, false );
	if ( ticketSource )
	{
		ticketSource->CollectAquiredTicketOwners( output );
	}
}

void CCombatDataComponent::funcTicketSourceOverrideRequest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, ticketName, CName::NONE );
	GET_PARAMETER( Int32, ticketCountMod, 0 );
	GET_PARAMETER( Float, importanceMod, 0.f );
	FINISH_PARAMETERS;

	Int32 ret = -1;
	CTicketSource* ticketSource = GetTicketSource( ticketName, false );
	if ( ticketSource )
	{
		ret = ticketSource->Override( Int16( ticketCountMod ), importanceMod );
	}
	RETURN_INT( ret );
}

void CCombatDataComponent::funcTicketSourceClearRequest( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, ticketName, CName::NONE );
	GET_PARAMETER( Int32, requestId, 0 );
	FINISH_PARAMETERS;

	Bool success = false;

	CTicketSource* ticketSource = GetTicketSource( ticketName, false );
	if ( ticketSource )
	{
		success = ticketSource->ClearOverride( requestId );
	}
	RETURN_BOOL( success );
}


void CCombatDataComponent::funcForceTicketImmediateImportanceUpdate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, ticketName, CName::NONE );
	FINISH_PARAMETERS;

	CTicketSource* ticketSource = GetTicketSource( ticketName, false );
	if ( ticketSource )
	{
		ticketSource->ForceImmediateImportanceUpdate();
	}
}

void CCombatDataComponent::funcHasAttackersInRange( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, range, 10.f );
	FINISH_PARAMETERS;

	Bool foundAttacker = false;

	if ( !m_attackers.Empty() )
	{
		Float rangeSq = range*range;
		const Vector& worldPos = GetEntity()->GetWorldPositionRef();

		for ( auto it = m_attackers.Begin(), end = m_attackers.End(); it != end; ++it )
		{
			const CAttacker& a = *it;
			CActor* actor = a.m_attacker.Get();
			if ( actor )
			{
				Float distSq = ( worldPos - actor->GetWorldPositionRef() ).SquareMag3();
				if ( distSq < rangeSq )
				{
					foundAttacker = true;
					break;
				}
			}
			
		}
	}
	
	RETURN_BOOL( foundAttacker );
}

//////////////////////////////////////////////////////////////////////////
// CCombatDataPtr
//////////////////////////////////////////////////////////////////////////
CCombatDataComponent* CCombatDataPtr::LazyCreateComponent( CActor* actor )
{
	const auto& componentsList = actor->GetComponents();
	// find allready existing component
	for( auto it = componentsList.Begin(), end = componentsList.End(); it != end; ++it )
	{
		CComponent* component = *it;
		if ( component->IsA< CCombatDataComponent >() )
		{
			CCombatDataComponent* combatData = static_cast< CCombatDataComponent* >( component );
			return combatData;
		}
	}
	// create new one
	SComponentSpawnInfo spawnInfo;
	CCombatDataComponent* combatData = static_cast< CCombatDataComponent* >( actor->CreateComponent( CCombatDataComponent::GetStaticClass(), spawnInfo ) );
	return combatData;
}