/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#pragma warning( push )
#pragma warning( disable:4530 ) // C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc
#include <vector>
#pragma warning( pop )

#include "fbxCommon.h"
#include "..\..\common\core\importer.h"
#include "..\..\common\engine\furMeshResource.h"

namespace
{
	FbxNode* findNodeByName(FbxScene* scene, const char* nodeName)
	{
		for (int i = 0; i < scene->GetNodeCount(); i++)
		{
			FbxNode* node = scene->GetNode(i);
			const char* name = node->GetName();
			if (!strcmp(nodeName, name))
				return node;
		}
		return 0;
	}

	Vector TransformFbxPoint(const Matrix& m, const FbxVector4& pt)
	{
		Vector result((Float) pt[0], (Float) pt[1], (Float) pt[2], (Float) pt[3]);
		result = m.TransformVectorAsPoint(result);
		return result;
	}

	Matrix ConvertFbxMatrix(const FbxAMatrix& m)
	{
		Matrix result;
		result.V[0].X = static_cast<Float>(m.Get(0,0));
		result.V[0].Y = static_cast<Float>(m.Get(1,0));
		result.V[0].Z = static_cast<Float>(m.Get(2,0));
		result.V[0].W = static_cast<Float>(m.Get(3,0));

		result.V[1].X = static_cast<Float>(m.Get(0,1));
		result.V[1].Y = static_cast<Float>(m.Get(1,1));
		result.V[1].Z = static_cast<Float>(m.Get(2,1));
		result.V[1].W = static_cast<Float>(m.Get(3,1));

		result.V[2].X = static_cast<Float>(m.Get(0,2));
		result.V[2].Y = static_cast<Float>(m.Get(1,2));
		result.V[2].Z = static_cast<Float>(m.Get(2,2));
		result.V[2].W = static_cast<Float>(m.Get(3,2));

		result.V[3].X = static_cast<Float>(m.Get(0,3));
		result.V[3].Y = static_cast<Float>(m.Get(1,3));
		result.V[3].Z = static_cast<Float>(m.Get(2,3));
		result.V[3].W = static_cast<Float>(m.Get(3,3));
		return result;
	}

	void InitializeSkinningDataFromFbxNode(
		FbxNode* bodyNode,
		Uint32   numPoints,
		Uint32&  numBones,
		Vector*  pBoneIndices,
		Vector*  pBoneWeights,
		TDynArray<CName>&   boneNames,
		TDynArray<Matrix>&  boneRigMatrices
		)
	{

		Uint32* indexCounts = new Uint32[numPoints];
		Red::System::MemorySet(indexCounts, 0, sizeof(Uint32) * numPoints);

		FbxGeometry* geometry = bodyNode->GetGeometry();
		FbxSkin* skin = (FbxSkin*)geometry->GetDeformer(0, FbxDeformer::eSkin);

		for (Uint32 i = 0; i < numPoints; i++)
		{
			pBoneIndices[i] = Vector(0, 0, 0, 0);
			pBoneWeights[i] = Vector(1.0f, 0, 0, 0);
		}

		numBones = skin->GetClusterCount();
		for (Uint32 i = 0; i < numBones ; i++)
		{
			FbxCluster* cluster = skin->GetCluster(i);

			const char* clusterName = cluster->GetName();

			FbxNode* node = cluster->GetLink();
			const char* name = node->GetName();

			FbxAMatrix fbxTransform;
			cluster->GetTransformLinkMatrix( fbxTransform );
			FbxAMatrix fbxTransformI;

			static Bool DO_INVERT = true;
			if ( DO_INVERT )
			{
				fbxTransformI = fbxTransform.Inverse();
			}
			else
			{
				fbxTransformI = fbxTransform;
			}

			/*fbxsdk_2014_1::FbxAMatrix fbxTransform2;
			cluster->GetTransformMatrix( fbxTransform2 );
			fbxsdk_2014_1::FbxAMatrix fbxTransform2I = fbxTransform2.Inverse();

			fbxsdk_2014_1::FbxAMatrix fbxTransform3;
			cluster->GetTransformParentMatrix( fbxTransform3 );
			fbxsdk_2014_1::FbxAMatrix fbxTransform3I = fbxTransform3.Inverse();

			fbxsdk_2014_1::FbxAMatrix fbxTransform4;
			cluster->GetTransformAssociateModelMatrix( fbxTransform4 );
			fbxsdk_2014_1::FbxAMatrix fbxTransform4I = fbxTransform4.Inverse();
			
			fbxsdk_2014_1::FbxAMatrix& fbxTransform5 = node->EvaluateGlobalTransform();
			fbxsdk_2014_1::FbxAMatrix fbxTransform5I = fbxTransform5.Inverse();

			fbxsdk_2014_1::FbxAMatrix& fbxTransform6 = node->EvaluateLocalTransform();
			fbxsdk_2014_1::FbxAMatrix fbxTransform6I = fbxTransform6.Inverse();*/


			Matrix engineTransform;

			//Red::System::MemoryCopy( &(engineTransform.V[0]), fbxTransform.Buffer(), 16 * sizeof( Float ) );
			double* matPtr = (double*)fbxTransformI;

			engineTransform.V[0].A[0] = (Float)matPtr[0];
			engineTransform.V[0].A[1] = (Float)matPtr[1];
			engineTransform.V[0].A[2] = (Float)matPtr[2];
			engineTransform.V[0].A[3] = (Float)matPtr[3];

			engineTransform.V[1].A[0] = (Float)matPtr[4];
			engineTransform.V[1].A[1] = (Float)matPtr[5];
			engineTransform.V[1].A[2] = (Float)matPtr[6];
			engineTransform.V[1].A[3] = (Float)matPtr[7];

			engineTransform.V[2].A[0] = (Float)matPtr[8];
			engineTransform.V[2].A[1] = (Float)matPtr[9];
			engineTransform.V[2].A[2] = (Float)matPtr[10];
			engineTransform.V[2].A[3] = (Float)matPtr[11];

			engineTransform.V[3].A[0] = (Float)matPtr[12] * 0.01f;
			engineTransform.V[3].A[1] = (Float)matPtr[13] * 0.01f;
			engineTransform.V[3].A[2] = (Float)matPtr[14] * 0.01f;
			engineTransform.V[3].A[3] = (Float)matPtr[15];

			boneNames.PushBack( CName( ANSI_TO_UNICODE( name ) ) );
			boneRigMatrices.PushBack( engineTransform );

			Uint32 cpCount = cluster->GetControlPointIndicesCount();
			int* indices = cluster->GetControlPointIndices();
			double*  weights = cluster->GetControlPointWeights();

			FbxTime lTime(0);

			for (Uint32 j = 0; j < cpCount; j++)
			{
				Uint32 idx = indices[j];
				Uint32& cnt = indexCounts[idx];
				if (cnt < 4)
				{
					pBoneIndices[idx].A[cnt] = (Float)i;
					pBoneWeights[idx].A[cnt] = float(weights[j]);
				}

				cnt++;
			}
		}
		delete [] indexCounts;
	}

	CFurMeshResource* LoadHairFromFbxNode( FbxNode* node, FbxNode* meshNode, FbxNode* skinNode = nullptr, CFurMeshResource* existingResource = nullptr )
	{
		if (!node || !meshNode)
			return 0;

		FbxLine* line = node->GetLine();
		if (!line)
			return 0;

		FbxMesh* mesh = meshNode->GetMesh();
		if (!mesh)
			return 0;

		const int indexSize = line->GetIndexArraySize();
		const int cpCount = line->GetControlPointsCount();
		const int curveCount = line->GetEndPointCount();
		
		FbxVector4* points = line->GetControlPoints();
		FbxArray<int>* indices = line->GetIndexArray();
		FbxArray<int>* endIndices = line->GetEndPointArray();

		// now start creating fur mesh resource
		CFurMeshResource* pResult = nullptr;
		if (existingResource)
		{
			pResult = existingResource;

			// clear the data, keep the setup
			pResult->m_positions.ClearFast();
			pResult->m_uvs.ClearFast();
			pResult->m_endIndices.ClearFast();
			pResult->m_faceIndices.ClearFast();
			pResult->m_boneIndices.ClearFast();
			pResult->m_boneWeights.ClearFast();
			pResult->m_boneNames.ClearFast();
			pResult->m_boneRigMatrices.ClearFast();
		}
		else
		{
			pResult = new CFurMeshResource();
		}

		const Matrix globalXform(ConvertFbxMatrix(node->EvaluateGlobalTransform()));
		pResult->m_importUnitsScale = (globalXform.V[0].X + globalXform.V[1].Y + globalXform.V[2].Z) / 3.0f;

		for (int i = 0; i < cpCount; i++)
			pResult->m_positions.PushBack(TransformFbxPoint(globalXform, points[i]*0.01f));

		for (int i = 0; i < curveCount; i++)
			pResult->m_endIndices.PushBack((*endIndices)[i]);

		// read mesh data
		FbxLayerElementUV* leUV = mesh->GetLayer(0)->GetUVs();
		int tcnt = 0;
		for (int i = 0; i < mesh->GetPolygonCount(); i++)
		{
			int vcnt = mesh->GetPolygonSize(i);
			for (int j = 0; j < (vcnt-2); j++)
			{      
			      Uint32 v0 = mesh->GetPolygonVertex(i, 0);
			      Uint32 v1 = mesh->GetPolygonVertex(i, j+1);
			      Uint32 v2 = mesh->GetPolygonVertex(i, j+2);

			      pResult->m_faceIndices.PushBack(v0);
			      pResult->m_faceIndices.PushBack(v1);
			      pResult->m_faceIndices.PushBack(v2);

			      Uint32 i0 = mesh->GetTextureUVIndex(i, 0);
			      Uint32 i1 = mesh->GetTextureUVIndex(i, 1);
			      Uint32 i2 = mesh->GetTextureUVIndex(i, 2);

				  // It's not clear why the V coords have to be flipped with 1.0f - texCoord[1].  I guess there is a
				  // flip somewhere in the max->FBX export pipe?  This seems visually correct.
			      FbxVector2 texCoord;

				  texCoord = leUV->GetDirectArray().GetAt(i0);
			      pResult->m_uvs.PushBack(Vector2((float)texCoord[0], 1.0f - (float)texCoord[1]));

			      texCoord = leUV->GetDirectArray().GetAt(i1);
			      pResult->m_uvs.PushBack(Vector2((float)texCoord[0], 1.0f - (float)texCoord[1]));

			      texCoord = leUV->GetDirectArray().GetAt(i2);
			      pResult->m_uvs.PushBack(Vector2((float)texCoord[0], 1.0f - (float)texCoord[1]));
			}
		}

		if (skinNode)
		{
			pResult->m_boneIndices.Resize( curveCount );
			pResult->m_boneWeights.Resize( curveCount );

			Uint32 numBones = 0;

			InitializeSkinningDataFromFbxNode( skinNode, curveCount, numBones, pResult->m_boneIndices.TypedData(), pResult->m_boneWeights.TypedData(), pResult->m_boneNames, pResult->m_boneRigMatrices);

			pResult->m_boneCount = numBones;
		}

		ASSERT(pResult->m_faceIndices.Size() % 3 == 0);
		ASSERT(pResult->m_uvs.Size() % 3 == 0);
		ASSERT(pResult->m_uvs.Size() == pResult->m_faceIndices.Size());
		ASSERT(pResult->m_positions.SizeInt()  == cpCount);
		ASSERT(pResult->m_endIndices.SizeInt() == curveCount);
		return pResult;
	}

	void resampleHairCurve(TDynArray<Vector>::const_iterator begin, TDynArray<Vector>::const_iterator end, TDynArray<Vector>& dst, Uint32 targetNbPoints)
	{
		// This is a safe assumption that we have less than MaxUint points in one hair strand.
		const Uint32 srcNbPoints = static_cast<Uint32>(end-begin);
		const Uint32 dstStartSize = dst.Size();
		std::vector<Float> accLengths(srcNbPoints+1);

		Float totalLength = 0;
		for (TDynArray<Vector>::const_iterator i = begin; i < end; i++)
		{
			Float length = (i == begin) ? 0.0f : (*i - *(i-1)).Mag3();
			totalLength += length;
			accLengths[i-begin] = totalLength;	
		}

		if (totalLength < FLT_EPSILON) // every vertex is at the same location
		{
			for (UINT i = 0; i < targetNbPoints; i++)
				dst.PushBack( *(begin+i) );
			return;
		}

		for (Uint32 i = 0; i < srcNbPoints; i++)
			accLengths[i] /= totalLength;

		accLengths[srcNbPoints] = 1.0f; 

		Uint32 searchStart = 0;
		for (Uint32 i = 0; i < targetNbPoints; i++)
		{
			Float t = i / Float(targetNbPoints-1);

			Uint32 j = searchStart;
			while ( j < srcNbPoints)
			{
				if (t < accLengths[j]) 
					break;			

				j++;
			}

			if (j >= srcNbPoints)
			{
				dst.PushBack( *(begin + srcNbPoints - 1) );
				continue;
			}

			const Vector& p0 = *(begin + j-1);
			const Vector& p1 = *(begin + j);
			Float delta = (t - accLengths[j-1]) / (accLengths[j] - accLengths[j-1] + FLT_EPSILON);

			dst.PushBack(p0 + (p1 - p0) * delta);
		}

		ASSERT(dst.Size() == dstStartSize + targetNbPoints);
	}

	// This function works in-place, it returns the pointer that it was given just for convenience
	CFurMeshResource* ResampleHairs( CFurMeshResource* pSrc, Uint32 targetNbPointsPerHair)
	{
		Uint32 numGuideCurves = pSrc->numGuideCurves();

		TDynArray< Vector > positions;
		positions.Reserve( pSrc->m_positions.Size() );

		for (Uint32 i = 0; i < numGuideCurves; i++)
		{
			Uint32 start, end;
			pSrc->getStartAndEnd(i, start, end);

			TDynArray<Vector>::const_iterator srcPoints = pSrc->m_positions.Begin() + start;

			// fill in resampled vertex data
			resampleHairCurve(srcPoints, srcPoints + end-start + 1, positions, targetNbPointsPerHair);
		}

		pSrc->m_positions = positions;
		pSrc->m_endIndices.ClearFast();

		for (Uint32 i = 0; i < numGuideCurves; i++)
			pSrc->m_endIndices.PushBack((i+1) * targetNbPointsPerHair - 1);

		ASSERT(pSrc->m_endIndices.Size() == pSrc->numGuideCurves());
		return pSrc;
	}
}

/// Importer for FBX files
class CFbxImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CFbxImporter, IImporter, 0 );

public:
	CFbxImporter();

	virtual CResource* DoImport( const ImportOptions& options );
};

BEGIN_CLASS_RTTI( CFbxImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CFbxImporter );

CFbxImporter::CFbxImporter()
{
	// Importer
	m_resourceClass = ClassID< CFurMeshResource >();
	m_formats.PushBack( CFileFormat( TXT("fbx"), TXT("Autodesk FBX") ) );

	// Load config
	LoadObjectConfig( TXT("User") );
}

CResource* CFbxImporter::DoImport( const ImportOptions& options )
{
	// Save
	SaveObjectConfig( TXT("User") );


	//////////////////////////////////////////////////////////////////////////
	// Prepare the FBX SDK.
	FbxManager* fbxManager = FbxManager::Create();
	if (!fbxManager)
	{
		RED_LOG_ERROR( CNAME( CFbxImporter ), TXT( "Unable to create the FBX SDK manager" ) );
		return nullptr;
	}

	// Create an IOSettings object.
	FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
	fbxManager->SetIOSettings( ios ); // Store IO settings here

	// Create an importer.
	FbxImporter* importer = FbxImporter::Create(fbxManager, "");

	// Initialize the importer.
	Bool importStatus = importer->Initialize( UNICODE_TO_ANSI(options.m_sourceFilePath.AsChar()), -1, fbxManager->GetIOSettings() );

	if( !importStatus )
	{
		RED_LOG_ERROR( CNAME( CFbxImporter ), TXT( "Call to FbxImporter::Initialize() failed." ) );
		RED_LOG_ERROR( CNAME( CFbxImporter ), TXT( "Error returned: %" ) RED_PRIWas, importer->GetStatus().GetErrorString() );
		return nullptr;
	}

	// File format version numbers to be populated.
	Int32 fileMajor;
	Int32 fileMinor;
	Int32 fileRevision;

	// Populate the FBX file format version numbers with the import file.
	importer->GetFileVersion( fileMajor, fileMinor, fileRevision );
	RED_LOG_SPAM( CNAME( CFbxImporter ), TXT( "File Version Major: %d Minor: %d Revision: %d" ) );

	// Create a new scene so it can be populated by the imported file.
	FbxScene* scene = FbxScene::Create( fbxManager, "importScene" );

	// Import the contents of the file into the scene.
	Bool importResult = importer->Import( scene );

	if( !importResult )
	{
		RED_LOG_ERROR( CNAME( CFbxImporter ), TXT( "Call to FbxImporter::Import() failed." ) );
		RED_LOG_ERROR( CNAME( CFbxImporter ), TXT( "Error returned: %" ) RED_PRIWas, importer->GetStatus().GetErrorString() );
		return nullptr;
	}

	// The file has been imported; we can get rid of the importer.
	importer->Destroy();

	//if( scene->GetGlobalSettings().GetSystemUnit() == FbxSystemUnit::cm )
	//{
	//	const FbxSystemUnit::ConversionOptions conversionOptions = {
	//		false, /* mConvertRrsNodes */
	//		true, /* mConvertAllLimits */
	//		true, /* mConvertClusters */
	//		true, /* mConvertLightIntensity */
	//		true, /* mConvertPhotometricLProperties */
	//		true  /* mConvertCameraClipPlanes */
	//	};

	//	// Convert the scene to meters using the defined options.
	//	FbxSystemUnit::m.ConvertScene( scene, conversionOptions );
	//}

	TDynArray< Vector > positions;
	TDynArray< Uint16 > indices;

	// This name is hard-coded and agreed in advance with the artists.  Could we supply a user-specified name to the import?
	const char* hairNodeName = "HairGuides";
	FbxNode* hairNode = findNodeByName(scene, hairNodeName);

	const char* meshNodeName = "GrowthMesh";
	FbxNode* meshNode = findNodeByName(scene, meshNodeName);

	if (hairNode && meshNode)
	{
		// checks for reimport
		CFurMeshResource* existingResource = nullptr;
		if (options.m_existingResource)
		{
			existingResource = static_cast< CFurMeshResource* >( options.m_existingResource );
			if (!existingResource)
			{
				RED_HALT( "Existing resource is not a fur resource while reimporting" );
			}
		}
		
		// For the sake of reimport we are doing everything in place, so these pointers are actually the same
		CFurMeshResource* pMeshRead = LoadHairFromFbxNode( hairNode, meshNode , meshNode, existingResource );
		CFurMeshResource* pResampled = ResampleHairs(pMeshRead, 5);
		return pResampled;
	}

	return nullptr;
}
