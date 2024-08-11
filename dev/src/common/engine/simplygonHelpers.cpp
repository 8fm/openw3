/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "simplygonHelpers.h"
#include "meshChunk.h"
#include "../../common/core/depot.h"

#ifdef USE_SIMPLYGON

#include "../../../external/Simplygon/SimplygonSDKLoader.cpp"


namespace SimplygonHelpers
{
	using namespace SimplygonSDK;

	void SimplygonProgressObserver::Execute( IObject* subject, rid EventId, void* EventParameterBlock, unsigned int EventParameterBlockSize )
	{
		if ( EventId == SG_EVENT_PROCESS_STARTED )
		{
			char* str = static_cast< char* >( EventParameterBlock );
			GFeedback->UpdateTaskInfo( ANSI_TO_UNICODE( str ) );
		}
		else if( EventId == SG_EVENT_PROGRESS )
		{
			// get the progress in percent
			int val = *static_cast< int * >( EventParameterBlock );
			// tell the process to continue
			// this is required by the progress event
			*static_cast< int * >( EventParameterBlock ) = 1;
			// output the progress update
			GFeedback->UpdateTaskProgress( val, 100 );
		}
	}


	class SimplygonErrorHandler : public rerrorhandler
	{
	public :
		// The HandleError receives processing errors
		virtual void HandleError( IObject* object, const char* interfacename, const char* methodname, rid errortype, const char* errortext ) override
		{
			RED_LOG_ERROR( RED_LOG_CHANNEL( Simplygon ), TXT("%hs"), errortext );
		}
	};

	static SimplygonErrorHandler GErrorHandler;

	Int32 InitSDK( ISimplygonSDK*& sdk )
	{
		sdk = nullptr;

		Int32 res = SimplygonSDK::Initialize( &sdk, TXT("x64/SimplygonSDKRuntimeReleasex64.dll") );

		if ( sdk != nullptr )
		{
			// set our own error handler to be able to log all the messages
			sdk->SetErrorHandler( &GErrorHandler );
		}

		return res;
	}

	void ShutdownSDK()
	{
		// deinit sdk
		SimplygonSDK::Deinitialize();
	}


	static Bool PrepareTempPath( const String& fileName, String& fullPath )
	{
		const String tmpDirName = TXT("simplygon_temp");

		CDirectory* tmpDir = GDepot->CreateNewDirectory( tmpDirName.AsChar() );
		if ( !tmpDir )
			return false;

		if ( !tmpDir->CreateOnDisk() )
			return false;				

		if ( CDiskFile* tmpFile = tmpDir->FindLocalFile( fileName ) )
		{
#ifndef NO_FILE_SOURCE_CONTROL_SUPPORT
			tmpFile->GetStatus(); // Without this Delete will fail ;)
			if ( !tmpFile->Delete( false, false ) )
#endif
			{
				return false;
			}
		}

		fullPath = tmpDir->GetAbsolutePath().AsChar() + fileName;
		return true;
	}

	Bool ExportToObj( ISimplygonSDK* sgSDK, spScene scene, const String& fileName )
	{
		String filePath;
		if ( !PrepareTempPath( fileName, filePath ) )
		{
			return false;
		}

		spWavefrontExporter objExp = sgSDK->CreateWavefrontExporter();
		objExp->SetExportFilePath( CUnicodeToAnsi( filePath.AsChar() ) );
		objExp->SetScene(scene);
		Bool success = objExp->RunExport();
		objExp = nullptr;
		return success;
	}

	spScene ImportFromObj( ISimplygonSDK* sgSDK, const String& fileName )
	{
		String filePath;
		if ( !PrepareTempPath( fileName, filePath ) )
		{
			return false;
		}

		spWavefrontImporter objImp = sgSDK->CreateWavefrontImporter();
		objImp->SetImportFilePath( CUnicodeToAnsi( filePath.AsChar() ) );
		objImp->RunImport();
		spScene scene = objImp->GetScene();
		objImp = nullptr;
		return scene;
	}

	const Char* GetErrorText( Int32 code )
	{
		switch ( code )
		{
		case SimplygonSDK::SG_ERROR_NOLICENSE: 
			return TXT("no license was found (or license expired)");
		case SimplygonSDK::SG_ERROR_NOTINITIALIZED: 
			return TXT("the SDK is not initialized, or no process object has been loaded/created");
		case SimplygonSDK::SG_ERROR_ALREADYINITIALIZED: 
			return TXT("the SDK is already initialized");
		case SimplygonSDK::SG_ERROR_FILENOTFOUND:
			return TXT("the specified file was not found");
		case SimplygonSDK::SG_ERROR_INVALIDPARAM:
			return TXT("an invalid parameter was passed to the method");
		case SimplygonSDK::SG_ERROR_FAILEDTESTING:
			return TXT("the reduction failed post-testing");
		case SimplygonSDK::SG_ERROR_WRONGVERSION:
			return TXT("the Simplygon DLL and header file interface versions do not match");
		case SimplygonSDK::SG_ERROR_LOADFAILED:
			return TXT("the Simplygon DLL failed loading, probably because of a missing dependency");
		case SimplygonSDK::SG_ERROR_FAILEDLOOKUP:
			return TXT("cannot reach the licensing server, cant look up server, check DNS");
		case SimplygonSDK::SG_ERROR_FAILEDCONTACT:
			return TXT("cannot contact the licensing server, check firewall/proxy server");
		case SimplygonSDK::SG_ERROR_FAILEDSEND:
			return TXT("cannot send data to the licensing server, check firewall/proxy server");
		case SimplygonSDK::SG_ERROR_FAILEDRCV:
			return TXT("cannot receive data from the licensing server, check firewall/proxy server");
		case SimplygonSDK::SG_ERROR_CORRUPTED:
			return TXT("data from licensing server is corrupted, try again, check connection");
		case SimplygonSDK::SG_ERROR_EXPIRED: 
		case SimplygonSDK::SG_ERROR_EXPIRED2: 
			return TXT("the license has expired");
		case SimplygonSDK::SG_ERROR_INVALIDLICENSE:
		case SimplygonSDK::SG_ERROR_INVALID_LICENSE2:
			return TXT("the license data is corrupted, please reinstall the license key");
		case SimplygonSDK::SG_ERROR_WRONGLICENSE:
			return TXT("the license is not for this product, please contact licensing, and replace license key");
		case SimplygonSDK::SG_ERROR_NONWCARD:
		case SimplygonSDK::SG_ERROR_NO_NWCARD:
			return TXT("no network card was found on the machine");
		case SimplygonSDK::SG_ERROR_DECODEFAILED:
		case SimplygonSDK::SG_ERROR_DECODEFAILED2:
		case SimplygonSDK::SG_ERROR_DECODEFAILED3:
		case SimplygonSDK::SG_ERROR_DECODE_FAILED:
		case SimplygonSDK::SG_ERROR_DECODEFAILED4:
		case SimplygonSDK::SG_ERROR_DECODEFAILED5:
			return TXT("could not decode license, it is corrupted");
		case SimplygonSDK::SG_ERROR_WRONGMACHINE:
		case SimplygonSDK::SG_ERROR_WRONG_MACHINE2:
			return TXT("the license is locked to another machine");
		case SimplygonSDK::SG_ERROR_INVALIDLICENSE2:
		case SimplygonSDK::SG_ERROR_INVALIDLICENSE3:
			return TXT("the license is invalid, please contact licensing");
		case SimplygonSDK::SG_ERROR_WRONG_LICENSE2:
			return TXT("the license is not for this product, please contact licensing, and replace license key");
		default:
			return TXT("Unknown error");
		}
	}

	spPackedGeometryData CreateGeometryFromMeshChunk( ISimplygonSDK* sgSDK, const SMeshChunk& chunk, Uint32 mtlIdOffset, Bool fixWinding )
	{
		//create the geometry

		//spGeometryData geom = sgSDK->CreateGeometryData();
		spPackedGeometryData packedGeom = sgSDK->CreatePackedGeometryData();
		spRealArray coords = packedGeom->GetCoords();
		spRidArray vertex_ids = packedGeom->GetVertexIds();

		// Must add texture channel before adding data to it. 
		packedGeom->AddTexCoords( 0 );
		spRealArray texcoords0 = packedGeom->GetTexCoords( 0 );
		texcoords0->SetAlternativeName( "TEXCOORD0" );

		// Must add texture channel before adding data to it. 
		packedGeom->AddTexCoords( 1 );
		spRealArray texcoords1 = packedGeom->GetTexCoords( 1 );
		texcoords1->SetAlternativeName( "TEXCOORD1" );

		// Must add normals channel before adding data to it. 
		packedGeom->AddNormals();
		spRealArray normals = packedGeom->GetNormals();

		// Must add tangents channel before adding data to it. 
		packedGeom->AddTangents( 0 );
		spRealArray tangents = packedGeom->GetTangents( 0 );
		spRealArray binormals = packedGeom->GetBitangents( 0 );

		// Must add color channel before adding data to it. 
		packedGeom->AddColors( 0 );
		spRealArray colors0 = packedGeom->GetColors( 0 );
		colors0->SetAlternativeName( "COLOR0" );

		packedGeom->AddBoneWeights( 4 );
		spRealArray boneWeights = packedGeom->GetBoneWeights();
		spRidArray boneIndices = packedGeom->GetBoneIds();

		packedGeom->AddMaterialIds();
		spRidArray materialIndices = packedGeom->GetMaterialIds();

		// HACK : Add a second vertex color. This will be vertexColor.gggg, which might be used to sample from a texture array.
		packedGeom->AddColors( 1 );
		spRealArray colors1 = packedGeom->GetColors( 1 );
		colors1->SetAlternativeName( "COLOR1" );

		// Set vertex- and triangle-counts for the Geometry. 
		// NOTE: The number of vertices and triangles has to be set before vertex- and triangle-data is loaded into the GeometryData.
		packedGeom->SetVertexCount( chunk.m_numVertices );
		packedGeom->SetTriangleCount( chunk.m_numIndices / 3 ); // dealing only with triangle lists

		// add vertex-coordinates
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			coords->SetTuple( vi , (real*)&(chunk.m_vertices[vi].m_position[0]) );
		}

		// add texture-coordinates
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			texcoords0->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_uv0[0]) );
		}

		// add texture-coordinates
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			texcoords1->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_uv1[0]) );
		}

		// add normals
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			normals->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_normal[0]) );
		}

		// add tangents
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			tangents->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_tangent[0]) );
		}

		// add binormals
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			binormals->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_binormal[0]) );
		}

		// add bone weights
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			boneWeights->SetTuple( vi, (real*)&(chunk.m_vertices[vi].m_weights[0]) );
		}

		// add bone id
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			Int32 indices[4] = { chunk.m_vertices[vi].m_indices[0], chunk.m_vertices[vi].m_indices[1], chunk.m_vertices[vi].m_indices[2], chunk.m_vertices[vi].m_indices[3] };
			boneIndices->SetTuple( vi, (rid*)&(indices[0]) );
		}

		// add vertex colors
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			Color color( chunk.m_vertices[vi].m_color );
			colors0->SetTuple( vi, (real*)&(color.ToVector().A[0]) );
		}

		// add indices
		for( Uint32 vi=0; vi<chunk.m_numIndices; ++vi )
		{
			vertex_ids->SetItem( vi , (rid)chunk.m_indices[vi] );
		}

		// add material IDs. This is constant for the full chunk, but required when dealing with multiple chunks together.
		for( Uint32 vi=0; vi<chunk.m_numIndices / 3; ++vi )
		{
			materialIndices->SetItem( vi, (rid)chunk.m_materialID + mtlIdOffset );
		}


		// HACK : add modified vertex color, using just G... used for sampling texture arrays...
		for( Uint32 vi=0; vi<chunk.m_numVertices; ++vi )
		{
			Uint8 g = (Uint8)(chunk.m_vertices[vi].m_color >> 8);
			Color color( g, g, g, g );
			colors1->SetTuple( vi, (real*)&(color.ToVector().A[0]) );
		}


		// fix for triangle winding
		if ( fixWinding )
		{
			for( Uint32 t = 0; t < packedGeom->GetTriangleCount(); t++ )
			{
				rid corner1 = vertex_ids->GetItem(t*3+1);
				rid corner2 = vertex_ids->GetItem(t*3+2);

				vertex_ids->SetItem(t*3+1, corner2);
				vertex_ids->SetItem(t*3+2, corner1);
			}
		}

		return packedGeom;
	}

	void ReduceGeometry( ISimplygonSDK* sgSDK, spScene scene, const SLODPresetDefinition& lodDefinition, Bool showProgress )
	{
		spReductionProcessor reducer = sgSDK->CreateReductionProcessor();
		reducer->SetScene(scene);

		// Set the Repair Settings. Current settings will mean that all visual gaps will remain in the geometry and thus 
		// hinder the reduction on geometries that contains gaps, holes and Tjunctions.
		spRepairSettings repair_settings = reducer->GetRepairSettings();
		// Only vertices that actually share the same position will be welded together
		repair_settings->SetWeldDist( 0.0f );
		// Only t-junctions with no actual visual distance will be fixed.
		repair_settings->SetTjuncDist( 0.0f );

		// Set the Reduction Settings.
		spReductionSettings reduction_settings = reducer->GetReductionSettings();

		//// These flags will make the reduction process respect group and material setups, 
		//// as well as preserve UV coordinates.
		//unsigned int FeatureFlagsMask = 0;
		//FeatureFlagsMask |= SG_FEATUREFLAGS_GROUP;
		//FeatureFlagsMask |= SG_FEATUREFLAGS_MATERIAL;
		//FeatureFlagsMask |= SG_FEATUREFLAGS_TEXTURE0;
		//reduction_settings->SetFeatureFlags( FeatureFlagsMask );

		reduction_settings->SetGeometryImportance( lodDefinition.m_geometryImportance );
		reduction_settings->SetTextureImportance( lodDefinition.m_textureImportance );
		reduction_settings->SetGroupImportance( lodDefinition.m_groupImportance );
		reduction_settings->SetMaterialImportance( lodDefinition.m_materialImportance );
		reduction_settings->SetVertexColorImportance( lodDefinition.m_vertexColorImportance );
		reduction_settings->SetShadingImportance( lodDefinition.m_shadingImportance );
		reduction_settings->SetSkinningImportance( 1.f );

		reduction_settings->SetKeepSymmetry(false); //Try, when possible to reduce symmetrically
		reduction_settings->SetUseAutomaticSymmetryDetection(false); //Auto-detect the symmetry plane, if one exists. Can, if required, be set manually instead.
		reduction_settings->SetUseHighQualityNormalCalculation(true); //Drastically increases the quality of the LODs normals, at the cost of extra processing time.
		reduction_settings->SetReductionHeuristics(SG_REDUCTIONHEURISTICS_CONSISTENT); //Choose between "fast" and "consistent" processing. Fast will look as good, but may cause inconsistent 
		//triangle counts when comparing MaxDeviation targets to the corresponding percentage targets.

		reduction_settings->SetStopCondition(SG_STOPCONDITION_ANY);					//The reduction stops when any of the targets below is reached
		reduction_settings->SetReductionTargets( SG_REDUCTIONTARGET_TRIANGLERATIO | SG_REDUCTIONTARGET_TRIANGLECOUNT );
		reduction_settings->SetTriangleRatio(lodDefinition.m_reduction);			//Targets at 50% of the original triangle count
		reduction_settings->SetTriangleCount(10);									//Targets when only 10 triangle remains
		reduction_settings->SetMaxDeviation(REAL_MAX);								//Targets when an error of the specified size has been reached. As set here it never happens.
		reduction_settings->SetOnScreenSize(50);									//Targets when the LOD is optimized for the selected on screen pixel size

		// Set the Normal Calculation Settings.
		spNormalCalculationSettings normal_settings = reducer->GetNormalCalculationSettings();

		// Will completely recalculate the normals.
		normal_settings->SetReplaceNormals( lodDefinition.m_recalculateNormals );
		normal_settings->SetHardEdgeAngleInRadians( DEG2RAD(lodDefinition.m_hardEdgeAngle) );

		// switch off mapping
		spMappingImageSettings mapping_settings = reducer->GetMappingImageSettings();
		mapping_settings->SetUseFullRetexturing( false );
		mapping_settings->SetGenerateMappingImage( false );
		mapping_settings->SetGenerateTexCoords( false );

		SimplygonHelpers::SimplygonProgressObserver progressObserver;

		// add observer for progress bar
		if ( showProgress )
		{
			GFeedback->BeginTask( TXT("Reducing geometry"), false );
			reducer->AddObserver( &progressObserver, SimplygonSDK::SG_EVENT_PROGRESS );
		}

		// reduce the mesh
		reducer->RunProcessing();

		if ( showProgress )
		{
			GFeedback->EndTask();
		}
	}

	void RemeshGeometry( ISimplygonSDK* sgSDK, spScene scene, Bool showProgress )
	{
		spRemeshingProcessor remesher = sgSDK->CreateRemeshingProcessor();
		remesher->GetRemeshingSettings()->SetOnScreenSize( 200 );
		remesher->GetMappingImageSettings()->SetGenerateMappingImage( false );
		remesher->SetScene(scene);

		SimplygonHelpers::SimplygonProgressObserver progressObserver;

		if ( showProgress )
		{
			GFeedback->BeginTask( TXT("Remeshing geometry"), false );
			remesher->AddObserver( &progressObserver, SimplygonSDK::SG_EVENT_PROGRESS );
		}

		remesher->RemeshGeometry();

		if ( showProgress ) 
		{
			GFeedback->EndTask();
		}
	}

	void CreateMeshChunkFromGeometry( ISimplygonSDK* sgSDK, spPackedGeometryData geom, Bool fixWinding, SMeshChunk& outChunk )
	{
		// we have to fix the winding on the packed geometry
		if ( fixWinding )
		{
			spRidArray vertex_ids = geom->GetVertexIds();
			for( Uint32 t = 0; t < geom->GetTriangleCount(); t++ )
			{
				rid corner1 = vertex_ids->GetItem(t*3+1);
				rid corner2 = vertex_ids->GetItem(t*3+2);

				vertex_ids->SetItem(t*3+1, corner2);
				vertex_ids->SetItem(t*3+2, corner1);
			}
		}

		spRidArray packedvertex_ids   = geom->GetVertexIds();
		spRealArray packedcoords      = geom->GetCoords();
		spRealArray packedtexcoords0  = geom->GetTexCoords( 0 );
		spRealArray packedtexcoords1  = geom->GetTexCoords( 1 );
		spRealArray packednormals     = geom->GetNormals();
		spRealArray packedtangents    = geom->GetTangents( 0 );
		spRealArray packedbinormals   = geom->GetBitangents( 0 );
		spRealArray packedcolors      = geom->GetColors( 0 );
		spRealArray packedboneweights = geom->GetBoneWeights();
		spRidArray packedboneindices  = geom->GetBoneIds();

		// add new chunk


		outChunk.m_numVertices = geom->GetVertexCount();
		outChunk.m_numIndices  = geom->GetTriangleCount() * 3;
		
		outChunk.m_vertices.Reserve( outChunk.m_numVertices );
		for ( Int32 vi = 0; vi < static_cast< Int32 >( outChunk.m_numVertices ); ++vi )
		{
			SMeshVertex vertex;
			//Float pos[3];
			spRealData pos = sgSDK->CreateRealData();
			//Float uv0[2];
			spRealData uv0 = sgSDK->CreateRealData();
			//Float uv1[2];
			spRealData uv1 = sgSDK->CreateRealData();
			//Float nrm[3];
			spRealData nrm = sgSDK->CreateRealData();
			//Float tan[3];
			spRealData tan = sgSDK->CreateRealData();
			//Float bnr[3];
			spRealData bnr = sgSDK->CreateRealData();
			//Float col[4];
			spRealData col = sgSDK->CreateRealData();
			//Float whs[4];
			spRealData whs = sgSDK->CreateRealData();
			//Int32 ids[4];
			spRidData ids = sgSDK->CreateRidData();

			packedcoords     ->GetTuple( vi, pos );
			packedtexcoords0 ->GetTuple( vi, uv0 );
			packedtexcoords1 ->GetTuple( vi, uv1 );
			packednormals    ->GetTuple( vi, nrm );
			packedtangents   ->GetTuple( vi, tan );
			packedbinormals  ->GetTuple( vi, bnr );
			packedcolors     ->GetTuple( vi, col );
			packedboneweights->GetTuple( vi, whs );
			packedboneindices->GetTuple( vi, ids );

			vertex.m_position[0] = pos[0];
			vertex.m_position[1] = pos[1];
			vertex.m_position[2] = pos[2];

			vertex.m_uv0[0] = uv0[0];
			vertex.m_uv0[1] = uv0[1];

			vertex.m_uv1[0] = uv1[0];
			vertex.m_uv1[1] = uv1[1];

			vertex.m_normal[0] = nrm[0];
			vertex.m_normal[1] = nrm[1];
			vertex.m_normal[2] = nrm[2];

			vertex.m_tangent[0] = tan[0];
			vertex.m_tangent[1] = tan[1];
			vertex.m_tangent[2] = tan[2];

			vertex.m_binormal[0] = bnr[0];
			vertex.m_binormal[1] = bnr[1];
			vertex.m_binormal[2] = bnr[2];

			vertex.m_weights[0] = whs[0];
			vertex.m_weights[1] = whs[1];
			vertex.m_weights[2] = whs[2];
			vertex.m_weights[3] = whs[3];

			vertex.m_indices[0] = static_cast<Uint8>( ids[0] );
			vertex.m_indices[1] = static_cast<Uint8>( ids[1] );
			vertex.m_indices[2] = static_cast<Uint8>( ids[2] );
			vertex.m_indices[3] = static_cast<Uint8>( ids[3] );

			vertex.m_color = Color( Vector( col ) ).ToUint32();

			outChunk.m_vertices.PushBack( vertex );
		}

		outChunk.m_indices.Reserve( outChunk.m_numIndices );
		for ( Uint32 ii = 0; ii < outChunk.m_numIndices; ++ii )
		{
			outChunk.m_indices.PushBack( static_cast< Uint16 >( packedvertex_ids->GetItem(ii) ) );
		}
	}


}

#endif
