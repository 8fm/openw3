/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fbxCommon.h"
#include "../../games/r4/cameraTools.h"
#include "../../common/engine/skeleton.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/material.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/bitmapTexture.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialParameterInstance.h"
#include "../../common/engine/textureArray.h"
#include "../../common/core/exporter.h"
#include "../../common/engine/animMath.h"
#include "fbxsdk/scene/animation/fbxanimcurve.h"
#include "fbxsdk/scene/geometry/fbxcluster.h"

#if _MSC_VER == 1700
#ifdef _DEBUG
#ifdef _WIN64
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2012\\x64\\debug\\libfbxsdk-mt.lib" )
#else
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2012\\x86\\debug\\libfbxsdk-mt.lib" )
#endif //_WIN64
#else
#ifdef _WIN64
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2012\\x64\\release\\libfbxsdk-mt.lib" )
#else
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2012\\x86\\release\\libfbxsdk-mt.lib" )
#endif //_WIN64
#endif //_DEBUG
#elif _MSC_VER == 1600
#ifdef _DEBUG
#ifdef _WIN64
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2010\\x64\\debug\\libfbxsdk-mt.lib" )
#else
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2010\\x86\\debug\\libfbxsdk-mt.lib" )
#endif //_WIN64
#else
#ifdef _WIN64
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2010\\x64\\release\\libfbxsdk-mt.lib" )
#else
#	pragma comment ( lib, "external\\FBX_SDK_2016_1_2\\lib\\vs2010\\x86\\release\\libfbxsdk-mt.lib" )
#endif //_WIN64
#endif //_DEBUG
#else
#error Unsupported compiler
#endif

//-----------------------------------------------------------------------------

FBXImportScene::FBXImportScene()
	: m_manager(NULL)
	, m_scene(NULL)
{
}

FBXImportScene::~FBXImportScene()
{
}

FBXImportScene* FBXImportScene::Load( const String& filename )
{
	FbxManager* manager = FbxManager::Create();
	if ( !manager )
	{
		RED_LOG_ERROR( CNAME( FBXImportScene ), TXT( "Unable to create the FBX SDK manager" ) );
		return nullptr;
	}

	// Print version
	RED_LOG_SPAM( CNAME( FBXImportScene ), TXT("Autodesk FBX SDK version %s"), ANSI_TO_UNICODE( manager->GetVersion() ) );

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create( manager, IOSROOT );
	manager->SetIOSettings(ios);

	// Get the file version number generate by the FBX SDK.
	int lFileMajor, lFileMinor, lFileRevision;
	int lSDKMajor,  lSDKMinor,  lSDKRevision;
	FbxManager::GetFileFormatVersion(lSDKMajor, lSDKMinor, lSDKRevision);

	// Create an importer.
	FbxImporter* lImporter = FbxImporter::Create(manager,"");

	// Initialize the importer by providing a filename.
	const bool lImportStatus = lImporter->Initialize( UNICODE_TO_ANSI(filename.AsChar()), -1, manager->GetIOSettings());
	lImporter->GetFileVersion(lFileMajor, lFileMinor, lFileRevision);

	if ( !lImportStatus )
	{
		RED_LOG_ERROR( CNAME( FBXImportScene ), TXT( "Call to FbxImporter::Initialize() failed." ) );
		RED_LOG_ERROR( CNAME( FBXImportScene ), TXT( "Error: %" ) RED_PRIWas, lImporter->GetStatus().GetErrorString() );
		return nullptr;
	}

	RED_LOG_SPAM( CNAME( FBXImportScene ), TXT("FBX file format version for this FBX SDK is %d.%d.%d"), lSDKMajor, lSDKMinor, lSDKRevision);

	if (lImporter->IsFBX())
	{
		RED_LOG_SPAM( CNAME( FBXImportScene ), TXT( "File Version Major: %d Minor: %d Revision: %d" ), lFileMajor, lFileMinor, lFileRevision );

		// Set the import states. By default, the import states are always set to 
		// true. The code below shows how to change these states.
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_MATERIAL,        true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_TEXTURE,         true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_LINK,            true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_SHAPE,           true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_GOBO,            true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_ANIMATION,       true);
		manager->GetIOSettings()->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);
	}

	// Import the scene.
	FbxScene* pScene = FbxScene::Create( manager, "ImportScene" );
	if ( !lImporter->Import(pScene) )
	{
		lImporter->Destroy();
		pScene->Destroy();
		return NULL;
	}

	// Convert axes
	FbxAxisSystem exportAxes( FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded );
	exportAxes.ConvertScene( pScene );
	//FbxAxisSystem::max.ConvertScene( pScene );

	// Scene units are meteres
	FbxSystemUnit::m.ConvertScene( pScene );

	// Return imported scene
	lImporter->Destroy();

	// Return imported scene
	FBXImportScene* retScene = new FBXImportScene();
	retScene->m_manager = manager;
	retScene->m_scene = pScene;
	return retScene;
}


FBXExportScene::FBXExportScene(const String& saveFilePath)
	: m_scene(NULL)
	, m_manager(NULL)
{
	// Create SDK objects
	m_manager = FbxManager::Create();
	if( !m_manager )
	{
		RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("Error: Unable to create FBX Manager!") );
		return;
	}

	String version = String::Printf(TXT("%ls"), ANSI_TO_UNICODE( m_manager->GetVersion() ) );
	// Print version
	RED_LOG_SPAM( CNAME( FBXExportScene ), TXT("Autodesk FBX SDK version %s"), ANSI_TO_UNICODE( m_manager->GetVersion() ) );

	//Create an IOSettings object. This object holds all import/export settings.
	FbxIOSettings* ios = FbxIOSettings::Create( m_manager, IOSROOT );
	m_manager->SetIOSettings(ios);

	// Make sure output path exists
	GFileManager->CreatePath( saveFilePath );

	// Create an FBX scene. This object holds most objects imported/exported from/to files.
	CFilePath filePath( saveFilePath );
	m_scene = FbxScene::Create( m_manager, UNICODE_TO_ANSI(filePath.GetFileName().AsChar()));
	if ( !m_scene )
	{
		RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("FBX: Unable to create FBX scene!") );
		m_manager->Destroy();
		return;
	}

	// Scene units are meters
	m_scene->GetGlobalSettings().SetSystemUnit( FbxSystemUnit::m );

	// Scene export up axis (Z)
	m_scene->GetGlobalSettings().SetAxisSystem( FbxAxisSystem( FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded ) );

	// convert scene
	FbxSystemUnit::ConversionOptions options;
	options.mConvertClusters = true;
	options.mConvertRrsNodes = true;
	FbxSystemUnit::cm.ConvertScene(m_scene, options);
}

FBXExportScene::~FBXExportScene()
{
	if ( NULL != m_scene )
	{
		m_scene->Destroy();
		m_scene = NULL;
	}

	if ( NULL != m_manager )
	{
		m_manager->Destroy();
		m_manager = NULL;
	}
}

Bool FBXExportScene::Save( const String& filePath, const EFBXFileVersion& version )
{
	Bool exportStatus = false;

	// Now create a bind pose with the link list
	if (m_clusteredFbxNodes.GetCount())
	{
		// A pose must be named. Arbitrarily use the name of the patch node.
		FbxPose* lPose = FbxPose::Create( m_scene, "BasePose" );
		lPose->SetIsBindPose(true);

		const Uint32 numBones = m_clusteredFbxNodes.GetCount();
		for (Uint32 i=0; i<numBones; i++)
		{
			FbxNode*  lKFbxNode   = m_clusteredFbxNodes.GetAt(i);
			FbxMatrix lBindMatrix = lKFbxNode->EvaluateGlobalTransform();
			lPose->Add(lKFbxNode, lBindMatrix);
		}

		// Add the pose to the scene
		m_scene->AddPose(lPose);
	}

	// Store rest pose ( the same as bind pose )
	{
		FbxPose* lPose = FbxPose::Create( m_scene, "RestPose" );
		lPose->SetIsBindPose(false);

		// Process all nodes
		for ( Uint32 i=0; i<m_nodes.Size(); ++i )
		{
			FbxNode* node = m_nodes[i];
			FbxMatrix lBindMatrix = node->EvaluateGlobalTransform();
			lPose->Add( node, lBindMatrix );
		}

		// Add the pose to the scene
		m_scene->AddPose(lPose);
	}

	// Save scene
	{
		// Create an exporter
		FbxExporter* lExporter = FbxExporter::Create( m_manager, "");

		// Use native format
		Int32 pFileFormat = m_manager->GetIOPluginRegistry()->GetNativeWriterFormat();

		if ( version == EFBXFileVersion::EFBX_2009 )
		{ 
			lExporter->SetFileExportVersion( FBX_2009_00_COMPATIBLE );
			pFileFormat = m_manager->GetIOPluginRegistry()->FindWriterIDByDescription("fbx 6.0 binary (*.fbx)");
		}
		else if ( version == EFBXFileVersion::EFBX_2010 )
		{
			lExporter->SetFileExportVersion( FBX_2010_00_COMPATIBLE );
			pFileFormat = m_manager->GetIOPluginRegistry()->FindWriterIDByDescription("fbx 6.0 binary (*.fbx)");
		}
		else if ( version == EFBXFileVersion::EFBX_2011 )
		{
			lExporter->SetFileExportVersion( FBX_2011_00_COMPATIBLE );
		}
		else if ( version == EFBXFileVersion::EFBX_2013 )
		{
			lExporter->SetFileExportVersion( FBX_2013_00_COMPATIBLE );
		}
		else
		{
			lExporter->SetFileExportVersion( FBX_DEFAULT_FILE_COMPATIBILITY );
		}

		// Set the export states. By default, the export states are always set to 
		// true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
		// shows how to change these states.
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_MATERIAL,        true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_TEXTURE,         true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_EMBEDDED,        true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_SHAPE,           true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GOBO,            true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_ANIMATION,       true);
		m_manager->GetIOSettings()->SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

		// Initialize the exporter by providing a filename.
		if ( !lExporter->Initialize( UNICODE_TO_ANSI(filePath.AsChar()), pFileFormat, m_manager->GetIOSettings()) )
		{
			RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("FBX: Call to FbxExporter::Initialize() failed.") );
			RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("FBX: Error returned: %s"), ANSI_TO_UNICODE( lExporter->GetStatus().GetErrorString() ) );
			exportStatus = false;
		}
		else
		{
			// Info
			int lMajor, lMinor, lRevision;
			FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
			RED_LOG_SPAM( CNAME( FBXExportScene ), TXT("FBX file format version %d.%d.%d"), lMajor, lMinor, lRevision);

			// Export the scene.
			if ( !lExporter->Export( m_scene ) )
			{
				RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("FBX: error exporting scene: %s"), ANSI_TO_UNICODE( lExporter->GetStatus().GetErrorString() ));
				exportStatus = false;
			}
			else
			{
				exportStatus = true;
			}
		}

		// Destroy the exporter.
		lExporter->Destroy();
	}

	if ( exportStatus )
	{
		CFilePath xmlPath( filePath );
		xmlPath.SetExtension( TXT("xml") );

		m_xmlWriter.Flush();

		if ( ! GFileManager->SaveStringToFile( xmlPath.ToString(), m_xmlWriter.GetContent() ) )
		{
			RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("Failed to save content manifest for file '%ls'"), xmlPath.GetPathString().AsChar() );
		}
	}

	// Return status
	return exportStatus;
}

Uint32 FBXExportScene::AppendPhongMaterial( FbxNode* lNode, const CName& name, const FbxDouble3& diffuse )
{
	FbxSurfacePhong* newMaterial = FbxSurfacePhong::Create( m_scene, name.AsAnsiChar() );
	newMaterial->ShadingModel.Set("Phong");
	newMaterial->Diffuse.Set( diffuse );
	return lNode->AddMaterial( newMaterial );
}

void FBXExportScene::ExportCollisionMesh( const Matrix& modelLocalToWorld, const ICollisionShape* srcCollisionMesh, const String& name )
{
	// Extract shape geometry
	TDynArray<Vector> vertices;
	TDynArray<Uint32> indices;
	srcCollisionMesh->GetShape( Matrix::IDENTITY, vertices, indices );

	if ( vertices.Empty() || indices.Empty() )
	{
		return;
	}

	// Create new mesh
	FbxMesh* newMesh = FbxMesh::Create( m_scene, srcCollisionMesh->GetClass()->GetName().AsAnsiChar() );//isConvex ? "CollisionConvex" : "CollisionMesh");

	// Create geometry
	const Uint32 numVertices = vertices.Size();
	newMesh->InitControlPoints( numVertices );

	// Create the default layer
	if ( !newMesh->GetLayer(0) )
	{
		newMesh->CreateLayer();
	}

	// Assemble node name
	String nodeName = name;
	String clsName = srcCollisionMesh->GetClass()->GetName().AsString();

	if ( clsName.ContainsSubstring( TXT("Convex") ) )
	{
		nodeName += TXT("_col");
	}
	else
	{
		nodeName += TXT("_tri");
	}

	// Create mesh node	
	FbxNode* lNode = FbxNode::Create( m_scene, UNICODE_TO_ANSI(nodeName.AsChar()));
	lNode->SetNodeAttribute(newMesh);
	lNode->SetShadingMode(FbxNode::eHardShading);

	// Create collision vertices
	FbxVector4* lControlPoints = newMesh->GetControlPoints();
	for (Uint32 i=0; i<numVertices; ++i)
	{
		const Vector vertexPosition = modelLocalToWorld.TransformPoint( vertices[i] );
		lControlPoints[i] = FbxVector4( vertexPosition.X, vertexPosition.Y, vertexPosition.Z, 1.0f );
	}

	// Create triangles and materials
	THashMap< CName, Uint32 > materialsMap;
	CStandardRand randGen;
	const Uint32 numIndices = indices.Size();

	if ( srcCollisionMesh->GetNumPhysicalMaterials() > 1 )
	{
		// Set material mapping.
		FbxGeometryElementMaterial* lMaterialElement = newMesh->CreateElementMaterial();
		lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		newMesh->GetLayer(0)->SetMaterials(lMaterialElement);
	}

	for ( Uint32 k = 0; k < numIndices; k += 3 )
	{
		CName polygonMat = srcCollisionMesh->GetPhysicalMaterial( k / 3 );
		if ( !materialsMap.KeyExist( polygonMat) )
		{
			materialsMap.Set( polygonMat, AppendPhongMaterial( lNode, polygonMat, FbxDouble3( randGen.Get< Float >(), randGen.Get< Float >(), randGen.Get< Float >() ) ) );
		}

		Uint32 materialIndex = materialsMap.GetRef( polygonMat );
		newMesh->BeginPolygon( materialIndex );
		newMesh->AddPolygon( indices[k+0] );
		newMesh->AddPolygon( indices[k+1] );
		newMesh->AddPolygon( indices[k+2] );
		newMesh->EndPolygon();
	}

	// Add mesh node to scene
	m_scene->GetRootNode()->AddChild( lNode );
}

static void AddNodeRecursively(FbxArray<FbxNode*>& pNodeArray, FbxNode* pNode)
{
	if (pNode)
	{
		AddNodeRecursively(pNodeArray, pNode->GetParent());

		if (pNodeArray.Find(pNode) == -1)
		{
			// Node not in the list, add it
			pNodeArray.Add(pNode);
		}
	}
}

void FBXExportScene::ExportXMLMeshData( const CMesh* srcModel )
{
	m_xmlWriter.BeginNode( TXT("mesh_data") );

	m_xmlWriter.Attribute( TXT("autohideDistance"), String::Printf( TXT("%.2f"), srcModel->GetAutoHideDistance() ) );
	m_xmlWriter.Attribute( TXT("isTwoSided"), ToString( srcModel->IsTwoSided() ) );
	m_xmlWriter.Attribute( TXT("useExtraStreams"), ToString( srcModel->CanUseExtraStreams() ) );
	m_xmlWriter.Attribute( TXT("mergeInGlobalShadowMesh"), ToString( srcModel->CanMergeIntoGlobalShadowMesh() ) );
	m_xmlWriter.Attribute( TXT("entityProxy"), ToString( srcModel->IsEntityProxy() ) );

	const Uint32 numLodLevels = srcModel->GetNumLODLevels();
	m_xmlWriter.BeginNode( TXT("LODs") );
	for ( Uint32 i = 0; i < numLodLevels; ++i )
	{
		const CMesh::LODLevel& lod = srcModel->GetMeshLODLevels()[ i ];
		m_xmlWriter.BeginNode( TXT("LOD_info") );
		m_xmlWriter.Attribute( TXT("distance"), String::Printf( TXT("%.2f"), lod.GetDistance() ) );
		m_xmlWriter.EndNode();
	}
	m_xmlWriter.EndNode(); //lods
	m_xmlWriter.EndNode(); //mesh_data
}

void FBXExportScene::ExportXMLCollisionData( const CCollisionMesh* collision )
{
	m_xmlWriter.BeginNode( TXT("collision") );
	m_xmlWriter.Attribute( TXT("attenuation"), String::Printf( TXT("%.2f"), collision->GetOcclusionAttenuation() ) );
	m_xmlWriter.Attribute( TXT("diagonalLimit"), String::Printf( TXT("%.2f"), collision->GetOcclusionDiagonalLimit() ) );
	m_xmlWriter.Attribute( TXT("swimmingRotationAxis"), ToString( collision->GetSwimmingRotationAxis() ) );
	m_xmlWriter.EndNode();
}

void FBXExportScene::ExportXMLMaterialData( const CMesh* srcModel, const IMaterial* material, Uint32 materialID, const FbxSurfacePhong* newMaterial )
{
	Bool alreadySavedInXML = m_xmlSavedMaterials.Exist( materialID );
	if ( !alreadySavedInXML )
	{
		m_xmlWriter.BeginNode( TXT("material") );
		Bool localInstance = material->GetParent() == srcModel;
		m_xmlWriter.Attribute( TXT("name"), String( ANSI_TO_UNICODE( newMaterial->GetName() ) ) );
		m_xmlWriter.Attribute( TXT("local"), ToString( localInstance ) );
		if ( localInstance )
		{
			if ( material->GetBaseMaterial() && material->GetBaseMaterial()->GetFile() )
			{
				m_xmlWriter.Attribute( TXT("base"), material->GetBaseMaterial()->GetFile()->GetDepotPath() );
			}

			if ( const CMaterialInstance* matInstance = Cast< CMaterialInstance >( material ) )
			{
				const TMaterialParameters& parameters = matInstance->GetParameters();
				for ( const MaterialParameterInstance& param : parameters )
				{
					m_xmlWriter.BeginNode( TXT("param") );
					m_xmlWriter.Attribute( TXT("name"), param.GetName().AsString() );

					String val;
					if ( param.GetType()->ToString( param.GetData(), val ) )
					{
						m_xmlWriter.Attribute( TXT("type"), param.GetType()->GetName().AsString() );
						m_xmlWriter.Attribute( TXT("value"), val );
					}
					else
					{
						// hmmmm...
						RED_LOG_ERROR( CNAME( FBXExportScene ), TXT("Parameter %ls of type %ls is not supported!"), 
							param.GetName().AsString(), param.GetType()->GetName().AsString() );
					}
					m_xmlWriter.EndNode();
				}
			}
		}
		else if ( const CDiskFile* matFile = material->GetFile() )
		{
			m_xmlWriter.Attribute( TXT("base"), matFile->GetDepotPath() );
		}

		m_xmlWriter.EndNode();
		m_xmlSavedMaterials.PushBack( materialID );
	}
}

void FBXExportScene::ExportMesh( const Matrix& modelLocalToWorld, Bool exportSkinning, const CMesh* srcModel, const String& name  )
{
	const Float exportScale = 100.f;

	const CMeshData data( srcModel );
	const TDynArray< SMeshChunk >& chunks = data.GetChunks();
	m_xmlWriter.BeginNode( TXT("mesh") );
	ExportXMLMeshData( srcModel );

	// Export collision meshes
	const CCollisionMesh* collision = srcModel->GetCollisionMesh();
	if (NULL != collision)
	{
		ExportXMLCollisionData( collision );

		Uint32 numShapes = 0;
		const TDynArray< ICollisionShape* >& shapes = collision->GetShapes();
		const Uint32 numCollision = shapes.Size();
		for (Uint32 i=0; i<numCollision; ++i)
		{
			const ICollisionShape* shape = shapes[i];
			if (NULL != shape)
			{
				ExportCollisionMesh(modelLocalToWorld, shape, name );
				numShapes += 1;
			}
		}

		RED_LOG_SPAM( CNAME( FBXExportScene ), TXT("%d collision shapes exported for '%s"), numShapes, name.AsChar() );
	}

	CStandardRand randGen;
	m_xmlSavedMaterials.Clear();

	// Export all LODs
	m_xmlWriter.BeginNode( TXT("materials") );
	const Uint32 numLodLevels = srcModel->GetNumLODLevels();
	for (Uint32 lodLevel=0; lodLevel<numLodLevels; ++lodLevel)
	{
		// Export LOD0 only
		const CMesh::LODLevel& lod0 = srcModel->GetMeshLODLevels()[ lodLevel ];

		// Mesh name
		String meshName = name;
		if (meshName.Empty())
		{
			CFilePath path(srcModel->GetDepotPath());
			meshName = path.GetFileName();
		}

		// Prefix Node name with lod index if there are LODs for this mesh
		String nodeName = meshName;
		if (numLodLevels > 1)
		{
			String lodSuffix = String::Printf(TXT("_lod%d"), lodLevel);
			nodeName = meshName + lodSuffix;
		}

		// Create new mesh for each chunk
		FbxMesh* newMesh = FbxMesh::Create(m_manager, UNICODE_TO_ANSI(meshName.AsChar()));

		// Count total vertices in the mesh ( in all chunks )
		Uint32 totalVertexCount = 0;
		Uint32 totalTriangleCount = 0;
		const Uint32 numChunks= lod0.m_chunks.Size();
		for ( Uint32 i=0; i<numChunks; ++i )
		{
			const Uint32 chunkIndex = lod0.m_chunks[i];
			const SMeshChunk& srcChunk = chunks[chunkIndex];
			totalVertexCount += srcChunk.m_numVertices;
			totalTriangleCount += srcChunk.m_numIndices / 3;
		}

		// Allocate vertices ( in FBX they are called Control Points )
		newMesh->InitControlPoints( totalVertexCount );

		// Create the default layer
		if ( !newMesh->GetLayer(0) )
		{
			newMesh->CreateLayer();
		}

		// Allocate normals
		FbxGeometryElementNormal* lNormalElement = newMesh->CreateElementNormal();
		lNormalElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);
		newMesh->GetLayer(0)->SetNormals(lNormalElement);
		
		// Create UV for Diffuse channel
		FbxGeometryElementUV* lUVDiffuseElement = newMesh->CreateElementUV( "DiffuseUV" );
		lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);
		newMesh->GetLayer(0)->SetUVs(lUVDiffuseElement);

		// Create second UV channel
		FbxGeometryElementUV* lUVSecondDiffuseElement = newMesh->CreateElementUV("SecondUV");
		lUVSecondDiffuseElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		lUVSecondDiffuseElement->SetReferenceMode(FbxGeometryElement::eDirect);
		newMesh->GetLayer(0)->SetUVs(lUVSecondDiffuseElement, FbxLayerElement::eTextureReflection);

		// Set material mapping.
		FbxGeometryElementMaterial* lMaterialElement = newMesh->CreateElementMaterial();
		lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);
		newMesh->GetLayer(0)->SetMaterials(lMaterialElement);

		FbxGeometryElementVertexColor* lVecrtexColorElement = newMesh->CreateElementVertexColor();
		lVecrtexColorElement->SetMappingMode(FbxGeometryElement::eByControlPoint);
		lVecrtexColorElement->SetReferenceMode(FbxGeometryElement::eDirect);
		newMesh->GetLayer(0)->SetVertexColors(lVecrtexColorElement);

		// Create mesh node
		FbxNode* lNode = FbxNode::Create( m_manager, UNICODE_TO_ANSI( nodeName.AsChar() ) );
		lNode->SetNodeAttribute( newMesh );
		lNode->SetShadingMode( FbxNode::eTextureShading );

		// Clusters for each bone
		TDynArray< FbxCluster* > boneClusters;
		TDynArray< Matrix > skinMatrices;
		TDynArray< Int32 > globalBonesMap;
		if (exportSkinning)
		{
			const Uint32 numBones = srcModel->GetBoneCount();
			const Matrix* meshRigMatrices = srcModel->GetBoneRigMatrices();
			const CName* meshBoneNames = srcModel->GetBoneNames();
			boneClusters.Resize(numBones);
			skinMatrices.Resize(numBones);
			globalBonesMap.Resize(numBones);
			for (Uint32 i=0; i<numBones; ++i)
			{
				// empty initial cluster
				boneClusters[i] = NULL;

				// Create skeleton
				FbxSkeleton* skeletonAttr = FbxSkeleton::Create( m_manager, "Skeleton" );
				skeletonAttr->SetSkeletonType( FbxSkeleton::eRoot );
				//skeletonAttr->Size.Set( 0.2 );

				// Find bone by name
				Int32 globalBoneIndex = -1;
				const String boneName = meshBoneNames[i].AsString();

				if ( m_nodeMap.Find( boneName, globalBoneIndex ) )
				{
					globalBonesMap[i] = globalBoneIndex;
				}

				if ( globalBoneIndex != -1 )
				{
					Matrix rigMatrix = meshRigMatrices[i];

					const Vector rigScale = rigMatrix.GetScale33();
					if ( rigScale.DistanceTo(Vector::ONES) > 0.001f )
					{
						RED_LOG(TXT("Mapped mesh '%s' bone '%s' has scale [%f,%f,%f]"),
							srcModel->GetDepotPath().AsChar(),
							boneName.AsChar(), 
							rigScale.X, rigScale.Y, rigScale.Z);

						rigMatrix.V[0].Normalize3();
						rigMatrix.V[1].Normalize3();
						rigMatrix.V[2].Normalize3();
					}

					skinMatrices[i] = rigMatrix * m_refNodes[globalBonesMap[i]];

					RED_LOG(TXT("Mapped mesh '%s' bone '%s' to index %d"),
						srcModel->GetDepotPath().AsChar(),
						boneName.AsChar(),
						globalBonesMap[i] );
				}
				else
				{
					// Create node
					FbxNode* newNode = FbxNode::Create( m_manager, UNICODE_TO_ANSI(boneName.AsChar()) );
					m_scene->GetRootNode()->AddChild( newNode );
					newNode->SetNodeAttribute(skeletonAttr);

					// Export transform
					{
						Matrix rigMatrix = srcModel->GetBoneRigMatrices()[i].FullInverted();
						Vector pos = rigMatrix.GetTranslation();
						pos *= exportScale;
						Vector q = rigMatrix.ToQuat();
						const FbxAMatrix rotM( FbxVector4(0,0,0,0), FbxQuaternion( q.X, q.Y, q.Z, q.W ), FbxVector4(1,1,1,1) );

						newNode->LclTranslation.Set( FbxDouble3( pos.X, pos.Y, pos.Z ) );
						newNode->LclRotation.Set( rotM.GetR() );
						newNode->RotationOrder = eEulerXYZ; // always XYZ in SDK 2013.1!!!
					}

					m_nodeMap.Insert( boneName, m_nodes.Size() );
					globalBonesMap[i] = m_nodes.Size();
					m_nodes.PushBack( newNode );
				}
			}
		}

		// Mesh mapped materials
		THashMap< Uint32, Int32 > meshMappedMaterials;

		// Output vertices
		Uint32 firstChunkVertex = 0;
		FbxVector4* lControlPoints = newMesh->GetControlPoints();

		// Export mesh chunks
		for ( Uint32 i=0; i<numChunks; ++i )
		{
			const Uint32 chunkIndex = lod0.m_chunks[i];
			const SMeshChunk* chunk = &chunks[chunkIndex];

			// Map chunk material to node material
			Int32 mappedMaterialIndex = -1;
			if (!meshMappedMaterials.Find(chunk->m_materialID, mappedMaterialIndex))
			{
				const Uint32 localMaterialIndex = chunk->m_materialID;
				const IMaterial* material = srcModel->GetMaterials()[chunk->m_materialID];

				// Get material name
				Bool bMaterialNameSet = false;
				String materialName;
				if (!srcModel->GetMaterialNames().Empty())
				{
					materialName = srcModel->GetMaterialNames()[chunk->m_materialID];
					bMaterialNameSet = true;
				}
				else if (NULL == material)
				{
					materialName = TXT("NullMaterial");
				}
				else if (!material->GetDepotPath().Empty())
				{
					const CFilePath filePath(material->GetDepotPath());
					materialName = String::Printf(TXT("%s%d"), filePath.GetFileName().AsChar(), localMaterialIndex);
				}
				else
				{
					materialName = String::Printf(TXT("Material%d"), localMaterialIndex);
				}

				// Create new material
				mappedMaterialIndex = AppendPhongMaterial( lNode, CName( materialName ), FbxDouble3( randGen.Get< Float >(), randGen.Get< Float >(), randGen.Get< Float >() ) );
				meshMappedMaterials.Set( localMaterialIndex, mappedMaterialIndex );
				FbxSurfacePhong* newMaterial = (FbxSurfacePhong*)lNode->GetMaterial( mappedMaterialIndex );

				// Extract material textures
				ExtractTextures( newMaterial, material, bMaterialNameSet );
				ExportXMLMaterialData( srcModel, material, chunk->m_materialID, newMaterial );
			}

			// Export vertices and normals
			const Uint32 numVertices = chunk->m_vertices.Size();
			const SMeshVertex* srcVertex = (const SMeshVertex*) chunk->m_vertices.Data();
			for ( Uint32 k=0; k<numVertices; ++k, ++srcVertex )
			{
				// Default crap
				const Uint32 controlPointIndex = firstChunkVertex+k;
				Vector vertexPosition( srcVertex->m_position[0], srcVertex->m_position[1], srcVertex->m_position[2] );
				Vector vertexNormal( srcVertex->m_normal[0], srcVertex->m_normal[1], srcVertex->m_normal[2] );

				vertexPosition *= exportScale;
				vertexNormal *= exportScale;

				// Skinning
				if ( exportSkinning && !srcModel->GetUsedBones().Empty() )
				{
					Matrix skinMatrix = Matrix::ZEROS;
					Float weightSum = 0.0f;

					// #horzel #todo we have 4 bones skinning ATM. create support for 8
					for ( Uint32 z=0; z<4; ++z )
					{
						const Float w = srcVertex->m_weights[z];
						if ( w > 0.0f )
						{
							// find global bone for mesh bone
							const Uint32 meshBoneIndex = srcVertex->m_indices[z];
							ASSERT( meshBoneIndex < srcModel->GetBoneCount() );

							// get global bone index
							const Int32 globalBoneIndex = globalBonesMap[meshBoneIndex];
							if (globalBoneIndex == -1)
							{
								continue;
							}

							// accumulate skinning matrix
							const Matrix& boneSkinMatrix = skinMatrices[meshBoneIndex];
							skinMatrix.V[0] += boneSkinMatrix.V[0] * w;
							skinMatrix.V[1] += boneSkinMatrix.V[1] * w;
							skinMatrix.V[2] += boneSkinMatrix.V[2] * w;
							skinMatrix.V[3] += boneSkinMatrix.V[3] * w;
							weightSum += w;

							// Create cluster on first use of bone
							FbxCluster* cluster = boneClusters[ meshBoneIndex ];
							if ( !cluster )
							{
								FbxNode* globalBone = m_nodes[globalBoneIndex];

								// Create cluster
								cluster = FbxCluster::Create( m_manager, "" );
								cluster->SetLink( globalBone );
								// #horzel changed from eAdditive
								cluster->SetLinkMode( FbxCluster::eNormalize );
								cluster->SetAssociateModel( lNode );
								boneClusters[ meshBoneIndex ] = cluster;

								// Register in model
								const FbxAMatrix lXMatrix = lNode->EvaluateGlobalTransform();
								cluster->SetTransformMatrix(lXMatrix);
								const FbxAMatrix lnXMatrix = globalBone->EvaluateGlobalTransform();
								cluster->SetTransformLinkMatrix(lnXMatrix);
							}

							// Add point influence
							const Float weight = (Float)srcVertex->m_weights[z];
							cluster->AddControlPointIndex( controlPointIndex, weight );
						}
					}

					if ( skinMatrix != Matrix::ZEROS )
					{
						// Skin
						if ( weightSum > 0.f )
						{
							skinMatrix.V[0] /= weightSum;
							skinMatrix.V[1] /= weightSum;
							skinMatrix.V[2] /= weightSum;
							skinMatrix.V[3] /= weightSum;

							vertexPosition = skinMatrix.TransformPoint(vertexPosition);
							vertexNormal = skinMatrix.TransformVector(vertexNormal);
						}
						else
						{
							RED_LOG( Editor, TXT("Zero skinning sum!") );
						}
					}
				}

				// Export vertex data
				lControlPoints[ controlPointIndex ] = FbxVector4( vertexPosition.X, vertexPosition.Y, vertexPosition.Z, 1.0f );
				lNormalElement->GetDirectArray().Add( FbxVector4( vertexNormal.X, vertexNormal.Y, vertexNormal.Z, 0.0f ) );
				lUVDiffuseElement->GetDirectArray().Add( FbxVector2( srcVertex->m_uv0[0], 1.0f - srcVertex->m_uv0[1] ) );
				lUVSecondDiffuseElement->GetDirectArray().Add( FbxVector2( srcVertex->m_uv1[0], 1.0f - srcVertex->m_uv1[1] ) );

				UINT32 color = srcVertex->m_color;
				Uint8 r = (Uint8)color;
				Uint8 g = (Uint8)(color>>8);
				Uint8 b = (Uint8)(color>>16);
				Uint8 a = (Uint8)(color>>24);
				lVecrtexColorElement->GetDirectArray().Add( FbxColor( (Float)r / 255.f, 
					(Float)g / 255.f, (Float)b / 255.f, (Float)a / 255.f) );
			}

			// Export polygons
			const Uint32 numTriangles = chunk->m_indices.Size() / 3;
			const Uint16* srcIndices = ( const Uint16* ) chunk->m_indices.Data();
			for ( Uint32 k=0; k<numTriangles; ++k, srcIndices += 3 )
			{
				newMesh->BeginPolygon( mappedMaterialIndex );
				newMesh->AddPolygon( (Uint32)srcIndices[2] + firstChunkVertex );
				newMesh->AddPolygon( (Uint32)srcIndices[1] + firstChunkVertex );
				newMesh->AddPolygon( (Uint32)srcIndices[0] + firstChunkVertex );
				newMesh->EndPolygon();
			}

			// Advance in-chunk vertex pointer
			firstChunkVertex += numVertices;
		}

		// Add mesh node to scene
		m_scene->GetRootNode()->AddChild( lNode );

		// Update skinning
		if (exportSkinning)
		{
			// Create skin modifier
			FbxGeometry* lPatchAttribute = (FbxGeometry*) lNode->GetNodeAttribute();
			FbxSkin* lSkin = FbxSkin::Create( m_manager, "" );
			for ( Uint32 i=0; i<boneClusters.Size(); ++i )
			{
				FbxCluster* cluster = boneClusters[i];
				if ( cluster != NULL )
				{
					lSkin->AddCluster(cluster);
				}
			}

			// Add skin
			lPatchAttribute->AddDeformer(lSkin);

			// Now list the all the link involve in the patch deformation
			if (lNode && lNode->GetNodeAttribute())
			{
				Uint32 lSkinCount=0;
				Uint32 lClusterCount=0;
				switch (lNode->GetNodeAttribute()->GetAttributeType())
				{
					case FbxNodeAttribute::eMesh:
					case FbxNodeAttribute::eNurbs:
					case FbxNodeAttribute::ePatch:
					{
						lSkinCount = ((FbxGeometry*)lNode->GetNodeAttribute())->GetDeformerCount(FbxDeformer::eSkin);
						for( Uint32 i=0; i<lSkinCount; ++i )
						{
							FbxSkin* lSkin = (FbxSkin*)( (FbxGeometry*)lNode->GetNodeAttribute() )->GetDeformer( i, FbxDeformer::eSkin );
							lClusterCount += lSkin->GetClusterCount();
						}
						break;
					}
				}

				if (lClusterCount)
				{
					// Again, go through all the skins get each cluster link and add them
					for (Uint32 i=0; i<lSkinCount; ++i)
					{
						FbxSkin *lSkin = (FbxSkin*)( (FbxGeometry*)lNode->GetNodeAttribute() )->GetDeformer( i, FbxDeformer::eSkin );
						lClusterCount = lSkin->GetClusterCount();
						for (Uint32 j=0; j<lClusterCount; ++j)
						{
							FbxNode* lClusterNode = lSkin->GetCluster(j)->GetLink();
							AddNodeRecursively(m_clusteredFbxNodes, lClusterNode);
						}
					}

					// Add the patch to the pose
					m_clusteredFbxNodes.Add(lNode);
				}
			}
		}
	}
	m_xmlWriter.EndNode(); //materials
	m_xmlWriter.EndNode(); //mesh
}

Bool FBXExportScene::ExtractTexture( const IMaterial* srcMaterial, const String& paramName, ITexture*& outTexture )
{
	const CMaterialInstance* matInstance = Cast< CMaterialInstance >( srcMaterial );
	while ( matInstance )
	{
		for ( const MaterialParameterInstance& param : matInstance->GetParameters() )
		{
			if ( param.GetName().AsString().EqualsNC( paramName ) || paramName.Empty() )
			{
				THandle<ITexture> texHandle = 0;
				matInstance->ReadParameter( param.GetName(), texHandle );
				if ( texHandle.IsValid() && ( texHandle->IsA< CBitmapTexture >() || texHandle->IsA< CTextureArray >() ) )
				{
					outTexture = texHandle.Get();
					return true;
				}
			}
		}
		matInstance = Cast< CMaterialInstance >( matInstance->GetBaseMaterial() );
	}

	// Not read
	return false;
}

Bool FBXExportScene::ExtractTextureData( const CBitmapTexture* bitmapTexture, String& outFileName )
{
	CFilePath savePath( bitmapTexture->GetFile()->GetAbsolutePath() );
	savePath.SetExtension( TXT("tga") );

	IExporter* exporter = IExporter::FindExporter( ClassID< CBitmapTexture >(), savePath.GetExtension() );

	if ( !exporter )
	{
		RED_LOG_SPAM( TXT("No valid exporter for extension '%s' for %s")
			, savePath.GetExtension().AsChar()
			, bitmapTexture->GetFriendlyName().AsChar() );
		return false;		
	}

	IExporter::ExportOptions exportOptions;
	exportOptions.m_resource = const_cast< CBitmapTexture* >( bitmapTexture );
	exportOptions.m_saveFileFormat = CFileFormat( savePath.GetExtension(), String::EMPTY );
	exportOptions.m_saveFilePath = savePath.ToString();

	RED_LOG_SPAM( TXT("Exporting resource %s as %s.")
		, exportOptions.m_resource->GetFriendlyName().AsChar()
		, exportOptions.m_saveFilePath.AsChar() );

	exporter->DoExport( exportOptions );
	outFileName = savePath.ToString();
	return true;
}

void FBXExportScene::ExtractTextures( FbxSurfacePhong* newMaterial, const IMaterial* srcMaterial, const Bool bMaterialNameSet )
{
	Bool materialNameSet = bMaterialNameSet;

	// Diffuse
	{
		// Get the source texture
		ITexture* diffuse = NULL;
		if ( !ExtractTexture( srcMaterial, TXT("DiffuseMap"), diffuse ))
		{
			if ( !ExtractTexture( srcMaterial, TXT("Diffuse"), diffuse ) )
			{
				if ( !ExtractTexture( srcMaterial, TXT("DiffuseArray"), diffuse ) )
				{
					if ( !ExtractTexture( srcMaterial, TXT("tex_diffuse"), diffuse ) )
					{
						// extract any texture
						ExtractTexture( srcMaterial, String::EMPTY, diffuse );
					}
				}
			}
		}

		// we're not gonna create local textures here as they're created in another step - we'll just use a path
		// for now I'll use the editor's bitmap exporter
		// Create local texture
		if ( NULL != diffuse )
		{
			// Change material name (will help with reimport)
			if (!materialNameSet)
			{
				const CFilePath diffuseFilePath(diffuse->GetDepotPath());
				newMaterial->SetName(UNICODE_TO_ANSI(diffuseFilePath.GetFileName().AsChar()));
				materialNameSet = true;
			}

			// Already exported ?
			FbxFileTexture* lTexture = NULL;
			if (m_textures.Find(diffuse->GetDepotPath(), lTexture))
			{
				newMaterial->Diffuse.ConnectSrcObject(lTexture);
			}
			else if ( diffuse->IsA< CBitmapTexture >() )
			{
				// Create temporary file name
				String textureFilePath;
				if ( ExtractTextureData( Cast< CBitmapTexture >( diffuse ), textureFilePath ) )
				{
					lTexture = FbxFileTexture::Create( m_scene, "Diffuse Texture" );

					// Setup texture
					lTexture->SetFileName( UNICODE_TO_ANSI(textureFilePath.AsChar()) );
					lTexture->SetTextureUse( FbxTexture::eStandard);
					lTexture->SetMappingType( FbxTexture::eUV );
					lTexture->SetMaterialUse( FbxFileTexture::eModelMaterial );
					lTexture->SetSwapUV(false);
					lTexture->SetTranslation(0.0, 0.0);
					lTexture->SetScale(1.0, 1.0);
					lTexture->SetRotation(0.0, 0.0);
					lTexture->UVSet.Set( "DiffuseUV");
					m_scene->AddTexture( lTexture );

					m_textures.Set(diffuse->GetDepotPath(), lTexture);

					// Connect to material
					newMaterial->Diffuse.ConnectSrcObject(lTexture);
				}
			}
		}
	}

	// Normal
	{
		// Get the source texture
		ITexture* normal = NULL;
		if ( !ExtractTexture( srcMaterial, TXT("NormalMap"), normal ))
		{
			if ( !ExtractTexture( srcMaterial, TXT("Normal"), normal ) )
			{
				if ( !ExtractTexture( srcMaterial, TXT("NormalArray"), normal ) )
				{
					if ( !ExtractTexture( srcMaterial, TXT("tex_normal"), normal ) )
					{
						normal = NULL;
					}
				}
			}
		}

		// Create local texture
		if ( NULL != normal )
		{
			// Change material name (will help with reimport)
			if (!materialNameSet)
			{
				const CFilePath normalFilePath(normal->GetDepotPath());
				newMaterial->SetName(UNICODE_TO_ANSI(normalFilePath.GetFileName().AsChar()));
				materialNameSet = true;
			}

			FbxFileTexture* lTexture = NULL;
			if (m_textures.Find(normal->GetDepotPath(), lTexture))
			{
				newMaterial->NormalMap.ConnectSrcObject(lTexture);
			}
			else if ( normal->IsA< CBitmapTexture >() )
			{
				// Create temporary file name
				String textureFilePath;
				if ( ExtractTextureData( Cast< CBitmapTexture >( normal ), textureFilePath ) )
				{
					FbxFileTexture* lTexture = FbxFileTexture::Create( m_scene, "Normal Texture" );

					// Setup texture
					lTexture->SetFileName( UNICODE_TO_ANSI(textureFilePath.AsChar()) );
					lTexture->SetTextureUse( FbxTexture::eBumpNormalMap);
					lTexture->SetMappingType( FbxTexture::eUV );
					lTexture->SetMaterialUse( FbxFileTexture::eModelMaterial );
					lTexture->SetSwapUV(false);
					lTexture->SetTranslation(0.0, 0.0);
					lTexture->SetScale(1.0, 1.0);
					lTexture->SetRotation(0.0, 0.0);
					lTexture->UVSet.Set( "DiffuseUV");
					m_scene->AddTexture( lTexture );

					m_textures.Set(normal->GetDepotPath(), lTexture);

					// Connect to material
					newMaterial->NormalMap.ConnectSrcObject(lTexture);
				}
			}
		}
	}
}

EFBXFileVersion FBXExportScene::GetFBXExportVersion( const IExporter::ExportOptions& options )
{
	const String& desc = options.m_saveFileFormat.GetDescription();
	
	if( desc.ContainsSubstring( TXT("2009") ) )
	{
		return EFBXFileVersion::EFBX_2009;
	}
	else if( desc.ContainsSubstring( TXT("2010") ) )
	{
		return EFBXFileVersion::EFBX_2010;
	}
	else if( desc.ContainsSubstring( TXT("2011") ) )
	{
		return EFBXFileVersion::EFBX_2011;
	}
	else if( desc.ContainsSubstring( TXT("2013") ) )
	{
		return EFBXFileVersion::EFBX_2013;
	}
	return EFBXFileVersion::EFBX_2016;
}

void FBXExportScene::FillExportFormat( TDynArray<CFileFormat>& fileFormatsToFill )
{
	fileFormatsToFill.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk fbx 2016") ) );
	fileFormatsToFill.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk fbx 2013") ) );
	fileFormatsToFill.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk fbx 2011") ) );
	fileFormatsToFill.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk fbx 2010") ) );
	fileFormatsToFill.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk fbx 2009") ) );
}

void FBXExportScene::ExportSkeleton( const CSkeleton* skeleton )
{
	const Float exportScale = 100.f;

	// Cleanup current skeleton
	m_nodes.Clear();
	m_nodeMap.Clear();
	m_refNodes.Clear();

	// Create the bones
	const Uint32 numBones = skeleton->GetBonesNum();
	for ( Uint32 i=0; i<numBones; ++i )
	{
		const String srcBoneName = skeleton->GetBoneName(i);
		const Int32 srcBoneParent = skeleton->GetParentBoneIndex(i);
		const AnimQsTransform srcPose = skeleton->GetBoneLS(i);
		const Matrix srcPoseM = skeleton->GetBoneMatrixLS(i);

		// Create skeleton
		FbxSkeleton* skeletonAttr = FbxSkeleton::Create( m_scene, "Skeleton" );
		//skeletonAttr->Size.Set( 0.2 );

		if (srcBoneParent == -1) 
		{
			skeletonAttr->SetSkeletonType( FbxSkeleton::eRoot );
		}
		else
		{
			skeletonAttr->SetSkeletonType( FbxSkeleton::eLimbNode ); //originally eLimbNode 
		}

		// Create node
		FbxNode* newNode = FbxNode::Create( m_scene, UNICODE_TO_ANSI(srcBoneName.AsChar()) );
		newNode->SetNodeAttribute(skeletonAttr);

		// Export transform
		{
			const RedQuaternion& rot = srcPose.GetRotation();
			const FbxQuaternion q( rot.Quat.X, rot.Quat.Y, rot.Quat.Z, rot.Quat.W );
			const FbxDouble3 pos( (Double)srcPose.Translation.X * exportScale, (Double)srcPose.Translation.Y * exportScale, (Double)srcPose.Translation.Z * exportScale);
			const FbxAMatrix rotM(FbxVector4(0,0,0,0), q, FbxVector4(1,1,1,1));

			newNode->LclTranslation.Set( pos );
			newNode->LclRotation.Set( rotM.GetR() );
			newNode->LclScaling.Set( FbxDouble3( 1 ) );
			newNode->RotationOrder = eEulerXYZ; //always XYZ in SDK 2013.1!!!
		}

		// Bind to parent
		if (srcBoneParent == -1) 
		{
			m_scene->GetRootNode()->AddChild( newNode );
		}
		else
		{
			FbxNode* parentNode = m_nodes[srcBoneParent];
			parentNode->AddChild( newNode );
		}

		// Export crap
		m_scene->AddNode( newNode );

		// Remember
		const Uint32 nodeIndex = m_nodes.Size();
		m_nodes.PushBack( newNode );
		m_nodeMap.Set( srcBoneName.AsChar(), nodeIndex );

		Matrix refMatrix = skeleton->GetBoneMatrixMS(i);
		const Vector refScale = refMatrix.GetScale33();
		if ( refScale.DistanceTo(Vector::ONES) > 0.001f )
		{
			refMatrix.V[0].Normalize3();
			refMatrix.V[1].Normalize3();
			refMatrix.V[2].Normalize3();
		}

		m_refNodes.PushBack( refMatrix );
	}
}

