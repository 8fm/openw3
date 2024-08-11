#include "build.h"
#include "questUserInputCondition.h"
#include "questsSystem.h"

IMPLEMENT_RTTI_ENUM( EInputCompareFunc );
IMPLEMENT_ENGINE_CLASS( CQuestInputCondition )

CQuestInputCondition::CQuestInputCondition()
	: m_valueCompareFunc( ICF_DontCare )
	, m_value( 0.0f )
	, m_isFulfilled( false )
{
}

void CQuestInputCondition::OnActivate()
{
	m_isFulfilled = false;
	if ( GCommonGame && GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->AttachGameInputListener( *this );
	}
}

void CQuestInputCondition::OnDeactivate()
{
	if ( GCommonGame && GCommonGame->GetSystem< CQuestsSystem >() )
	{
		GCommonGame->GetSystem< CQuestsSystem >()->DetachGameInputListener( *this );
	}
}

void CQuestInputCondition::OnEvent( const CName& event, Float value )
{
	if ( m_isFulfilled || m_gameInput != event )
	{
		return;
	}

	switch( m_valueCompareFunc )
	{
	case ICF_Less:							m_isFulfilled = ( value <= m_value ); break;
	case ICF_Greater:						m_isFulfilled = ( value >= m_value ); break;
	default: case ICF_DontCare:				m_isFulfilled = true; break;
	}
}

Bool CQuestInputCondition::OnIsFulfilled()
{
	return m_isFulfilled;
}
