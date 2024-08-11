/**
   * Copyright © 2010 CD Projekt Red. All Rights Reserved.
   */
#pragma once

#if 0
/// Special area to control camera
class CCameraAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CCameraAreaComponent, CTriggerAreaComponent, 0 );

private:
	Bool		m_isPlayerInside;			//!< Is the player inside the trigger ?

protected:
	CName		m_enterEvent;				//!< Event to send when player enters the area
	CName		m_exitEvent;				//!< Event to send when player exits the area
	CName		m_tickEvent;				//!< Event to send each frame while player is in the area

public:
	CCameraAreaComponent();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

	// Timer
	virtual void OnTickPrePhysics( Float timeDelta );

public:
	// Something have entered zone
	virtual void EnteredArea( CComponent* component );

	// Something have exited zone
	virtual void ExitedArea( CComponent* component );
};

BEGIN_CLASS_RTTI( CCameraAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_enterEvent, TXT("Event to send when player enters the area") );
	PROPERTY_EDIT( m_exitEvent, TXT("Event to send when player exits the area") );
	PROPERTY_EDIT( m_tickEvent, TXT("Event to send each frame while player is in the area") );
END_CLASS_RTTI();
#endif