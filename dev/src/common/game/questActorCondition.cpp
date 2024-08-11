#include "build.h"
#include "questActorCondition.h"

#include "../game/characterStats.h"
#include "../engine/tagManager.h"


IMPLEMENT_RTTI_ENUM( EQuestActorConditionLogicOperation );

IMPLEMENT_ENGINE_CLASS( IActorConditionType );
IMPLEMENT_ENGINE_CLASS( CQuestActorCondition );
IMPLEMENT_ENGINE_CLASS( CQuestManyActorsCondition );
IMPLEMENT_ENGINE_CLASS( CQuestNoLivingActorsCondition );

RED_DEFINE_NAME( OnActorLost );

Bool IActorConditionType::Evaluate( CActor* actor )
{
	if ( actor == nullptr )
	{
		return EvaluateNullActor();
	}
	const Bool result = EvaluateImpl( actor );
	return m_inverted ? result == false : result;
}

///////////////////////////////////////////////////////////////

CQuestActorCondition::CQuestActorCondition()
	: m_checkType( nullptr )
	, m_actorWatcher( nullptr )
{
}

CQuestActorCondition::~CQuestActorCondition()
{
	delete m_actorWatcher;
}

void CQuestActorCondition::OnActivate()
{
	TBaseClass::OnActivate();

	if ( m_actorWatcher == nullptr )
	{
		m_actorWatcher = new TActorWatcher( m_actorTag );
	}
	EEntityWatcherStateChange state = m_actorWatcher->Enable( true );
	if ( state == EWST_EntityFound && m_checkType != nullptr )
	{
		CActor* actor = m_actorWatcher->Get();
		if ( actor != nullptr )
		{
			m_checkType->OnActivate( actor );
		}
	}
}

void CQuestActorCondition::OnDeactivate()
{
	if ( m_checkType != nullptr )
	{
		CActor* actor = m_actorWatcher->Get();
		if ( actor != nullptr )
		{
			m_checkType->OnDeactivate( actor );
		}
	}
	m_actorWatcher->Enable( false );

	TBaseClass::OnDeactivate();
}

Bool CQuestActorCondition::OnIsFulfilled()
{
	if ( m_checkType == nullptr || m_actorTag == CName::NONE )
	{
		return false;
	}

	CActor* actor = nullptr;
	EEntityWatcherStateChange state = m_actorWatcher->Update();
	if ( state == EWST_EntityFound )
	{
		actor = m_actorWatcher->Get();
		if ( actor != nullptr )
		{
			m_checkType->OnActivate( actor );
		}
	}
	else if ( state == EWST_EntityLost )
	{
		m_checkType->OnActorLost();
	}
	else // state == EWST_NothingChanged
	{
		actor = m_actorWatcher->Get();
	}

	if ( actor != nullptr )
	{
		return m_checkType->Evaluate( actor );
	}
	return m_checkType->EvaluateNullActor();
}

/////////////////////////////////////////////////////////////////////////////// 

CQuestManyActorsCondition::CQuestManyActorsCondition()
	: m_logicOperation( ACTORS_All )
	, m_condition( nullptr )
	, m_wasRegistered( false )
	, m_findActors( false )
{
}

CQuestManyActorsCondition::~CQuestManyActorsCondition()
{
	RegisterCallback( false );
}

void CQuestManyActorsCondition::OnActivate()
{
	TBaseClass::OnActivate();

	m_findActors = true;
	m_wasRegistered = false;
	RegisterCallback( true );
	if ( m_condition != nullptr )
	{
		m_condition->OnActivate( nullptr );
	}
}

void CQuestManyActorsCondition::OnDeactivate()
{
	if ( m_condition != nullptr )
	{
		m_condition->OnDeactivate( nullptr );
	}
	m_actors.Clear();
	RegisterCallback( false );

	TBaseClass::OnDeactivate();
}

Bool CQuestManyActorsCondition::OnIsFulfilled()
{
	if ( !m_condition )
	{
		return false;
	}

	if ( m_findActors )
	{
		FindActors();
		m_findActors = false;
	}

	CActor* actor;
	Uint32 hitCount = 0;
	Uint32 checkedActors = 0;

	for ( Uint32 i = 0; i < m_actors.Size(); ++i )
	{
		actor = m_actors[ i ].Get();	
		if ( actor )
		{
			++checkedActors;		
			hitCount += m_condition->Evaluate( actor ) ? 1 : 0;   // not checking m_logicOperation inside the loop to reduce number of branches
		}
	}

	if ( checkedActors == 0 )
	{
		return false;
	}

	if ( m_logicOperation == ACTORS_All )
	{
		return hitCount == checkedActors;
	}

	if ( m_logicOperation == ACTORS_AtLeastOne )
	{
		return hitCount > 0;
	}

	if ( m_logicOperation == ACTORS_OneAndOnly )
	{
		return hitCount == 1;
	}

	return false;
}

void CQuestManyActorsCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && m_actorTags.HasTag( param.Get< CName >() ) )
	{
		m_findActors = true;
	}
}

Bool CQuestManyActorsCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_actorTags.GetTags() );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_actorTags.GetTags() );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestManyActorsCondition::FindActors()
{
	m_actors.ClearFast();

	if ( m_actorTags.Empty() || GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		return;
	}

	struct Functor : public Red::System::NonCopyable
	{
		Functor( TDynArray< THandle< CActor > >& actors )
			: m_actors( actors ) {}

		RED_INLINE Bool EarlyTest( CNode* node ) const
		{
			return node->IsA< CActor >();
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			if ( isGuaranteedUnique )
			{
				m_actors.PushBack( static_cast< CActor* >( node ) );
			}
			else
			{
				m_actors.PushBackUnique( static_cast< CActor* >( node ) );
			}
		}

		TDynArray< THandle< CActor > >& m_actors;
	} functor( m_actors );

	GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_actorTags, functor );
}

///////////////////////////////////////////////////////////////////////////////

CQuestNoLivingActorsCondition::CQuestNoLivingActorsCondition()
	: m_wasRegistered( false )
	, m_findActors( false )
{
}

CQuestNoLivingActorsCondition::~CQuestNoLivingActorsCondition()
{
	RegisterCallback( false );
}

void CQuestNoLivingActorsCondition::OnActivate()
{
	TBaseClass::OnActivate();
	m_findActors = true;
	m_wasRegistered = false;
	RegisterCallback( true );
}

void CQuestNoLivingActorsCondition::OnDeactivate()
{
	TBaseClass::OnDeactivate();
	m_actors.Clear();
	RegisterCallback( false );
}

Bool CQuestNoLivingActorsCondition::OnIsFulfilled()
{
	if ( m_findActors )
	{
		FindActors();
		m_findActors = false;
	}

	if ( m_actors.Size() == 0 )
	{
		return true;
	}

	CActor* actor;
	Uint32 hitCount = 0;
	Uint32 checkedActors = 0;

	for ( Uint32 i = 0; i < m_actors.Size(); ++i )
	{
		actor = m_actors[ i ].Get();	
		if ( actor )
		{
			++checkedActors;		
			hitCount += actor->IsAlive() == false ? 1 : 0;   // not checking m_logicOperation inside the loop to reduce number of branches
		}
	}

	return hitCount == checkedActors;
}

void CQuestNoLivingActorsCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && m_actorTags.HasTag( param.Get< CName >() ) )
	{
		m_findActors = true;
	}
}

Bool CQuestNoLivingActorsCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_actorTags.GetTags() );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_actorTags.GetTags() );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestNoLivingActorsCondition::FindActors()
{
	m_actors.ClearFast();

	if ( m_actorTags.Empty() || GGame == nullptr || GGame->GetActiveWorld() == nullptr )
	{
		return;
	}

	struct Functor : public Red::System::NonCopyable
	{
		Functor( TDynArray< THandle< CActor > >& actors )
			: m_actors( actors ) {}

		RED_INLINE Bool EarlyTest( CNode* node ) const
		{
			return node->IsA< CActor >();
		}
		RED_INLINE void Process( CNode* node, Bool isGuaranteedUnique )
		{
			if ( isGuaranteedUnique )
			{
				m_actors.PushBack( static_cast< CActor* >( node ) );
			}
			else
			{
				m_actors.PushBackUnique( static_cast< CActor* >( node ) );
			}
		}

		TDynArray< THandle< CActor > >& m_actors;
	} functor( m_actors );

	GGame->GetActiveWorld()->GetTagManager()->IterateTaggedNodes( m_actorTags, functor );
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCIsAlive )

Bool CQCIsAlive::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );
	return actor->IsAlive();
}

Bool CQCIsAlive::EvaluateNullActor()
{
	// If actor is null we consider him "not alive".
	// It returns false for "normal" condition and true for inverted one.
	return m_inverted;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCActorScriptedCondition )

CQCActorScriptedCondition::CQCActorScriptedCondition()
	: m_evaluateFunction( nullptr )
{
}

void CQCActorScriptedCondition::OnActivate( CActor* actor )
{
	CacheScriptedFunctions();
	THandle< CActor > hActor( actor );
	CallFunction( this, CNAME( OnActivate ), hActor );
}

void CQCActorScriptedCondition::OnDeactivate( CActor* actor )
{
	THandle< CActor > hActor( actor );
	CallFunction( this, CNAME( OnDeactivate ), hActor );
	m_evaluateFunction = nullptr;
}

void CQCActorScriptedCondition::OnActorLost()
{
	CallFunction( this, CNAME( OnActorLost ) );
}

Bool CQCActorScriptedCondition::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );
	Bool result = false;
	if ( m_evaluateFunction != nullptr )
	{
		THandle< CActor > hActor( actor );	
		m_evaluateFunction->Call( this, &hActor, &result );
	}
	return result;
}

void CQCActorScriptedCondition::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();
	CacheScriptedFunctions();
}

void CQCActorScriptedCondition::CacheScriptedFunctions()
{
	m_evaluateFunction = nullptr;
	IScriptable* context = this;
	m_evaluateFunction = this->FindFunction( context, CNAME( Evaluate ), false );
	RED_ASSERT( m_evaluateFunction != nullptr );
	RED_ASSERT( m_evaluateFunction->GetNumParameters() == 1 );
	RED_ASSERT( m_evaluateFunction->GetParameter( 0 ) != nullptr );
	RED_ASSERT( m_evaluateFunction->GetParameter( 0 )->GetType()->GetName() == TTypeName< THandle< CActor > >::GetTypeName() );
	RED_ASSERT( m_evaluateFunction->GetReturnValue() != nullptr );
	RED_ASSERT( m_evaluateFunction->GetReturnValue()->GetType()->GetName() == TTypeName< Bool >::GetTypeName() );
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCHasAbility )

Bool CQCHasAbility::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );

	CCharacterStats* stats = actor->GetCharacterStats();
	if ( !stats )
	{
		return false;
	}

	return stats->HasAbility( m_ability );
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCActorInventory )

CQCActorInventory::CQCActorInventory()
	: m_isFulfilled( false )
	, m_wasRegistered( false )
	, m_multipleActorsCondition( false )
{}

void CQCActorInventory::OnActivate( CActor* actor )
{
	TBaseClass::OnActivate( actor );

	// if actor is nullptr -> the condition is used in CQuestManyActorsCondition
	// so we cannot register callback
	if ( actor == nullptr )
	{
		m_multipleActorsCondition = true;
	}
	else
	{
		m_isFulfilled = false;
		m_wasRegistered = false;
		CInventoryComponent* inventory = actor->GetInventoryComponent();
		if ( inventory != nullptr )
		{
			m_isFulfilled = EvaluateImpl( inventory );
		}
		if ( !m_isFulfilled )
		{
			RegisterCallback( actor, true );
		}
	}
}

void CQCActorInventory::OnDeactivate( CActor* actor )
{
	if ( actor == nullptr )
	{
		RED_ASSERT( m_multipleActorsCondition );
	}
	else
	{
		RegisterCallback( actor, false );
		m_wasRegistered = false;
	}

	TBaseClass::OnDeactivate( actor );
}

void CQCActorInventory::OnActorLost()
{
	m_wasRegistered = false;
}

Bool CQCActorInventory::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );

	if ( m_multipleActorsCondition )
	{
		m_isFulfilled = EvaluateImpl( actor->GetInventoryComponent() );
	}
	else if ( !m_isFulfilled && !m_wasRegistered )
	{		
		if ( RegisterCallback( actor, true ) )
		{
			m_isFulfilled = EvaluateImpl( actor->GetInventoryComponent() );
		}
	}
	return m_isFulfilled;
}

Bool CQCActorInventory::RegisterCallback( CActor* actor, Bool reg )
{
	if ( reg != m_wasRegistered && actor->GetInventoryComponent() != nullptr )
	{
		if ( reg )
		{
			actor->GetInventoryComponent()->AddListener( this );
		}
		else
		{
			actor->GetInventoryComponent()->RemoveListener( this );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

Bool CQCActorInventory::EvaluateImpl( CInventoryComponent* inventory )
{
	return false;
}

void CQCActorInventory::OnInventoryEvent( CInventoryComponent* inventory, EInventoryEventType eventType, SItemUniqueId itemId, Int32 quantity, Bool fromAssociatedInventory )
{
	m_isFulfilled = EvaluateImpl( inventory );
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCHasItem )

CQCHasItem::CQCHasItem()
	: m_quantity( 1 )
	, m_compareFunc( CF_GreaterEqual )
{
}

Bool CQCHasItem::EvaluateImpl( CInventoryComponent* inventory )
{
	if ( inventory  == nullptr )
	{
		return false;
	}

	Uint32 queryValue = 0;
	if ( m_item != CName::NONE )
	{
		queryValue = static_cast< Int32 >( inventory->GetItemQuantityByName( m_item, true ) );
	}
	else if ( m_itemCategory != CName::NONE )
	{
		queryValue = inventory->GetItemQuantityByCategory( m_itemCategory, true );
	}
	else if ( m_itemTag != CName::NONE )
	{
		queryValue = inventory->GetItemQuantityByTag( m_itemTag, true );
	}
	else
	{
		return false;
	}

	if ( m_compareFunc == CF_Equal )
	{
		return queryValue == m_quantity;
	}
	else if ( m_compareFunc == CF_NotEqual )
	{
		return queryValue != m_quantity;
	}
	else if ( m_compareFunc == CF_Less )
	{
		return queryValue < m_quantity;
	}
	else if ( m_compareFunc == CF_LessEqual )
	{
		return queryValue <= m_quantity;
	}
	else if ( m_compareFunc == CF_Greater )
	{
		return queryValue > m_quantity;
	}
	else if ( m_compareFunc == CF_GreaterEqual )
	{
		return queryValue >= m_quantity;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CQCItemQuantity )

CQCItemQuantity::CQCItemQuantity()
	: m_quantity( 1 )
	, m_compareFunc( CF_GreaterEqual )
{
}

Bool CQCItemQuantity::EvaluateImpl( CInventoryComponent* inventory )
{
	if ( inventory == nullptr )
	{
		return false;
	}

	Uint32 queryValue = 0;
	if ( m_item != CName::NONE )
	{
		queryValue = inventory->GetItemQuantityByName( m_item, true );
	}
	else if ( m_itemCategory != CName::NONE )
	{
		queryValue = inventory->GetItemQuantityByCategory( m_itemCategory, true );
	}
	else if ( m_itemTag != CName::NONE )
	{
		queryValue = inventory->GetItemQuantityByTag( m_itemTag, true );
	}
	else
	{
		return false;
	}

	if ( m_compareFunc == CF_Equal )
	{
		return queryValue == m_quantity;
	}
	else if ( m_compareFunc == CF_NotEqual )
	{
		return queryValue != m_quantity;
	}
	else if ( m_compareFunc == CF_Less )
	{
		return queryValue < m_quantity;
	}
	else if ( m_compareFunc == CF_LessEqual )
	{
		return queryValue <= m_quantity;
	}
	else if ( m_compareFunc == CF_Greater )
	{
		return queryValue > m_quantity;
	}
	else if ( m_compareFunc == CF_GreaterEqual )
	{
		return queryValue >= m_quantity;
	}

	return true;
}

/////////////////////////////////////////////////////////////////////////////// 
IMPLEMENT_ENGINE_CLASS( CQCAttitude );

CQCAttitude::CQCAttitude()
: m_attitude( AIA_Hostile )
{
}

Bool CQCAttitude::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );
	EAIAttitude currAttitude = actor->GetAttitude( GCommonGame->GetPlayer() );
	return currAttitude == m_attitude;
}

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CQCDistanceTo );

CQCDistanceTo::CQCDistanceTo()
	: m_compareFunc( CF_LessEqual )
	, m_distance( 1.f )
	, m_cachedNode( nullptr )
	, m_wasRegistered( false )
{
}

CQCDistanceTo::~CQCDistanceTo()
{
	RegisterCallback( false );
}

void CQCDistanceTo::OnActivate( CActor* actor )
{
	TBaseClass::OnActivate( actor );

	FindTargetNode();
	m_wasRegistered = false;
	RegisterCallback( true );
}

void CQCDistanceTo::OnDeactivate( CActor* actor )
{
	RegisterCallback( false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate( actor );
}

Bool CQCDistanceTo::EvaluateImpl( CActor* actor )
{
	RED_ASSERT( actor != nullptr );

	// get the cached node, or try to cache it
	CNode* cachedNode = m_cachedNode.Get();

	// node is cached, we may proceed with the test
	if ( cachedNode != nullptr )
	{
		const Float squaredDistanceToCheckAgainst = m_distance * m_distance;
		const Float sqaredDistance = ( actor->GetWorldPosition() - cachedNode->GetWorldPosition() ).SquareMag3();
		switch ( m_compareFunc )
		{
			case	CF_NotEqual:		return sqaredDistance != squaredDistanceToCheckAgainst; 
			case	CF_Less:			return sqaredDistance <  squaredDistanceToCheckAgainst; 
			case	CF_LessEqual:		return sqaredDistance <= squaredDistanceToCheckAgainst; 
			case	CF_Greater:			return sqaredDistance >  squaredDistanceToCheckAgainst; 
			case	CF_GreaterEqual:	return sqaredDistance >= squaredDistanceToCheckAgainst; 
			case 	CF_Equal:			return sqaredDistance == squaredDistanceToCheckAgainst; 
		}
	}
	else if ( !m_wasRegistered )
	{
		RegisterCallback( true );
	}

	// failed, return false
	return false;
}

void CQCDistanceTo::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && param.Get< CName >() == m_targetNodeTag )
	{
		FindTargetNode();
	}
}

Bool CQCDistanceTo::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_targetNodeTag );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_targetNodeTag );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQCDistanceTo::FindTargetNode()
{
	CWorld* world = GGame->GetActiveWorld();
	if ( world && world->GetTagManager() )
	{
		CNode* cachedNode = world->GetTagManager()->GetTaggedNode( m_targetNodeTag );
		if ( cachedNode != nullptr )
		{
			m_cachedNode = cachedNode;
			return;
		}
	}
	m_cachedNode = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
