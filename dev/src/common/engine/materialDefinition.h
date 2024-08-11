/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "materialCompiler.h"
#include "material.h"

enum ERenderingBlendMode : CEnum::TValueType;
enum ERenderingSortGroup : CEnum::TValueType;

typedef THashMap< Uint32, TPair< Uint64, Uint64 > > CRCMap;

/// Material definition is a shader that has user editable parameters
/// Material definition compiles to different techniques depending on rendering context
class IMaterialDefinition : public IMaterial
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMaterialDefinition, IMaterial );

public:
	/// Material dynamic parameter type
	enum EParameterType
	{
		PT_None,			//!< No parameter
		PT_Texture,			//!< Texture
		PT_Color,			//!< Color
		PT_Cube,			//!< Cubemap
		PT_Vector,			//!< 4 element vector
		PT_Scalar,			//!< Single scalar
		PT_Atlas,			//!< by Dex: altas parameters are about to be removed 
		PT_Bool,			//!< bool 
		PT_TextureArray,
	};

	/// Material parameter 
	class Parameter
	{
	public:
		CName			m_name;			//!< Parameter name
		Uint8			m_offset;		//!< Offset in parameter table
		Uint8			m_type;			//!< Type of parameter

	public:
		RED_INLINE Parameter()
			: m_type( PT_None )
			, m_offset( 0 )
		{};

		RED_INLINE Parameter( EParameterType type, const CName& name, Uint8 offset )
			: m_type( (Uint8) type )
			, m_name( name )
			, m_offset( offset )
		{};

	public:
		// Serialization
		friend IFile& operator<<( IFile& file, Parameter& param )
		{
			file << param.m_type;
			file << param.m_offset;
			file << param.m_name;
			return file;
		}
	};
	typedef TDynArray< Parameter, MC_MaterialParameters > TParameterArray;

protected:
	TParameterArray							m_pixelParameters;				//!< Material parameters
	TParameterArray							m_vertexParameters;				//!< Material parameters
	Uint32									m_pixelParamBlockSize;			//!< Size of parameters data
	Uint32									m_vertexParamBlockSize;			//!< Size of parameters data
	Vector									m_offset;						//!< Background offset
	Bool									m_compileAllTechniques;			//!< Compile all allowed techniques for this material
	Bool									m_canUseOnMeshes;				//!< We can use this material on meshes
	Bool									m_canUseOnDestructionMeshes;		//!< We can use this material on destruction meshes
	Bool									m_canUseOnApexMeshes;			//!< We can use this material on apex meshes
	Bool									m_canUseOnParticles;			//!< We can use this material on particles	
	Bool									m_canUseOnCollapsableObjects;	//!< We can use this material on collapsible objects
	Bool									m_canUseSkinning;				//!< We can use this material on skinned meshes
	Bool									m_canUseSkinnedInstancing;		//!< We can use this material on skinned instanced meshes
	Bool									m_canUseOnMorphMeshes;			//!< We can use this material on meshes using morphing

public:
	IMaterialDefinition();

	// Get material parameters
	RED_INLINE const TParameterArray& GetPixelParameters() const { return m_pixelParameters; }

	// Get material parameters
	RED_INLINE const TParameterArray& GetVertexParameters() const { return m_vertexParameters; }

	// Get size of compiled parameters block
	RED_INLINE Uint32 GetPixelParamBlockSize() const { return m_pixelParamBlockSize; }
	
	// Get size of compiled parameters block
	RED_INLINE Uint32 GetVertexParamBlockSize() const { return m_vertexParamBlockSize; }

	// Get the base material definition
	RED_INLINE IMaterialDefinition* GetMaterialDefinition() { return this; }

	// Get the priority of the resource reloading for this resource
	RED_INLINE ResourceReloadPriority GetReloadPriority() { return 10; }

	// Can we use this material on meshes ( static and skinned ) ?
	RED_INLINE Bool CanUseOnMeshes() const { return m_canUseOnMeshes; }
	RED_INLINE void SetUseOnMeshes( Bool val ) { m_canUseOnMeshes = val; }

	RED_INLINE Bool CanUseOnDestructionMeshes() const { return m_canUseOnDestructionMeshes; }

	// Can we use this material on apex meshes
	RED_INLINE Bool CanUseOnApexMeshes() const { return m_canUseOnApexMeshes; }

	// Can we use this material on particles ?
	RED_INLINE Bool CanUseOnParticles() const { return m_canUseOnParticles; }
	RED_INLINE void SetUseOnParticles( Bool val ) { m_canUseOnParticles = val; }
	
	// Can we use this material on collapsable objects
	RED_INLINE Bool CanUseOnCollapsableObjects() const { return m_canUseOnCollapsableObjects; }
	RED_INLINE void SetUseOnCollapsableObjects( Bool val ) { m_canUseOnCollapsableObjects = val; }

	// Compile all techniques for this material
	RED_INLINE Bool CompileAllTechniques() const { return m_compileAllTechniques; }

	RED_INLINE void SetUseSkinning( Bool val ) { m_canUseSkinning = val; }
	RED_INLINE Bool CanUseSkinning() const { return m_canUseSkinning; }

	RED_INLINE void SetUseSkinnedInstancing( Bool val ) { m_canUseSkinnedInstancing = val; }
	RED_INLINE Bool CanUseSkinnedInstancing() const { return m_canUseSkinnedInstancing; }

	// Can we use this material on morphing meshes
	RED_INLINE Bool CanUseOnMorphMeshes() const { return m_canUseOnMorphMeshes; }
	RED_INLINE void SetUseOnMorphMeshes( Bool val ) { m_canUseOnMorphMeshes = val; }

	String GetShaderName() const;

public:
	// Determine the sort group to use with this material
	virtual ERenderingSortGroup GetRenderingSortGroup() const=0;

	// Determine the blend mode to use with this material
	virtual ERenderingBlendMode GetRenderingBlendMode() const=0;

	// Get material parameter mask
	virtual Uint32 GetRenderingFragmentParameterMask() const=0;

	// Is this material two sided ?
	virtual Bool IsTwoSided() const=0;

	// Is this material using emissive ?
	virtual Bool IsEmissive() const=0;

	// Is this material using forward rendering ?
	virtual Bool IsForwardRendering() const=0;

	// Is this material using accumulative refraction ?
	virtual Bool IsAccumulativelyRefracted() const=0;

	// Is this material reflective masked
	virtual Bool IsReflectiveMasked() const=0;

	// Is this mimic material
	virtual Bool IsMimicMaterial() const=0;

	// Is this skin
	virtual Bool IsSkin() const=0;

	// Is this eye
	virtual Bool IsEye() const=0;

	// Can an instance using this definition change whether it's masked or not?
	virtual Bool CanInstanceOverrideMasked() const=0;

	// Is this Volume
	virtual Bool IsUsedByVolumeRendering() const=0;

public:
	virtual Bool TryGetCRC( Uint32 contextId, Uint64& vsCRC, Uint64& psCRC ) const = 0;
	
	virtual Uint64 GetVSCRC( Uint32 contextId ) const = 0;
	virtual Uint64 GetPSCRC( Uint32 contextId ) const = 0;

	virtual const CRCMap& GetCRCMap() const = 0;

#ifndef NO_RUNTIME_MATERIAL_COMPILATION
	// Compile material code
	virtual void Compile( IMaterialCompiler* compiler ) const=0;
	virtual void CacheCRCs( IMaterialCompiler* compiler ) = 0;

	virtual Bool SupportsContext( const MaterialRenderingContext& context ) const;
#endif

public:
	// Serialize material
	virtual void OnSerialize( IFile& file );
};

BEGIN_ABSTRACT_CLASS_RTTI( IMaterialDefinition );
	PARENT_CLASS( IMaterial );
	PROPERTY_RO( m_pixelParamBlockSize, TXT("Pixel params data size") ); // hack to allow creation of CMaterialGroupItem
	PROPERTY_RO( m_vertexParamBlockSize, TXT("Vertex params data size") ); // hack to allow creation of CMaterialGroupItem
	PROPERTY_EDIT( m_compileAllTechniques, TXT("Compile all techniques for this material. SLOW!!.") );
	PROPERTY_EDIT( m_canUseOnMeshes, TXT("We can use this material on meshes") );
	PROPERTY_EDIT( m_canUseOnDestructionMeshes, TXT("We can use this material on destruction meshes") );
	PROPERTY_EDIT( m_canUseOnApexMeshes, TXT("We can use this material on apex meshes") );
	PROPERTY_EDIT( m_canUseOnParticles, TXT("We can use this material on particles") );	
	PROPERTY_EDIT( m_canUseOnCollapsableObjects, TXT("We can use this material on collapsable objects") );
	PROPERTY_EDIT( m_canUseSkinning, TXT("We can use this material on skinned meshes") );
	PROPERTY_EDIT( m_canUseSkinnedInstancing, TXT("We can use this material on skinned instanced meshes") );
	PROPERTY_EDIT( m_canUseOnMorphMeshes, TXT("We can use this material on morph meshes. (This is NOT blending material, but material used on source or target mesh!) )") );
END_CLASS_RTTI();
