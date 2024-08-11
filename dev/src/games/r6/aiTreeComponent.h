/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "r6aiSystem.h"
#include "r6behTreeInstance.h"

class CAITreeComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CAITreeComponent, CComponent, 0 )

protected:
	// editable properties
	THandle< CBehTree >			m_resource;
	TDynArray< IAIParameters* >	m_parameters;
	EAITickPriorityGroup		m_priorityGroup;

	// runtime data
	CR6BehTreeInstance*			m_instance;
	TDynArray< CComponent* >	m_attachedPerformers;

public:
	CAITreeComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	void OnWorldEnd();

	void Update( Float timeSinceLastUpdate, Float thisFrameTimeDelta );

	EAITickPriorityGroup GetCurrentPriorityGroup() const;

	virtual void OnChildAttachmentAdded( IAttachment* attachment );
	virtual void OnChildAttachmentBroken( IAttachment* attachment );

	RED_INLINE const TDynArray< CComponent* >& GetPerformers() const { return m_attachedPerformers; } 

protected:
	void DestroyInstance();
};

BEGIN_CLASS_RTTI( CAITreeComponent )
	PARENT_CLASS( CComponent )
	PROPERTY_EDIT( m_resource, TXT("Base AI tree for this component to evaluate.") )
	PROPERTY_EDIT( m_parameters, TXT("AI parameters.") )
	PROPERTY_EDIT( m_priorityGroup, TXT("Defines how often this component will recive ticks by default (impacts performance).") )
	PROPERTY( m_instance )
	PROPERTY( m_attachedPerformers )
END_CLASS_RTTI()
