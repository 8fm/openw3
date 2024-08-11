/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/importer.h"
#include "../../common/engine/physicsDestructionResource.h"
#include "../../common/engine/engineTypeRegistry.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshLODGenerator.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/core/dataError.h"
#include "ReFileHelpers.h"

#ifndef NO_EDITOR
#include "../../common/core/feedback.h"
#endif

static const Float SCALE = 0.01f;

class CDestructionImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CDestructionImporter, IImporter, 0 );

protected:
	Bool	m_autoRemoveUnusedBones;

public:
	CDestructionImporter();
	virtual CResource*	DoImport( const ImportOptions& options );
	virtual Bool		PrepareForImport( const String&, ImportOptions& options ) override;
};

BEGIN_CLASS_RTTI( CDestructionImporter )
	PARENT_CLASS( IImporter )
	PROPERTY_EDIT( m_autoRemoveUnusedBones, TXT("Removes unused skinned bones for imported mesh") );
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDestructionImporter );

Bool CDestructionImporter::PrepareForImport( const String& str, ImportOptions& options )
{
	return CReFileHelpers::ShouldImportFile( options );
}

CDestructionImporter::CDestructionImporter( )
{
	m_resourceClass = ClassID< CPhysicsDestructionResource >();
	m_formats.PushBack( CFileFormat( TXT("re"), TXT("Re File") ) );
	m_autoRemoveUnusedBones = false;
	LoadObjectConfig( TXT("User") );
}

namespace
{
	const StringAnsi STR_COLL( "_COLL" );
	const StringAnsi STR_NAV( "_NAV" );

	void ImportMaterials( const TDynArray< ReFileMesh >& meshes, CPhysicsDestructionResource::FactoryInfo& buildData, TDynArray< Int32 >& materialsid )
	{
		TDynArray< String > materials;
		Uint32 meshesSize = meshes.Size();

		for ( Uint32 m=0; m < meshesSize; m++ )
		{
			const ReFileMesh& mesh = meshes[m];
			for ( Int32 i=0; i < mesh.getNumChunks(); i++ )
			{
				String matName( ANSI_TO_UNICODE( mesh.getMeshChunk( i ).getMaterial().getMaterialName().getData() ) );
				int index = -1;
				Bool is = false;

				for ( Uint32 el=0; el<materials.Size(); el++ )
				{
					if ( materials[el]==matName )
					{
						is=true;
						index=el;
						break;
					}
				}

				if ( is )
				{
					materialsid.PushBack( index );
				}
				else
				{
					materialsid.PushBack( materials.Size() );
					materials.PushBack( matName );
				}
			}
		}
		buildData.m_materialNames.Clear();
		buildData.m_materialNames.Reserve( materials.Size() );
		for ( Uint32 i=0; i<materials.Size(); i++ )
		{
			buildData.m_materialNames.PushBack( materials[i] );
		}
	}

	void ImportRigs( const TDynArray< ReFileMesh >& meshes, CPhysicsDestructionResource::FactoryInfo& buildData, TDynArray< TDynArray<Int32> >& rigMapping )
	{
		rigMapping.Resize( meshes.Size() );
		Uint32 meshesSize = meshes.Size();

		for ( Uint32 m = 0; m < meshesSize; m++ )
		{
			const ReFileMesh& mesh = meshes[ m ];
			Int32 numRigMatrices = mesh.getNumRigMatrices();
			if( numRigMatrices > 0 )
			{
				rigMapping[m].Resize( numRigMatrices );

				for ( Int32 i=0; i < numRigMatrices; i++ )
				{
					//check if its already there
					Int32 indexInArray = 0;
					Bool alreadyAdded = false;

					for ( Int32 j=0; j < buildData.m_boneNames.SizeInt(); j++ )
					{
						if ( strcmp( UNICODE_TO_ANSI( buildData.m_boneNames[j].AsChar() ), mesh.getRigMatrix(i).getName().getData() ) == 0 )
						{
							alreadyAdded = true;
							indexInArray = j;
							break;
						}
					}

					if ( !alreadyAdded )
					{
						indexInArray = buildData.m_boneNames.Size();

						buildData.m_boneNames.PushBack( CName( ANSI_TO_UNICODE( mesh.getRigMatrix(i).getName().getData() ) ) );
						buildData.m_boneVertexEpsilons.PushBack( 0.0f );

						Matrix* newRigMatrix = ::new ( buildData.m_boneRigMatrices ) Matrix;

						newRigMatrix->SetIdentity();

						Vector xx;
						Vector yy;
						Vector zz;

						xx.SetX( (Float)meshes[m].mRigMatrices[i].mRigMatrix[0] );
						xx.SetY( (Float)meshes[m].mRigMatrices[i].mRigMatrix[1] );
						xx.SetZ( (Float)meshes[m].mRigMatrices[i].mRigMatrix[2] );

						yy.SetX( (Float)meshes[m].mRigMatrices[i].mRigMatrix[3] );
						yy.SetY( (Float)meshes[m].mRigMatrices[i].mRigMatrix[4] );
						yy.SetZ( (Float)meshes[m].mRigMatrices[i].mRigMatrix[5] );

						zz.SetX( (Float)meshes[m].mRigMatrices[i].mRigMatrix[6] );
						zz.SetY( (Float)meshes[m].mRigMatrices[i].mRigMatrix[7] );
						zz.SetZ( (Float)meshes[m].mRigMatrices[i].mRigMatrix[8] );

						newRigMatrix->Set33( xx,yy,zz );

						newRigMatrix->SetTranslation(
							(Float)meshes[m].mRigMatrices[i].mRigMatrix[9]  * SCALE,
							(Float)meshes[m].mRigMatrices[i].mRigMatrix[10] * SCALE,
							(Float)meshes[m].mRigMatrices[i].mRigMatrix[11] * SCALE
							);
					}
					rigMapping[m][i] = indexInArray;
				}
			}
		}
	}

	void ImportCollisions( const TDynArray< ReFileCollisionConvex >& trimeshes, CMesh* existingMesh, const IImporter::ImportOptions& options, CPhysicsDestructionResource::FactoryInfo& buildData )
	{
		for ( Uint32 i=0; i<trimeshes.Size(); ++i )
		{
			const ReFileCollisionConvex& trimesh = trimeshes[i];

			StringAnsi str( trimesh.getCollisionName().getData() );

			if ( str.EndsWith( STR_NAV ) )
			{
#ifndef NO_OBSTACLE_MESH_DATA
				Int32 numv = trimesh.getNumVertices();

				Box bbox( Box::RESET_STATE );
				TDynArray< Vector2 > vertsInput;
				TDynArray< Vector2 > verts;
				vertsInput.Resize( numv );

				// compute bbox z bounding
				for ( Int32 i = 0; i < numv; ++i )
				{
					float x = trimesh.mVertices[(i*3)+0] * SCALE;
					float y = trimesh.mVertices[(i*3)+1] * SCALE;
					float z = trimesh.mVertices[(i*3)+2] * SCALE;

					vertsInput[ i ].Set( x,y );
					bbox.AddPoint( Vector3( x, y, z ) );
				}

				MathUtils::GeometryUtils::ComputeConvexHull2D( vertsInput, verts );
				buildData.m_navigationObstacle.PushShape( Move( verts ), bbox );
#endif
			}
			else
			{
				buildData.m_importCollision = true;
				buildData.m_buildConvexCollision = false;

				int numv = trimesh.getNumVertices();
				int numf = trimesh.getNumFaces();

				CMesh::CollisionMesh* newColMesh = new ( buildData.m_collisionMeshes ) CMesh::CollisionMesh;
				if( str.EndsWith( STR_COLL ) )
				{
					newColMesh->m_type = CMesh::CollisionMesh::ECMT_Convex;

					if ( trimesh.getNumMaterials() > 1 )
					{
						DATA_HALT( DES_Major, existingMesh, TXT("Mesh collision import"), TXT("Convex can have only one physical material stored, only the first define will be used! Path: %s "), options.m_sourceFilePath );
					}

					String materialName = ANSI_TO_UNICODE( trimesh.mMaterials->getData() );// ??
					materialName.MakeLower();
					newColMesh->m_physicalMaterials.PushBack( CName( materialName ) );
				}
				else
				{
					if( trimesh.getNumMaterials() < 0 )
					{
						DATA_HALT( DES_Major, existingMesh, TXT("Mesh import"), TXT("Mesh can't have negative amount of materials Path: %s "), options.m_sourceFilePath );
					}
					if( trimesh.getNumMaterials() > MAXINT16 )
					{
						DATA_HALT( DES_Major, existingMesh, TXT("Mesh import"), TXT("Maximum amount of materials exceeded Path: %s "), options.m_sourceFilePath );
					}

					newColMesh->m_type = CMesh::CollisionMesh::ECMT_Trimesh;
					for( Uint16 i = 0; i != trimesh.getNumMaterials(); ++i )
					{
						String materialName = ANSI_TO_UNICODE( trimesh.mMaterials[ i ].getData() );
						materialName.MakeLower();
						newColMesh->m_physicalMaterials.PushBack( CName( materialName ) );
					}
				}

				newColMesh->m_vertices.Resize( numv );
				Vector* writeVertex = &newColMesh->m_vertices[0];
				for( Int32 v=0; v<numv; v++, writeVertex++ )
				{
					float x = trimesh.mVertices[(v*3)+0];
					float y = trimesh.mVertices[(v*3)+1];
					float z = trimesh.mVertices[(v*3)+2];
					writeVertex->X = x * SCALE;
					writeVertex->Y = y * SCALE;
					writeVertex->Z = z * SCALE;
				}

				newColMesh->m_indices.Resize( numf*3 );
				newColMesh->m_physicalMaterialIndexes.Resize( numf );

				Uint32* writeIndex = &newColMesh->m_indices[0];
				for( Int32 k=0; k<numf; k++, writeIndex+=3 )
				{
					int x = trimesh.mIndices[ (k*4)+0 ];
					int y = trimesh.mIndices[ (k*4)+1 ];
					int z = trimesh.mIndices[ (k*4)+2 ];
					int mat = trimesh.mIndices[ (k*4)+3 ];
					newColMesh->m_physicalMaterialIndexes[ k ] = static_cast<Uint16>( mat );
					writeIndex[0] = x;
					writeIndex[1] = y;
					writeIndex[2] = z;
				}
			}
		}
	}

	void ImportBoxes( const TDynArray< ReFileCollisionBox >& boxes, CPhysicsDestructionResource::FactoryInfo& buildData )
	{
		for ( Uint32 i=0; i<boxes.Size(); ++i )
		{
			const ReFileCollisionBox& box = boxes[ i ];
			Matrix pose(
				Vector( box.mTransformMatrix[ 0 ],			box.mTransformMatrix[ 1 ],			box.mTransformMatrix[ 2 ], 0.0f ),
				Vector( box.mTransformMatrix[ 3 ],			box.mTransformMatrix[ 4 ],			box.mTransformMatrix[ 5 ], 0.0f ),
				Vector( box.mTransformMatrix[ 6 ],			box.mTransformMatrix[ 7 ],			box.mTransformMatrix[ 8 ], 0.0f ),
				Vector( box.mTransformMatrix[ 9 ] * SCALE,	box.mTransformMatrix[ 10 ] * SCALE,	box.mTransformMatrix[ 11 ] * SCALE )
				);
			pose.V[ 0 ].Normalize3();
			pose.V[ 1 ].Normalize3();
			pose.V[ 2 ].Normalize3();

			Vector halfExtends = Vector( box.getLength() * SCALE, box.getWidth() * SCALE, box.getHeight() * SCALE ) / 2;

			StringAnsi str( box.getCollisionName().getData() );

			if ( str.EndsWith( STR_NAV ) )
			{
#ifndef NO_OBSTACLE_MESH_DATA

				Vector v[ 8 ];
				v[ 0 ].Set3(  halfExtends.X,  halfExtends.Y,  halfExtends.Z );
				v[ 1 ].Set3( -halfExtends.X,  halfExtends.Y,  halfExtends.Z );
				v[ 2 ].Set3( -halfExtends.X, -halfExtends.Y,  halfExtends.Z );
				v[ 3 ].Set3(  halfExtends.X, -halfExtends.Y,  halfExtends.Z );
				v[ 4 ].Set3(  halfExtends.X,  halfExtends.Y, -halfExtends.Z );
				v[ 5 ].Set3( -halfExtends.X,  halfExtends.Y, -halfExtends.Z );
				v[ 6 ].Set3( -halfExtends.X, -halfExtends.Y, -halfExtends.Z );
				v[ 7 ].Set3(  halfExtends.X, -halfExtends.Y, -halfExtends.Z );

				Box naviBBox( Box::RESET_STATE );

				for ( Uint32 i = 0; i < 8; ++i )
				{
					v[ i ] = pose.TransformPoint( v[ i ] );
					naviBBox.AddPoint( v[ i ] );
				}

				TDynArray< Vector2 > input( 8 );
				TDynArray< Vector2 > convex;

				for ( Uint32 i = 0; i < 8; ++i )
				{
					input[ i ] = v[ i ].AsVector2();
				}

				MathUtils::GeometryUtils::ComputeConvexHull2D( input, convex );
				buildData.m_navigationObstacle.PushShape( Move( convex ), naviBBox );
#endif
			}
			else
			{
				buildData.m_importCollision = true;
				buildData.m_buildConvexCollision = false;
				CMesh::CollisionMesh* newColMesh = new ( buildData.m_collisionMeshes ) CMesh::CollisionMesh;
				newColMesh->m_type = CMesh::CollisionMesh::ECMT_Box;

				newColMesh->m_physicalMaterials.Resize( 1 );
				newColMesh->m_physicalMaterials[ 0 ] = CName( ANSI_TO_UNICODE( box.getMaterialName().getData() ) );
				newColMesh->m_vertices.Resize( 5 );
				newColMesh->m_vertices[ 0 ] = pose.V[ 0 ];
				newColMesh->m_vertices[ 1 ] = pose.V[ 1 ];
				newColMesh->m_vertices[ 2 ] = pose.V[ 2 ];
				newColMesh->m_vertices[ 3 ] = pose.V[ 3 ];
				newColMesh->m_vertices[ 4 ] = halfExtends;
			}
		}
	}

	void ImportSpheres( const TDynArray< ReFileCollisionSphere >& spheres, CPhysicsDestructionResource::FactoryInfo& buildData )
	{
		for ( Uint32 i=0; i<spheres.Size(); ++i )
		{
			const ReFileCollisionSphere& sphere = spheres[ i ];

			buildData.m_importCollision = true;
			buildData.m_buildConvexCollision = false;
			CMesh::CollisionMesh* newColMesh = new ( buildData.m_collisionMeshes ) CMesh::CollisionMesh;
			newColMesh->m_type = CMesh::CollisionMesh::ECMT_Sphere;
			newColMesh->m_physicalMaterials.PushBack( CName( ANSI_TO_UNICODE( sphere.getMaterialName().getData() ) ) );
			newColMesh->m_vertices.PushBack( Vector( sphere.mTransformMatrix[ 0 ], sphere.mTransformMatrix[ 1 ], sphere.mTransformMatrix[ 2 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( sphere.mTransformMatrix[ 3 ], sphere.mTransformMatrix[ 4 ], sphere.mTransformMatrix[ 5 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( sphere.mTransformMatrix[ 6 ], sphere.mTransformMatrix[ 7 ], sphere.mTransformMatrix[ 8 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( sphere.mTransformMatrix[ 9 ] * SCALE, sphere.mTransformMatrix[ 10 ] * SCALE, sphere.mTransformMatrix[ 11 ] * SCALE ) );
			newColMesh->m_vertices.PushBack( Vector( sphere.getRadius() * SCALE, sphere.getRadius() * SCALE, sphere.getRadius() * SCALE ) );
		}
	}

	void ImportCapsules( const TDynArray< ReFileCollisionCapsule >& capsules, CPhysicsDestructionResource::FactoryInfo& buildData )
	{
		for ( Uint32 i=0; i<capsules.Size(); ++i)
		{
			const ReFileCollisionCapsule& capsule = capsules[ i ];

			buildData.m_importCollision = true;
			buildData.m_buildConvexCollision = false;
			CMesh::CollisionMesh* newColMesh = new ( buildData.m_collisionMeshes ) CMesh::CollisionMesh;
			newColMesh->m_type = CMesh::CollisionMesh::ECMT_Capsule;
			newColMesh->m_physicalMaterials.PushBack( CName( ANSI_TO_UNICODE( capsule.getMaterialName().getData() ) ) );
			newColMesh->m_vertices.PushBack( Vector( capsule.mTransformMatrix[ 0 ], capsule.mTransformMatrix[ 1 ], capsule.mTransformMatrix[ 2 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( capsule.mTransformMatrix[ 3 ], capsule.mTransformMatrix[ 4 ], capsule.mTransformMatrix[ 5 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( capsule.mTransformMatrix[ 6 ], capsule.mTransformMatrix[ 7 ], capsule.mTransformMatrix[ 8 ], 0.0f ) );
			newColMesh->m_vertices.Back().Normalize3();
			newColMesh->m_vertices.PushBack( Vector( capsule.mTransformMatrix[ 9 ] * SCALE, capsule.mTransformMatrix[ 10 ] * SCALE, capsule.mTransformMatrix[ 11 ] * SCALE ) );
			newColMesh->m_vertices.PushBack( Vector( capsule.getRadius() * SCALE, capsule.getRadius() * SCALE, capsule.getHeight() * SCALE ) );
		}
	}

	void ImportChunks( const TDynArray< ReFileMesh >& meshes, const SLODPreset& lodPreset, const TDynArray< Int32 >& materialsid, const TDynArray< TDynArray<Int32> >& rigMapping, CPhysicsDestructionResource::FactoryInfo& buildData )
	{
		Int32 Index = 0;
		Uint32 meshesSize = meshes.Size();
		for ( Uint32 i=0; i < meshesSize; i++ )
		{
			const ReFileMesh& mesh = meshes[i];
			Int32 numChunks = mesh.getNumChunks();

			if ( numChunks == 0 )
			{
				continue;
			}

			CMesh::LODLevel* newLevel = new ( buildData.m_lodLevels ) CMesh::LODLevel;

			if ( i > 0 && i-1 < lodPreset.m_definitions.Size() )
			{
				// found LOD distance in previous data - keep it
				newLevel->m_meshTypeLOD.m_distance = lodPreset.m_definitions[ i-1 ].m_distance;
			}
			else
			{
				newLevel->m_meshTypeLOD.m_distance = CMeshTypeResource::GetDefaultLODDistance( i );
			}

			RED_ASSERT( numChunks <= MAXINT16, TXT("too many chunks in imported mesh, data will be broken") );

			for ( Int32 c=0; c<numChunks; c++ )
			{
				newLevel->m_chunks.PushBack( static_cast<Uint16>( buildData.m_chunks.Size() ) );
				SMeshChunk* newChunk = new ( buildData.m_chunks ) SMeshChunk;
				newChunk->m_materialID = materialsid[Index];Index++;
				newChunk->m_numBonesPerVertex = 0;
				newChunk->m_numVertices = mesh.getMeshChunk( c ).getNumVertices();
				newChunk->m_numIndices  = mesh.getMeshChunk( c ).getNumFaces() * 3; // TODO ?? why * 3 ??

				// In W3 we support only 4 bones skinning so please
				// use ReFileFatVertex with care regarding getExtra4Weights and getExtra4BoneIndices
				ReFileFatVertex* vert = mesh.getMeshChunk(c).mVertices;
				ReFileFace* faces = mesh.getMeshChunk( c ).mFaces;

				for ( Int32 v=0; v < mesh.getMeshChunk( c ).getNumVertices(); v++ )
				{
					SMeshVertex* writeVertex = new ( newChunk->m_vertices ) SMeshVertex;

					writeVertex->m_position[0] = vert[v].getPosition().x * SCALE;
					writeVertex->m_position[1] = vert[v].getPosition().y * SCALE;
					writeVertex->m_position[2] = vert[v].getPosition().z * SCALE;
					writeVertex->m_normal[0] = vert[v].getNormal().x;
					writeVertex->m_normal[1] = vert[v].getNormal().y;
					writeVertex->m_normal[2] = vert[v].getNormal().z;

					BYTE ccc[4];
					ccc[0]=(BYTE)(vert[v].getColor().x * 255);
					ccc[1]=(BYTE)(vert[v].getColor().y * 255);
					ccc[2]=(BYTE)(vert[v].getColor().z * 255);
					ccc[3]=(BYTE)(vert[v].getColor().w * 255);
					int cc=*((int*)ccc);

					writeVertex->m_color = cc;
					writeVertex->m_tangent[0] = vert[v].getTangent().x;
					writeVertex->m_tangent[1] = vert[v].getTangent().y;
					writeVertex->m_tangent[2] = vert[v].getTangent().z;
					writeVertex->m_binormal[0] = vert[v].getBinormal().x;
					writeVertex->m_binormal[1] = vert[v].getBinormal().y;
					writeVertex->m_binormal[2] = vert[v].getBinormal().z;

					writeVertex->m_uv0[0] = vert[v].getUV1().x;
					writeVertex->m_uv0[1] = vert[v].getUV1().y;

					writeVertex->m_uv1[0] = vert[v].getUV2().x;
					writeVertex->m_uv1[1] = vert[v].getUV2().y;

					writeVertex->m_weights[0] = vert[v].getWeights().x;
					writeVertex->m_weights[1] = vert[v].getWeights().y;
					writeVertex->m_weights[2] = vert[v].getWeights().z;
					writeVertex->m_weights[3] = vert[v].getWeights().w;

					if ( rigMapping[i].Size() )
					{
						writeVertex->m_indices[0] = (Uint8)rigMapping[i][(Int32)vert[v].getBoneIndices().x ];
						writeVertex->m_indices[1] = (Uint8)rigMapping[i][(Int32)vert[v].getBoneIndices().y ];
						writeVertex->m_indices[2] = (Uint8)rigMapping[i][(Int32)vert[v].getBoneIndices().z ];
						writeVertex->m_indices[3] = (Uint8)rigMapping[i][(Int32)vert[v].getBoneIndices().w ];
					}
					else
					{
						writeVertex->m_indices[0] = 0;
						writeVertex->m_indices[1] = 0;
						writeVertex->m_indices[2] = 0;
						writeVertex->m_indices[3] = 0;
					}
				}

				for ( Int32 ff=0; ff < mesh.getMeshChunk( c ).getNumFaces(); ff++ )
				{
					Uint32 x = mesh.getMeshChunk( c ).mFaces[ff].mIndices[0];
					Uint32 y = mesh.getMeshChunk( c ).mFaces[ff].mIndices[1];
					Uint32 z = mesh.getMeshChunk( c ).mFaces[ff].mIndices[2];
					newChunk->m_indices.PushBack( (Uint16)z );
					newChunk->m_indices.PushBack( (Uint16)y );
					newChunk->m_indices.PushBack( (Uint16)x );
				}

				if ( buildData.m_boneNames.Size()>0 )
				{
					newChunk->m_numBonesPerVertex = 4;
					newChunk->m_vertexType = MVT_DestructionMesh;
				}
				else
				{
					newChunk->m_numBonesPerVertex = 0;
					newChunk->m_vertexType = MVT_StaticMesh;
				}
			}
		}
	}
} // namespace

CResource* CDestructionImporter::DoImport( const ImportOptions& options )
{
	CPhysicsDestructionResource* existingMesh = Cast< CPhysicsDestructionResource >( options.m_existingResource );

	SLODPreset lodPreset;
#ifndef NO_EDITOR
	TDynArray< Float >	storedCollisionDensityScalers;
	Float				storedCollisionOcclusionAttenuation = -1.f;
	Float				storedCollisionOcclusionDiagonalLimit = -1.f;
#endif

	if ( existingMesh )
	{
		lodPreset = DeductLODsPresetFromMesh( existingMesh );

#ifndef NO_EDITOR
		if( const CCollisionMesh* collMesh = existingMesh->GetCollisionMesh() )
		{
			storedCollisionDensityScalers = collMesh->GetDensityScalers();
			storedCollisionOcclusionAttenuation = collMesh->GetOcclusionAttenuation();
			storedCollisionOcclusionDiagonalLimit = collMesh->GetOcclusionDiagonalLimit();
		}
#endif
	}

	SaveObjectConfig( TXT("User") );

	const char* path = UNICODE_TO_ANSI( options.m_sourceFilePath.AsChar() );
	ReFile reFile = ReFileLoader::OpenFile( path );
	if ( !reFile )
	{
		return nullptr;
	}

	String author;
	String source;
	String baseResourceFilePath;

	TDynArray< ReFileMesh >				meshes;
	TDynArray< ReFileCollisionConvex >	trimeshes;
	TDynArray< ReFileCollisionBox >		boxes;
	TDynArray< ReFileCollisionSphere >	spheres;
	TDynArray< ReFileCollisionCapsule >	capsules;

	ReFileHeader2 hdr;
	if( !ReFileLoader::Read( reFile, hdr ) ) 
	{
		return nullptr;
	}

	const ReFileString* currentVersionPtr = &hdr.getReFileVersion();
	for ( int i = 0; i < reFile.mHeaders.size(); ++i )
	{
		const ReFileArchiveHeader& header = reFile.mHeaders[i];

		if ( header.mType == 'mesh' )
		{
			meshes.PushBack( ReFileMesh() );
			ReFileLoader::Read( reFile.mFile, header, meshes.Back(), currentVersionPtr );
		}
		else if ( header.mType == 'cl00' )
		{
			trimeshes.PushBack( ReFileCollisionConvex() );
			ReFileLoader::Read( reFile.mFile, header, trimeshes.Back(), currentVersionPtr );
		}
		else if ( header.mType == 'hedr' )
		{
			RED_HALT( "This file is not supported because of header file type. Export this file again. File: %ls", options.m_sourceFilePath.AsChar() );
			return nullptr;
		}
		else if ( header.mType == 'hed2' )
		{
			ReFileHeader2 hdr;
			ReFileLoader::Read( reFile.mFile, header, hdr, currentVersionPtr );
			author = ANSI_TO_UNICODE( hdr.getAuthor().getData() );
			source = ANSI_TO_UNICODE( hdr.getSourceFilePath().getData() );
			baseResourceFilePath = ANSI_TO_UNICODE( hdr.getSourceFilePath().getData() );
		}
		else if ( header.mType == 'pbox' )
		{
			boxes.PushBack( ReFileCollisionBox() );
			ReFileLoader::Read( reFile.mFile, header, boxes.Back(), currentVersionPtr );
		}
		else if ( header.mType == 'psph' )
		{
			spheres.PushBack( ReFileCollisionSphere() );
			ReFileLoader::Read( reFile.mFile, header, spheres.Back(), currentVersionPtr );
		}
		else if ( header.mType =='pcap' )
		{
			capsules.PushBack( ReFileCollisionCapsule() );
			ReFileLoader::Read( reFile.mFile, header, capsules.Back(), currentVersionPtr );
		}
	}

	CPhysicsDestructionResource::FactoryInfo buildData;
	buildData.m_parent = options.m_parentObject;
	buildData.m_reuse = existingMesh;
	buildData.m_authorName = author;
	buildData.m_baseResourceFilePath = baseResourceFilePath;
	buildData.m_reuseMesh    = options.m_params && static_cast< SMeshImporterParams* >( options.m_params )->m_reuseMesh;
	buildData.m_reuseVolumes = options.m_params && static_cast< SMeshImporterParams* >( options.m_params )->m_reuseVolumes;
	Bool regenerateVolumes = options.m_params &&  static_cast< SMeshImporterParams* >( options.m_params )->m_regenerateVolumes;

	if ( existingMesh )
	{
		buildData.m_entityProxy = existingMesh->IsEntityProxy();
	}

	RED_ASSERT( !( regenerateVolumes && buildData.m_reuseVolumes ), TXT("Cannot use both reuse and regenerate volumes options together") );

	// Try to figure out existing collision mesh type from the first shape (all meshes we generate use a single shape)
	enum { ECMT_BOX, ECMT_SPHERE, ECMT_CAPSULE, ECMT_CONVEX, ECMT_TRIMESH } existingCollisionMeshType = ECMT_BOX;
	if ( regenerateVolumes && existingMesh->GetCollisionMesh() != nullptr && !existingMesh->GetCollisionMesh()->GetShapes().Empty() )
	{
		// We check the name since the types are not exposed
		String clsName = existingMesh->GetCollisionMesh()->GetShapes()[0]->GetClass()->GetName().AsString();
		if ( clsName.ContainsSubstring( TXT("Sphere") ) )
		{
			existingCollisionMeshType = ECMT_SPHERE;
		}
		else if ( clsName.ContainsSubstring( TXT("Capsule") ) )
		{
			existingCollisionMeshType = ECMT_CAPSULE;
		}
		else if ( clsName.ContainsSubstring( TXT("Convex") ) )
		{
			existingCollisionMeshType = ECMT_CONVEX;
		}
		else if ( clsName.ContainsSubstring( TXT("TriMesh") ) )
		{
			existingCollisionMeshType = ECMT_TRIMESH;
		}
		else
		{
			RED_HALT( "Failed to figure out existing collision mesh type from shape class name '%ls'", clsName.AsChar() );
		}
	}

	if ( !buildData.m_reuseMesh )
	{
		TDynArray< Int32 > materialsid;
		TDynArray< TDynArray<Int32> > rigMapping;

		ImportMaterials( meshes, buildData, materialsid );
		ImportRigs( meshes, buildData, rigMapping );
		ImportChunks( meshes, lodPreset, materialsid, rigMapping, buildData );
	}

	if ( !buildData.m_reuseVolumes )
	{
		ImportCollisions( trimeshes, existingMesh, options, buildData );
		ImportBoxes( boxes, buildData );
		ImportSpheres( spheres, buildData );
		ImportCapsules( capsules, buildData );
	}

	// NOTE: The 'newMesh' is either an actually created mesh, if 'existingMesh' is null, or just the 'existingMesh' if it's not null.
	// In either case, to prevent a resource leak, we have to pass it outside - so not early returns with 'nullptr' result below
	// this point are allowed.
	CPhysicsDestructionResource* newMesh = CPhysicsDestructionResource::Create( buildData );

	if ( existingMesh && newMesh )
	{
		// Re-build some stuff based on the previous mesh on re-importing
#ifndef NO_EDITOR
		if ( newMesh->GetCollisionMesh() == nullptr && regenerateVolumes )
		{
			switch ( existingCollisionMeshType )
			{
			case ECMT_BOX:
				newMesh->AddBoxCollision();
				break;
			case ECMT_SPHERE:
				newMesh->AddSphereCollision();
				break;
			case ECMT_CAPSULE:
				newMesh->AddCapsuleCollision();
				break;
			case ECMT_CONVEX:
				newMesh->AddConvexCollision();
				break;
			case ECMT_TRIMESH:
				newMesh->AddTriMeshCollision();
				break;
			default: ;
			}
		}

		if ( newMesh->GetCollisionMesh() && !buildData.m_reuseVolumes )
		{
			const_cast< CCollisionMesh* >( newMesh->GetCollisionMesh() )->FillDensityScalers( storedCollisionDensityScalers );
			if ( storedCollisionOcclusionAttenuation != -1.f && storedCollisionOcclusionDiagonalLimit != -1. )
			{
				const_cast< CCollisionMesh* >( newMesh->GetCollisionMesh() )->SetOcclusionDiagonalLimit( storedCollisionOcclusionDiagonalLimit );
				const_cast< CCollisionMesh* >( newMesh->GetCollisionMesh() )->SetOcclusionAttenuation( storedCollisionOcclusionAttenuation );
			}
		}
#endif

#ifdef USE_SIMPLYGON
		if ( newMesh->GetNumLODLevels() == 1 && !buildData.m_reuseMesh ) // if there are LODs defined in the source, keep them untouched; also, don't bother if we are keeping the mesh
		{
			// Re-create LODs
			String errorMsg;
			Bool success = GenerateLODsForMesh( newMesh, lodPreset, errorMsg, 
				[ ]( const String& msg ) { 
					// no messages are shown for now
			} );
		}
#endif
	}

	if (  newMesh )
	{
		// We can't have a no collision destruction - it will cause crashes
#ifndef NO_EDITOR
		if ( newMesh->GetCollisionMesh() == nullptr )
		{
			GFeedback->ShowMsg( TXT("RE file warning."), TXT("Destruction do not have collision, importing it will cause crashes. Delete and fix it first! \nPath: %s "), options.m_sourceFilePath.AsChar() );
		}
#endif // !NO_EDITOR
	}
	if ( m_autoRemoveUnusedBones )
	{
		const Uint32 prevBoneCount = newMesh->GetBoneCount();

		newMesh->RemoveUnusedBones();
	}

	return newMesh;
}