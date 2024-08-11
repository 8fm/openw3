#pragma once 



class CStatsContainerComponent;

class CBaseUpgradeStatsModyfier : public CObject
{
	DECLARE_ENGINE_CLASS( CBaseUpgradeStatsModyfier, CObject, 0 );

public:
	virtual void ApplyChanges( CStatsContainerComponent* statsContainer, CEntity* ownerEnt );
};

BEGIN_CLASS_RTTI( CBaseUpgradeStatsModyfier )	
		PARENT_CLASS( CObject );
END_CLASS_RTTI();