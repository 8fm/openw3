#include "build.h"
#include "questGameplayEntCondition.h"

IMPLEMENT_ENGINE_CLASS( IGameplayEntConditionType )
IMPLEMENT_ENGINE_CLASS( CQuestGameplayEntCondition )

CQuestGameplayEntCondition::CQuestGameplayEntCondition()
	: m_checkType( nullptr )
	, m_entityWatcher( nullptr )
{
}

CQuestGameplayEntCondition::~CQuestGameplayEntCondition()
{
	delete m_entityWatcher;
}

void CQuestGameplayEntCondition::OnActivate()
{
	TBaseClass::OnActivate();

	if ( m_entityWatcher == nullptr )
	{
		m_entityWatcher = new TGameplayEnityWatcher( m_entityTag );
	}
	EEntityWatcherStateChange state = m_entityWatcher->Enable( true );
	if ( state == EWST_EntityFound && m_checkType != nullptr )
	{
		CGameplayEntity* entity = m_entityWatcher->Get();
		if ( entity != nullptr )
		{
			m_checkType->OnActivate( *entity );
		}
	}
}

void CQuestGameplayEntCondition::OnDeactivate()
{
	if ( m_checkType != nullptr )
	{
		CGameplayEntity* entity = m_entityWatcher->Get();
		if ( entity != nullptr )
		{
			m_checkType->OnDeactivate( *entity );
		}
	}
	m_entityWatcher->Enable( false );

	TBaseClass::OnDeactivate();
}

Bool CQuestGameplayEntCondition::OnIsFulfilled()
{
	if ( m_checkType == nullptr || m_entityTag == CName::NONE )
	{
		return false;
	}

	CGameplayEntity* entity = nullptr;
	EEntityWatcherStateChange state = m_entityWatcher->Update();
	if ( state == EWST_EntityFound )
	{
		entity = m_entityWatcher->Get();
		if ( entity != nullptr )
		{
			m_checkType->OnActivate( *entity );
		}
	}
	else if ( state == EWST_EntityLost )
	{
		m_checkType->OnEntityLost();
	}
	else // state == EWST_NothingChanged
	{
		entity = m_entityWatcher->Get();
	}

	if ( entity != nullptr )
	{
		return m_checkType->Evaluate( *entity );
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCHasItemGE )

CQCHasItemGE::CQCHasItemGE()
	: IGameplayEntConditionType()
	, m_quantity( 1 )
	, m_compareFunc( CF_GreaterEqual )
	, m_wasRegistered( false )
	, m_isFulfilled( false )
{
}

void CQCHasItemGE::OnActivate( CGameplayEntity& entity )
{
	TBaseClass::OnActivate( entity );

	m_isFulfilled = false;
	m_wasRegistered = false;
	CInventoryComponent* inventory = entity.GetInventoryComponent();
	if ( inventory != nullptr )
	{
		m_isFulfilled = EvaluateImpl( inventory );
	}
	if ( !m_isFulfilled )
	{
		RegisterCallback( entity, true );
	}
}

void CQCHasItemGE::OnDeactivate( CGameplayEntity& entity )
{
	RegisterCallback( entity, false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate( entity );
}

void CQCHasItemGE::OnEntityLost()
{
	m_wasRegistered = false;
}

Bool CQCHasItemGE::Evaluate( CGameplayEntity& entity )
{
	if ( !m_isFulfilled && !m_wasRegistered )
	{
		if ( RegisterCallback( entity, true ) )
		{
			m_isFulfilled = EvaluateImpl( entity.GetInventoryComponent() );
		}
	}
	return m_isFulfilled;
}

Bool CQCHasItemGE::RegisterCallback( CGameplayEntity& entity, Bool reg )
{
	if ( reg != m_wasRegistered && entity.GetInventoryComponent() != nullptr )
	{
		if ( reg )
		{
			entity.GetInventoryComponent()->AddListener( this );
		}
		else
		{
			entity.GetInventoryComponent()->RemoveListener( this );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

Bool CQCHasItemGE::EvaluateImpl( CInventoryComponent* inventory )
{
	if ( inventory == nullptr )
	{
		return false;
	}

	Uint32 queryValue = 0;
	if ( m_item != CName::NONE )
	{
		queryValue =  inventory->GetItemQuantityByName( m_item, true );
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

void CQCHasItemGE::OnInventoryEvent( CInventoryComponent* inventory, EInventoryEventType eventType, SItemUniqueId itemId, Int32 quantity, Bool fromAssociatedInventory )
{
	m_isFulfilled = EvaluateImpl( inventory );
}

/////////////////////////////////////////////////////////////////////////////// 

IMPLEMENT_ENGINE_CLASS( CQCAnimationState )

CQCAnimationState::CQCAnimationState()
	: IGameplayEntConditionType()
	, m_isPlaying( false )
	, m_wasRegistered( false )
	, m_isFulfilled( false )
{
}

void CQCAnimationState::OnActivate( CGameplayEntity& entity )
{
	TBaseClass::OnActivate( entity );

	m_isFulfilled = false;
	m_wasRegistered = false;
	CPropertyAnimationSet* animationSet = entity.GetPropertyAnimationSet();
	if ( animationSet != nullptr )
	{
		m_isFulfilled = EvaluateImpl( animationSet );
	}
	if ( !m_isFulfilled )
	{
		RegisterCallback( entity, true );
	}
}

void CQCAnimationState::OnDeactivate( CGameplayEntity& entity )
{
	RegisterCallback( entity, false );
	m_wasRegistered = false;

	TBaseClass::OnDeactivate( entity );
}

void CQCAnimationState::OnEntityLost()
{
	m_wasRegistered = false;
}

Bool CQCAnimationState::Evaluate( CGameplayEntity& entity )
{
	if ( !m_isFulfilled && !m_wasRegistered )
	{
		if ( RegisterCallback( entity, true ) )
		{
			m_isFulfilled = EvaluateImpl( entity.GetPropertyAnimationSet() );
		}
	}
	return m_isFulfilled;
}

Bool CQCAnimationState::RegisterCallback( CGameplayEntity& entity, Bool reg )
{
	if ( reg != m_wasRegistered && entity.GetPropertyAnimationSet() != nullptr )
	{
		if ( reg )
		{
			entity.GetPropertyAnimationSet()->AddListener( this );
		}
		else
		{
			entity.GetPropertyAnimationSet()->RemoveListener( this );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

Bool CQCAnimationState::EvaluateImpl( CPropertyAnimationSet* animationSet )
{
	if ( animationSet == nullptr )
	{
		return false;
	}
	return m_isPlaying == animationSet->IsPlaying( m_animationName );
}

void CQCAnimationState::OnStateChanged( CPropertyAnimationSet* animationSet )
{
	m_isFulfilled = EvaluateImpl( animationSet );
}
