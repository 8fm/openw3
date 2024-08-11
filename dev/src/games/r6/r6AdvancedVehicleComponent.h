
#pragma once

#include "../../common/game/advancedVehicle.h"


/// @todo MS: if this class is not filled with anything after making test AV, remove it.
class CR6AdvancedVehicleComponent : public CAdvancedVehicleComponent
{
	DECLARE_ENGINE_CLASS( CR6AdvancedVehicleComponent, CAdvancedVehicleComponent, 0 );

public:
	virtual void OnPilotMounted	( CPilotComponent* pilot ) override;
};


BEGIN_CLASS_RTTI( CR6AdvancedVehicleComponent );
	PARENT_CLASS( CAdvancedVehicleComponent );
END_CLASS_RTTI();
