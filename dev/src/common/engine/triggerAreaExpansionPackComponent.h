/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "triggerAreaComponent.h"

class CTriggerAreaExpansionPackComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CTriggerAreaExpansionPackComponent, CTriggerAreaComponent, 0 );

public:
	CTriggerAreaExpansionPackComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnTick( Float timeDelta );

protected:
	// Tag of node where player will be teleported
	CName m_targetTag;

	// Seconds to deactivate this trigger
	Int32 m_deactivateTime;

	// ITriggerCallback interface
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator );

private:
	GameTime m_attachTime;

	void Deactivate();
	Int32 GetSecondsSinceAttach() const;
	CEntity* GetPlayerToTeleport( const class ITriggerActivator* activator );
	void TeleportPlayerToTargetNode( CEntity* entity ) const;
};

BEGIN_CLASS_RTTI( CTriggerAreaExpansionPackComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_targetTag, TXT("Player will be teleported to the object specified by this tag if he's standing in this trigger after loading game") );
	PROPERTY_EDIT( m_deactivateTime, TXT("Seconds to deactivate this trigger") );
END_CLASS_RTTI();