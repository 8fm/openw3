/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "interactionComponent.h"

class CInteractionTooltipComponent : public CInteractionComponent
{
	DECLARE_ENGINE_CLASS( CInteractionTooltipComponent, CInteractionComponent, 0 );

public:
	CInteractionTooltipComponent();

	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld *world );

	virtual void OnActivate( CEntity* activator );	
	virtual void OnDeactivate( CEntity* activator = NULL );

	virtual Bool VisibilityTest( const Vector& activatorPos ) const;
};

DEFINE_SIMPLE_RTTI_CLASS( CInteractionTooltipComponent, CInteractionComponent );
