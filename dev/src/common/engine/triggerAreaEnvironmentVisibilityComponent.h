/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "triggerAreaComponent.h"

class CTriggerAreaEnvironmentVisibilityComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CTriggerAreaEnvironmentVisibilityComponent, CTriggerAreaComponent, 0 );

public:
	CTriggerAreaEnvironmentVisibilityComponent();

	virtual void OnAttached( CWorld* world );

	virtual void OnDetached( CWorld* world );

	virtual void OnTick( Float timeDelta );

protected:

	// ITriggerCallback interface
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator );

	virtual void OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator );
private:

	Bool	m_hideTerrain;	//! Hide terrain when within trigger area
	Bool	m_hideFoliage;	//! Hide foliage when within trigger area
	Bool	m_hideWater;	//! Hide water when within trigger area

	CWorld* m_world;		//! Cache world trigger is attached to

	void HideEnvironmentElements();
	void ShowEnvironmentElements();
};

BEGIN_CLASS_RTTI( CTriggerAreaEnvironmentVisibilityComponent );
PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_hideTerrain, TXT("Wether to hide terrain when within the trigger area.") );
	PROPERTY_EDIT( m_hideFoliage, TXT("Wether to hide foliage when within the trigger area.") );
	PROPERTY_EDIT( m_hideWater, TXT("Wether to hide water when within the trigger area.") );
END_CLASS_RTTI();