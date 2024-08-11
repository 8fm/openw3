#pragma once

#include "selfUpdatingComponent.h"

class CEntityUpdaterComponent : public CSelfUpdatingComponent
{
	DECLARE_ENGINE_CLASS( CEntityUpdaterComponent, CSelfUpdatingComponent, 0 );

protected:
	virtual void CustomTick( Float timeDelta )	override;
	virtual void DecideIfItHasToTickScript()	override;
};


BEGIN_CLASS_RTTI( CEntityUpdaterComponent );
	PARENT_CLASS( CSelfUpdatingComponent );
END_CLASS_RTTI();