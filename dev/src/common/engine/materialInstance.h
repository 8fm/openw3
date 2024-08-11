/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "material.h"
#include "materialParameterInstance.h"

typedef TDynArray< MaterialParameterInstance, MC_MaterialParameters >		TMaterialParameters;

/// Instance of material
class CMaterialInstance : public IMaterial
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMaterialInstance, IMaterial, "w2mi", "Material instance" );

protected:
	TMaterialParameters		m_parameters;		//!< Local instance parameters
	Bool					m_enableMask;		//!< Whether masking should be performed for this material instance (does not include things like dissolves).

public:
	//! Get parameters map
	RED_INLINE const TMaterialParameters& GetParameters() const { return m_parameters; }

	//! Is any of the parameters instanced in this material instance ?
	RED_INLINE Bool IsAnyParameterInstanced() const { return m_parameters.Size() > 0; }

#ifndef NO_EDITOR
	Bool HasAnyBlockParameters() const override;
#endif

public:
	CMaterialInstance();

	// Create material instance from other material. If runtime compilation is available and compileNow is true, then ForceRecompilation
	// will be called, causing the render resources to be created immediately.
	CMaterialInstance( CObject* parent, IMaterial* material, Bool createRenderResource = true );

	// Serialization
	virtual void OnSerialize( IFile& file ) override;

	// Property changed
	virtual void OnPropertyPostChange( IProperty* property ) override;

#ifndef NO_DEBUG_PAGES
	virtual void OnDebugPageInfo( class CDebugPageHTMLDocument& doc ) override;
#endif

#ifndef NO_EDITOR
	virtual void OnPostLoad();
#endif

public:
	//! Set base material
	void SetBaseMaterial( IMaterial* material );

	//! Clear all instanced parameters
	void ClearInstanceParameters();

	//! Is given parameter instanced in this instance ?
	Bool IsParameterInstanced( const CName& name ) const;

	//! Auto assign textures to this material instance ( crappy editor code )
	void AutoAssignTextures();

	//! Copy parameters from a given source material into this material.
	void CopyParametersFrom( IMaterial* sourceMaterial, Bool recreateResource = true );

public:

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	//! Update rendering parameters ( sort group and such )
	virtual void UpdateRenderingParams() override;
#endif

	//! Get base material graph 
	virtual IMaterialDefinition* GetMaterialDefinition() override;

	//! Get base material graph 
	virtual const IMaterialDefinition* GetMaterialDefinition() const;

	//! Collect names of all parameters in this material. Each name is collected only once.
	virtual void GetAllParameterNames( TDynArray< CName >& outNames ) const override;

	//! Write parameter data
	virtual Bool WriteParameterRaw( const CName& name, const void* data, Bool recreateResource = true ) override;

	//! Read parameter data
	virtual Bool ReadParameterRaw( const CName& name, void* data ) const override;

	//! Material instance is masked if its definition is masked and masking has been enabled (m_enableMask).
	virtual Bool IsMasked() const override;


	RED_INLINE Bool IsMaskEnabled() const { return m_enableMask; }
	//! Set whether this instance should use masking (if available). Render resource needs to be recreated after a call to this. This
	//! function will _not_ force a recreate.
	RED_INLINE void SetMaskEnabled( Bool enable ) { m_enableMask = enable; }
};

BEGIN_CLASS_RTTI( CMaterialInstance );
	PARENT_CLASS( IMaterial );
	PROPERTY_EDIT( m_baseMaterial, TXT("Base material") );

	// If this property's name is changed, make sure to update editor/materialInstanceGroupItem.cpp as well!
	PROPERTY_EDIT( m_enableMask, TXT("Whether to enable masking for the material") );
END_CLASS_RTTI();