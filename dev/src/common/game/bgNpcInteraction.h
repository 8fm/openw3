
#pragma once

#include "../../common/game/interactionComponent.h"

class CBgInteractionComponent : public CInteractionComponent
{
	DECLARE_ENGINE_CLASS( CBgInteractionComponent, CInteractionComponent, 0 );

public:
	CBgInteractionComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld *world );

	virtual void SetEnabled( Bool enabled );

	virtual void OnPostLoad();

protected:
	virtual void OnExecute();

	virtual Bool ActivationTest( CEntity* activator, const SActivatorData& activatorData ) const override;

	virtual void OnActivate( CEntity* activator );
	virtual void OnDeactivate( CEntity* activator );

#ifndef NO_EDITOR
public:
	void InitializeComponent();
#endif
};

BEGIN_CLASS_RTTI( CBgInteractionComponent )
	PARENT_CLASS( CInteractionComponent )
END_CLASS_RTTI();
