#include "build.h"
#include "questCameraFocusCondition.h"
#include "../engine/tagManager.h"

IMPLEMENT_ENGINE_CLASS( CQuestCameraFocusCondition );
IMPLEMENT_RTTI_ENUM( ECameraFocusConditionLineOfSightSource );

CQuestCameraFocusCondition::CQuestCameraFocusCondition()
	: m_angleTolerance( 30.0f )
	, m_isLookingAtNode( true )
	, m_targetNode( nullptr )
	, m_targetEntity( nullptr )
	, m_testLineOfSight( false )
	, m_lineOfSightSource( CFCLOS_Camera )
	, m_wasRegistered( false )
{
}

CQuestCameraFocusCondition::~CQuestCameraFocusCondition()
{
	RegisterCallback( false );
}

void CQuestCameraFocusCondition::OnActivate()
{
	m_angleCos = MCos( DEG2RAD( m_angleTolerance ) );
	RegisterCallback( true );
	FindNode();
}

void CQuestCameraFocusCondition::OnDeactivate()
{
	RegisterCallback( false );
	m_targetNode = nullptr;
	m_targetEntity = nullptr;
}

Bool CQuestCameraFocusCondition::OnIsFulfilled()
{
	if ( !m_wasRegistered )
	{
		RegisterCallback( true );
	}

	if ( m_targetNode.Get() == nullptr )
	{
		return false;
	}

	const Vector camForward = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraForward();

	// calculate the direction to the target node
	const Vector& playerPos = GCommonGame->GetPlayer()->GetWorldPositionRef();
	const Vector& targetNodePos = m_targetNode.Get()->GetWorldPositionRef();
	Vector dirToNode = ( targetNodePos - playerPos ).Normalized3();

	// calculate the dot product between the two vectors
	Float dot = camForward.Dot3( dirToNode );
	Bool isLookingAtNode = dot >= m_angleCos;

	// check line of sight if needed
	if ( isLookingAtNode && m_testLineOfSight )
	{
		static TDynArray< const CEntity* > ignoreEntities( 1 );
		ignoreEntities[ 0 ] = m_targetEntity.Get();
		Vector sourcePosition;
		if ( m_lineOfSightSource == CFCLOS_Camera )
		{
			sourcePosition = GGame->GetActiveWorld()->GetCameraDirector()->GetCameraPosition();
		}
		else // if m_lineOfSightSource == CFCLOS_Player
		{
			sourcePosition = GCommonGame->GetPlayer()->GetHeadPosition();
		}
		if ( m_targetNode->IsA< CActor >() )
		{
			isLookingAtNode = GGame->GetActiveWorld()->TestLineOfSight( sourcePosition,	static_cast< CActor* >( m_targetNode.Get() )->GetHeadPosition(), &ignoreEntities );
		}
		else
		{
			isLookingAtNode = GGame->GetActiveWorld()->TestLineOfSight( sourcePosition,	m_targetNode.Get(), true, &ignoreEntities );
		}
	}

	return isLookingAtNode == m_isLookingAtNode;
}

void CQuestCameraFocusCondition::OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param )
{
	if ( eventCategory == GEC_Tag && param.Get< CName >() == m_nodeTag )
	{
		FindNode();
	}
}

Bool CQuestCameraFocusCondition::RegisterCallback( Bool reg )
{
	if ( reg != m_wasRegistered && GGame != nullptr && GGame->GetGlobalEventsManager() != nullptr )
	{
		if ( reg )
		{
			GGame->GetGlobalEventsManager()->AddFilteredListener( GEC_Tag, this, m_nodeTag );
		}
		else
		{
			GGame->GetGlobalEventsManager()->RemoveFilteredListener( GEC_Tag, this, m_nodeTag );
		}
		m_wasRegistered = reg;
		return true;
	}
	return false;
}

void CQuestCameraFocusCondition::FindNode()
{
	if ( GGame != nullptr && GGame->GetActiveWorld() != nullptr )
	{
		m_targetNode = GGame->GetActiveWorld()->GetTagManager()->GetTaggedNode( m_nodeTag );
		m_targetEntity = m_targetNode.IsValid() ? Cast< CEntity >( m_targetNode.Get() ) : nullptr;
	}
	else
	{
		m_targetNode = nullptr;
		m_targetEntity = nullptr;
	}
}