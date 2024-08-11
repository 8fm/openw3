/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../core/resource.h"

/// The definition of renderable material
class ITexture;
class IMaterialDefinition;
class CCubeTexture;
class CTextureArray;
class CRenderFrame;
class CEntity;
class CComponent;
class IRenderResource;

/// Color channel
enum EColorChannel
{
	COLCHANNEL_Red,
	COLCHANNEL_Green,
	COLCHANNEL_Blue,
	COLCHANNEL_Alpha,
};

BEGIN_ENUM_RTTI( EColorChannel );
	ENUM_OPTION( COLCHANNEL_Red );
	ENUM_OPTION( COLCHANNEL_Green );
	ENUM_OPTION( COLCHANNEL_Blue );
	ENUM_OPTION( COLCHANNEL_Alpha );
END_ENUM_RTTI()

/// Context of material parameters update
struct CMaterialUpdateContext
{
	CEntity				*m_entity;				//!< entity used to obtain new values
	CComponent			*m_component;			//!< component
	CRenderFrame		*m_frame;				//!< current render frame
};

/// Base material
class IMaterial : public CResource
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMaterial, CResource );	

#ifndef NO_RUNTIME_MATERIAL_COMPILATION

protected:
	static THashSet< IMaterial* >	st_allMaterials;		//!< All used engine materials
	static Red::Threads::CMutex		st_accessMutex;			//!< Mutex for accessing the list

#endif

protected:
	IRenderResource*		m_renderResource;		//!< Rendering resource created for this material definition
	THandle< IMaterial >	m_baseMaterial;			//!< Material on which we are based on
	Bool					m_proxyFailed;			//!< Creating rendering proxy for material has failed

public:
	// Get base material
	RED_INLINE IMaterial* GetBaseMaterial() const { return m_baseMaterial.Get(); }

	// Is this material compiled
	RED_INLINE Bool IsCompiled() const { return m_renderResource != NULL; }

#ifndef NO_EDITOR
	virtual Bool HasAnyBlockParameters() const { return false; }
#endif

public:
	IMaterial( IMaterial* baseMaterial );
	virtual ~IMaterial();


	virtual void OnAllHandlesReleased() override;


	// Is this material using mask ?
	virtual Bool IsMasked() const = 0;


	// Force material recompilation
	void ForceRecompilation( Bool createRenderResource = true );

	// Get the rendering proxy for this material
	IRenderResource* GetRenderResource() const;

	// Does the material currently have a render resource created?
	RED_INLINE Bool HasRenderResource() const { return m_renderResource != nullptr; }

public:
	//! Get base material
	virtual IMaterialDefinition* GetMaterialDefinition()=0;

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	//! Update rendering parameters ( sort group and such )
	virtual void UpdateRenderingParams() = 0;
#endif

	//! Collect names of all parameters in this material. Each name is collected only once.
	virtual void GetAllParameterNames( TDynArray< CName >& outNames ) const = 0;

	//! Write material parameter data. Optionally force a recreate on the material render resource.
	virtual Bool WriteParameterRaw( const CName& name, const void* data, Bool recreateResource = true )=0;

	//! Read parameter data
	virtual Bool ReadParameterRaw( const CName& name, void* data ) const=0;

public:
	// Predefined read function - to minimalize the chance of mistake
	RED_INLINE Bool ReadParameter( const CName& name, Float& val ) const { return ReadParameterRaw( name, &val );	}
	RED_INLINE Bool ReadParameter( const CName& name, THandle< ITexture >& val ) const { return ReadParameterRaw( name, &val ); }
	RED_INLINE Bool ReadParameter( const CName& name, THandle< CTextureArray >& val ) const { return ReadParameterRaw( name, &val );	}
	RED_INLINE Bool ReadParameter( const CName& name, THandle< CCubeTexture >& val ) const { return ReadParameterRaw( name, &val ); }
	RED_INLINE Bool ReadParameter( const CName& name, THandle< CBitmapTexture >& val ) const { return ReadParameterRaw( name, &val ); }
	RED_INLINE Bool ReadParameter( const CName& name, Color & val ) const { return ReadParameterRaw( name, &val ); }
	RED_INLINE Bool ReadParameter( const CName& name, Vector& val ) const { return ReadParameterRaw( name, &val ); }
	RED_INLINE Bool ReadParameter( const CName& name, Matrix& val ) const { return ReadParameterRaw( name, &val ); }

	// Predefined write function - to minimalize the chance of mistakes
	RED_INLINE Bool WriteParameter( const CName& name, const Float& val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const THandle< ITexture >& val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const THandle< CBitmapTexture >& val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const THandle< CTextureArray >& val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const THandle< CCubeTexture >& val, Bool recreateResource = true )	{ return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const Color & val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const Vector & val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }
	RED_INLINE Bool WriteParameter( const CName& name, const Matrix & val, Bool recreateResource = true ) { return WriteParameterRaw( name, &val, recreateResource ); }

public:
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	//! Recompile all material using given texture
	static void RecompileMaterialsUsingTexture( ITexture* texture );

	//! Recompile all material using given cubemap
	static void RecompileMaterialsUsingCube( CCubeTexture* texture );

	//! Recompile all material using given texture array
	static void RecompileMaterialsUsingTextureArray( const CTextureArray* textureArray );
#endif

public:
	// Create rendering resource for material
	void CreateRenderResource();

	// Remove rendering proxy resource for this material
	void RemoveRenderResource();

	// Gather the texture this material is using
	void GatherTexturesUsed( IMaterial* material, Uint32 materialIndex, TDynArray< CBitmapTexture* >& textures );
	// Doesn't addref anything.
	void GatherTextureRenderResources( TDynArray< IRenderResource* >& textures );
};

BEGIN_ABSTRACT_CLASS_RTTI( IMaterial );
	PARENT_CLASS( CResource );
END_CLASS_RTTI();
