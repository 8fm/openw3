#include "build.h"
#include "fxTrackItemSetDissolve.h"
#include "..\..\entity.h"

IMPLEMENT_ENGINE_CLASS( CFXTrackItemSetDissolve );

//////////////////////////////////////////////////////////////
CFXTrackItemSetDissolve::CFXTrackItemSetDissolve()
	: m_DisableAllDissolves( true )
{
	m_timeDuration = 0.0f;
}

//////////////////////////////////////////////////////////////
IFXTrackItemPlayData* CFXTrackItemSetDissolve::OnStart( CFXState& fxState ) const 
{
	fxState.GetEntity()->SetDisableAllDissolves( m_DisableAllDissolves );
	return nullptr;
}