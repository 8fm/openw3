#include "build.h"
#include "entityParams.h"
#include "attackRange.h"

IMPLEMENT_ENGINE_CLASS( CCharacterStatsParam );
IMPLEMENT_ENGINE_CLASS( CAutoEffectsParam );
IMPLEMENT_ENGINE_CLASS( CAttackableArea );
IMPLEMENT_ENGINE_CLASS( CPlayerTargetPriority );
IMPLEMENT_ENGINE_CLASS( CAlternativeDisplayName );
IMPLEMENT_ENGINE_CLASS( CBloodTrailEffect );

//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CAttackRangeParam );

void CAttackRangeParam::OnPostLoad()
{
	for( Int32 i = m_attackRanges.Size() - 1; i >= 0; --i  )
	{
		if ( m_attackRanges[ i ] == nullptr )
		{
			m_attackRanges.RemoveAt( i );
		}
	}

	TBaseClass::OnPostLoad();
}

const CAIAttackRange* CAttackRangeParam::GetAttackRange( const CName& attackRangeName /*= CName::NONE */ ) const
{
	if ( m_attackRanges.Empty() == true )
	{
		return NULL;
	}

	if ( attackRangeName == CName::NONE )
	{
		return m_attackRanges[ 0 ];
	}

	for( TDynArray< CAIAttackRange* >::const_iterator rangeIter = m_attackRanges.Begin(); rangeIter != m_attackRanges.End(); ++rangeIter )
	{
		const CAIAttackRange* attackRange = *rangeIter;
		if ( attackRange->m_name == attackRangeName )
		{
			return attackRange;
		}
	}
	return NULL;
}

void CBloodTrailEffect::funcGetEffectName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_NAME( m_effect );
}