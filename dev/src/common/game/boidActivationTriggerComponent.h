#pragma once


#include "../engine/triggerAreaComponent.h"

/////////////////////////////////////////////////////////////////////////
class CBoidActivationTriggerComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CBoidActivationTriggerComponent, CTriggerAreaComponent, 0 );
protected:
	EntityHandle						m_lair;

	Bool								m_handleEnter;
	Bool								m_handleExit;

public:
	CBoidActivationTriggerComponent();

	void EnteredArea( CComponent* component ) override;
	void ExitedArea( CComponent* component ) override;

	void OnAttachFinished( CWorld* world ) override;
	void OnDetached( CWorld* world ) override;
	void OnPostLoad()override;

	void SetupFromTool( CEntity * lair ){ m_lair.Set( lair ); }
};

BEGIN_CLASS_RTTI( CBoidActivationTriggerComponent )
	PARENT_CLASS( CTriggerAreaComponent )
	PROPERTY_EDIT( m_lair, TXT("Lair to active") )
END_CLASS_RTTI()

