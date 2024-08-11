/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fbxCommon.h"
#include "fbxMeshBuilder.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/material.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/texture.h"
#include "../../common/engine/textureArray.h"
#include "../../common/engine/collisionMesh.h"

#define MAX_LOD_INDEX 6

class CFBXMeshImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CFBXMeshImporter, IImporter, 0 );

public:
	Bool			m_importAsSkinnedMesh;
	Bool			m_generateCollisionIfMissing;

public:
	FbxManager*		m_manager;

public:
	CFBXMeshImporter();
	virtual CResource* DoImport( const ImportOptions& options );

private:
	void WalkNodes(const FbxNode* node, FBXMeshBuilder& builder);
	void CreateMeshChunks(const Uint32 numBones, const Uint32 pointCount, const FBXMeshBuilder::Triangle* triList, CMesh::LODLevel& outLevel, CMesh::FactoryInfo& outInfo);
	void ImportXMLCollisionData( CMesh* retMesh, CXMLReader& xmlReader );
	void ImportXMLMaterialsData( CMesh* retMesh, CXMLReader& xmlReader );
};

BEGIN_CLASS_RTTI( CFBXMeshImporter )
	PARENT_CLASS(IImporter)
	PROPERTY_EDIT(m_importAsSkinnedMesh, TXT("Create fake bones and skinning"));
	PROPERTY_EDIT(m_generateCollisionIfMissing, TXT("Generate collision if missing"));
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS(CFBXMeshImporter);

CFBXMeshImporter::CFBXMeshImporter()
{
	m_resourceClass = ClassID< CMesh >();
	m_formats.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk FBX") ) );
	m_importAsSkinnedMesh = false;
	m_generateCollisionIfMissing = false;
}

static void ReadXmlAttribute( CXMLReader& reader, const String& attrName, Float& retVal )
{
	String val;
	if ( reader.Attribute( attrName, val ) )
	{
		Float tmpVal = 0.f;
		if ( FromString( val, tmpVal ) && tmpVal >= 0.f )
		{
			retVal = tmpVal;
		}
	}
}

static void ReadXmlAttribute( CXMLReader& reader, const String& attrName, Bool& retVal )
{
	String val;
	if ( reader.Attribute( attrName, val ) )
	{
		Bool tmpVal = false;
		if ( FromString( val, tmpVal ) )
		{
			retVal = tmpVal;
		}
	}
}

CResource* CFBXMeshImporter::DoImport( const ImportOptions& options )
{
	// Load scene
	FBXImportScene* scene = FBXImportScene::Load(options.m_sourceFilePath);
	if ( !scene )
	{
		RED_LOG_ERROR( CNAME( CFBXMeshImporter ), TXT("Unable to load scene from '%s'"), options.m_sourceFilePath.AsChar() );
		return nullptr;
	}

	// Walk the input nodes
	CMesh::FactoryInfo initInfo;
	initInfo.m_buildConvexCollision = false;
	initInfo.m_parent = options.m_parentObject;
	initInfo.m_reuse = Cast<CMesh>(options.m_existingResource);
	//initInfo.m_lodDegenerationStartDistance = -1.0f;

	// Walk the nodes - collecting meshes
	FBXMeshBuilder builder;
	FbxNode* rootNode = scene->m_scene->GetRootNode();
	WalkNodes(rootNode, builder);

	// No data imported
	if ( builder.m_lods.Empty() )
	{
		RED_LOG_ERROR( CNAME( CFBXMeshImporter ), TXT("No geometry data in file '%s'"), options.m_sourceFilePath.AsChar() );
		return nullptr;
	}

	// Automatic LOD distances
	Float lodDistances[MAX_LOD_INDEX+1] = { 0.0f, 10.0f, 20.0f, 30.0f, 40.0f, 80.0f, 120.0f };
	Float autohideDistance = -1.0f;
	Bool useExtraStreams = false, isTwoSided = false, mergeInGlobalShadowMesh = true, entityProxy = false;

	String xmlContent;
	CFilePath xmlPath( options.m_sourceFilePath );
	xmlPath.SetExtension( TXT("xml") );
	GFileManager->LoadFileToString( xmlPath.ToString(), xmlContent, true );

	CXMLReader xmlReader( xmlContent );
	Uint32 lodCounter = 0;
	Bool xmlWellFormed = true;
	if ( !xmlReader.BeginNode( TXT("mesh") ) )
	{
		xmlWellFormed = false;
	}

	if ( xmlWellFormed && xmlReader.BeginNode( TXT("mesh_data") ) )
	{
		ReadXmlAttribute( xmlReader, TXT("autohideDistance"), autohideDistance );
		ReadXmlAttribute( xmlReader, TXT("isTwoSided"), isTwoSided );
		ReadXmlAttribute( xmlReader, TXT("useExtraStreams"), useExtraStreams );
		ReadXmlAttribute( xmlReader, TXT("mergeInGlobalShadowMesh"), mergeInGlobalShadowMesh );
		ReadXmlAttribute( xmlReader, TXT("entityProxy"), entityProxy );

		if ( xmlReader.BeginNode( TXT("LODs") ) )
		{
			while ( xmlReader.BeginNextNode() )
			{
				String nodeName;
				if ( xmlReader.GetNodeName( nodeName ) && nodeName.EqualsNC( TXT("LOD_info") ) && lodCounter <= MAX_LOD_INDEX )
				{
					ReadXmlAttribute( xmlReader, TXT("distance"), lodDistances[ lodCounter++ ] );
				}
				xmlReader.EndNode();
			}
			xmlReader.EndNode();
		}
		xmlReader.EndNode();
	}

	// Copy material lists
	for (Uint32 i=0; i<builder.m_materials.Size(); ++i)
	{
		const FBXMeshBuilder::Material& m = builder.m_materials[i];
		initInfo.m_materialNames.PushBack(m.m_name);
	}

	// Copy bones
	RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Found %d bones"), builder.m_bones.Size() );
	TDynArray< Uint32 > rigMapping( builder.m_bones.Size() );
	for (Uint32 i=0; i<builder.m_bones.Size(); ++i)
	{
		const FBXMeshBuilder::Bone& b = builder.m_bones[i];

		initInfo.m_boneNames.PushBack( CName( b.m_name.AsChar() ) );

		Matrix t = b.m_localToWorld;
		t.V[0].Normalize3();
		t.V[1].Normalize3();
		t.V[2].Normalize3();		

		initInfo.m_boneRigMatrices.PushBack( t.FullInverted() );
		initInfo.m_boneVertexEpsilons.PushBack( 0.0f );
	}

	// Copy collision data
	RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Found %d collision shapes"), builder.m_collision.Size() );
	for (Uint32 i=0; i<builder.m_collision.Size(); ++i)
	{
		const FBXMeshBuilder::CollisionMesh* c = builder.m_collision[i];

		CMesh::CollisionMesh* outMesh = new (initInfo.m_collisionMeshes) CMesh::CollisionMesh;
		switch ( c->m_collisionType )
		{
		case FBXMeshBuilder::CollisionMesh::CT_Convex:
			outMesh->m_type = CMesh::CollisionMesh::ECMT_Convex;
			break;

		case FBXMeshBuilder::CollisionMesh::CT_Trimesh:
		default:
			outMesh->m_type = CMesh::CollisionMesh::ECMT_Trimesh;
			break;
		}

		outMesh->m_vertices = c->m_vertices;
		outMesh->m_indices = c->m_indices;
		outMesh->m_physicalMaterials.PushBack( c->m_materialNames );
		outMesh->m_physicalMaterialIndexes.PushBack( c->m_materialIndexes );
	}
	initInfo.m_importCollision = !builder.m_collision.Empty() || m_generateCollisionIfMissing;

	// Build the chunks
	Uint32 realLodIndex = 0;
	for (Uint32 lodIndex=0; lodIndex<=MAX_LOD_INDEX; ++lodIndex)
	{
		// Get lod data
		FBXMeshBuilder::LODLevel* lod = builder.GetLodLevel(lodIndex);
		if (lod->m_triangles.Empty())
		{
			continue;
		}

		// Create LOD level
		CMesh::LODLevel* outLevel = new (initInfo.m_lodLevels) CMesh::LODLevel;
		outLevel->m_meshTypeLOD.m_distance = lodDistances[realLodIndex];

		// Build chunks
		lod->GenerateConnectivity();

		// Update tangent space vectors (only triangles without valid tangent space)
		lod->CalculateNormals();
		lod->CalculateTangentSpace();

		// Build triangle lists
		lod->BuildTriangleLists();

		// Convert each triangle list into actual mesh chunks
		for (Uint32 i=0; i<lod->m_materialTriangles.Size(); ++i)
		{
			const FBXMeshBuilder::Triangle* triList = lod->m_materialTriangles[i];
			if (NULL != triList)
			{
				CreateMeshChunks(initInfo.m_boneNames.Size(), lod->m_points.GetNumPoints(), triList, *outLevel, initInfo);
			}
		}

		// Done
		RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("%d final chunks created for LOD%d"), outLevel->m_chunks.Size(), realLodIndex );
		realLodIndex += 1;
	}

	// Create mesh
	CMesh* retMesh = CMesh::Create( initInfo );
	retMesh->SetAutoHideDistance( autohideDistance );
	retMesh->SetIsTwoSided( isTwoSided );
	retMesh->SetUseExtractStreams( useExtraStreams );
	retMesh->SetMergeIntoGlobalShadowMesh( mergeInGlobalShadowMesh );
	retMesh->SetEntityProxy( entityProxy );

	if ( xmlWellFormed )
	{
		ImportXMLCollisionData( retMesh, xmlReader );
		ImportXMLMaterialsData( retMesh, xmlReader );
		xmlReader.EndNode(); //mesh
	}

	return retMesh;
}

void CFBXMeshImporter::ImportXMLCollisionData( CMesh* retMesh, CXMLReader& xmlReader )
{
	CCollisionMesh* collMesh = const_cast< CCollisionMesh* >( retMesh->GetCollisionMesh() );
	if ( collMesh &&  xmlReader.BeginNode( TXT("collision") ) )
	{
		Float tmpVal = -1.f;
		String stringVal;
		if ( xmlReader.Attribute( TXT("attenuation"), stringVal ) && FromString( stringVal, tmpVal ) )
		{
			collMesh->SetOcclusionAttenuation( tmpVal );
		}

		if ( xmlReader.Attribute( TXT("diagonalLimit"), stringVal ) && FromString( stringVal, tmpVal ) )
		{
			collMesh->SetOcclusionDiagonalLimit( tmpVal );
		}

		Int32 rotAxis = 0;
		if ( xmlReader.Attribute( TXT("swimmingRotationAxis"), stringVal ) && FromString( stringVal, rotAxis ) )
		{
			collMesh->SetSwimmingRotationAxis( rotAxis );
		}
		xmlReader.EndNode();
	}
}

void CFBXMeshImporter::ImportXMLMaterialsData( CMesh* retMesh, CXMLReader& xmlReader )
{

	if ( xmlReader.BeginNode( TXT("materials") ) )
	{
		TDynArray< String >& meshMatNames = retMesh->GetMaterialNames();
		CMeshTypeResource::TMaterials& meshMaterials = retMesh->GetMaterials();

		while ( xmlReader.BeginNextNode() )
		{
			String nodeName;
			if ( xmlReader.GetNodeName( nodeName ) && nodeName.EqualsNC( TXT("material") ) )
			{
				String matName;
				Int32 matInd = -1;
				if ( !xmlReader.Attribute( TXT("name"), matName ) || ( matInd = (Int32)meshMatNames.GetIndex( matName ) ) < 0 )
				{
					xmlReader.EndNode();
					continue;
				}

				IMaterial* mat = meshMaterials[ matInd ];

				Bool localInstance = true;
				String localInstanceString;
				if ( xmlReader.Attribute( TXT("local"), localInstanceString ) && FromString( localInstanceString, localInstance ) )
				{
					String baseMat;
					if ( xmlReader.Attribute( TXT("base"), baseMat ) )
					{
						// check if the base material is set properly
						CDiskFile* baseMaterialFile = mat->GetParent() == retMesh ? 
							( mat->GetBaseMaterial() ? mat->GetBaseMaterial()->GetFile() : nullptr ) : mat->GetFile();

						if ( !baseMaterialFile || baseMaterialFile->GetDepotPath() != baseMat )
						{
							// well, it's different than it should be, change it
							IMaterial* newMat = Cast< IMaterial >( GDepot->LoadResource( baseMat ) ).Get();
							if ( newMat )
							{
								mat = newMat;
								retMesh->SetMaterial( matInd, newMat );
								mat = meshMaterials[ matInd ];
							}
						}

						if ( localInstance )
						{
							if ( mat->GetParent() == nullptr )
							{
								CMaterialInstance* instance = new CMaterialInstance( retMesh, mat );
								retMesh->SetMaterial( matInd, instance );
								mat = instance;
							}

							// set all parameters...
							while ( xmlReader.BeginNextNode() )
							{
								if ( xmlReader.GetNodeName( nodeName ) && nodeName == TXT("param") )
								{
									String paramType, paramVal;
									// don't bother if we're missing param type or value
									if ( xmlReader.Attribute( TXT("type"), paramType ) && xmlReader.Attribute( TXT("value"), paramVal ) )
									{
										String paramName;
										if ( xmlReader.Attribute( TXT("name"), paramName ) )
										{
											if ( IRTTIType* type = SRTTI::GetInstance().FindType( CName( paramType ) ) )
											{
												void* paramData = RED_MEMORY_ALLOCATE_ALIGNED( MemoryPool_SmallObjects, MC_MaterialParameters, 
													type->GetSize(), DEFAULT_MATERIAL_PARAMETER_INSTANCE_ALIGNMENT );
												Red::System::MemorySet( paramData, 0, type->GetSize() );
												type->Construct( paramData );

												if ( type->FromString( paramData, paramVal ) && paramData )
												{
													mat->WriteParameterRaw( CName( paramName ), paramData );
												}

												type->Destruct( paramData );
												// Free memory allocated for parameter	
												RED_MEMORY_FREE( MemoryPool_SmallObjects, MC_MaterialParameters, paramData );
											}
										}
									}
								}
								xmlReader.EndNode();
							}
						}
					}
				}
			}
			xmlReader.EndNode();
		}
		xmlReader.EndNode();
	}

	retMesh->CreateRenderResource();
	CDrawableComponent::RecreateProxiesOfRenderableComponents();
}

void CFBXMeshImporter::WalkNodes(const FbxNode* node, FBXMeshBuilder& builder)
{
	// Get the local to parent matrix
	const FbxAMatrix matrix = ((FbxNode*)node)->EvaluateGlobalTransform(FBXSDK_TIME_INFINITE, FbxNode::eSourcePivot, false, true);
	Matrix localToWorld;
	localToWorld.SetIdentity();
	localToWorld.V[0].X = (Float)matrix.Get(0,0);
	localToWorld.V[0].Y = (Float)matrix.Get(0,1);
	localToWorld.V[0].Z = (Float)matrix.Get(0,2);
	localToWorld.V[1].X = (Float)matrix.Get(1,0);
	localToWorld.V[1].Y = (Float)matrix.Get(1,1);
	localToWorld.V[1].Z = (Float)matrix.Get(1,2);
	localToWorld.V[2].X = (Float)matrix.Get(2,0);
	localToWorld.V[2].Y = (Float)matrix.Get(2,1);
	localToWorld.V[2].Z = (Float)matrix.Get(2,2);
	localToWorld.V[3].X = (Float)matrix.Get(3,0);
	localToWorld.V[3].Y = (Float)matrix.Get(3,1);
	localToWorld.V[3].Z = (Float)matrix.Get(3,2);

	// Recurse to child nodes
	const Uint32 numChildren = node->GetChildCount();
	for (Uint32 i=0; i<numChildren; ++i)
	{
		const FbxNode* childNode = node->GetChild(i);
		if (childNode != NULL)
		{
			WalkNodes(childNode, builder);
		}
	}

	if (node->GetNodeAttribute() != NULL)
	{
		if (node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eMesh)
		{
			const FbxMesh* mesh = (const FbxMesh*)node->GetNodeAttribute();

			((FbxMesh*)mesh)->ApplyPivot();

			const String nodeName(ANSI_TO_UNICODE(node->GetName()));
			const String nodeNameLower = nodeName.ToLower();
	
			if ( nodeNameLower.EndsWith( TXT("_tri") ) )
			{
				RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Found triangle collsion mesh data on node '%s'"), ANSI_TO_UNICODE( node->GetName() ) );
				builder.ExtractCollisionMesh( mesh, localToWorld, FBXMeshBuilder::CollisionMesh::CT_Trimesh );
			}
			else if ( nodeNameLower.EndsWith(TXT("_coll") ) || nodeNameLower.EndsWith( TXT("_convex") ) 
				|| nodeNameLower.EndsWith( TXT("_col") ) || nodeNameLower.EndsWith( TXT("_cvx") ) )
			{
				RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Found convex collsion mesh data on node '%s'"), ANSI_TO_UNICODE( node->GetName() ) );
				builder.ExtractCollisionMesh( mesh, localToWorld, FBXMeshBuilder::CollisionMesh::CT_Convex );
			}
			else
			{
				Uint32 lodIndex = 0;

				// extract LOD index
				if ( nodeNameLower.EndsWith( TXT("_lod0") ))
				{
					lodIndex = 0;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod1") ) )
				{
					lodIndex = 1;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod2") ) )
				{
					lodIndex = 2;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod3") ) )
				{
					lodIndex = 3;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod4") ) )
				{
					lodIndex = 4;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod5") ) )
				{
					lodIndex = 5;
				}
				else if ( nodeNameLower.EndsWith( TXT("_lod6") ) )
				{
					lodIndex = 6;
				}

				RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Found mesh data on node '%s' (LOD%d)"), ANSI_TO_UNICODE( node->GetName() ), lodIndex );
				if (m_importAsSkinnedMesh)
				{
					builder.ExtractMesh(lodIndex, mesh, localToWorld, node);
				}
				else
				{
					builder.ExtractMesh(lodIndex, mesh, localToWorld);
				}
			}
		}
	}
}

static inline bool MatchVertex(const SMeshVertex& a, const SMeshVertex& b)
{
	return 0 == memcmp(&a, &b, sizeof(a));
}

void CFBXMeshImporter::CreateMeshChunks(const Uint32 numBones, const Uint32 pointCount, const FBXMeshBuilder::Triangle* triList, CMesh::LODLevel& outLevel, CMesh::FactoryInfo& outInfo)
{
	const Uint32 maxVerticesPerChunk = 65535;
	const Uint32 maxBonesPerChunk = 60;

	while (triList != NULL)
	{
		// Add chunk to lod
		const Uint16 chunkIndex = (Uint16)outInfo.m_chunks.Size();
		outLevel.m_chunks.PushBack(chunkIndex);

		// Create new chunk
		SMeshChunk* outChunk = new (outInfo.m_chunks) SMeshChunk;
		outChunk->m_materialID = triList->m_material;
		outChunk->m_vertices.Reserve(65536+8);
		outChunk->m_indices.Reserve(65536*3);
		outChunk->m_numBonesPerVertex = outInfo.m_boneNames.Empty() ? 0 : 4;

		// Flags
		Uint8 hasUV1 = 0;
		Uint8 hasColor = 0;
		Uint8 hasSkinning = 0;

		// Per point list of matching vertices
		TDynArray<TDynArray<Uint32>> generatedVertices;
		generatedVertices.Resize(pointCount);

		// Bone mapping
		TDynArray<Int32> boneMapping;
		boneMapping.Resize(numBones);
		for (Uint32 j=0; j<numBones; ++j)
		{
			boneMapping[j] = -1;
		}

		// Add triangles
		while (triList != NULL)
		{
			const Uint32 curVertexCount = outChunk->m_vertices.Size();
			//const Uint32 curBoneCount = outChunk->m_boneIndices.Size();
			const Uint32 curIndexCount = outChunk->m_indices.Size();

			// add vertices
			Uint32 indices[3];
			for (Uint32 j=0; j<3; ++j)
			{
				const FBXMeshBuilder::Vertex& v = triList->m_v[j];

				// create export vertex
				SMeshVertex exportVertex;
				exportVertex.m_position[0] = v.m_pos.X;
				exportVertex.m_position[1] = v.m_pos.Y;
				exportVertex.m_position[2] = v.m_pos.Z;
				exportVertex.m_normal[0] = v.m_normal.X;
				exportVertex.m_normal[1] = v.m_normal.Y;
				exportVertex.m_normal[2] = v.m_normal.Z;
				exportVertex.m_tangent[0] = v.m_tangent.X;
				exportVertex.m_tangent[1] = v.m_tangent.Y;
				exportVertex.m_tangent[2] = v.m_tangent.Z;
				exportVertex.m_binormal[0] = v.m_bitangent.X;
				exportVertex.m_binormal[1] = v.m_bitangent.Y;
				exportVertex.m_binormal[2] = v.m_bitangent.Z;
				exportVertex.m_uv0[0] = v.m_uv0.X;
				exportVertex.m_uv0[1] = v.m_uv0.Y;
				exportVertex.m_uv1[0] = v.m_uv1.X;
				exportVertex.m_uv1[1] = v.m_uv1.Y;
				exportVertex.m_weights[0] = v.m_weights[0];
				exportVertex.m_weights[1] = v.m_weights[1];
				exportVertex.m_weights[2] = v.m_weights[2];
				exportVertex.m_weights[3] = v.m_weights[3];

				Color cColor;

				cColor.R = (Uint8)(v.m_color.X * 255.f);
				cColor.G = (Uint8)(v.m_color.Y * 255.f);
				cColor.B = (Uint8)(v.m_color.Z * 255.f);
				cColor.A = (Uint8)(v.m_color.W * 255.f);

				exportVertex.m_color = cColor.ToUint32();

				// map bone indices
				for (Uint32 k=0; k<4; ++k)
				{
					if (v.m_weights[k] > 0.0f)
					{
						/*Int32 localBoneIndex = boneMapping[v.m_indices[k]];
						if (localBoneIndex == -1)
						{
						localBoneIndex = outChunk->m_boneIndices.Size();
						outChunk->m_boneIndices.PushBack((Uint16)v.m_indices[k]);
						boneMapping[v.m_indices[k]] = localBoneIndex;
						}*/

						exportVertex.m_indices[k] = v.m_indices[ k ];//(Uint8)localBoneIndex;
						hasSkinning = true;
						//break;
					}
					else
					{
						exportVertex.m_indices[k] = 0;
					}
				}

				// try to match vertex
				Bool matched = false;
				TDynArray<Uint32>& verticesForPoint = generatedVertices[v.m_point];
				for (Uint32 k=0; k<verticesForPoint.Size(); ++k)
				{
					const Uint32 vertexIndex = verticesForPoint[k];
					const SMeshVertex& testVertex = outChunk->m_vertices[vertexIndex];
					if (MatchVertex(exportVertex, testVertex))
					{
						matched = true;
						indices[j] = vertexIndex;
						break;
					}
				}

				// Vertex data not matched with any existing vertex, create new vertex
				if (!matched)
				{
					const Uint32 vertexIndex = outChunk->m_vertices.Size();
					outChunk->m_vertices.PushBack(exportVertex);
					verticesForPoint.PushBack(vertexIndex);
					indices[j] = vertexIndex;
				}
			}

			// chunk limit was reached
			if (//outChunk->m_boneIndices.Size() >= maxBonesPerChunk || 
				outChunk->m_vertices.Size() >= maxVerticesPerChunk)
			{
				outChunk->m_indices.Resize(curIndexCount);
				//outChunk->m_boneIndices.Resize(curBoneCount);
				outChunk->m_vertices.Resize(curVertexCount);
				break;
			}

			// Create indices
			ASSERT(indices[0] <= maxVerticesPerChunk);
			ASSERT(indices[1] <= maxVerticesPerChunk);
			ASSERT(indices[2] <= maxVerticesPerChunk);
			outChunk->m_indices.PushBack((Uint16)indices[2]);
			outChunk->m_indices.PushBack((Uint16)indices[1]);
			outChunk->m_indices.PushBack((Uint16)indices[0]);

			// Go to next triangle
			triList = triList->m_next;
		}
		
		// Stats
		RED_LOG_SPAM( CNAME( CFBXMeshImporter ), TXT("Created mesh chunk %d (material %d) (%d vertices, %d indices, %d bones)"), 
			outLevel.m_chunks.Size(),
			outChunk->m_materialID,
			outChunk->m_vertices.Size(),
			outChunk->m_indices.Size()/*,
			outChunk->m_boneIndices.Size()*/ );

		// Assign vertex type
		if (hasSkinning)
		{
			outChunk->m_numBonesPerVertex = 4; // let's play it safe
			outChunk->m_vertexType = /*hasUV1 ? MVT_SkinnedMeshSecondUV :*/ MVT_SkinnedMesh;
		}
		else
		{
			outChunk->m_numBonesPerVertex = 0;
			outChunk->m_vertexType = /*hasUV1 ? MVT_StaticMeshSecondUV :*/ MVT_StaticMesh;
		}

		// Save memory
		outChunk->m_vertices.Shrink();
		outChunk->m_indices.Shrink();
		//outChunk->m_boneIndices.Shrink();
	}
}