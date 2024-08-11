#pragma once

#include "questCondition.h"
#include "../engine/globalEventsManager.h"

///////////////////////////////////////////////////////////////////////////////

enum ECameraFocusConditionLineOfSightSource
{
	CFCLOS_Camera,
	CFCLOS_Player,
};

BEGIN_ENUM_RTTI( ECameraFocusConditionLineOfSightSource )
	ENUM_OPTION( CFCLOS_Camera );
	ENUM_OPTION( CFCLOS_Player );
END_ENUM_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CQuestCameraFocusCondition : public IQuestCondition, IGlobalEventsListener
{
	DECLARE_ENGINE_CLASS( CQuestCameraFocusCondition, IQuestCondition, 0 )

private:
	CName					m_nodeTag;
	Float					m_angleTolerance;
	Bool					m_isLookingAtNode;
	Bool					m_testLineOfSight;
	ECameraFocusConditionLineOfSightSource	m_lineOfSightSource;
	
	THandle< CNode >		m_targetNode;
	THandle< CEntity >		m_targetEntity;
	Float					m_angleCos;
	Bool					m_wasRegistered;

public:
	CQuestCameraFocusCondition();
	virtual ~CQuestCameraFocusCondition();

#ifndef NO_EDITOR_PROPERTY_SUPPORT
	virtual String GetDescription() const 
	{ 
		return String::Printf( TXT( "Node tag: %s, Angle tolerance: %f" ), m_nodeTag.AsString().AsChar(), m_angleTolerance ); 
	}
#endif

protected:
	//! IQuestCondition implementation
	virtual void OnActivate() override;
	virtual void OnDeactivate() override;
	virtual Bool OnIsFulfilled() override;

	//! IGlobalEventManager
	void OnGlobalEvent( EGlobalEventCategory eventCategory, EGlobalEventType eventType, const SGlobalEventParam& param ) override;
	
	Bool RegisterCallback( Bool reg );
	void FindNode();
};

BEGIN_CLASS_RTTI( CQuestCameraFocusCondition )
	PARENT_CLASS( IQuestCondition )
	PROPERTY_EDIT( m_nodeTag, TXT( "Tag of a node we want to focus camera on." ) )
	PROPERTY_EDIT( m_angleTolerance, TXT( "Look direction range angle." ) )
	PROPERTY_EDIT( m_isLookingAtNode, TXT("Flag whether player should be looking at node") )
	PROPERTY_EDIT( m_testLineOfSight, TXT("Use line of sight test") )
	PROPERTY_EDIT( m_lineOfSightSource, TXT("Source position for line of sight test") )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////
