/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../engine/triggerAreaComponent.h"


/// Component which defines sound material area
class CStandPhysicalMaterialAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CStandPhysicalMaterialAreaComponent, CTriggerAreaComponent, 0 );

public:
	CStandPhysicalMaterialAreaComponent ();

	// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	// Something have exited zone
	virtual void ExitedArea( CComponent* component );

	//! Get contour rendering color
	virtual Color CalcLineColor() const;

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

protected:
	CName m_physicalMaterialName;

};

BEGIN_CLASS_RTTI( CStandPhysicalMaterialAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_physicalMaterialName, TXT("Physical material name") );
END_CLASS_RTTI();