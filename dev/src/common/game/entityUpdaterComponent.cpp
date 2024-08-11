#include "build.h"

#include "entityUpdaterComponent.h"

IMPLEMENT_ENGINE_CLASS( CEntityUpdaterComponent );


void CEntityUpdaterComponent::CustomTick( Float deltaTime )
{
	if( m_scriptNeedsTicking )
	{
		if( GGame->IsActive() )
		{
			GetEntity()->CallEvent( CNAME( OnComponentTick ), deltaTime );
		}
	}
}

void CEntityUpdaterComponent::DecideIfItHasToTickScript()
{
	// If we find the function, it means there is a script deriving from thins and we need to tick it later
	const CFunction* function	= NULL;
	IScriptable* context = GetEntity();
	m_scriptNeedsTicking = ::FindFunction( context, CNAME( OnComponentTick ), function );
}