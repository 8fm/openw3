/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "meshTypeResource.h"
#include "navigationObstacle.h"
#include "renderSkinningData.h"
#include "meshChunk.h"
#include "../core/latentLoader.h"
#include "gpuDataBuffer.h"
#include "../core/deferredDataBuffer.h"
#include "../core/weakPtr.h"
#include "../core/importer.h"
#include "meshEnum.h"

#ifndef NO_OBSTACLE_MESH_DATA
	class CNavigationObstacle;
#endif

class ISkeletonDataProvider;
class CCollisionMesh;
class IRenderProxy;
enum EMaterialVertexFactory : CEnum::TValueType;

typedef TDynArray< SMeshChunk > MeshUnpackedData;
typedef Red::TAtomicSharedPtr< MeshUnpackedData > MeshUnpackedDataHandle;

////////////////////////////////////////////////////////

/// Mesh rendering style, for debug stuff
enum EMeshDebugStyle
{
	MDS_FaceNormals,			// XYZ encoded normals PER FACE
	MDS_FaceFakeLighting,		// Fake lighting from directional light, PER FACE NORMAL
	MDS_Normals,				// XYZ encoded normals from vertices
	MDS_FakeLighting,			// Fake lighting from directional light, normals taken from vertices
};

////////////////////////////////////////////////////////

struct SMeshCookedData
{
	DECLARE_RTTI_STRUCT( SMeshCookedData );

public:
	Vector									m_collisionInitPositionOffset;	// Offset applied by drop physics component. Based on collision shape position.
	Vector									m_dropOffset;					// Offset applied by drop physics component. Based on collision shape position.
	TDynArray< Vector, MC_CookedHeader >	m_bonePositions;				// Model-space positions of each bone
	TDynArray< Float, MC_CookedHeader >		m_renderLODs;					// Cooked LOD distances
	TDynArray< Uint8 , MC_CookedHeader>		m_renderChunks;					// Cooked render chunk data
	DeferredDataBuffer						m_renderBuffer;					// Cooked vertex & index data (vertex first, index after)
	Uint32									m_vertexBufferSize;				// Size of vertex data
	Uint32									m_indexBufferSize;				// Size of index data
	Uint32									m_indexBufferOffset;			// Offset of index data in the render data (vertex data has implicit offset of 0)
	Vector									m_quantizationScale;			// Quantization scale for vertices
	Vector									m_quantizationOffset;			// Quantization offsets for vertices

	SMeshCookedData();

	// calculate size of the cooked data
	const Uint32 CalcSize() const;
};

BEGIN_CLASS_RTTI( SMeshCookedData );
	PROPERTY( m_collisionInitPositionOffset );
	PROPERTY( m_dropOffset );
	PROPERTY( m_bonePositions );
	PROPERTY( m_renderLODs );
	PROPERTY( m_renderChunks );
	PROPERTY( m_renderBuffer );
	PROPERTY( m_quantizationScale );
	PROPERTY( m_quantizationOffset );
	PROPERTY( m_vertexBufferSize );
	PROPERTY( m_indexBufferSize );
	PROPERTY( m_indexBufferOffset );
END_CLASS_RTTI();

// Mesh sound data
struct SMeshSoundInfo
{
	DECLARE_RTTI_STRUCT( SMeshSoundInfo );

	CName										m_soundTypeIdentification;				// Sound type identification
	CName										m_soundSizeIdentification;				// Sound size identification
	CName										m_soundBoneMappingInfo;					// Mesh sound bone mapping preset

	SMeshSoundInfo()
		: m_soundTypeIdentification( CName::NONE )
		, m_soundSizeIdentification( CName::NONE )
		, m_soundBoneMappingInfo( CName::NONE )
		{};
	
};

BEGIN_CLASS_RTTI( SMeshSoundInfo );
	PROPERTY( m_soundTypeIdentification );
	PROPERTY( m_soundSizeIdentification );
	PROPERTY( m_soundBoneMappingInfo );
END_CLASS_RTTI();

////////////////////////////////////////////////////////
// Importer params
struct SMeshImporterParams : public IImporter::ImportOptions::ImportParams
{
	SMeshImporterParams()
		: m_reuseMesh( false )
		, m_reuseVolumes( false )
		, m_regenerateVolumes( false ){}
	SMeshImporterParams( Bool reuseMesh, Bool reuseVolumes ) 
		: m_reuseMesh( reuseMesh )
		, m_reuseVolumes( reuseVolumes )
		, m_regenerateVolumes( false ){}
	Bool m_reuseMesh;
	Bool m_reuseVolumes;
	Bool m_regenerateVolumes;
};

////////////////////////////////////////////////////////
// LOD generation parameters
// HACK this shouldn't be here but the LOD generation function is implemented in the mesh which forces us to do this
struct SLODPresetDefinition
{
	SLODPresetDefinition() 
		: m_reduction( 0.5f )
		, m_distance( 10.f )
		, m_chunkFaceLimit( 0 )
		, m_removeSkinning( false ) 
		, m_recalculateNormals( false )
		, m_hardEdgeAngle( 80 )
		, m_geometryImportance( 1.f )
		, m_textureImportance( 1.f )
		, m_materialImportance( 1.f )
		, m_groupImportance( 1.f )
		, m_vertexColorImportance( 1.f )
		, m_shadingImportance( 1.f )
	{}

	SLODPresetDefinition( Float reduction, Float distance, Uint32 faceLimit, Bool removeSkinning ) 
		: m_reduction( reduction )
		, m_distance( distance )
		, m_chunkFaceLimit( faceLimit )
		, m_removeSkinning( removeSkinning )
		, m_recalculateNormals( false )
		, m_hardEdgeAngle( 80 )
		, m_geometryImportance( 1.f )
		, m_textureImportance( 1.f )
		, m_materialImportance( 1.f )
		, m_groupImportance( 1.f )
		, m_vertexColorImportance( 1.f )
		, m_shadingImportance( 1.f )
	{}

	Float	m_reduction;
	Float	m_distance;
	Uint32	m_chunkFaceLimit;
	Bool	m_removeSkinning;
	Bool	m_recalculateNormals;
	Uint32	m_hardEdgeAngle;
	Float	m_geometryImportance;
	Float	m_textureImportance;
	Float	m_materialImportance;
	Float	m_groupImportance;
	Float	m_vertexColorImportance;
	Float	m_shadingImportance;
};

////////////////////////////////////////////////////////
// Base mesh
class CMesh : public CMeshTypeResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMesh, CMeshTypeResource, "w2mesh", "Mesh" );

public:	
	struct LODLevel;
	typedef TDynArray< LODLevel, MC_RenderData > TLODLevelArray;
	typedef TDynArray< CName, MC_RenderData > TSkeletonBoneNameArray;
	typedef TDynArray< Matrix, MC_RenderData > TSkeletonBoneMatrixArray;
	typedef TDynArray< Float, MC_RenderData > TSkeletonBoneEpsilonArray;
	typedef TDynArray< Uint16 > TChunkArray;

	// LOD level
	struct LODLevel
	{
		TChunkArray					m_chunks;			//!< Chunks to draw for this LOD
		SMeshTypeResourceLODLevel	m_meshTypeLOD;

		LODLevel() {}

		LODLevel( Float distance )
		{
			m_meshTypeLOD.m_distance = distance;
		}

		friend IFile& operator<<( IFile& file, LODLevel& lod )
		{
			file << lod.m_chunks;
			file << lod.m_meshTypeLOD;
			return file;
		}

		RED_FORCE_INLINE Bool IsUsedOnPlatform() const 
		{
			return m_meshTypeLOD.IsUsedOnPlatform();
		}

		RED_MOCKABLE RED_FORCE_INLINE Float GetDistance() const
		{
			return m_meshTypeLOD.GetDistance();
		}
	};

	// Collision build data
	struct CollisionMesh
	{
		enum CollisionMeshType
		{
			ECMT_Trimesh,
			ECMT_Convex,
			ECMT_Box,
			ECMT_Sphere,
			ECMT_Capsule
		};
		CollisionMeshType			m_type;
		TDynArray< Vector >			m_vertices;
		TDynArray< Uint32 >			m_indices;
		TDynArray< unsigned short >	m_physicalMaterialIndexes;
		TDynArray< CName >			m_physicalMaterials;
	};

	// Build data
	struct FactoryInfo : public CResource::FactoryInfo< CMesh >
	{
		FactoryInfo ()
			: m_importCollision( false )
			, m_buildConvexCollision( false )
			, m_entityProxy( false )
			, m_reuseMesh( false )
			, m_reuseVolumes( false )
			, m_assignDefaultMaterials( false )
		{}
		TDynArray< String >				m_materialNames;				// Name of the materials
		TDynArray< SMeshChunk >			m_chunks;						// Mesh chunks
		TSkeletonBoneNameArray			m_boneNames;
		TSkeletonBoneMatrixArray		m_boneRigMatrices;
		TSkeletonBoneEpsilonArray		m_boneVertexEpsilons;
		TDynArray< CollisionMesh >		m_collisionMeshes;				// Collision meshes
		TLODLevelArray					m_lodLevels;					// LOD levels
		Bool							m_importCollision;				// Import collision meshes
		Bool							m_buildConvexCollision;			// Automatic convex
#ifndef NO_OBSTACLE_MESH_DATA
		CNavigationObstacle				m_navigationObstacle;
#endif
		String							m_authorName;
		String							m_baseResourceFilePath;
		Bool							m_entityProxy;
		Bool							m_reuseMesh;
		Bool							m_reuseVolumes;
		Bool							m_assignDefaultMaterials;		// assign default material, do not map
	};

	struct BatchOperation
	{
		BatchOperation( CMesh* mesh );
		~BatchOperation();
		void Disable();
		CMesh* m_mesh;
		Bool   m_enabled;
	};


public:
	CMesh();
	virtual ~CMesh();

	// Serialisation
	virtual void OnSerialize( IFile& file );
	virtual void OnPreSave();
	virtual void OnPostLoad();
	virtual Bool OnPropertyMissing( CName propertyName, const CVariant& readValue );
	virtual void ForceFullyLoad();

	// Editor / cooking
	virtual void OnPropertyPostChange( IProperty* property );
	virtual void GetAdditionalInfo( TDynArray< String >& info ) const;
#ifndef NO_EDITOR
	static void ActivatePipelineMesh() { s_pipelineActive = true; }
	static void DeactivatePipelineMesh() { s_pipelineActive = false; }
#endif

	// Resource system stuff
	virtual CGatheredResource* GetDefaultResource();
	virtual void CreateRenderResource();
	RED_INLINE const String& GetBaseResourceFilePath() const { return m_baseResourceFilePath; }
#ifndef NO_DEBUG_PAGES
	RED_INLINE Bool GetResourceLoadError() const { return m_resourceLoadError; }
	Bool ErrorOccuredDuringLoading() const { return m_resourceLoadError; }
#endif

	// LOD&Chunks
	virtual Uint32 GetNumLODLevels() const { return m_lodLevelInfo.Size(); }
	virtual const SMeshTypeResourceLODLevel& GetLODLevel( Uint32 level ) const;
	RED_MOCKABLE RED_INLINE const TLODLevelArray& GetMeshLODLevels() const { return m_lodLevelInfo; }
	RED_INLINE const TDynArray< SMeshChunkPacked >& GetChunks() const { return m_chunks; }

	// Low-level data access
	RED_INLINE const DeferredDataBuffer& GetRawVertices() const { return m_rawVertices; }
	RED_INLINE const DeferredDataBuffer& GetRawIndices() const { return m_rawIndices; }

	// Cooked data
	RED_INLINE const SMeshCookedData& GetCookedData() const { return m_cookedData; }
	RED_INLINE const Uint32 GetCookedDataSize() const { return m_cookedData.CalcSize(); }
	void SetCookedData( const SMeshCookedData& data );

#ifndef NO_RESOURCE_IMPORT
	void UncookData( BufferHandle renderData );
#endif

	// Shadows
	RED_INLINE const Float GetGeneralizedMeshRadius() const { return m_generalizedMeshRadius; }
	RED_INLINE void SetGeneralizedMeshRadius(const Float radius) { m_generalizedMeshRadius = radius; }

	// Shadow mesh generation helpers
	void GetChunksForShadowMesh( TDynArray< Uint32 >& outChunks ) const;

	// Merging
	RED_INLINE const Bool CanMergeIntoGlobalShadowMesh() const { return m_mergeInGlobalShadowMesh; }
	RED_INLINE void SetMergeIntoGlobalShadowMesh( Bool value ) { m_mergeInGlobalShadowMesh = value; }

	// Sounds
#ifndef NO_EDITOR
	RED_INLINE SMeshSoundInfo* GetMeshSoundInfo() const { return m_soundInfo;}

	RED_INLINE void SetMeshSoundInfo( SMeshSoundInfo* nam )  
	{ 
		if( m_soundInfo ) 
		{ 
			delete m_soundInfo; 
			m_soundInfo = nullptr; 
		}  
		m_soundInfo = nam; 
	}
#else
	RED_INLINE const SMeshSoundInfo* GetMeshSoundInfo() const { return m_soundInfo;}
#endif
	// Rendering metadata
	RED_INLINE Bool CanUseExtraStreams() const { return m_useExtraStreams; }
	RED_INLINE void SetUseExtractStreams( Bool value ) { m_useExtraStreams = value; }
	Uint32 EstimateMemoryUsageCPU( const Int32 lodLevel ) const;	// -1 to get static memory usage
	Uint32 EstimateMemoryUsageGPU( const Int32 lodLevel ) const;	// -1 to get static memory usage
	RED_INLINE Bool IsStatic() const { return m_isStatic; }
	RED_INLINE Bool IsOccluder() const { return m_isOccluder; }
	RED_INLINE Float GetSmallestHoleOverride() const { return m_smallestHoleOverride; }
	RED_INLINE Bool HasSmallestHoleOverride() const { return m_smallestHoleOverride > 0.0f; }
	RED_INLINE Bool IsEntityProxy() const { return m_entityProxy; }

	// Skinning
	virtual Uint32 GetBoneCount() const { return m_boneNames.Size(); }
	virtual const CName* GetBoneNames() const { return m_boneNames.TypedData(); }
	virtual const Matrix* GetBoneRigMatrices() const { return m_boneRigMatrices.TypedData(); }
	virtual const Float* GetBoneVertexEpsilons() const { return m_boneVertexEpsilons.TypedData(); }
	RED_INLINE size_t GetBoneNamesDataSize() const { return m_boneNames.DataSize(); }
	RED_INLINE size_t GetBoneRigMatricesDataSize() const { return m_boneRigMatrices.DataSize(); }
	RED_INLINE size_t GetBoneVertexEpsilonsDataSize() const { return m_boneVertexEpsilons.DataSize(); }
	void GetBonePositions( TDynArray< Vector >& positions ) const;	// Fill array with model-space bone positions

	// Collision
	Bool HasCollision() const;

	// Navmesh
#ifndef NO_OBSTACLE_MESH_DATA
	RED_INLINE const CNavigationObstacle& GetNavigationObstacle() const { return m_navigationObstacle; }
#endif

	// Misc
	const Vector& GetInitPositionOffset() const;

public:
#ifndef NO_RESOURCE_IMPORT

	// Physics
	Bool GetCollisionData( TDynArray< Vector >& vertices ) const;
	Bool GetCollisionData( TDynArray< Vector >& vertices, TDynArray< Uint32 >& indices, TDynArray< Uint16 >& materialIndices ) const;
	void RemoveCollision();
	void RemoveCollisionShape( Uint32 index );
	Uint32 AddBoxCollision( Float scale=1.0f );
	Uint32 AddConvexCollision( Float scale=1.0f );
	Uint32 AddTriMeshCollision( Float scale=1.0f );
	Uint32 AddSphereCollision( Float scale=1.0f );
	Uint32 AddCapsuleCollision( Float scale=1.0f );
	RED_INLINE const CCollisionMesh* GetCollisionMesh() const { return m_collisionMesh.Get(); }		// Null if cooked or no collision.

	void SetIsOccluder( Bool val ){ m_isOccluder = val; }
	void SetIsStatic( Bool val ){ m_isStatic = val; }
	void SetEntityProxy( Bool val ){ m_entityProxy = val; }

	// Mesh area
	Float GetChunkSurfaceArea( Uint32 chunk_index ) const;
	static Float GetChunkSurfaceArea( const SMeshChunk& chunkData );

	// Data fixup
	Uint32 RemoveUnusedMaterials();
	void RemoveUnusedBones();
	void RemoveSkinning();	//! Remove skinning data from mesh (also removes bones)
	Bool RemoveSkinningFromLOD( Uint32 index );		//! Remove skinning data from given LOD. Returns true if the skinning was found and removed.
	Bool RemoveSkinningFromChunk( Uint32 index );	//! Remove skinning data from given chunk. Returns true if the skinning was found and removed.
	RED_INLINE const TDynArray< Uint32 >& GetUsedBones() const { return m_usedBones; }

	// create all of internal mesh data from specified factory data
	static CMesh* Create( const FactoryInfo& data );

	// validate mesh data
	static Bool ValidateMeshData( const TDynArray< SMeshChunk >& chunks, const TLODLevelArray& lodData, const Uint32 numMaterials );

	// set new mesh's geometry, can fail if checkout fails
	Bool SetMeshData( const TDynArray< SMeshChunk >& chunks, const TLODLevelArray& lodData, const Bool recreateRenderingData = true, const Bool optimizeMeshData = true );

	//! Combines two meshes together
	void AddMesh( const CMesh& source, const Matrix& transform, Bool removeRedundantLODs = true, Bool mergeChunks = true );

	//! Extracts mesh collapsed into one chunk with specified vertex type and a default material. lodLevel == -1 means lowest possible
	CMesh* ExtractCollapsed( EMeshVertexType vertexType, Int32 lodLevel = -1 ) const;

	//! Generate a automatic LOD, lodIndex of -1 means "add at the end"
	Bool GenerateLODWithSimplygon( Int32 lodIndex, const SLODPresetDefinition& lodDefinition , Uint32& numOfRemovedChunks, String& message, Bool showProgress = true );

	// LOD manipulation
	virtual Bool UpdateLODSettings( Uint32 level, const SMeshTypeResourceLODLevel& lodSettings );
	virtual Uint32 CountLODTriangles( Uint32 level ) const;
	virtual Uint32 CountLODVertices( Uint32 level ) const;
	virtual Uint32 CountLODChunks( Uint32 level ) const;
	virtual Uint32 CountLODMaterials( Uint32 level ) const;
	Uint32 CountLODBones( Uint32 level ) const;

	//! Remove LOD. Optionally, chunks that were contained in removed LOD are moved to outChunks.
	Bool RemoveLOD( Uint32 index );

	// Chunks manipulation
	Bool RemoveChunk( Uint32 index );
	Bool MergeChunks( const TDynArray< Uint32 >& chunkIndices, Uint32 materialIndex, String* outErrorStr = nullptr, Bool optimizeMesh = true );
	Int32 MergeChunks( Uint32 lodIndex, Bool countOnly = false, String* outErrorStr = nullptr, Bool optimizeMesh = true );	// Return number of chunks merged for the LOD
	Int32 MergeChunks( Bool countOnly = false, String* outErrorStr = nullptr, Bool optimizeMesh = true );	//Returns number of chunks that were (or could be) removed.
	void SetChunkMaterialId( Uint32 chunkIdx, Uint32 materialID );

	// Render mask manipulation, returns false if checkout failed
	Bool ForceRenderMaskOnAllChunks( const Uint8 newRenderMask, const Bool recreateRenderingData = true );
	Bool SetRenderMaskOnAllChunks( const Uint8 bitsToSet, const Bool recreateRenderingData = true );
	Bool ClearRenderMaskOnAllChunks( const Uint8 bitsToClear, const Bool recreateRenderingData = true );
	Bool ForceRenderMaskOnChunk( const Uint32 chunkIndex, const Uint8 newRenderMask, const Bool recreateRenderingData = true );

	// Shadow mesh options manipulation
	Bool ForceShadowMeshFlagOnChunk( const Uint32 chunkIndex, const Bool flag );

	// Compile debug mesh for easier display
	IRenderResource* CompileDebugMesh( const EMeshDebugStyle renderStyle, const Int32 lod, const Uint8 renderMask, const Color& debugColor ) const;

#endif // !NO_RESOURCE_IMPORT

private:
	// Get cached mesh data
	MeshUnpackedDataHandle GetUnpackedData() const;
	void ClearUnpackedData();

	// Resource import / data fixup
#ifndef NO_RESOURCE_IMPORT
	// Regenerate mesh data
	void RegenerateMeshData();

	// Optimize mesh data
	Bool OptimizeMeshChunk( const SMeshChunk& sourceChunk, TDynArray< Uint16 >& optimizedIndices, TDynArray< SMeshVertex >& optimizedVertices );

	// Regenerate material mapping
	void RebuildMaterialMap();

	// Compute chunk mask for shadow meshes
	void RecomputePerChunkShadowMaskOptions();

	// Keep skinning data per vertex sorted by decreasing weights. These should be #ifndef NO_RESOURCE_IMPORT, except that we need to
	// sort old meshes when loading, or else they won't have proper data for the renderer and things might not work right. Once we can
	// be sure that all meshes are converted, it can be #ifdef'd out.
	static void SortSkinningForChunk( SMeshChunk& chunk );

	// Changed to protected, in order to inherit this in CPhysicsDestructionResource, which is basically a mesh with some extra stuff.
protected:
	// Compute default render masks
	void RecomputeDefaultRenderMask();

	// Compute best shadow fade distance
	void RecomputeShadowFadeDistance();


	// Map material name to index
	Uint32 MapMaterial( const String& materialName );	// Map material, tries to reuse existing one, if fails, creates a new one with specified name
#endif
private:
#ifndef NO_EDITOR
	static Bool s_pipelineActive;
#endif
		
	Float										m_generalizedMeshRadius;		// Generalized size of the mesh (based on the feature size)
	Bool										m_mergeInGlobalShadowMesh;		// Allow chunks to be extracted into global shadow mesh, saves CPU/GPU time, increases memory footprint

	TDynArray< Uint32 >							m_usedBones;					// List of used bone indices

	TLODLevelArray								m_lodLevelInfo;					// LOD levels

	SMeshSoundInfo*								m_soundInfo;					// Mesh sound info

	SMeshCookedData								m_cookedData;					// Cooked data

	Bool										m_useExtraStreams;				// Use vertex color and second UV mapping on this mesh
	Bool										m_isStatic;
	Bool										m_isOccluder;
	Float										m_smallestHoleOverride;			// Temporary workaround for windows - override the smallest hole parameter for this mesh (-1 is default)

#ifndef NO_DEBUG_PAGES
	Bool										m_resourceLoadError;			// Error happened when loading resource
#endif
	Int32										m_batchOperationCounter;

	TDynArray< SMeshChunkPacked >				m_chunks;						// Packed raw chunks
	DeferredDataBuffer							m_rawVertices;					// Raw vertex data (delayed loading)
	DeferredDataBuffer							m_rawIndices;					// Raw index data (delayed loading)

	// Changed to protected, in order to inherit this in CPhysicsDestructionResource, which is basically a mesh with some extra stuff.
protected:
	TSkeletonBoneNameArray						m_boneNames;
	TSkeletonBoneMatrixArray					m_boneRigMatrices;
	TSkeletonBoneEpsilonArray					m_boneVertexEpsilons;

	String										m_baseResourceFilePath;

	TDynArray< Uint16 >							m_finalIndices;					// Final indices list, not kept for regular meshes

	Bool										m_entityProxy;					// Was this mesh generated as a distant entity proxy

	THandle< CCollisionMesh >					m_collisionMesh;				// Collision data for this mesh

#ifndef NO_OBSTACLE_MESH_DATA
	CNavigationObstacle							m_navigationObstacle;			// Precomputed navigation obstacle for this mesh
#endif

private:
	// cached chunks
	mutable MeshUnpackedDataHandle				m_cachedUnpackedChunks;			// Cached data for unpacked chunks

	// "versioning"
	static const Uint8							MESH_VERSION_CURRENT;
	static const Uint8							MESH_VERSION_SHADOW_MASK_PER_CHUNK;
	static const Uint8							MESH_VERSION_PER_CHUNK_SHADOW_MASK_OPTIONS;
	Uint8										m_internalVersion;

	// LEGACY
	DeferredDataBuffer							m_chunksBuffer;
	friend class CMeshData;
	friend class CMeshCooker;
	friend class CUseForShadowMeshCommandlet;
};

BEGIN_CLASS_RTTI( CMesh );
	PARENT_CLASS( CMeshTypeResource );
#ifndef NO_RESOURCE_IMPORT
	PROPERTY_RO_NOT_COOKED( m_baseResourceFilePath, TXT("Base Resource File Path") );
#endif
#ifndef NO_OBSTACLE_MESH_DATA
	PROPERTY_EDIT_NOT_COOKED( m_navigationObstacle, TXT("Obstacle used by navigation system") );
#endif

	PROPERTY_NOT_COOKED( m_collisionMesh );
	PROPERTY_EDIT( m_useExtraStreams, TXT("Use vertex color and second UV on this mesh") );
	PROPERTY_RO( m_generalizedMeshRadius, TXT("Genertalized mesh size") );
	PROPERTY_EDIT( m_mergeInGlobalShadowMesh, TXT("Allow chunks to be extracted into global shadow mesh, saves CPU/GPU time, increases memory footprint") );
	PROPERTY_EDIT_NOT_COOKED( m_isOccluder, TXT("Is mesh used as occluder") );	
	PROPERTY_EDIT_NOT_COOKED( m_smallestHoleOverride, TXT("Temporary override for the smallest hole parameter for this mesh (-1 is default)") );
	PROPERTY( m_chunks );
	PROPERTY_NOT_COOKED( m_rawVertices );
	PROPERTY_NOT_COOKED( m_rawIndices );
	PROPERTY_RO( m_isStatic, TXT("Is static mesh?") );
	PROPERTY_RO( m_entityProxy, TXT("Is this a generated entity proxy?") );
	PROPERTY( m_cookedData );
	PROPERTY( m_soundInfo );
	PROPERTY( m_internalVersion );

	// LEGACY
	PROPERTY_NOT_COOKED( m_chunksBuffer );
END_CLASS_RTTI();

namespace MeshUtilities
{
	Uint32 GetSkinningMatricesAndBoxMS( const ISkeletonDataProvider* provider, 
		const TDynArray< Int16 >& boneMapping,
		const Matrix* rigMatrices, const Float* vertexEpsilons,
		void* skinningMatrices, ESkinningDataMatrixType outMatricesType,
		Box& outBoxMS );

	Bool UpdateTransformAndSkinningDataMS( const ISkeletonDataProvider* provider, 
		const CMesh* mesh, 
		const TDynArray< Int16 >& boneMapping, 
		const Matrix& localToWorld, 
		IRenderProxy* proxy, 
		IRenderSkinningData* skinningData,
		Box& boxMS );

	Float CalcFovDistanceMultiplierNoClamp( Float fov );
	Float CalcFovDistanceMultiplier( Float fov );

	extern GpuApi::eBufferChunkType GetVertexTypeForMeshChunk( const Bool extraStreams, const SMeshChunk& sourceChunk );
	extern EMaterialVertexFactory GetVertexFactoryForMeshChunk( const SMeshChunk& sourceChunk );
	GpuApi::VertexPacking::PackingVertexLayout GetVertexLayout();
}
