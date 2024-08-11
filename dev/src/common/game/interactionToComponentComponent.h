#pragma once

#include "interactionComponent.h"

class CInteractionToComponentComponent : public CInteractionComponent
{
	DECLARE_ENGINE_CLASS( CInteractionToComponentComponent, CInteractionComponent, 0 );

protected:
	String					m_targetComponentName;
	THandle< CComponent >	m_listeningComponent;

protected:
	//! Interaction was activated by entity
	virtual void OnActivate( CEntity* activator );	

	//! Interaction was deactivated by entity
	virtual void OnDeactivate( CEntity* activator );

	//! Called when interaction is executed
	virtual void OnExecute();

	//! Get listening component
	CComponent* GetListeningComponent();

	//! Find listening component
	void FindListeningComponent();
};


BEGIN_CLASS_RTTI( CInteractionToComponentComponent )
	PARENT_CLASS( CInteractionComponent )
	PROPERTY_EDIT( m_targetComponentName, TXT("Component to which event is send.") );
END_CLASS_RTTI();
