/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "triggerAreaComponent.h"

/// Trigger component with optional beveling
class CSoftTriggerAreaComponent : public CTriggerAreaComponent
{
	DECLARE_ENGINE_CLASS( CSoftTriggerAreaComponent, CTriggerAreaComponent, 0 );

private:
	//! Special drawing mesh for beveled geometry
	IRenderResource*		m_compiledBevelDebugMesh;

	//! Outer area trigger
	ITriggerObject*			m_outerTriggerObject;

	//! Negative areas that should be subtracted from outer trigger AFTER beveling
	TagList					m_outerClippingAreaTags;

	//! Invert the 0-1 range returned by CalcPenetrationFraction
	Bool					m_invertPenetrationFraction;

protected:
	//! Trigger channels for outer area
	Uint32					m_outerIncludedChannels;
	Uint32					m_outerExcludedChannels;

public:
	CSoftTriggerAreaComponent();

	//! Attached to world
	virtual void OnAttached( CWorld* world );

	//! Detached from world
	virtual void OnDetached( CWorld* world );

	//! Tramsformation changed
	virtual void OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld ) override;

	//! Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Compiled shape of this area has changed
	virtual void OnAreaShapeChanged();

	// Property changed
	virtual void OnPropertyPostChange( CProperty* prop );

public:
	//! Calculate beveling radius (used when inserting the trigger shape into the manager)
	virtual Float CalcBevelRadius() const;

	//! Calculate vertical beveling radius (used when inserting the trigger shape into the manager)
	virtual Float CalcVerticalBevelRadius() const;

	//! Calculate 0-1 penetration for given position
	//! 0 - outside the beveled area
	//! 1 - totally inside the beveled area
	//! inbetween - distance fraction
	const Float CalcPenetrationFraction( const Vector& point, Vector* outClosestPoint = 0 ) const;

	//! Recreate the beveled trigger shape
	void RecreateOuterShape();

public:
	//! Soft trigger area interface - Activator entered the OUTER (beveled) area of the trigger
	virtual void EnteredOuterArea( CComponent* component, const class ITriggerActivator* activator );

	//! Soft trigger area interface - Activator exited the OUTER (beveled) area of the trigger
	virtual void ExitedOuterArea( CComponent* component, const class ITriggerActivator* activator );

protected:
	// ITriggerCallback interface
	virtual void OnActivatorEntered( const class ITriggerObject* object, const class ITriggerActivator* activator );
	virtual void OnActivatorExited( const class ITriggerObject* object, const class ITriggerActivator* activator );
};

BEGIN_CLASS_RTTI( CSoftTriggerAreaComponent );
	PARENT_CLASS( CTriggerAreaComponent );
	PROPERTY_EDIT( m_outerClippingAreaTags, TXT("Negative areas that should be subtracted from outer trigger AFTER beveling") );
	PROPERTY_BITFIELD_EDIT( m_outerIncludedChannels, ETriggerChannel, TXT("Included trigger channels for outer trigger area") );
	PROPERTY_BITFIELD_EDIT( m_outerExcludedChannels, ETriggerChannel, TXT("Excluded trigger channels for outer trigger area") );
	PROPERTY_EDIT( m_invertPenetrationFraction, TXT("Invert the 0-1 range returned by CalcPenetrationFraction") );
END_CLASS_RTTI();
