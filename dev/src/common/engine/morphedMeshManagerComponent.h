/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once
#include "component.h"


class CMorphedMeshManagerComponent : public CComponent
{
	DECLARE_ENGINE_CLASS( CMorphedMeshManagerComponent, CComponent, 0 )

protected:
	Float											m_defaultMorphRatio;
	THandle< CCurve >								m_morphCurve;

protected:
	Float											m_morphRatio;

	Float											m_blendingTo;
	Float											m_blendTimeLeft;
	Float											m_blendTime;

	Bool											m_addedToTickGroups;

	THashSet< THandle< CMorphedMeshComponent > >	m_morphedMeshComponents;		// Cached list of morph components from the entity.

private:
#ifndef NO_EDITOR
	Bool								m_testBlend;
#endif

public:
	void SetMorphRatio( Float morphRatio, Bool force = false );
	void SetMorphBlend( Float morphTarget, Float blendTime );

	RED_FORCE_INLINE Float GetMorphRatio() const { return m_morphRatio; }

protected:
	void InternalSetMorphRatio( Float morphRatio, Bool force = false );

public:
	CMorphedMeshManagerComponent();

	// Object serialization interface
	virtual void OnSerialize( IFile& file ) override;

	virtual void OnPostLoad() override;

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property ) override;

	// Called when component is spawned ( usually called in entity template editor )
	virtual void OnSpawned( const SComponentSpawnInfo& spawnInfo ) override;

	// Component update, update morph ratio
	virtual void OnTick( Float timeDelta ) override;

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world ) override;


	void OnMorphedMeshAttached( CMorphedMeshComponent* component );
	void OnMorphedMeshDetached( CMorphedMeshComponent* component );

	virtual bool UsesAutoUpdateTransform() override { return false; }

private:
	void ActivateInTickGroups();
	void DeactivateInTickGroups();

	void CreateMorphCurveIfNonExistent();

private:
	void funcSetMorphBlend( CScriptStackFrame& stack, void* result );	
	void funcGetMorphBlend( CScriptStackFrame& stack, void* result );	

};

BEGIN_CLASS_RTTI( CMorphedMeshManagerComponent );
	PARENT_CLASS( CComponent );
	PROPERTY_EDIT_NAME( m_defaultMorphRatio, TXT("Default morph ratio"), TXT("Accepts only values 0 or 1") );
	PROPERTY_CUSTOM_EDIT( m_morphCurve, TXT("Morph curve"), TXT("CurveSelection") );
	PROPERTY_CUSTOM_EDIT_RANGE( m_morphRatio, TXT("morph ratio. For preview only, will not be saved."), TXT("Slider"), 0.0f, 1.0f );
#ifndef NO_EDITOR
	PROPERTY_EDIT( m_testBlend, TXT("Test blending morph") );
	NO_PROPERTY_SERIALIZATION( m_testBlend );
#endif
	NO_PROPERTY_SERIALIZATION( m_morphRatio );

	NATIVE_FUNCTION( "SetMorphBlend", funcSetMorphBlend );
	NATIVE_FUNCTION( "GetMorphBlend", funcGetMorphBlend );
END_CLASS_RTTI();
