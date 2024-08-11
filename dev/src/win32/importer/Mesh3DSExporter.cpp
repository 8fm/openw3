/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "fbxCommon.h"
#include "../../common/engine/terrainTile.h"
#include "../../common/engine/foliageEditionController.h"
#include "../../common/core/exporter.h"
#include "../../common/engine/meshVertex.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/engine/clipMap.h"
#include "../../common/core/feedback.h"
#include "../../common/core/diskFile.h"
#include "../../common/engine/baseTree.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/layerInfo.h"
#include "..\..\common\engine\meshComponent.h"
#include "..\..\common\engine\entity.h"
#include "..\..\common\engine\layer.h"
#include "..\..\common\engine\worldIterators.h"


/// 3DS mesh exporter
class CMesh3DSExporter : public IExporter
{
	DECLARE_ENGINE_CLASS( CMesh3DSExporter, IExporter, 0 );

public:
	CMesh3DSExporter();
	virtual Bool DoExport( const ExportOptions& options );
};

BEGIN_CLASS_RTTI( CMesh3DSExporter )
	PARENT_CLASS(IExporter)
	END_CLASS_RTTI()

	IMPLEMENT_ENGINE_CLASS(CMesh3DSExporter);

Bool GExportVolumeMeshes = false;
Bool GExportWorldSelectionOnly = false;
Bool GExportWorldMeshes = false;
Bool GExportWorldTerrain = false;
Bool GExportWorldTerrainAllTiles = false;
Bool GExportWorldFoliage = false;
Float GExportWorldFoliageRadius = 100.0f;
Int32 GExportIgnoreFoliageSmallerThan = 2;
Int32 GExportWorldTerrainTilesAmount = 1;
int GExportWorldTerrainMip = 1;
Vector GExportWorldOffset = Vector::ZEROS;
Vector GExportWorldTerrainPoint = Vector::ZEROS;
Box GExportWorldTerrainBox = Box::EMPTY;

CMesh3DSExporter::CMesh3DSExporter()
{
	// Supported class
	m_resourceClass = ClassID< CWorld >();

	// Supported formats
	FBXExportScene::FillExportFormat( m_formats );
	m_formats.PushBack( CFileFormat( TXT("3ds"), TXT("3d Studio Mesh") ) );
	m_formats.PushBack( CFileFormat( TXT("obj"), TXT("Lightwave OBJ") ) );
}

//////////////////////////// 3ds
#include <lib3ds.h>
#ifdef _WIN64
#ifdef _DEBUG
#pragma comment ( lib, "..\\..\\..\\external\\lib3ds\\debug_static\\lib3ds-2_0_x64.lib" )
#else
#pragma comment ( lib, "..\\..\\..\\external\\lib3ds\\release_static\\lib3ds-2_0_x64.lib" )
#endif
#else
#ifdef _DEBUG
#pragma comment ( lib, "..\\..\\..\\external\\lib3ds\\debug_static\\lib3ds-2_0.lib" )
#else
#pragma comment ( lib, "..\\..\\..\\external\\lib3ds\\release_static\\lib3ds-2_0.lib" )
#endif
#endif

namespace
{
	typedef float         Vertex3ds[3];
	typedef SMeshVertex VertexLava;

	typedef THashMap<CLayer *,  Lib3dsNode *> TLayersMap;
	typedef THashMap<CEntity *, Lib3dsNode *> TEntitiesMap;

	void AddMeshComponent( Lib3dsFile *file3ds, CMeshComponent *component, Uint32 iComponent,
		TLayersMap   &layers3ds,
		TEntitiesMap &entities3ds )
	{	
		CEntity *entity = component->GetEntity();

		//////////////////////////////////////////////////////////////////////////
		// Do not export characters
		if ( entity->QueryActorInterface() != NULL )
		{
			return;
		}

		CMesh* meshLava = NULL;
		CMeshTypeResource* meshTypeResource = component->GetMeshNow();
		if ( meshTypeResource && meshTypeResource->IsA<CMesh>() )
		{
			meshLava = SafeCast< CMesh >( meshTypeResource );
		}

		if (!meshLava)
			return;

		const CMeshData data( meshLava );
		const auto& chunks = data.GetChunks();

		//////////////////////////////////////////////////////////////////////////
		// No vertices -> nothing to export
		Uint32 totalMeshVertices = 0;
		for ( Uint32 chunk_i=0; chunk_i<chunks.Size(); ++chunk_i )
		{
			if ( chunk_i < chunks.Size() )
			{
				totalMeshVertices += chunks[chunk_i].m_vertices.Size();
			}
		}
		if ( totalMeshVertices == 0 )
			return;

		String name = (component->GetName().LeftString(4) + ToString(iComponent)).RightString(8);

		if (totalMeshVertices > 65532)
		{
			WARN_IMPORTER(TXT("Too many vertices to export in the mesh %s"), (entity->GetName() + TXT(":") + component->GetName()).AsChar() );
			return;
		}

		//////////////////////////////////////////////////////////////////////////
		// Create mesh
		Lib3dsMesh *mesh3ds = lib3ds_mesh_new( UNICODE_TO_ANSI( name.AsChar() ) );

		//////////////////////////////////////////////////////////////////////////
		// Init transformation
		Matrix localToWorld  = component->GetLocalToWorld();
		for (Uint32 iRow = 0; iRow < 4; ++iRow)
			for (Uint32 iCol = 0; iCol < 4; ++iCol)
				mesh3ds->matrix[iRow][iCol] = localToWorld.V[iRow].A[iCol];

		//////////////////////////////////////////////////////////////////////////
		// Copy vertices
		lib3ds_mesh_resize_vertices(mesh3ds, totalMeshVertices, FALSE, FALSE);		
		for ( Uint32 iChunk = 0, vertexOffset3ds = 0; iChunk < chunks.Size(); ++iChunk )
		{
			if ( iChunk < chunks.Size() )
			{
				const SMeshChunk &chunk = chunks[iChunk];
				for ( Uint32 iVertex = 0; iVertex < chunk.m_vertices.Size(); ++iVertex )
				{
					const VertexLava &vertexLava = chunk.m_vertices[iVertex];
					Vertex3ds        &vertex3ds  = mesh3ds->vertices[vertexOffset3ds + iVertex];

					vertex3ds[0] = vertexLava.m_position[0] - GExportWorldOffset.X;
					vertex3ds[1] = vertexLava.m_position[1] - GExportWorldOffset.Y;
					vertex3ds[2] = vertexLava.m_position[2] - GExportWorldOffset.Z;
				}
				vertexOffset3ds += chunk.m_vertices.Size();
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Copy faces
		Uint32 iFace = 0;
		TDynArray<Lib3dsFace> faces;
		for( Uint16 iChunk = 0, vertexOffset = 0; iChunk < chunks.Size(); ++iChunk )
		{
			const SMeshChunk &currChunk = chunks[iChunk];

			for( Uint32 iChunkIndice = 0; iChunkIndice < currChunk.m_indices.Size(); iChunkIndice += 3 )
			{
				Lib3dsFace face;
				face.index[0] = currChunk.m_indices[ iChunkIndice + 0 ] + vertexOffset;
				face.index[2] = currChunk.m_indices[ iChunkIndice + 1 ] + vertexOffset;
				face.index[1] = currChunk.m_indices[ iChunkIndice + 2 ] + vertexOffset;
				face.smoothing_group = 1 << iChunk;
				face.flags           = 0;
				face.material        = 0;
				faces.PushBack(face);
			}

			RED_ASSERT( vertexOffset + currChunk.m_vertices.Size() <= MAXINT16, TXT("too many vertices in exported mesh, the data will be broken") );

			vertexOffset += static_cast<Uint16>( currChunk.m_vertices.Size() );
		}

		if (faces.Size() > 65532)
		{
			WARN_IMPORTER(TXT("Too many faces to export in the mesh %s"), (entity->GetName() + TXT(":") + component->GetName()).AsChar() );
			lib3ds_mesh_free(mesh3ds);
			return;
		}

		lib3ds_mesh_resize_faces(mesh3ds, faces.Size());
		for ( Uint32 iFace = 0; iFace < faces.Size(); ++iFace )
			mesh3ds->faces[iFace] = faces[iFace];

		//////////////////////////////////////////////////////////////////////////
		// Create mesh instance within the layer->entity->mesh tree
		Lib3dsNode *parent = NULL;
		if (!entities3ds.Find(entity, parent))
		{
			Lib3dsNode *layer = NULL;
			if (!layers3ds.Find(entity->GetLayer(), layer))
			{
				String name = entity->GetLayer()->GetFile()->GetFileName().LeftString(8);
				layer = (Lib3dsNode*)lib3ds_node_new_mesh_instance(NULL, UNICODE_TO_ANSI( name.AsChar() ), NULL, NULL, NULL);
				lib3ds_file_append_node(file3ds, layer, NULL);
				layers3ds.Insert(entity->GetLayer(), layer);
			}

			String name = (entity->GetName().LeftString(4) + ToString(iComponent)).RightString(8);
			parent = (Lib3dsNode*)lib3ds_node_new_mesh_instance(NULL, UNICODE_TO_ANSI( name.AsChar() ), NULL, NULL, NULL);
			lib3ds_file_append_node(file3ds, parent, layer);
			entities3ds.Insert(entity, parent);
		}

		// Append mesh
		lib3ds_file_insert_mesh(file3ds, mesh3ds, -1);
		Lib3dsMeshInstanceNode *inst = lib3ds_node_new_mesh_instance(mesh3ds, UNICODE_TO_ANSI( name.AsChar() ), NULL, NULL, NULL);
		lib3ds_file_append_node(file3ds, (Lib3dsNode*)inst, parent);
	}

	Bool ExportWorldTo3ds( CWorld *world, const String &fileName )
	{
		ASSERT( world );

		Lib3dsFile *file3ds = lib3ds_file_new();

		{
			Lib3dsMaterial *mat = lib3ds_material_new("entities");
			lib3ds_file_insert_material(file3ds, mat, -1);
			mat->diffuse[0] = 0.8;
			mat->diffuse[1] = 0.6;
			mat->diffuse[2] = 0.3;

			mat = lib3ds_material_new("terrain");
			lib3ds_file_insert_material(file3ds, mat, -1);
			mat->diffuse[0] = 0.1;
			mat->diffuse[1] = 0.6;
			mat->diffuse[2] = 0.1;
		}

		TLayersMap   layers3ds;
		TEntitiesMap entities3ds;

		Uint32 iComponent = 0;
		for ( WorldAttachedComponentsIterator it( world ); it; ++it, ++iComponent )
		{
			CComponent *component = *it;
			if ( !component || !component->GetLayer() || !component->GetLayer()->GetLayerInfo() || !component->GetLayer()->GetLayerInfo()->IsVisible() )
				continue;

			// skip not selected shit
			if ( GExportWorldSelectionOnly && !component->IsSelected() )
				continue;

			// Export mesh
			CMeshComponent *meshComponent = Cast<CMeshComponent>( component );
			if ( GExportWorldMeshes && meshComponent )
			{
				if ( meshComponent->GetMeshNow() )
				{
					AddMeshComponent( file3ds, meshComponent, iComponent, layers3ds, entities3ds );
					continue;
				}
			}
		}

		// Save generated file
		Bool res = lib3ds_file_save( file3ds, UNICODE_TO_ANSI( fileName.AsChar() ) ) == TRUE;
		lib3ds_file_free( file3ds );

		// Done
		return res;
	}
}

//////////////////////////// obj
namespace
{
	typedef SMeshVertex VertexLava;

	void AddMeshComponent( FILE *fileObj, CMeshComponent *component, Uint32 iComponent, Uint32& vNum, Uint32& vnNum, Uint32& vtNum )
	{	
		CEntity *entity = component->GetEntity();

		// Do not export characters
		if ( entity->QueryActorInterface() != NULL )
		{
			return;
		}

		// Do not export empty meshes
		CMesh *meshLava = NULL;
		CMeshTypeResource* meshTypeResource = component->GetMeshNow();
		if ( meshTypeResource && meshTypeResource->IsA< CMesh >() )
		{
			meshLava = SafeCast< CMesh >( meshTypeResource );
		}

		if ( !meshLava )
		{
			return;
		}

		// Get placement matrix
		Matrix localToWorld = component->GetLocalToWorld();

		// Preload mesh data
		const CMesh::TLODLevelArray &lodChunks = meshLava->GetMeshLODLevels();
		if ( lodChunks.Size() && lodChunks[0].m_chunks.Size() )
		{
			// Begin entity
			String name = component->GetFriendlyName();
			fprintf( fileObj, "o %s\n", UNICODE_TO_ANSI( name.AsChar() ) );


			const CMeshData data( meshLava );
			const auto& chunks = data.GetChunks();

			// Export each chunk from LOD0
			for ( Uint32 i=0; i<lodChunks[0].m_chunks.Size(); i++ )
			{
				if ( lodChunks[0].m_chunks[i] >= chunks.Size() )
				{
					continue;
				}

				// Get the chunk to export
				const SMeshChunk& sourceChunk = chunks[ lodChunks[0].m_chunks[i] ];

				// Name group using material name
				String groupName = String::Printf( TXT("group%i"), i );
				if ( sourceChunk.m_materialID < meshLava->GetMaterialNames().Size() )
				{
					String materialName = meshLava->GetMaterialNames()[ sourceChunk.m_materialID ];
					if ( !materialName.Empty() )
					{
						groupName = materialName;
					}
				}
				fprintf( fileObj, "g %s\n", UNICODE_TO_ANSI( groupName.AsChar() ) );

				// Emit vertices
				Uint32 baseV = vNum;
				Uint32 baseVT = vnNum;
				Uint32 baseVN = vnNum;
				for ( Uint32 j=0; j<sourceChunk.m_vertices.Size(); j++ )
				{
					const SMeshVertex& sourceVertex = sourceChunk.m_vertices[j];
					Vector pos = localToWorld.TransformPoint( Vector( sourceVertex.m_position ) ) - GExportWorldOffset;
					Vector normal = localToWorld.TransformVector( Vector( sourceVertex.m_normal ) ).Normalized3();
					fprintf( fileObj, "v %f %f %f\n", pos.X, pos.Y, pos.Z );
					fprintf( fileObj, "vn %f %f %f\n", normal.X, normal.Y, normal.Z );
					fprintf( fileObj, "vt %f %f\n", sourceVertex.m_uv0[0], 1.0f - sourceVertex.m_uv0[1] );
					vNum++;
					vtNum++;
					vnNum++;
				}

				// Emit faces
				for ( Uint32 j=0; j<sourceChunk.m_indices.Size(); j+=3 )
				{
					Uint32 a = (Uint32) sourceChunk.m_indices[ j+2 ];
					Uint32 b = (Uint32) sourceChunk.m_indices[ j+1 ];
					Uint32 c = (Uint32) sourceChunk.m_indices[ j+0 ];
					fprintf( fileObj, "f %i/%i/%i %i/%i/%i %i/%i/%i\n", a + baseV, a + baseVT, a + baseVN, b + baseV, b + baseVT, b + baseVN, c + baseV, c + baseVT, c + baseVN );
				}
			}
		}
	}

	Bool ExportWorldToObj( CWorld *world, const String &fileName )
	{
		ASSERT( world );

		// Open file name
		FILE* fileObj = fopen( UNICODE_TO_ANSI( fileName.AsChar() ), "w" );
		if ( !fileObj )
		{
			return false;
		}

		// Export crap
		Uint32 vNum = 1, vtNum = 1, vnNum = 1;
		Uint32 iComponent = 0;
		for ( WorldAttachedComponentsIterator it( world ); it; ++it, ++ iComponent )
		{
			CComponent *component = *it;
			if ( !component || !component->GetLayer() || !component->GetLayer()->GetLayerInfo() || !component->GetLayer()->GetLayerInfo()->IsVisible() )
				continue;

			// skip not selected shit
			if ( GExportWorldSelectionOnly && !component->IsSelected() )
				continue;

			// Export mesh
			CMeshComponent *meshComponent = Cast<CMeshComponent>( component );
			if ( GExportWorldMeshes && meshComponent )
			{
				if ( meshComponent->GetMeshNow() )
				{
					AddMeshComponent( fileObj, meshComponent, iComponent, vNum, vtNum, vnNum );
					continue;
				}
			}
		}

		// Save generated file
		fclose( fileObj );
		return true;
	}
}


struct FbxVectorHashFunc
{
	static RED_FORCE_INLINE Uint32 GetHash( const Vector& key )
	{
		return ( *((Uint32*)&key.X) ) ^ ( *((Uint32*)&key.Y) ) ^ ( *((Uint32*)&key.Z) );
	}
};

namespace
{
	Bool FbxSaveScene( FbxManager* fbxManager, FbxScene* fbxScene, const char* filename, EFBXFileVersion version, bool embedMedia = false )
	{
		bool status = true;
		// Create an exporter.
		FbxExporter* exporter = FbxExporter::Create( fbxManager, "" );

		// Write in fall back format if pEmbedMedia is true
		Int32 fileFormat = fbxManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		if ( version == EFBXFileVersion::EFBX_2009 )
		{ 
			exporter->SetFileExportVersion( FBX_2009_00_COMPATIBLE );
			fileFormat = fbxManager->GetIOPluginRegistry()->FindWriterIDByDescription("fbx 6.0 binary (*.fbx)");
		} 
		else if ( version == EFBXFileVersion::EFBX_2010 )
		{ 
			exporter->SetFileExportVersion( FBX_2010_00_COMPATIBLE );
			fileFormat = fbxManager->GetIOPluginRegistry()->FindWriterIDByDescription("fbx 6.0 binary (*.fbx)");
		} 
		else if ( version == EFBXFileVersion::EFBX_2013 )
		{ 
			exporter->SetFileExportVersion( FBX_2013_00_COMPATIBLE );
		} 
		else
		{ 
			exporter->SetFileExportVersion( FBX_DEFAULT_FILE_COMPATIBILITY );
		}

		int major, minor, revision;
		FbxManager::GetFileFormatVersion( major, minor, revision );
		RED_LOG_SPAM( CNAME( CMesh3DSExporter ), TXT("FBX version number for this version of the FBX SDK is %d.%d.%d\n\n"), major, minor, revision );

		if ( !embedMedia )
		{
			//Try to export in ASCII if possible
			int formatIndex, formatCount = fbxManager->GetIOPluginRegistry()->GetWriterFormatCount();

			for (formatIndex=0; formatIndex<formatCount; formatIndex++)
			{
				if (fbxManager->GetIOPluginRegistry()->WriterIsFBX( formatIndex ))
				{
					StringAnsi description = fbxManager->GetIOPluginRegistry()->GetWriterFormatDescription( formatIndex );
					if ( description.ContainsSubstring( "ascii" ) )
					{
						fileFormat = formatIndex;
						break;
					}
				}
			}
		}

		if ( fbxManager->GetIOPluginRegistry()->WriterIsFBX( fileFormat ) )
		{
			// Export options determine what kind of data is to be imported.
			// The default (except for the option eEXPORT_TEXTURE_AS_EMBEDDED)
			// is true, but here we set the options explicitly.
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_MATERIAL,        true );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_TEXTURE,         true );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_EMBEDDED,        embedMedia );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_SHAPE,           true );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_GOBO,            true );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_ANIMATION,       true );
			fbxManager->GetIOSettings()->SetBoolProp( EXP_FBX_GLOBAL_SETTINGS, true );
		}

		// Set the file format
		// Initialize the exporter by providing a filename.
		// Initialize the exporter by providing a filename.
		if( exporter->Initialize( filename, fileFormat, fbxManager->GetIOSettings() ) == false)
		{
			RED_LOG_SPAM( CNAME( CMesh3DSExporter ), TXT("Call to KFbxExporter::Initialize() failed.") );
			//RED_LOG_SPAM( CNAME( CMesh3DSExporter ), TXT("Error returned: %s"), exporter->GetStatus().GetErrorString() );
			return false;
		}

		// Export the scene.
		status = exporter->Export( fbxScene );

		// Destroy the exporter.
		exporter->Destroy();

		return status;
	}

	void AddNewTerrainTile( TDynArray<Vector>& vertices, const Float baseX, const Float baseY, const Float tileSize, const SClipmapParameters& params, const Uint16* normalizedHeightArray, const TControlMapType* clipArray, int mip )
	{
		const Uint16* normHeight = normalizedHeightArray;
		const Float tileHeight = params.highestElevation - params.lowestElevation;
		const Uint32 tileRes = params.tileRes/(1<<mip);
		const Float cellSize = tileSize/(Float)tileRes;

		for ( Uint32 ty=0; ty<tileRes - 1; ty++ )
		{
			for ( Uint32 tx=0; tx<tileRes - 1; tx++ )
			{
				if ( (clipArray[ty*tileRes + tx] == 0) ) continue;

				Float x1 = baseX + tx*cellSize;
				Float y1 = baseY + ty*cellSize;
				Float x2 = baseX + tx*cellSize + cellSize;
				Float y2 = baseY + ty*cellSize + cellSize;

				Float height1 = params.lowestElevation + tileHeight*(((Float)(normHeight[ty*tileRes + tx]))/65536.f);
				Float height2 = params.lowestElevation + tileHeight*(((Float)(normHeight[ty*tileRes + tx + 1]))/65536.f);
				Float height3 = params.lowestElevation + tileHeight*(((Float)(normHeight[(ty + 1)*tileRes + tx + 1]))/65536.f);
				Float height4 = params.lowestElevation + tileHeight*(((Float)(normHeight[(ty + 1)*tileRes + tx]))/65536.f);

				vertices.PushBack( Vector( x1, y1, height1 ) );
				vertices.PushBack( Vector( x2, y1, height2 ) );
				vertices.PushBack( Vector( x2, y2, height3 ) );
				vertices.PushBack( Vector( x1, y2, height4 ) );
			}
		}
	}

	void AddNewTerrain( FbxMesh* mesh, CClipMap* clipMap )
	{
		const Float tileSize = clipMap->GetBoxForTile( 1, 0, 0 ).Min.X - clipMap->GetBoxForTile( 0, 0, 0 ).Min.X;

		TDynArray<Vector> vertices;

		SClipmapParameters params;
		clipMap->GetClipmapParameters( &params );

		Int32 cx, cy;
		clipMap->GetTileFromPosition( GExportWorldTerrainPoint, cx, cy );
		Int32 startx = 0, starty = 0, stopx = clipMap->GetNumTilesPerEdge() - 1, stopy = clipMap->GetNumTilesPerEdge() - 1;
		if ( !GExportWorldTerrainAllTiles )
		{
			Int32 radius = GExportWorldTerrainTilesAmount;
			startx = cx - radius;
			starty = cy - radius;
			stopx = cx + radius;
			stopy = cy + radius;
		}

		for ( Int32 row= starty; row <= stopy; ++row )
		{
			for ( Int32 column= startx; column <= stopx; ++column )
			{
				// skip invalid tile coordinates
				if ( column < 0 || row < 0 || column >= (Int32)clipMap->GetNumTilesPerEdge() || row >= (Int32)clipMap->GetNumTilesPerEdge() )
				{
					continue;
				}

				const Box box = clipMap->GetBoxForTile( column, row, 0 );
				CTerrainTile* tile = clipMap->GetTile( column, row );
				int mip = GExportWorldTerrainMip;
				const Uint16* normalizedHeightArray = NULL;
				const TControlMapType* clipArray = NULL;
				while ( mip >= 0 && normalizedHeightArray == NULL && clipArray == NULL )
				{
					normalizedHeightArray = tile->GetLevelSyncHM( mip );
					clipArray = tile->GetLevelSyncCM( mip );
					if ( normalizedHeightArray == NULL || clipArray == NULL )
					{
						mip--;
					}
				}

				AddNewTerrainTile( vertices, box.Min.X, box.Min.Y, tileSize, params, normalizedHeightArray, clipArray, mip );
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Copy vertices
		THashMap< Vector, Uint32, FbxVectorHashFunc > uniqueVertices;
		TDynArray< Vector > uniques;
		for ( auto it=vertices.Begin(); it != vertices.End(); ++it )
		{
			const Vector& v = *it;
			if ( !uniqueVertices.KeyExist( v ) )
			{
				uniqueVertices.Insert( v, uniques.Size() );
				uniques.PushBack( v );
			}
		}

		mesh->InitControlPoints( uniques.Size() );
		FbxVector4 *verticesFbx = mesh->GetControlPoints();


		for ( Uint32 iVertex = 0; iVertex < uniques.Size(); ++iVertex )
		{
			const Vector& vertexLava = uniques[iVertex];
			FbxVector4& vertexFbx  = verticesFbx[iVertex];

			vertexFbx[0] = vertexLava.X - GExportWorldOffset.X;
			vertexFbx[1] = vertexLava.Y - GExportWorldOffset.Y;
			vertexFbx[2] = vertexLava.Z - GExportWorldOffset.Z;
			vertexFbx[3] = 1.0f;
		}

		//////////////////////////////////////////////////////////////////////////
		// Copy faces
		for ( Uint32 i=3; i<vertices.Size(); i += 4 )
		{
			mesh->BeginPolygon( 0 );
			mesh->AddPolygon( uniqueVertices[ vertices[ i - 3 ] ] );
			mesh->AddPolygon( uniqueVertices[ vertices[ i - 2 ] ] );
			mesh->AddPolygon( uniqueVertices[ vertices[ i - 1 ] ] );
			mesh->AddPolygon( uniqueVertices[ vertices[ i ] ] );
			mesh->EndPolygon();
		}
	}

	Uint32 CountMeshLOD0Vertices( const TDynArray< SMeshChunk > & chunks, CMesh::LODLevel currentLevelInfo )
	{
		Uint32 totalMeshVertices = 0;
		for ( Uint32 i = 0; i < currentLevelInfo.m_chunks.Size(); ++i )
		{
			Uint32 chunk_i = currentLevelInfo.m_chunks[i];
			if ( chunk_i < chunks.Size() )
			{
				totalMeshVertices += chunks[chunk_i].m_vertices.Size();
			}
		}
		return totalMeshVertices;
	}

	void CopyVertices( FbxVector4 *verticesFbx, const CMesh::LODLevel& currentLevel, const CMeshData& meshData, const Matrix& localToWorld = Matrix::IDENTITY )
	{
		const auto& chunks = meshData.GetChunks();
		for ( Uint32 i = 0, iVerticesOffset = 0; i < currentLevel.m_chunks.Size(); ++i )
		{
			Uint32 iChunk = currentLevel.m_chunks[i];
			if ( iChunk < chunks.Size() )
			{
				const SMeshChunk &chunk = chunks[iChunk];

				for ( Uint32 iVertex = 0; iVertex < chunk.m_vertices.Size(); ++iVertex )
				{
					Vector vertexLava = localToWorld.TransformPoint( chunk.m_vertices[iVertex].m_position );
					FbxVector4 &vertexFbx = verticesFbx[iVerticesOffset + iVertex];

					vertexFbx[0] = vertexLava.X - GExportWorldOffset.X;
					vertexFbx[1] = vertexLava.Y - GExportWorldOffset.Y;
					vertexFbx[2] = vertexLava.Z - GExportWorldOffset.Z;
					vertexFbx[3] = 1.0f;
				}

				iVerticesOffset += chunk.m_vertices.Size();
			}
		}
	}

	void CopyUV( FbxMesh *mesh, const CMesh::LODLevel& currentLevel, const CMeshData& meshData )
	{
		const auto& chunks = meshData.GetChunks();
		for ( Uint32 i = 0, iVerticesOffset = 0; i < currentLevel.m_chunks.Size(); ++i )
		{
			Uint32 iChunk = currentLevel.m_chunks[i];
			if ( iChunk < chunks.Size() )
			{
				const SMeshChunk &chunk = chunks[iChunk];

				for ( Uint32 iVertex = 0; iVertex < chunk.m_vertices.Size(); ++iVertex )
				{
					Float u = chunk.m_vertices[iVertex].m_uv0[0];
					Float v = 1.f - chunk.m_vertices[iVertex].m_uv0[1];
					mesh->AddTextureUV( FbxVector2( u, v ) );
				}

				iVerticesOffset += chunk.m_vertices.Size();
			}
		}
	}

	void CopyFaces( FbxMesh *mesh, Uint32& localVertexOffset, Uint32& nMeshVertexOffset, const CMesh::LODLevel& currentLevel, const CMeshData& meshData, Int32 materialId )
	{
		const auto& chunks = meshData.GetChunks();
		for( Uint32 i = 0; i < currentLevel.m_chunks.Size(); ++i )
		{
			Uint32 iChunk = currentLevel.m_chunks[i];

			if ( iChunk < chunks.Size() )
			{
				const SMeshChunk &currChunk = chunks[iChunk];
				Uint32 material = materialId < 0 ? currChunk.m_materialID : materialId;

				for( Uint32 iChunkIndice = 0; iChunkIndice < currChunk.m_indices.Size(); iChunkIndice += 3 )
				{
					mesh->BeginPolygon( material );
					mesh->AddPolygon( currChunk.m_indices[ iChunkIndice + 0 ] + localVertexOffset + nMeshVertexOffset );
					mesh->AddPolygon( currChunk.m_indices[ iChunkIndice + 2 ] + localVertexOffset + nMeshVertexOffset );
					mesh->AddPolygon( currChunk.m_indices[ iChunkIndice + 1 ] + localVertexOffset + nMeshVertexOffset );
					mesh->EndPolygon();
				}

				localVertexOffset += currChunk.m_vertices.Size();
			}
		}
		nMeshVertexOffset += localVertexOffset;
	}

	void AddMeshComponent( FbxMesh *mesh, CMeshComponent *component, Uint32 &nMeshVertexOffset, Int32 materialIndex )
	{	
		CMesh *meshLava = component->GetMeshNow();
		if ( meshLava == NULL )
			return;

		const CMesh::TLODLevelArray& lodLvlInfo = meshLava->GetMeshLODLevels();
		if ( lodLvlInfo.Size() == 0 )
		{
			return;
		}

		CMesh::LODLevel currentLevel = lodLvlInfo[0];
		CMeshData meshData( meshLava );

		//////////////////////////////////////////////////////////////////////////
		// No vertices -> nothing to export
		Uint32 totalMeshVertices =  CountMeshLOD0Vertices( meshData.GetChunks(), currentLevel );
		if ( totalMeshVertices == 0 )
			return;

		//////////////////////////////////////////////////////////////////////////
		// Init transformation
		Matrix localToWorld  = component->GetLocalToWorld();

		//////////////////////////////////////////////////////////////////////////
		// Copy vertices
		//mesh->InitControlPoints( vertices.Size() );
		FbxVector4 *verticesFbx = mesh->GetControlPoints() + nMeshVertexOffset;
		CopyVertices( verticesFbx, currentLevel, meshData, localToWorld );

		//////////////////////////////////////////////////////////////////////////
		// Copy faces
		Uint32 localVertexOffset = 0;
		CopyFaces( mesh, localVertexOffset, nMeshVertexOffset, currentLevel, meshData, materialIndex );

		ASSERT( localVertexOffset == totalMeshVertices );
	}

	void CreateFoliageTubePrimitive( const Vector& point1, const Vector& point2, Float radius, const Vector& position, FbxMesh* mesh, Uint32& vertexOffset )
	{		
		const Uint16 numPoints = 16;

		TDynArray< Vector > vertices;
		vertices.Reserve( ( numPoints + 1 ) * 2 );

		TDynArray< Uint16 > indices;
		indices.Reserve( numPoints * 6 );

		const Float angle = DEG2RAD( 90.f );

		Vector norm = ( point2 - point1 );
		norm.W = 0;
		norm.Normalize3();

		Float a = MAcos( Vector::Dot3( norm, Vector( 0, 0, 1 ) ) );
		Plane p( norm, Vector::ZERO_3D_POINT );
		Vector v( 1, 1, 1 );
		v = p.Project( v );
		if ( v.SquareMag3() <= NumericLimits<Float>::Epsilon() )
		{
			v = Vector( 1, -2, 1 );
			v = p.Project( v );
		}
		v.Normalize3();

		Float qSin = MSin( M_PI / (Float)numPoints );
		Float qCos = MCos( M_PI / (Float)numPoints );
		Vector quat( norm.X * qSin, norm.Y * qSin, norm.Z * qSin, qCos );
		Matrix m;
		m.BuildFromQuaternion( quat );

		for ( Uint32 i=0; i<=numPoints; i++ )
		{
			vertices.PushBack( Vector( point1 + ( v * radius ) ) );
			vertices.PushBack( Vector( point2 + ( v * radius ) ) );
			v = m.TransformVector( v );
		}

		vertices.PushBack( point1 );
		vertices.PushBack( point2 );

		// add control points
		FbxVector4* lControlPoints = mesh->GetControlPoints();
		for( Uint32 i=0; i< vertices.Size(); ++i )
		{
			lControlPoints[i+vertexOffset].Set( position.X+vertices[i].X, position.Y+vertices[i].Y, position.Z+vertices[i].Z );
		}

		// Generate indices
		for ( Uint16 i=0; i<numPoints; i++ )
		{
			indices.PushBack( i * 2 + 2 );
			indices.PushBack( i * 2 );
			indices.PushBack( i * 2 + 1 );
			indices.PushBack( i * 2 + 3 );
			indices.PushBack( i * 2 + 2 );
			indices.PushBack( i * 2 + 1 );
		}
		for ( Uint16 i=0; i<numPoints; i++ )
		{
			indices.PushBack( i * 2 + 2 );
			indices.PushBack( (Uint16)vertices.Size() - 2 );
			indices.PushBack( i * 2 );
			indices.PushBack( i * 2 + 1);
			indices.PushBack( (Uint16)vertices.Size() - 1 );
			indices.PushBack( i * 2 + 3 );
		}

		for( Uint32 i = 0; i < indices.Size(); i+=6 )
		{
			mesh->BeginPolygon();
			mesh->AddPolygon( indices[i+0]+vertexOffset );
			mesh->AddPolygon( indices[i+1]+vertexOffset );
			mesh->AddPolygon( indices[i+2]+vertexOffset );
			mesh->AddPolygon( indices[i+3]+vertexOffset );
			mesh->AddPolygon( indices[i+4]+vertexOffset );
			mesh->AddPolygon( indices[i+5]+vertexOffset );
			mesh->EndPolygon();
		}

		// update offset for mesh
		vertexOffset += vertices.Size();
	}

	void AddFoliageInstance( CSRTBaseTree* baseTree, SFoliageInstance & foliageInstance, FbxMesh* meshBasedOnPhysXRepresentation, FbxMesh* meshBasedOnSpeedTreeRepresentation, Uint32& meshOffsetBasedOnPhysXRepresentation, Uint32& meshOffsetBasedOnSpeedTreeRepresentation )
	{
		//////////////////////////////////////////////////////////////////////////
		// Set default values
		Float scale = foliageInstance.GetScale();
		Vector position = foliageInstance.GetPosition();
		Vector extents = baseTree->GetBBox().CalcExtents();
		Vector center1 = Vector( extents.X, extents.Z, 0 );
		Vector center2 = Vector( extents.X, extents.Z, extents.Y );

		//////////////////////////////////////////////////////////////////////////
		// Get collision for tree instance
		TDynArray< Sphere > collisionShapes;
		GRender->GetSpeedTreeResourceCollision( baseTree->GetRenderObject().Get(), collisionShapes );
		if( collisionShapes.Size() > 0 )
		{
			const Uint32 collisionShapeCount = collisionShapes.Size();
			for( Uint32 i=0; i<collisionShapeCount; i+=2 )
			{
				// set proper values
				position = foliageInstance.GetPosition();
				center1 = collisionShapes[ i ].GetCenter() * scale;
				center2 = collisionShapes[ i+1 ].GetCenter() * scale;
				float radius = collisionShapes[ i ].GetRadius()  * scale;

				// fill mesh
				CreateFoliageTubePrimitive( center1, center2, radius, position, meshBasedOnPhysXRepresentation, meshOffsetBasedOnPhysXRepresentation );
			}
		}
		else
		{
			//////////////////////////////////////////////////////////////////////////
			// Ignore foliage instance smaller than value in GExportIgnoreFoliageSmallerThan variable
			if( baseTree->GetBBox().CalcExtents().Mag3() > (Float)GExportIgnoreFoliageSmallerThan )
			{
				// if tree doesn't have collision representation, create default collision mesh
				CreateFoliageTubePrimitive( center1, center2, scale, position, meshBasedOnSpeedTreeRepresentation, meshOffsetBasedOnSpeedTreeRepresentation );
			}
		}		
	}

	void ExportFoliageToFBX( CWorld* world, FbxManager* fbxManager, FbxScene* fbxScene )
	{
		GFeedback->UpdateTaskInfo( TXT("Exporting foliage collisions to file") );
		CFoliageEditionController & foliage = world->GetFoliageEditionController();

		//////////////////////////////////////////////////////////////////////////
		// Create materials for trees geometry collision
		const FbxDouble3 black(0.0, 0.0, 0.0);
		const FbxDouble3 green( 0.0, 1.0, 0.0 );
		const FbxDouble3 red( 1.0, 0.0, 0.0 );

		//////////////////////////////////////////////////////////////////////////
		// Create node, mesh and material based on physX representation
		FbxSurfacePhong *materialBasedOnPhysXRepresentation = FbxSurfacePhong::Create( fbxManager, "Material based on PhysX representation" );
		materialBasedOnPhysXRepresentation->TransparencyFactor.Set( 0.0 );
		materialBasedOnPhysXRepresentation->ShadingModel.Set( "Phong" );
		materialBasedOnPhysXRepresentation->Emissive.Set( black );
		materialBasedOnPhysXRepresentation->Ambient.Set( green );
		materialBasedOnPhysXRepresentation->Diffuse.Set( green );
		materialBasedOnPhysXRepresentation->Shininess.Set( 0.5 );
		// create node
		FbxNode* treeNodeBasedOnPhysXRepresentation = FbxNode::Create( fbxManager, "Tree collisions node based on PhysX representation" );
		// create mesh
		FbxMesh* collisionMeshBasedOnPhysXRepresentation = FbxMesh::Create( fbxManager, "Collision based on PhysX representation" );
		treeNodeBasedOnPhysXRepresentation->SetNodeAttribute( collisionMeshBasedOnPhysXRepresentation ); 
		// attach material
		FbxGeometryElementMaterial* materialElementBasedOnPhysXRepresentation = collisionMeshBasedOnPhysXRepresentation->CreateElementMaterial();
		materialElementBasedOnPhysXRepresentation->SetMappingMode( FbxGeometryElement::eByPolygon );
		materialElementBasedOnPhysXRepresentation->SetReferenceMode( FbxGeometryElement::eIndexToDirect );
		treeNodeBasedOnPhysXRepresentation->AddMaterial( materialBasedOnPhysXRepresentation );


		//////////////////////////////////////////////////////////////////////////
		// Create node, mesh and material based on Speed Tree representation
		FbxSurfacePhong *materialBasedOnSpeedTreeRepresentation = FbxSurfacePhong::Create( fbxManager, "Material based on Speed Tree representation" );
		materialBasedOnSpeedTreeRepresentation->TransparencyFactor.Set( 0.0 );
		materialBasedOnSpeedTreeRepresentation->ShadingModel.Set( "Phong" );
		materialBasedOnSpeedTreeRepresentation->Emissive.Set( black );
		materialBasedOnSpeedTreeRepresentation->Ambient.Set( red );
		materialBasedOnSpeedTreeRepresentation->Diffuse.Set( red );
		materialBasedOnSpeedTreeRepresentation->Shininess.Set( 0.5 );
		// create node
		FbxNode* treeNodeBasedOnSpeedTreeRepresentation = FbxNode::Create( fbxManager, "Tree collisions node based on Speed Tree representation" );
		// create mesh
		FbxMesh* collisionMeshBasedOnSpeedTreeRepresentation = FbxMesh::Create( fbxManager, "Collision based on PhysX representation" );
		treeNodeBasedOnSpeedTreeRepresentation->SetNodeAttribute( collisionMeshBasedOnSpeedTreeRepresentation ); 
		// attach material
		FbxGeometryElementMaterial* materialElementBasedOnSpeedTreeRepresentation = collisionMeshBasedOnSpeedTreeRepresentation->CreateElementMaterial();
		materialElementBasedOnSpeedTreeRepresentation->SetMappingMode( FbxGeometryElement::eByPolygon );
		materialElementBasedOnSpeedTreeRepresentation->SetReferenceMode( FbxGeometryElement::eIndexToDirect );
		treeNodeBasedOnSpeedTreeRepresentation->AddMaterial( materialBasedOnSpeedTreeRepresentation );


		//////////////////////////////////////////////////////////////////////////
		// Helper variables
		Vector cameraPosition = GExportWorldTerrainPoint * Vector( 1, 1, 0, 1 );
		Float distance = GExportWorldFoliageRadius;
		Uint32 vertexCountBasedOnSpeedTreeRepresentation = 0;
		Uint32 vertexCountBasedOnPhysXRepresentation = 0;
		Uint32 vertexCountForSingleMesh = 192;
		Uint32 meshOffsetBasedOnPhysXRepresentation = 0;
		Uint32 meshOffsetBasedOnSpeedTreeRepresentation = 0;


		//////////////////////////////////////////////////////////////////////////
		// Get all types of trees on the world
		TDynArray< SFoliageInstanceCollection > collection;
		foliage.GetInstancesFromArea( cameraPosition, distance, collection );
		const Uint32 foliageCount = collection.Size();


		//////////////////////////////////////////////////////////////////////////
		// Collect information about count of vertices and meshes
		for( Uint32 i=0; i<foliageCount; ++i )
		{
			// get all instances of the type of tree
			const Uint32 intanceCount = collection[i].m_instances.Size();

			TDynArray< Sphere > collisionShapes;
			GRender->GetSpeedTreeResourceCollision( collection[i].m_baseTree->GetRenderObject().Get(), collisionShapes );
			if( collisionShapes.Size() > 0 )
			{
				vertexCountBasedOnPhysXRepresentation += ( vertexCountForSingleMesh * ( ( collisionShapes.Size() / 2 ) * intanceCount ) );
			}
			else
			{
				if( collection[i].m_baseTree->GetBBox().CalcExtents().Mag3() > (Float)GExportIgnoreFoliageSmallerThan )
				{
					vertexCountBasedOnSpeedTreeRepresentation += ( vertexCountForSingleMesh * intanceCount );
				}
			}
		}

		// reserve memory for vertex and attach node to scene
		if( vertexCountBasedOnSpeedTreeRepresentation > 0)
		{
			fbxScene->GetRootNode()->AddChild( treeNodeBasedOnSpeedTreeRepresentation );
			collisionMeshBasedOnSpeedTreeRepresentation->InitControlPoints( vertexCountBasedOnSpeedTreeRepresentation );
		}
		if( vertexCountBasedOnPhysXRepresentation > 0)
		{
			fbxScene->GetRootNode()->AddChild( treeNodeBasedOnPhysXRepresentation );
			collisionMeshBasedOnPhysXRepresentation->InitControlPoints( vertexCountBasedOnPhysXRepresentation );
		}


		//////////////////////////////////////////////////////////////////////////
		// Export vertices
		for( Uint32 i=0; i<foliageCount; ++i )
		{
			GFeedback->UpdateTaskProgress( i, foliageCount );

			// export instances to file
			const Uint32 treeCount = collection[i].m_instances.Size();
			for( Uint32 j=0; j<treeCount; ++j )
			{
				GFeedback->UpdateTaskInfo( String::Printf( TXT("Exporting foliage collisions to file - Type: %d / %d , Instance: %d / %d"), i, foliageCount, j, treeCount ).AsChar() );

				// generate mesh based on the tree collision
				AddFoliageInstance( collection[i].m_baseTree.Get(), collection[i].m_instances[j], collisionMeshBasedOnPhysXRepresentation, collisionMeshBasedOnSpeedTreeRepresentation, meshOffsetBasedOnPhysXRepresentation, meshOffsetBasedOnSpeedTreeRepresentation );
			}
		}

	}

	void ExportTerrain( FbxManager* fbxManager, FbxScene* fbxScene, CWorld* world )
	{
		//////////////////////////////////////////////////////////////////////////
		// Save new-style clipmap-based terrain

		FbxNode* fbxNode = FbxNode::Create( fbxManager, "TerrainNode" );
		fbxScene->GetRootNode()->AddChild( fbxNode );

		FbxMesh* fbxMesh = FbxMesh::Create( fbxManager, "TerrainMesh" );
		fbxNode->SetNodeAttribute( fbxMesh );

		//////////////////////////////////////////////////////////////////////////
		// Create material for the terrain

		FbxDouble3 lBlack( 0.0, 0.0, 0.0 );
		FbxDouble3 lAmbient( 0.2, 0.2, 0.2 );
		FbxDouble3 lColor( 0.3, 0.6, 0.1 );

		FbxString materialName( "TerrainMaterial" );
		FbxSurfacePhong *material = FbxSurfacePhong::Create( fbxScene, materialName.Buffer() );
		material->Emissive.Set( lBlack );
		material->Ambient.Set( lAmbient );
		material->Diffuse.Set( lColor );
		material->TransparencyFactor.Set( 0.0 );
		material->Shininess.Set( 0.5 );

		fbxNode->AddMaterial( material );

		CClipMap* clipMap = world->GetTerrain();
		AddNewTerrain( fbxMesh, clipMap );
	}

	Int32 GenerateRandomMaterialForNode( FbxManager* fbxManager, FbxNode* fbxNode, Uint32 iLayer)
	{
		CStandardRand randGen;

		FbxDouble3 lBlack(0.0, 0.0, 0.0);
		FbxDouble3 lAmbient(0.2, 0.2, 0.2);
		FbxDouble3 lColor( randGen.Get< float >(), randGen.Get< float >(), randGen.Get< float >() );

		FbxString materialName( "material" );
		materialName += (Int32)iLayer;
		FbxSurfacePhong *material = FbxSurfacePhong::Create( fbxManager, materialName.Buffer() );
		material->Emissive.Set( lBlack );
		material->Ambient.Set( lAmbient );
		material->Diffuse.Set( lColor );
		material->TransparencyFactor.Set( 0.0 );
		material->Shininess.Set( 0.5 );

		return fbxNode->AddMaterial( material );
	}

	Bool ExportLayers( FbxManager* fbxManager, FbxScene* fbxScene, CWorld* world )
	{
		//////////////////////////////////////////////////////////////////////////
		// Scan all layers
		Uint32 iLayer = 0;
		for ( WorldAttachedLayerIterator it( world ); it; ++it, ++iLayer )
		{
			CLayer* layer = *it;

			if ( !layer->GetLayerInfo() || !layer->GetLayerInfo()->IsVisible() )
				continue;

			Uint32 nMeshVertices     = 0;
			Uint32 nMeshVertexOffset = 0;

			TDynArray< CEntity* > entities;
			layer->GetEntities( entities ); // TODO: kill it with fire, should use iterator instead

			TDynArray< CMeshComponent* > componentsToAdd;

			//////////////////////////////////////////////////////////////////////////
			// Count vertices in all entities
			for ( Uint32 iEntity = 0; iEntity < entities.Size(); ++iEntity )
			{
				if ( GFeedback->IsTaskCanceled() )
				{
					fbxManager->Destroy();
					GFeedback->EndTask();
					return false;
				}
				GFeedback->UpdateTaskProgress( iEntity, entities.Size() );

				CEntity *entity = entities[iEntity];

				// Do not export not selected shit
				if ( GExportWorldSelectionOnly && !entity->IsSelected() )
					continue;

				//////////////////////////////////////////////////////////////////////////
				// Do not export characters
				if ( entity->QueryActorInterface() != NULL )
				{
					continue;
				}

				//////////////////////////////////////////////////////////////////////////
				// Scan all components
				for ( BaseComponentIterator it( entity ); it; ++it )
				{
					CMeshComponent *meshComponent = Cast<CMeshComponent>( *it );
					if ( meshComponent )
					{
						CMesh *mesh = meshComponent->GetMeshNow();
						if ( mesh == NULL || ( !GExportVolumeMeshes && mesh->GetFile()->GetFileName().ContainsSubstring( TXT( "volume" ) ) ) )
							continue;

						const CMesh::TLODLevelArray& lodLvlInfo = mesh->GetMeshLODLevels();
						if ( lodLvlInfo.Size() == 0 )
						{
							continue;
						}

						CMesh::LODLevel currentLevel = lodLvlInfo[0];

						const CMeshData data( mesh );
						const auto& chunks = data.GetChunks();

						Uint32 nComponentVertices = CountMeshLOD0Vertices( chunks, currentLevel );
						if ( nComponentVertices > 0 )
						{
							nMeshVertices += nComponentVertices;
							componentsToAdd.PushBack( meshComponent );
						}
					}
				}
			}

			if ( nMeshVertices == 0 )
				continue;

			//////////////////////////////////////////////////////////////////////////
			// Create node and mesh for the current layer
			FbxNode *fbxNode = FbxNode::Create( fbxManager, UNICODE_TO_ANSI( layer->GetFile()->GetDepotPath().AsChar() ) );
			fbxScene->GetRootNode()->AddChild( fbxNode );

			FbxMesh *fbxMesh = FbxMesh::Create( fbxManager, UNICODE_TO_ANSI( layer->GetFile()->GetDepotPath().AsChar() ) );
			fbxNode->SetNodeAttribute( fbxMesh );

			fbxMesh->InitControlPoints( nMeshVertices );

			//////////////////////////////////////////////////////////////////////////
			// Generate random material per layer
			Int32 materialIndex = GenerateRandomMaterialForNode( fbxManager, fbxNode, iLayer );

			//////////////////////////////////////////////////////////////////////////
			// Dump all collected components to mesh
			for ( Uint32 iComponent = 0; iComponent < componentsToAdd.Size(); ++iComponent )
			{
				if ( GFeedback->IsTaskCanceled() )
				{
					fbxManager->Destroy();
					GFeedback->EndTask();
					return false;
				}
				GFeedback->UpdateTaskProgress( iComponent, componentsToAdd.Size() );

				AddMeshComponent( fbxMesh, componentsToAdd[iComponent], nMeshVertexOffset, materialIndex );
			}
		}
		return true;
	}

	Bool ExportWorldToFbx( CWorld *world, const String &fileName, EFBXFileVersion fileVersion )
	{
		//////////////////////////////////////////////////////////////////////////
		// Prepare the FBX SDK.
		FbxManager* fbxManager = FbxManager::Create();
		if ( !fbxManager )
		{
			RED_LOG_WARNING( CNAME( CMesh3DSExporter ), TXT("Unable to create the FBX SDK manager") );
			return false;
		}

		// Create an IOSettings object.
		FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
		fbxManager->SetIOSettings( ios ); // Store IO settings here

		FbxScene* fbxScene = FbxScene::Create( fbxManager, "exportScene" );
		fbxScene->GetGlobalSettings().SetSystemUnit( FbxSystemUnit::m );
		fbxScene->GetGlobalSettings().SetAxisSystem( FbxAxisSystem( FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded ) );

		GFeedback->BeginTask( TXT("Exporting world to FBX"), true );

		//////////////////////////////////////////////////////////////////////////
		// Save new-style clipmap-based terrain
		ExportTerrain( fbxManager, fbxScene, world );

		//////////////////////////////////////////////////////////////////////////
		// Export entities
		if ( GExportWorldMeshes == true )
		{
			if ( !ExportLayers( fbxManager, fbxScene, world ) )
			{
				return false;
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Export foliage
		if ( GExportWorldFoliage == true )
		{
			ExportFoliageToFBX( world, fbxManager, fbxScene );
		}

		//////////////////////////////////////////////////////////////////////////
		// Save file
		GFeedback->UpdateTaskInfo( TXT("Saving data to file") );
		Bool success = FbxSaveScene( fbxManager, fbxScene, UNICODE_TO_ANSI( fileName.AsChar() ), fileVersion, true ); // pEmbedMedia forces use of native format ( floats are not rounded by 3dMax :) )
		if ( !success )
		{
			WARN_IMPORTER(TXT("An error occured while saving the scene!"));
		}

		//////////////////////////////////////////////////////////////////////////
		// Destroy all objects created by the FBX SDK.
		fbxManager->Destroy();

		//////////////////////////////////////////////////////////////////////////
		// End task
		GFeedback->EndTask();
		return success;
	}

	void AddFullMesh( FbxScene *scene, FbxMesh *mesh, CMesh *meshLava, Uint32 lodToExport )
	{	
		if ( meshLava == NULL )
			return;

		const CMesh::TLODLevelArray& lodLvlInfo = meshLava->GetMeshLODLevels();
		if ( lodLvlInfo.Size() == 0 )
		{
			return;
		}

		CMesh::LODLevel currentLevel = lodLvlInfo[ lodToExport ];
		CMeshData meshData( meshLava );

		//////////////////////////////////////////////////////////////////////////
		// No vertices -> nothing to export
		Uint32 totalMeshVertices =  CountMeshLOD0Vertices( meshData.GetChunks(), currentLevel );
		if ( totalMeshVertices == 0 )
			return;

		mesh->InitControlPoints( totalMeshVertices );

		//////////////////////////////////////////////////////////////////////////
		// Create and add materials
		FbxNode *meshNode = mesh->GetNode();
		if ( meshNode )
		{
			TDynArray< String >& meshMatNames = meshLava->GetMaterialNames();
			if ( !meshMatNames.Empty() )
			{
				Float colorStep = 3.f / (Float)meshMatNames.Size();
				Float currColor = 0.f;
				for ( Uint32 i = 0; i < meshMatNames.Size(); ++i )
				{
					currColor += colorStep;

					FbxString matName ( UNICODE_TO_ANSI( meshMatNames[ i ].AsChar() ) );
					FbxSurfacePhong *material = FbxSurfacePhong::Create( scene, matName.Buffer() );
					FbxDouble3 color;
					if ( currColor > 2.f )
					{
						color = FbxDouble3( currColor - 2.f, 1, 1 );
					}
					else if ( currColor > 1.f )
					{
						color = FbxDouble3( 0, currColor - 1.f, 1 );
					}
					else
					{
						color = FbxDouble3( 0, 0, currColor );
					}
					material->Diffuse.Set( color );
					meshNode->AddMaterial( material );
				}
			}
		}

		FbxGeometryElementMaterial *lMaterialElement = mesh->CreateElementMaterial();
		lMaterialElement->SetMappingMode( FbxGeometryElement::eByPolygon );
		lMaterialElement->SetReferenceMode( FbxGeometryElement::eIndexToDirect );

		FbxGeometryElementUV *lUVElement = mesh->CreateElementUV( "UVSet" );
		lUVElement->SetMappingMode( FbxGeometryElement::eByControlPoint );
		lUVElement->SetReferenceMode( FbxGeometryElement::eDirect );

		//////////////////////////////////////////////////////////////////////////
		// Copy vertices and uvs
		FbxVector4 *verticesFbx = mesh->GetControlPoints();
		CopyVertices( verticesFbx, currentLevel, meshData );
		CopyUV( mesh, currentLevel, meshData );

		//////////////////////////////////////////////////////////////////////////
		// Copy faces
		Uint32 localVertexOffset = 0, nMeshVertexOffset = 0;
		CopyFaces( mesh, localVertexOffset, nMeshVertexOffset, currentLevel, meshData, -1 );

		ASSERT( localVertexOffset == totalMeshVertices );
	}

	Bool ExportMeshToFbx( CMesh *mesh, const String &filename, Uint32 lodToExport, EFBXFileVersion ver )
	{
		//////////////////////////////////////////////////////////////////////////
		// Prepare the FBX SDK.
		FbxManager* fbxManager = FbxManager::Create();
		if ( !fbxManager )
		{
			RED_LOG_WARNING( CNAME( CMesh3DSExporter ), TXT("Unable to create the FBX SDK manager") );
			return false;
		}

		// Create an IOSettings object.
		FbxIOSettings * ios = FbxIOSettings::Create( fbxManager, IOSROOT );
		fbxManager->SetIOSettings( ios ); // Store IO settings here

		FbxScene* fbxScene = FbxScene::Create( fbxManager, "exportScene" );
		fbxScene->GetGlobalSettings().SetSystemUnit( FbxSystemUnit::m );
		fbxScene->GetGlobalSettings().SetAxisSystem( FbxAxisSystem( FbxAxisSystem::eZAxis, FbxAxisSystem::eParityOdd, FbxAxisSystem::eRightHanded ) );

		GFeedback->BeginTask( TXT("Exporting mesh to FBX"), true );

		//////////////////////////////////////////////////////////////////////////
		// Create node and mesh
		CFilePath path( filename );
		FbxNode *fbxNode = FbxNode::Create( fbxManager, UNICODE_TO_ANSI( path.GetFileName().AsChar() ) );
		fbxScene->GetRootNode()->AddChild( fbxNode );

		FbxMesh *fbxMesh = FbxMesh::Create( fbxManager, UNICODE_TO_ANSI( path.GetFileName().AsChar() ) );
		fbxNode->SetNodeAttribute( fbxMesh );

		Bool success = true;
		const CMesh::TLODLevelArray& lodLvlInfo = mesh->GetMeshLODLevels();
		if ( lodLvlInfo.Size() > 0 )
		{
			CMesh::LODLevel currentLevel = lodLvlInfo[ lodToExport ];
			const CMeshData data( mesh );
			const auto& chunks = data.GetChunks();

			//////////////////////////////////////////////////////////////////////////
			// Export mesh
			AddFullMesh( fbxScene, fbxMesh, mesh, lodToExport );

			//////////////////////////////////////////////////////////////////////////
			// Save file
			GFeedback->UpdateTaskInfo( TXT("Saving data to file") );
			success = FbxSaveScene( fbxManager, fbxScene, UNICODE_TO_ANSI( filename.AsChar() ), ver, true ); // pEmbedMedia forces use of native format ( floats are not rounded by 3dMax :) )
			if ( !success )
			{
				WARN_IMPORTER(TXT("An error occured while saving the scene!"));
			}
		}

		//////////////////////////////////////////////////////////////////////////
		// Destroy all objects created by the FBX SDK.
		fbxManager->Destroy();

		//////////////////////////////////////////////////////////////////////////
		// End task
		GFeedback->EndTask();
		return success;
	}
}


//////////////////////////// common
Bool CMesh3DSExporter::DoExport( const ExportOptions& options )
{
	CWorld *world = Cast<CWorld>( options.m_resource );
	CMesh *mesh = Cast< CMesh >( options.m_resource );
	ASSERT( world || mesh );

	EFBXFileVersion version = FBXExportScene::GetFBXExportVersion(options);

	String ext = options.m_saveFileFormat.GetExtension();
	if ( ext == TXT("3ds") )
	{
		return ExportWorldTo3ds( world, options.m_saveFilePath );
	}
	else if ( ext == TXT("fbx") )
	{
		if ( world )
		{
			return ExportWorldToFbx( world, options.m_saveFilePath, version );
		}
		else if ( mesh )
		{
			return ExportMeshToFbx( mesh, options.m_saveFilePath, options.m_lodToUse, version );
		}
	}
	else if ( ext == TXT("obj") )
	{
		return ExportWorldToObj( world, options.m_saveFilePath );
	}

	// Not exported
	return false;
}
