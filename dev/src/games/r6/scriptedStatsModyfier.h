#pragma once

#include "build.h"

#include "baseUpgradeStatsModyfier.h"

class CScriptedStatsModyfier : public CBaseUpgradeStatsModyfier
{
	DECLARE_ENGINE_CLASS( CScriptedStatsModyfier, CBaseUpgradeStatsModyfier, 0 );
public:
	void ApplyChanges( CStatsContainerComponent* statsContainer, CEntity* ownerEnt ) override;	
};

BEGIN_CLASS_RTTI( CScriptedStatsModyfier )	
	PARENT_CLASS( CBaseUpgradeStatsModyfier );
END_CLASS_RTTI();
