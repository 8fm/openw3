/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/performableAction.h"

enum EEntityTargetingActionMechanism
{
	ETAM_Self,
	ETAM_ByHandle,
	ETAM_Activator
};

BEGIN_ENUM_RTTI( EEntityTargetingActionMechanism )
	ENUM_OPTION( ETAM_Self );
	ENUM_OPTION( ETAM_ByHandle );
	ENUM_OPTION( ETAM_Activator );
END_ENUM_RTTI()

class IEntityTargetingAction : public IPerformableAction
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEntityTargetingAction, IPerformableAction );
private:
	EEntityTargetingActionMechanism		m_entitySelectionType;
	EntityHandle						m_entityHandle;
protected:
	CEntity*							GetTarget( CEntity* owner, CNode* activator = NULL );

	virtual void						PerformOnEntity( CEntity* entity ) = 0;
public:
	IEntityTargetingAction( EEntityTargetingActionMechanism defaultSelection = ETAM_ByHandle )
		: m_entitySelectionType( defaultSelection )													{}

	void Perform( CEntity* parent ) override;
	void Perform( CEntity* parent, CNode* node ) override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IEntityTargetingAction )
	PARENT_CLASS( IPerformableAction )
	PROPERTY_EDIT( m_entitySelectionType, TXT("Entity selection mechanism.") )
	PROPERTY_EDIT( m_entityHandle, TXT("Entity directly targeted. Used with 'ETAM_ByHandle' selection mechanism.") )
END_CLASS_RTTI();

class IComponentTargetingAction : public IEntityTargetingAction
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IComponentTargetingAction, IEntityTargetingAction );
private:
	String							m_componentName;

	void							PerformOnEntity( CEntity* entity ) override;
protected:
	virtual void					PerformOnComponent( CComponent* component ) = 0;
	virtual CClass*					SupportedComponentClass();
};

BEGIN_ABSTRACT_CLASS_RTTI( IComponentTargetingAction )
	PARENT_CLASS( IEntityTargetingAction )
	PROPERTY_EDIT( m_componentName, TXT("Name of component on which to perform action. Leave empty for 'all'.") )
END_CLASS_RTTI();