
#include "build.h"

#include "scriptedStatsModyfier.h"
#include "statsContainerComponent.h"

IMPLEMENT_ENGINE_CLASS( CScriptedStatsModyfier );

void CScriptedStatsModyfier::ApplyChanges( CStatsContainerComponent* statsContainer, CEntity* ownerEnt )
{
	THandle< CStatsContainerComponent > statsContainerHandle	= statsContainer;
	THandle< CEntity >					ownerEntHandle			= ownerEnt;

	CallFunction( this, CNAME( Export_ApplyChanges ), statsContainerHandle, ownerEntHandle );
}
