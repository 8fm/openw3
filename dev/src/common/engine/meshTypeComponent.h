/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "drawableComponent.h"
#include "skeletonConsumer.h"
#include "meshEnum.h"
#include "meshRenderSettings.h"

class CMeshTypeResource;

// Base for mesh-based components. Provides some basic things for forced LODs, coloring, etc.
class CMeshTypeComponent	: public CDrawableComponent
							, public ISkeletonDataConsumer
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMeshTypeComponent, CDrawableComponent );

protected:
	Int32						m_forceLODLevel;						//!< Force LOD on mesh ( -1 means no LOD is forced )
	Uint16						m_forceAutoHideDistance;				//!< Force autohide distance on Mesh
	EMeshShadowImportanceBias	m_shadowImportanceBias;					//!< Shadow importance bias

	// TODO DREY figure out a better place for these
	Vector						m_defaultEffectParams;					//!< Custom effect parameters to be used by effect editor
	Color						m_defaultEffectColor;					//!< Custom effect color to be used by effect editor

public:

	CMeshTypeComponent();
	virtual ~CMeshTypeComponent();

	// Get a CMeshTypeResource that is somehow "represented" by this component.
	virtual CMeshTypeResource* GetMeshTypeResource() const = 0;

	// Get shadow importance bias
	RED_FORCE_INLINE EMeshShadowImportanceBias GetShadowImportanceBias() const { return m_shadowImportanceBias; }


	virtual const ISkeletonDataConsumer* QuerySkeletonDataConsumer() const { return this; }

	// Generate editor fragments
	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

	// Color variant of parent entity has changed
	virtual void OnColorVariantChanged();

	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );
	// Called when component is detached from world
	virtual void OnDetached( CWorld* world );

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnInitializeProxy();

	virtual Bool IsDynamicGeometryComponent() const { return IsSkinned() || TBaseClass::IsDynamicGeometryComponent(); }

	// Should we update transform this node automatically when parent is update transformed /
	virtual Bool UsesAutoUpdateTransform();

	// Update world space bounding box based on the mesh returned by GetMesh.
	virtual void OnUpdateBounds();

	// Explicitly set bounding box. If this is skinned, the bounding box will not be updated automatically in OnUpdateBounds.
	RED_FORCE_INLINE void SetBoundingBox( const Box& b ) { m_boundingBox = b; }

	virtual Float GetAutoHideDistance() const;
	virtual Float GetDefaultAutohideDistance() const { return 30.0f; }

	virtual CMesh*	TryGetMesh() const { return nullptr; }

	// Is this mesh skinned
	Bool IsSkinned() const;

	// skinning update
	virtual void OnUpdateSkinning( const ISkeletonDataProvider* provider, IRenderSkinningData* renderSkinningData, const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext );
	virtual void OnUpdateTransformWithoutSkinning( const Box& boxMS, const Matrix& l2w, SMeshSkinningUpdateContext& skinningContext );

	//! Get forced LOD level
	RED_FORCE_INLINE Int32 GetForcedLODLevel() const { return m_forceLODLevel; }

	// Force LOD level on this mesh, -1 to disable forced LOD
	void ForceLODLevel( Int32 forceLODLevel );

	Bool HasForcedAutoHideDistance() const;
	void ForceAutoHideDistance( Uint16 forceAutoHideDistance );

	RED_FORCE_INLINE const Vector& GetDefaultEffectParams() const { return m_defaultEffectParams; }
	RED_FORCE_INLINE const Color& GetDefaultEffectColor() const { return m_defaultEffectColor; }

	virtual Bool GetEffectParameterValue( CName paramName, EffectParameterValue& outValue ) const;
	virtual Bool SetEffectParameterValue( CName paramName, const EffectParameterValue& value );

	//! Enumerate list of effect parameters
	virtual void EnumEffectParameters( CFXParameters& outEffectParams );

	void SendColorShiftMatrices( const Matrix& region0, const Matrix& region1 );

	// Appearance changed
	void OnAppearanceChanged( Bool added ) override;

	// Created from streaming
	void OnStreamIn() override;

	virtual Uint32 GetMinimumStreamingDistance() const override;

#ifndef NO_EDITOR
	void UpdateRenderDistanceParams( const SMeshRenderParams& params );
#endif

#ifndef NO_DATA_VALIDATION
	// Check data
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

	//! Is component ready to be rendered?
	virtual Bool IsRenderingReady() const override;

protected:
	//! Apply coloring to mesh, calculates coloring matrices and sends them to rendering thread
	void ApplyMeshColoring();
};


BEGIN_ABSTRACT_CLASS_RTTI( CMeshTypeComponent );
PARENT_CLASS( CDrawableComponent );
PROPERTY_EDIT( m_forceLODLevel, TXT("Forced LOD level for this mesh") );
PROPERTY_EDIT( m_forceAutoHideDistance, TXT("Forced autohide distance for this mesh") );
PROPERTY_EDIT( m_shadowImportanceBias, TXT("Shadow importance bias") );
PROPERTY_EDIT( m_defaultEffectParams, TXT("Default value for effect params") );
PROPERTY_EDIT( m_defaultEffectColor, TXT("Default value for effect color") );
END_CLASS_RTTI();
