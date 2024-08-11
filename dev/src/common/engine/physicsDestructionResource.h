/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "collisionContent.h"
#include "mesh.h"

#ifndef PHYSICS_DESTRUCTION_RESOURCE_WRAPPER_H
#define PHYSICS_DESTRUCTION_RESOURCE_WRAPPER_H

enum EPhysicsDestructionSimType
{
	EPDST_Dynamic,
	EPDST_Static
};

struct SBoneIndicesHelper
{
	TDynArray< Uint16 > m_activeIndices;
	TDynArray< Uint32 > m_chunkOffsets;
	TDynArray< Uint32 > m_chunkNumIndices;
};

// This is per debris chunk additional info. At the moment - only stores information about whether it's static or dynamic,
// in the future it will have more (for example, velocity on destruction, so we can have predetermined, pseudo animated explosions, 
// depth of the chunk, so we can control which should disappear after time and which not, etc)
struct SPhysicsDestructionAdditionalInfo
{
	DECLARE_RTTI_STRUCT( SPhysicsDestructionAdditionalInfo );

	EPhysicsDestructionSimType							m_simType;												
	Vector												m_initialVelocity;
	Bool												m_overrideCollisionMasks;
	CPhysicalCollision									m_collisionType;
};
BEGIN_CLASS_RTTI( SPhysicsDestructionAdditionalInfo );
	PROPERTY_EDIT( m_initialVelocity, TXT( "m_initialVelocity" ) );
	PROPERTY_EDIT( m_overrideCollisionMasks, TXT( "m_overrideCollisionMasks" ) );
	PROPERTY_CUSTOM_EDIT_NAME( m_collisionType, TXT( "m_collisionType" ), TXT( "Defines what of types it is from physical collision point of view" ), TXT("PhysicalCollisionTypeSelector") );

END_CLASS_RTTI();

struct SBoneIndiceMapping
{
	DECLARE_RTTI_STRUCT( SBoneIndiceMapping );

	Uint32	m_startingIndex;
	Uint32	m_endingIndex;
	Uint32	m_chunkIndex;
	Uint32	m_boneIndex;

	SBoneIndiceMapping()
	{

	}


	SBoneIndiceMapping( Uint32 start, Uint32 end, Uint32 chunkIndex, Uint32 boneIndex )
	{
		m_startingIndex	= start;
		m_endingIndex	= end;
		m_chunkIndex	= chunkIndex;
		m_boneIndex	= boneIndex;
	}
};
BEGIN_CLASS_RTTI( SBoneIndiceMapping );
PROPERTY( m_startingIndex );
PROPERTY( m_endingIndex );
PROPERTY( m_chunkIndex );
PROPERTY( m_boneIndex );
END_CLASS_RTTI();

class CPhysicsDestructionResource : public CMesh, public ICollisionContent
{
	DECLARE_ENGINE_RESOURCE_CLASS( CPhysicsDestructionResource, CMesh, "reddest", "Physics destruction resource" );

protected:
	TDynArray< SPhysicsDestructionAdditionalInfo >		m_additionalInfo;
	TDynArray< SBoneIndiceMapping >						m_boneIndicesMapping;		// Sorted by startingIndex - when we remove unused bones they keep the order of indiceBuffer
	Uint32												m_chunkNumber;
#ifndef NO_EDITOR

#else
	CompiledCollisionPtr								m_compiledBuffer;
#endif

public:
	struct FactoryInfo : public CResource::FactoryInfo< CPhysicsDestructionResource >
	{
		FactoryInfo ()
			: m_importCollision( false )
			, m_buildConvexCollision( false )
			, m_entityProxy( false )
			, m_reuseMesh( false )
			, m_reuseVolumes( false )
			, m_assignDefaultMaterials( false )
		{}
		TDynArray< String >								m_materialNames;				// Name of the materials
		TDynArray< SMeshChunk >							m_chunks;						// Mesh chunks
		TSkeletonBoneNameArray							m_boneNames;
		TSkeletonBoneMatrixArray						m_boneRigMatrices;
		TSkeletonBoneEpsilonArray						m_boneVertexEpsilons;
		TDynArray< CollisionMesh >						m_collisionMeshes;				// Collision meshes
		TLODLevelArray									m_lodLevels;					// LOD levels
		Bool											m_importCollision;				// Import collision meshes
		Bool											m_buildConvexCollision;			// Automatic convex
#ifndef NO_OBSTACLE_MESH_DATA
		CNavigationObstacle								m_navigationObstacle;
#endif
		String											m_authorName;
		String											m_baseResourceFilePath;
		Bool											m_entityProxy;
		Bool											m_reuseMesh;
		Bool											m_reuseVolumes;
		Bool											m_assignDefaultMaterials;		// assign default material, do not map
	};


														CPhysicsDestructionResource(void);
														~CPhysicsDestructionResource(void);

	virtual CompiledCollisionPtr						CompileCollision(CObject* parent) const;
	virtual void										OnSerialize( IFile& file );

	CMesh*												AsCMesh() { return this; }

	SBoneIndicesHelper									GetIndicesForActiveBones( const TDynArray< Bool >& bonesActive );

	SPhysicsDestructionAdditionalInfo*					GetAdditionalInfoPtr( Uint16 index);
	const TDynArray<SPhysicsDestructionAdditionalInfo>& GetAdditionalInfoArray();
	EPhysicsDestructionSimType							GetSimTypeForChunk(Uint16 index);
	Vector												GetInitialVelocityForChunk(Uint16 chunkResIndex);

#ifndef NO_RESOURCE_COOKING
	virtual void										OnCook( ICookerFramework& cooker ) override;
	static CPhysicsDestructionResource*					Create(CPhysicsDestructionResource::FactoryInfo buildData);

	void 												SetSimTypeForChunk(Uint16 index, EPhysicsDestructionSimType simType);
	void												DecideToIncludeToCollisionCache();
#endif

};

BEGIN_CLASS_RTTI( CPhysicsDestructionResource );
	PARENT_CLASS( CMesh );
	PROPERTY( m_boneIndicesMapping )
		PROPERTY( m_finalIndices )
		PROPERTY( m_chunkNumber )
END_CLASS_RTTI();

#endif