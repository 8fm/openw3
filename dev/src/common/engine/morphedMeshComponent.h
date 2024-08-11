/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "meshTypeComponent.h"


class CMorphedMeshComponent : public CMeshTypeComponent
{
	DECLARE_ENGINE_CLASS( CMorphedMeshComponent, CMeshTypeComponent, 0 )

protected:
	THandle< CMesh >				m_morphSource;					//!< Mesh to morph from
	THandle< CMesh >				m_morphTarget;					//!< Mesh to morph into

	typedef TDynArray< THandle< CBitmapTexture > > TControlTextures;
	TControlTextures				m_morphControlTextures;			//!< One texture per source material, determines how the materials should dissolve/blend (dark goes source->target first)

	TDynArray< Bool >				m_useMorphBlendMaterials;		//!< One per source material. If true, that material should be blended with matching target material.

	CName							m_morphComponentId;				//!< This id will be used inisted of component string name

	Bool							m_useControlTexturesForMorph;	//!< Whether the control textures should be used for vertex interpolation.

	Float							m_morphRatio;

public:

	virtual CMeshTypeResource* GetMeshTypeResource() const override;

	void SetMorphRatio( Float morphRatio );
	RED_FORCE_INLINE Float GetMorphRatio() const { return m_morphRatio; }

	// Get morph source mesh
	RED_FORCE_INLINE CMesh* GetMorphSource() const { return m_morphSource.Get(); }

	// Get morph target mesh
	RED_FORCE_INLINE CMesh* GetMorphTarget() const { return m_morphTarget.Get(); }

	// Get attachment group for this component - it determines the order
	RED_FORCE_INLINE virtual EAttachmentGroup GetAttachGroup() const { return ATTACH_GROUP_A2; }

	// Get morph component id
	RED_FORCE_INLINE const CName& GetMorphComponentId() const { return m_morphComponentId; }

public:
	CMorphedMeshComponent();

	virtual void OnAttached( CWorld* world ) override;
	virtual void OnDetached( CWorld* world ) override;

	// Set morph source
	void SetMorphSource( CMesh* mesh );

	// Set morph target
	void SetMorphTarget( CMesh* mesh );

	// Property has changed
	virtual void OnPropertyPostChange( IProperty* property );

	// Update world space bounding box
	virtual void OnUpdateBounds();

#ifndef NO_DATA_VALIDATION
	// Check data
	virtual void OnCheckDataErrors( Bool isInTemplate ) const;
#endif // NO_DATA_VALIDATION

	Bool UseControlTexturesForMorph() const { return m_useControlTexturesForMorph; }

	CBitmapTexture* GetMorphControlTexture( Uint32 sourceMaterialIndex ) const { return sourceMaterialIndex < m_morphControlTextures.Size() ? m_morphControlTextures[ sourceMaterialIndex ].Get() : nullptr; }

	// Create an instance of a blend material for the given source/target material combination. The created instance will have its parameters
	// set up from the appropriate mesh materials, and the control texture set.
	//
	// If a blend material is not available (either the morphed mesh is not using blending for the source material, or the source/target materials
	// are incompatible with blending), this will return null.
	//
	// The caller can Discard the material when it is done with it.
	IMaterial* CreateBlendMaterialInstance( Uint32 sourceMaterialIndex, Uint32 targetMaterialIndex ) const;

private:
	// Get a material which can be used to blend between source and target meshes. Will return null if the given source material has not been
	// set to use blending, or if no compatible blend material exists.
	IMaterialDefinition* GetMorphBlendMaterial( Uint32 sourceMaterialIndex, Uint32 targetMaterialIndex ) const;

	// Given a material in the source mesh, find a matching material in the target mesh. A matching material is one such that if chunks I and J from the source mesh
	// are using the given source material, then chunks I and J in the target mesh are using the matching material. So it gives a one-to-one mapping between source
	// and target materials.
	//
	// If no matching material can be found, will return -1.
	//
	// This does not work when one or both meshes are cooked, but is only used for data validation in the editor, so that should not be a problem.
	Int32 FindMatchingTargetMaterialIdForBlend( Uint32 sourceMaterialId ) const;
};

BEGIN_CLASS_RTTI( CMorphedMeshComponent );
	PARENT_CLASS( CMeshTypeComponent );
	PROPERTY_EDIT( m_morphSource, TXT("Morph source") );
	PROPERTY_EDIT( m_morphTarget, TXT("Morph target") );
	PROPERTY_CUSTOM_EDIT( m_morphControlTextures, TXT("Morph control textures. Uses the source mesh's UV layout."), TXT("MaterialValueMapping") );
	PROPERTY_CUSTOM_EDIT( m_useMorphBlendMaterials, TXT("Whether a blend material should be used for each mesh material."), TXT("MaterialValueMapping") );
	PROPERTY_EDIT( m_useControlTexturesForMorph, TXT("Whether to use the control textures for vertex interpolation.") );
	PROPERTY_CUSTOM_EDIT_RANGE( m_morphRatio, TXT("morph ratio. For preview only, will not be saved."), TXT("Slider"), 0.0f, 1.0f );
	PROPERTY_EDIT( m_morphComponentId, TXT("This id will be used for external systems to find proper morph process") );
	NO_PROPERTY_SERIALIZATION( m_morphRatio );
END_CLASS_RTTI();
