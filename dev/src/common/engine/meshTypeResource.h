#pragma once

#include "renderSettings.h"
#include "meshSkeletonMappingCache.h"
#include "../core/resource.h"

class IMaterial;
class IRenderResource;

// Basic data about a single LOD level for a mesh-type resource.
struct SMeshTypeResourceLODLevel
{
	DECLARE_RTTI_STRUCT( SMeshTypeResourceLODLevel );

	Float					m_distance;			//!< Distance at which this LOD level starts to show up

	SMeshTypeResourceLODLevel()
		: m_distance( 0 )
	{}

	RED_INLINE Bool operator ==( const SMeshTypeResourceLODLevel& lvl ) const
	{
		return m_distance == lvl.m_distance;
	}
	RED_INLINE Bool operator !=( const SMeshTypeResourceLODLevel& lvl ) const { return !operator ==( lvl ); }

	friend IFile& operator<<( IFile& file, SMeshTypeResourceLODLevel& lod )
	{
		file << lod.m_distance;

#ifndef NO_EDITOR
		// Read in remaining data that used to be in CMesh::LODLevel.
		if ( file.IsReader() && file.GetVersion() < VER_REMOVE_SEPARATE_MESH_LOD_DISTANCES )
		{
			Float distanceXenon;
			Bool useOnPC, useOnXenon;
			file << distanceXenon;
			file << useOnPC;
			file << useOnXenon;
		}
#endif

		return file;
	}


	RED_INLINE Bool IsUsedOnPlatform() const 
	{
		return true;
	}

	RED_INLINE Float GetDistance() const
	{
		return m_distance * Config::cvMeshLODDistanceScale.Get();
	}

};
DEFINE_SIMPLE_RTTI_TYPE( SMeshTypeResourceLODLevel );


////////////////////////////////////////////////////////

class CMeshTypeResource : public CResource
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMeshTypeResource, CResource );

public:
	typedef TDynArray< THandle< IMaterial > > TMaterials;

public:
	//! Get materials
	RED_INLINE const TMaterials& GetMaterials() const { return m_materials; }
	RED_INLINE TMaterials& GetMaterials() { return m_materials; }

	//! Get bounding box
	RED_INLINE const Box& GetBoundingBox() const { return m_boundingBox; }

	//! Get mesh auto hide distance
	RED_MOCKABLE RED_INLINE Float GetAutoHideDistance() const { return m_autoHideDistance; }

#ifndef NO_RESOURCE_IMPORT
	RED_INLINE const String& GetAuthorName() const { return m_authorName; }

	//! Get material names, editor usage only
	RED_INLINE const TDynArray< String >& GetMaterialNames() const { return m_materialNames; }
	RED_INLINE TDynArray< String >& GetMaterialNames() { return m_materialNames; }
#endif

	// Is the mesh two sided (may be rendered twice)
	RED_INLINE Bool IsTwoSided() const { return m_isTwoSided; }
	RED_INLINE void SetIsTwoSided( Bool value ) { m_isTwoSided = value; }

	// Get bone mapping for skinning
	RED_INLINE CMeshSkeletonMappingCache& GetSkeletonMappingCache() const { return m_skeletonMappingCache; }

	CMeshTypeResource();
	virtual ~CMeshTypeResource();

public:
	virtual void OnPostLoad() override;
	virtual void OnPreSave() override;

	virtual void OnAllHandlesReleased() override;

	virtual void OnSerialize( IFile & file ) override;

	//! Old property was missing
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );

	//! Property values was changed in the editor
	virtual void OnPropertyPostChange( IProperty* property );

	// Change mesh material at given index
	void SetMaterial( Uint32 index, IMaterial* material );

	// Modify auto hide distance, should not be negative
	void SetAutoHideDistance( Float distance );

public:
	//! Get rendering resource
	virtual IRenderResource* GetRenderResource() const;

	//! Create rendering resources
	virtual void CreateRenderResource();

	//! Release rendering resources
	virtual void ReleaseRenderResource();

	//! Calculate bounding box
	virtual void CalculateBoundingBox() { }

	//! Is resource ready to be rendered?
	virtual Bool IsRenderingReady() const;

	//! Return the number of bones used by this mesh.
	virtual Uint32 GetBoneCount() const { return 0; }

	//! Get an array containing the names of each bone. Must have GetBoneCount() elements, or can be NULL if 0 bones.
	virtual const CName* GetBoneNames() const { return nullptr; }

	//! Get the matrices for bones. Must have GetBoneCount() elements, or can be NULL if 0 bones.
	virtual const Matrix* GetBoneRigMatrices() const { return nullptr; }

	//! Get the "vertex epsilons" for bones. Used for expanding bounding box calculations. Can be NULL if 0 bones, or if all
	//! epsilons should be treated as 0.0f. If it is non-NULL, must have GetBoneCount() elements.
	virtual const Float* GetBoneVertexEpsilons() const { return nullptr; }

	//! Get number of LOD levels in this mesh 
	virtual Uint32 GetNumLODLevels() const { return 0; }

	// Can return base implementation if level out of range or something.
	virtual const SMeshTypeResourceLODLevel& GetLODLevel( Uint32 level ) const;

	// Get default LOD hide distance for given LOD level
	static Float GetDefaultLODDistance( Int32 level );

#ifndef NO_RESOURCE_IMPORT

	//! Update the LOD settings
	virtual Bool UpdateLODSettings( Uint32 level, const SMeshTypeResourceLODLevel& lodSettings ) { return false; }

	//! Count triangles for given LOD
	virtual Uint32 CountLODTriangles( Uint32 level ) const { return 0; }

	//! Count vertices for given LOD
	virtual Uint32 CountLODVertices( Uint32 level ) const { return 0; }

	//! Count chunks for given LOD
	virtual Uint32 CountLODChunks( Uint32 level ) const { return 0; }

	//! Count material for given LOD
	virtual Uint32 CountLODMaterials( Uint32 level ) const { return 0; }

#endif

protected:
	TMaterials								m_materials;					// Mesh materials
	Box										m_boundingBox;					// General mesh bounding box ( vertices only )

	IRenderResource*						m_renderResource;				// Rendering resource
	Bool									m_isTwoSided;

	Float									m_autoHideDistance;				// Hide mesh automatically after this distance

	mutable CMeshSkeletonMappingCache		m_skeletonMappingCache;

#ifndef NO_RESOURCE_IMPORT
	String									m_authorName;
	TDynArray< String >						m_materialNames;				// Material names
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( CMeshTypeResource );
	PARENT_CLASS( CResource );

#ifndef NO_RESOURCE_IMPORT
	PROPERTY_NOT_COOKED( m_materialNames );
	PROPERTY_RO_NOT_COOKED( m_authorName, TXT("Author of this mesh") );
#endif

	PROPERTY( m_materials );
	PROPERTY_EDIT( m_boundingBox, TXT("Mesh bounding box") );
	PROPERTY_EDIT( m_autoHideDistance, TXT("Hide mesh after this distance") );
	PROPERTY_EDIT( m_isTwoSided, TXT("IsTwoSided") );
END_CLASS_RTTI();
