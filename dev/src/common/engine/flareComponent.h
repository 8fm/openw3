/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "flareParameters.h"
#include "drawableComponent.h"


/// Component rendering flare
class CFlareComponent : public CDrawableComponent
{
	DECLARE_ENGINE_CLASS( CFlareComponent, CDrawableComponent, 0 )

protected:
	THandle< CMaterialInstance >	m_material;
	SFlareParameters				m_parameters;

public:
	CFlareComponent();

	virtual Bool ShouldPerformProxyAttachesOnDissolve() const override { return false; }

	//! Get material
	CMaterialInstance* GetMaterial() const { return m_material.Get(); }

	//! Set material
	void SetMaterial( CMaterialInstance* material ) { m_material = material; }

	//! Get parameters
	const SFlareParameters& GetParameters() const { return m_parameters; }

	//! Set parameters
	void SetFlareParameters(const SFlareParameters& parameters) { m_parameters = parameters; }

public:
	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Update world space bounding box
	virtual void OnUpdateBounds();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world
	virtual void OnDetached( CWorld* world );

	// Get flare radius
	Float GetFlareRadius() const { return m_parameters.m_flareRadius; }

	// Set flare radius
	void SetFlareRadius( Float flareRadius ) { m_parameters.m_flareRadius = flareRadius; }

	virtual Uint32 GetMinimumStreamingDistance() const;

public:
	//! Get value of effect parameter
	virtual Bool GetEffectParameterValue( CName paramName, EffectParameterValue &value /* out */ ) const;

	//! Set value of effect parameter
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue &value );

	//! Enumerate list of effect parameters
	virtual void EnumEffectParameters( CFXParameters &effectParams /* out */ );

protected:
#ifndef NO_DATA_VALIDATION
	// Can be called either for node on the level or for node in the template
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

#ifndef NO_RESOURCE_USAGE_INFO
	//! Resource usage reporting
	virtual void CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const override;
#endif
};

BEGIN_CLASS_RTTI( CFlareComponent );
	PARENT_CLASS( CDrawableComponent );	
	PROPERTY_INLINED( m_material,	TXT("Material override.") );
	PROPERTY_EDIT( m_parameters,		TXT("Parameters") );
END_CLASS_RTTI();
