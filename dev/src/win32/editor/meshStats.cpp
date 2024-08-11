/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "meshEditor.h"
#include "materialListManager.h"
#include "meshMaterialList.h"
#include "assetBrowser.h"
#include "meshStats.h"
#include "meshPhysicalRepresentation.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/meshDataBuilder.h"
#include "../../common/core/thumbnail.h"
#include "../../common/core/feedback.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/collisionMesh.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialParameterInstance.h"
#include "../../common/engine/cubeTexture.h"

#define MAX_REASONABLE_MESH_TRIANGLES			45000
#define MAX_REASONABLE_SKINNED_MESH_TRIANGLES	20000
#define MAX_ALLOWED_TRIANGLES_WITHOUT_LOD		1000
#define MAX_TEXTURES_DATA_ALLOWED				20*1024*1024

using namespace MeshStatsNamespace;

ECmpTexture MeshTextureInfo::comparer1 = CmpByRefCount;
ECmpTexture MeshTextureInfo::comparer2 = CmpByDataSize;

void CEdMeshEditor::OnUpdateStats( wxCommandEvent& event )
{
	UpdateMeshStatsNow();
}

void CEdMeshEditor::ShowMaterialsTabAndHighlightMaterial( const String& materialName )
{
	// Change the tab to the "Materials" tab
	SetSelectedPage( MEP_Materials );

	// Select material
	m_materialList->SelectByName( materialName );
}

void CEdMeshEditor::OnLinkClicked( wxHtmlLinkEvent& event )
{
	// Get the link
	const wxHtmlLinkInfo& linkInfo = event.GetLinkInfo();
	wxString href = linkInfo.GetHref();

	// Material ?
	if ( href.StartsWith( wxT("mat:") ) )
	{
		wxString materialName = href.AfterFirst( ':' );
		ShowMaterialsTabAndHighlightMaterial( materialName.wc_str() );
		return;
	}

	// Remove collision
	if ( href == wxT("RemoveCollision") )
	{
		// Remove collision
		wxCommandEvent fakeEvent;
		m_physicalRepresentation->OnMeshCollisionRemove( fakeEvent );

		// Update stats
		UpdateMeshStatsNow();
		return;
	}

	// Remove unused materials
	if ( href == wxT("RemoveUnusedMaterials") )
	{
		// Remove collision
		wxCommandEvent fakeEvent;
		OnRemoveUnusedMaterials( fakeEvent );

		// Update stats
		UpdateMeshStatsNow();
		return;
	}

	// Remove unused bones
	if ( href == wxT("RemoveUnusedBones") )
	{
		// Remove bones
		wxCommandEvent fakeEvent;
		OnRemoveUnusedBones( fakeEvent );

		// Update stats
		UpdateMeshStatsNow();
		return;
	}

	// Remove skinning data
	if ( href == wxT("RemoveSkinningData") )
	{
		// Remove bones
		wxCommandEvent fakeEvent;
		OnRemoveSkinningData( fakeEvent );

		// Update stats
		UpdateMeshStatsNow();
		return;
	}

	// Merge duplicated chunks
	if ( href == wxT("MergeChunks") )
	{
		DoAutoMergeChunks();
		return;
	}

	// Select asset
	String depotPath = href.wc_str();
	SEvents::GetInstance().DispatchEvent( CNAME( SelectAsset ), CreateEventData( depotPath ) );
}

wxString MeshStatsNamespace::MemSizeToText( Uint32 memSize )
{
	return wxString::Format( wxT("%1.4f"), memSize / (1024.0f*1024.0f) );
}

wxString MeshStatsNamespace::RawCountToText( Uint32 count )
{
	// Get the number
	wxString txt = wxString::Format( wxT("%i"), count );

	// Format string (NOT VERY OPTIMAL :P)
	wxString txtNew = wxT("");
	for ( Int32 i=txt.Length()-1; i>=0; i-- )
	{
		// Extra spaces
		if ( txtNew.Length() == 3 ) txtNew = wxT(" ") + txtNew;
		if ( txtNew.Length() == 7 ) txtNew = wxT(" ") + txtNew;
		if ( txtNew.Length() == 11 ) txtNew = wxT(" ") + txtNew;

		// Add char
		wxChar str[2] = { txt[i], 0 };
		txtNew = str + txtNew;
	}

	return txtNew;
}

Uint32 MeshStatsNamespace::CalcMeshRenderDataSize( CMesh* mesh )
{
	Uint32 rawDataSize = 0;
	return rawDataSize;
}

Uint32 MeshStatsNamespace::CalcMeshLodRenderDataSize( const CMesh* mesh, Uint32 lodIndex )
{
	Uint32 rawDataSize = 0;

	// Chunks size
	const auto& chunks = mesh->GetChunks();

	const auto& lod = mesh->GetMeshLODLevels()[ lodIndex ];
	for ( Uint32 i=0; i<lod.m_chunks.Size(); i++ )
	{
		const Uint32 chunkIndex = lod.m_chunks[i];
		if ( chunkIndex < chunks.Size() )
		{
			rawDataSize += chunks[ chunkIndex ].m_numIndices * sizeof(Uint16);
			rawDataSize += chunks[ chunkIndex ].m_numVertices * sizeof(SMeshVertex);
		}
	}

	return rawDataSize;
}

Uint32 MeshStatsNamespace::CalcMeshRawDataSize( CMesh* mesh )
{
	Uint32 rawDataSize = 0;

	// Mesh size
	rawDataSize += sizeof( CMesh );

	// Chunks size
	const auto& chunks = mesh->GetChunks();

	rawDataSize += chunks.DataSize();

	// Size of materials
	const TDynArray< String >& materialNames = mesh->GetMaterialNames();
	rawDataSize += materialNames.DataSize();
	for ( Uint32 i=0; i<materialNames.Size(); i++ )
	{
		rawDataSize += materialNames[i].DataSize();
	}

	// Materials
	const CMeshTypeResource::TMaterials& materials = mesh->GetMaterials();
	rawDataSize += materials.DataSize();
	for ( Uint32 i=0; i<materials.Size(); i++ )
	{
		CMaterialInstance* matInstance = Cast< CMaterialInstance >( materials[i].Get() );
		if ( matInstance )
		{
			rawDataSize += sizeof( CMaterialInstance );
			const TMaterialParameters& params = matInstance->GetParameters();
			rawDataSize += params.DataSize();
			for ( Uint32 j=0; j<params.Size(); j++ )
			{
				const MaterialParameterInstance& param = params[j];
				rawDataSize += sizeof( MaterialParameterInstance );
				rawDataSize += param.GetType()->GetSize();
			}
		}
	}

	// Bones
	rawDataSize += mesh->GetBoneNamesDataSize();
	rawDataSize += mesh->GetBoneRigMatricesDataSize();
	rawDataSize += mesh->GetBoneVertexEpsilonsDataSize();

	// LODs
	const CMesh::TLODLevelArray& lods = mesh->GetMeshLODLevels();
	rawDataSize += lods.DataSize();
	for ( Uint32 i=0; i<lods.Size(); i++ )
	{
		const CMesh::LODLevel& lod = lods[i];
		rawDataSize += lod.m_chunks.DataSize();
	}

	// Return total size
	return rawDataSize;
}

Uint32 MeshStatsNamespace::CalcMeshCollisionDataSize( CMesh* mesh )
{
	Uint32 rawDataSize = 0;
	return rawDataSize;
}

Uint32 MeshStatsNamespace::CalcTextureDataSize( const CBitmapTexture* texture )
{
	Uint32 rawData = 0;

	const CBitmapTexture::MipArray& mips = texture->GetMips();
	for ( Uint32 i=0; i<mips.Size(); i++ )
	{
		rawData += mips[i].m_data.GetSize();
	}

	return rawData;
}

wxString MeshStatsNamespace::MeshVertexTypeToName( EMeshVertexType vt )
{
	switch ( vt )
	{
		case MVT_StaticMesh: return wxT("MVT_StaticMesh");
		case MVT_SkinnedMesh: return wxT("MVT_SkinnedMesh");
		case MVT_DestructionMesh: return wxT("MVT_DestructionMesh");
	}

	return wxT("Unknown");
}

void MeshStatsNamespace::GatherTexturesUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< MeshTextureInfo* >& textures )
{
	if ( material )
	{
		// Get definition for this material 
		IMaterialDefinition* def = material->GetMaterialDefinition();
		if ( def )
		{
			// Scan for texture parameters
			const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
			for ( Uint32 i=0; i<params.Size(); i++ )
			{
				const IMaterialDefinition::Parameter& param = params[i];
				if ( param.m_type == IMaterialDefinition::PT_Texture )
				{
					// Get value ( the bounded texture )
					THandle< CBitmapTexture > texture;
					material->ReadParameter( param.m_name, texture );

					// Add to list
					if ( texture.IsValid() )
					{
						// Find existing slot
						Bool added = false;
						for ( Uint32 j=0; j<textures.Size(); j++ )
						{
							MeshTextureInfo* texInfo = textures[j];
							if ( texInfo->m_texture == texture.Get() )
							{
								texInfo->m_usedByMaterials.PushBack( materialIndex );
								added = true;
								break;
							}
						}

						// Add new info struct
						if ( !added )
						{
							MeshTextureInfo* info = new MeshTextureInfo( texture.Get() );
							info->m_usedByMaterials.PushBack( materialIndex );
							textures.PushBack( info );
						}
					}					
				}
			}
		}
	}
}

void MeshStatsNamespace::GatherTexturesUsedByMesh( const CMesh* mesh, TDynArray< MeshTextureInfo* >& textures )
{
	const CMeshTypeResource::TMaterials& materials = mesh->GetMaterials();
	for ( Uint32 i=0; i<materials.Size(); i++ )
	{
		GatherTexturesUsedByMaterial( materials[i].Get(), i, textures );
	}
}

void MeshStatsNamespace::GatherCubeNamesUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< String >& cubeNames )
{
	if ( material )
	{
		// Get definition for this material 
		IMaterialDefinition* def = material->GetMaterialDefinition();
		if ( def )
		{
			// Scan for texture parameters
			const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
			for ( Uint32 i=0; i<params.Size(); i++ )
			{
				const IMaterialDefinition::Parameter& param = params[i];
				if (param.m_type == IMaterialDefinition::PT_Cube)
				{
					// Get value ( the bounded cube )
					THandle< CCubeTexture > cube;
					material->ReadParameter( param.m_name, cube );
					if (!cube.IsValid() )
					{
						GFeedback->ShowWarn( TXT("Cubemap parameter empty in %s"), material->GetMaterialDefinition()->GetFriendlyName().AsChar() );
						continue;
					}
					cubeNames.PushBackUnique(cube->GetFile()->GetDepotPath());
				}
			}
		}
	}
}

void MeshStatsNamespace::GatherCubeNamesUsedByMesh( CMesh* mesh, TDynArray< String >& cubeNames )
{
	const CMeshTypeResource::TMaterials& materials = mesh->GetMaterials();
	for ( Uint32 i=0; i<materials.Size(); i++ )
	{
		GatherCubeNamesUsedByMaterial( materials[i].Get(), i, cubeNames );
	}
}

wxString MeshStatsNamespace::ExtractTextureThumbnail( CBitmapTexture* texture )
{
	// No thumb
	if ( !texture || !texture->GetFile() || !texture->GetFile()->LoadThumbnail() )
	{
		return wxEmptyString;
	}
	
	// Get thumbnails 
	const TDynArray< CThumbnail* >& thumbs = texture->GetFile()->GetThumbnails();
	if ( thumbs.Empty() || !thumbs[0] )
	{
		return wxEmptyString;
	}

	// Format temp path
	Char buf[256], buf2[256];
	GetTempPath( 256, buf );
	GetTempFileName( buf, TXT("thumb"), 0, buf2 );

	// Save thumb data to file ( HACK ! it assumes it's a readable image format ! )
	FILE* outputFile = _wfopen( buf2, TXT("wb") );
	if ( outputFile )
	{
		// Export data
		const DataBuffer& thumbData = thumbs[0]->GetData();
		fwrite( thumbData.GetData(), thumbData.GetSize(), 1, outputFile );
		fclose( outputFile );

		// Return file name
		return buf2;
	}

	// Not saved
	return wxEmptyString;
}

const Double MeshStatsNamespace::CalcChunkArea( const CMesh* mesh, Uint32 chunkIndex )
{
	Double area = 0.0;

	const CMeshData data( mesh );
	const auto chunks = data.GetChunks();

	const SMeshChunk& chunk = chunks[ chunkIndex ];
	for ( Uint32 i=0; i<chunk.m_indices.Size(); i+=3 )
	{
		// Get the vertices
		const Vector v0 = chunk.m_vertices[ chunk.m_indices[i+0] ].m_position;
		const Vector v1 = chunk.m_vertices[ chunk.m_indices[i+1] ].m_position;
		const Vector v2 = chunk.m_vertices[ chunk.m_indices[i+2] ].m_position;

		// Calculate edges
		const Double abx = v1.X - v0.X;
		const Double aby = v1.Y - v0.Y;
		const Double abz = v1.Z - v0.Z;
		const Double acx = v2.X - v0.X;
		const Double acy = v2.Y - v0.Y;
		const Double acz = v2.Z - v0.Z;

		// Calculate area
		Double abab = abx*abx + aby*aby + abz*abz;
		Double acac = acx*acx + acy*acy + acz*acz;
		Double abac = abx*acx + aby*acy + abz*acz;

		// Calculate area
		Double triArea = sqrt( abab * acac - abac * abac ) / 2.0;
		area += triArea; 
	}

	// Return calculated area
	return area;
}

const Double MeshStatsNamespace::CalcChunkAverageTriangleArea( const CMesh* mesh, Uint32 chunkIndex )
{
	Double area = 0.0;
	
	area = CalcChunkArea(mesh, chunkIndex);

	const CMeshData data( mesh );
	const auto chunks = data.GetChunks();

	const SMeshChunk& chunk = chunks[ chunkIndex ];
	area /= chunk.m_indices.Size()/3;

	// Return calculated area
	return area;
}

void MeshStatsNamespace::CalcChunkSkinningHistogram( const SMeshChunk& chunk, Uint32& s1, Uint32& s2, Uint32& s3, Uint32& s4 )
{
	for ( Uint32 i=0; i<chunk.m_vertices.Size(); i++ )
	{
		const SMeshVertex& v = chunk.m_vertices[i];
		if ( v.m_weights[0] > 0.0f )
		{
			if ( v.m_weights[1] > 0.0f )
			{
				if ( v.m_weights[2] > 0.0f )
				{
					if ( v.m_weights[3] > 0.0f )
					{
						s4++;
					}
					else
					{
						s3++;
					}
				}
				else
				{
					s2++;
				}
			}
			else
			{
				s1++;
			}
		}
	}
}

void MeshStatsNamespace::CalcChunkSkinningUsedBones( const SMeshChunk& chunk, TDynArray< Uint32 >& allUsedBones )
{
	for ( Uint32 i=0; i<chunk.m_vertices.Size(); i++ )
	{
		const SMeshVertex& v = chunk.m_vertices[i];
		for ( Uint32 j=0; j<4; j++ )
		{
			if ( v.m_weights[j] > 0.0f )
			{
				const Uint32 boneIndex = v.m_indices[j];
				ASSERT( boneIndex < allUsedBones.Size() );
				allUsedBones[ boneIndex ]++;
			}
		}
	}
}

wxArrayString MeshStatsNamespace::GetCmpTextureFriendlyNames()
{
	wxArrayString result;

	result.Add( wxT("Reference Count") );
	result.Add( wxT("Data Size") );
	result.Add( wxT("Dimensions") );
	result.Add( wxT("Depot Path") );
	result.Add( wxT("File Name") );

	return result;
}

void MeshStatsNamespace::GatherTextureArraysUsedByMaterial( IMaterial* material, Uint32 materialIndex, TDynArray< SMeshTextureArrayInfo* >& textureArrays )
{
	if( material != nullptr )
	{
		// Get definition for this material 
		IMaterialDefinition* def = material->GetMaterialDefinition();
		if( def != nullptr )
		{
			// Scan for texture parameters
			const IMaterialDefinition::TParameterArray& params = def->GetPixelParameters();
			for( Uint32 i=0; i<params.Size(); i++ )
			{
				const IMaterialDefinition::Parameter& param = params[i];
				if( param.m_type == IMaterialDefinition::PT_TextureArray )
				{
					// Get value ( the bounded texture )
					THandle< CTextureArray > textureArray;
					material->ReadParameter( param.m_name, textureArray );

					// Add to list
					if( textureArray.IsValid() )
					{
						// Find existing slot
						Bool added = false;
						for( Uint32 j=0; j<textureArrays.Size(); ++j )
						{
							SMeshTextureArrayInfo* texArrayInfo = textureArrays[j];
							if( texArrayInfo->m_textureArray == textureArray )
							{
								texArrayInfo->m_usedByMaterials.PushBack( materialIndex );
								added = true;
								break;
							}
						}

						// Add new info struct
						if( added == false )
						{
							SMeshTextureArrayInfo* info = new SMeshTextureArrayInfo( textureArray.Get() );
							info->m_usedByMaterials.PushBack( materialIndex );
							textureArrays.PushBack( info );
						}
					}					
				}
			}
		}
	}
}

void MeshStatsNamespace::GatherTextureArraysUsedByMesh( const CMesh* mesh, TDynArray< SMeshTextureArrayInfo* >& textureArrays )
{
	const CMeshTypeResource::TMaterials& materials = mesh->GetMaterials();
	for ( Uint32 i=0; i<materials.Size(); i++ )
	{
		GatherTextureArraysUsedByMaterial( materials[i].Get(), i, textureArrays );
	}
}

void CEdMeshEditor::UpdateMeshStats()
{
	// Regenerate stats
	if ( IsPageSelected( MEP_Report ) )
	{
		m_statsGenerated = true;
		UpdateMeshStatsNow();
	}
	else
	{
		m_statsGenerated = false;
	}
}

void CEdMeshEditor::UpdateMeshStatsNow()
{
	if ( !m_mesh->IsA< CMesh >() )
	{
		return;
	}

	CMesh* mesh = SafeCast< CMesh >( m_mesh );
	const CMeshData meshData( mesh );
	const auto& chunks = meshData.GetChunks();

	// Format source code
	wxString sourceCode;
	
	// Problem raporting
	ProblemList problemList;
	Bool foundNonOptimalMaterials = false;

	// Calculate area of triangles in each chunk
	Double totalArea = 0.0;
	TDynArray< Double > chunkAreas;
	{
		const Uint32 numChunks = chunks.Size();
		for ( Uint32 i=0; i<numChunks; i++ )
		{
			// Calculate area
			const Double area = CalcChunkArea( mesh, i );

			Double averageArea = CalcChunkAverageTriangleArea(mesh, i);
			if ( averageArea < 0.001)
			{
				problemList.EmitProblem( TXT("Too small triangles in chunk %u"), i );
			}

			chunkAreas.PushBack( area );
			totalArea += area;
		}
	}

	// Gather used textures
	TDynArray< MeshTextureInfo* > usedTextures;
	GatherTexturesUsedByMesh( mesh, usedTextures );

	// Analyze textures
	Uint32 textureDataSize = 0;
	for ( Uint32 i=0; i<usedTextures.Size(); i++ )
	{
		// Accumulate shit
		MeshTextureInfo* texInfo = usedTextures[i];
		textureDataSize += texInfo->m_dataSize;

		// Extract thumbnail
		texInfo->m_thumbnailFile = ExtractTextureThumbnail( texInfo->m_texture );
	}

	// To many textures
	if ( textureDataSize > MAX_TEXTURES_DATA_ALLOWED )
	{
		problemList.EmitProblem( TXT("Too much texture memory used by mesh! Reduce textures!") );
	}

	// Not used materials
	{
		// Initialize with all materials
		TDynArray< Uint32 > notUsedMaterials;
		for ( Uint32 i=0; i<mesh->GetMaterials().Size(); i++ )
		{
			notUsedMaterials.PushBack( i );
		}

		for ( Uint32 i=0; i<chunks.Size(); i++ )
		{
			const SMeshChunk& chunk = chunks[i];
			notUsedMaterials.Remove( chunk.m_materialID );
		}

		// Report any unused materials !
		for ( Uint32 i=0; i<notUsedMaterials.Size(); i++ )
		{
			const String materialName = m_mesh->GetMaterialNames()[ notUsedMaterials[i] ];
			problemList.EmitProblem( wxT("Material \"%s\" is not used! <a href=\"RemoveUnusedMaterials\">Fix!</a>"), materialName.AsChar() );
		}
	}

	// Used bones map
	TDynArray< Uint32 > allUsedBones( mesh->GetBoneCount() );
	Red::System::MemorySet( allUsedBones.TypedData(), 0, allUsedBones.DataSize() );

	// Per LOD info
	TDynArray< Uint32 > lodTriangles;
	TDynArray< Double > lodArea;
	const CMesh::TLODLevelArray& lods = mesh->GetMeshLODLevels();
	Bool isAnythingSkinned = false;
	for ( Uint32 i=0; i<lods.Size(); i++ )
	{
		const Uint32 lodIndex = i;
		const CMesh::LODLevel& lod = lods[i];

		{
			CHTMLTag tagP( sourceCode, wxT("p") );
			sourceCode += wxString::Format( wxT( "<h1>LOD %i</h1>"), i );

			// Chunk count
			sourceCode += wxT( "Chunks: <b>");
			sourceCode += RawCountToText( lod.m_chunks.Size() );
			sourceCode += wxT( "</b><br>");

			// Count chunk data
			Uint32 numTriangles = 0;
			Uint32 numVertices = 0;
			Bool isSkinned = false;

			for ( Uint32 j=0; j<lod.m_chunks.Size(); j++ )
			{
				const SMeshChunk& chunk = chunks[ lod.m_chunks[j] ];
				numTriangles += chunk.m_numIndices / 3;
				numVertices += chunk.m_numVertices;

				if ( chunk.m_vertexType == MVT_SkinnedMesh || chunk.m_vertexType == MVT_DestructionMesh)
				{
					isAnythingSkinned = true;
					isSkinned = true;
				}
			}

			// Check mesh size
			if ( i == 0 )
			{
				const Uint32 limit = isSkinned ? MAX_REASONABLE_SKINNED_MESH_TRIANGLES : MAX_REASONABLE_MESH_TRIANGLES;
				if ( numTriangles > limit )
				{
					problemList.EmitProblem( wxT("Base mesh has to many triangles. Limit is %i."), limit );
				}

				if ( numTriangles > MAX_ALLOWED_TRIANGLES_WITHOUT_LOD && lods.Size() == 1 )
				{
					problemList.EmitProblem( wxT("LODs are required for this mesh.") );
				}
			}
			else
			{
				const Uint32 prevLodTriangleCount = lodTriangles[ i-1 ];
				if ( numTriangles > ( prevLodTriangleCount/2 ) )
				{
					problemList.EmitWarning( wxT("LOD%i is not optimal. It should have at most 50%% triangles of LOD%i."), i, i-1 );
				}

				if ( isAnythingSkinned && !isSkinned )
				{
					problemList.EmitProblem( wxT("LOD%i is not skinned while the base mesh is. This wont work!"), lodIndex);
				}
			}

			// Vertex count
			sourceCode += wxT( "Num vertices: <b>");
			sourceCode += RawCountToText( numVertices );
			sourceCode += wxT( "</b><br>");

			// Triangles count
			sourceCode += wxT( "Num triangles: <b>");
			sourceCode += RawCountToText( numTriangles );
			sourceCode += wxT( "</b><br>");

			// Render data
			sourceCode += wxT( "Render data: <b>");
			sourceCode += MemSizeToText( CalcMeshLodRenderDataSize( mesh, i ) );
			sourceCode += wxT( "</b><br>");

			// Used materials
			{
				sourceCode += wxT("<h3>Used materials:</h3><br>");
				sourceCode += wxT("<table border=1>");

				// Gather used materials
				Uint32 totalLODTriangles = 0;
				Double totalLODArea = 0.0;
				TDynArray< Uint32 > usedMaterials;

				for ( Uint32 j=0; j<lod.m_chunks.Size(); j++ )
				{
					// Get chunk
					const Uint32 chunkIndex = lod.m_chunks[j];
					const SMeshChunk& chunk = chunks[ chunkIndex ];
					usedMaterials.PushBackUnique( chunk.m_materialID );

					// Calculate totals
					totalLODTriangles += chunk.m_numIndices / 3;
					totalLODArea += chunkAreas[ chunkIndex ];
				}

				// Save for future :)
				lodTriangles.PushBack( totalLODTriangles );
				lodArea.PushBack( totalLODArea );

				// Header
				sourceCode += wxT("<tr><th width=100 align=center><i>ID</i></th>");
				sourceCode += wxT("<th align=center><i>Material Name</i></th>");
				sourceCode += wxT("<th align=center><i>Triangles</i></th>");
				sourceCode += wxT("<th align=center><i>% Triangles</i></th>");
				sourceCode += wxT("<th align=center><i>% Area</i></th></tr>");

				// Add row for each material
				for ( Uint32 i=0; i<usedMaterials.Size(); i++ )
				{
					const Uint32 materialIndex = usedMaterials[i];
					
					// Calculate number of triangles with this material
					Double materialArea = 0.0;
					Uint32 materialTriangles = 0;
					for ( Uint32 j=0; j<lod.m_chunks.Size(); j++ )
					{
						// Get chunk
						const Uint32 chunkIndex = lod.m_chunks[j];
						const SMeshChunk& chunk = chunks[ chunkIndex ];
						if ( chunk.m_materialID == materialIndex )
						{
							materialTriangles += chunk.m_numIndices / 3;
							materialArea += chunkAreas[ chunkIndex ];	
						}
					}

					// Usage %
					const Float prcTriangles = (Float)materialTriangles / (Float )totalLODTriangles;
					const Float prcArea = (Float)( materialArea / totalLODArea);
					const String materialName = m_mesh->GetMaterialNames()[ materialIndex ];

					// To many triangles in to small area
					if ( prcArea < 0.15f )
					{
						if ( prcArea < 0.01f )
						{
							// Very non optimal materials
							problemList.EmitProblem( wxT("Material <a href=\"mat:%s\">%s</a> is almost not used by LOD%i!"), materialName.AsChar(), materialName.AsChar(), lodIndex );

							// Color!
							sourceCode += wxT("<tr bgcolor=\"#FF6060\">");
						}
						else if ( prcTriangles > 0.05f || prcArea < 0.05f )
						{
							sourceCode += wxT("<tr bgcolor=\"#FFC0C0\">");
						}
						else
						{
							sourceCode += wxT("<tr bgcolor=\"#FFFFC0\">");
						}

						// Non optimal materials
						if ( !foundNonOptimalMaterials )
						{
							foundNonOptimalMaterials = true;
							problemList.EmitWarning( wxT("Non optimal material usage.") );
						}
					}
					else
					{
						sourceCode += wxT("<tr>");
					}

					// Print stuff
					sourceCode += wxString::Format( wxT("<td>%i</td>"), materialIndex );
					sourceCode += wxString::Format( wxT("<td><a href=\"mat:%s\">%s</a></td>"), materialName.AsChar(), materialName.AsChar() );
					sourceCode += wxString::Format( wxT("<td>%i</td>"), materialTriangles );
					sourceCode += wxString::Format( wxT("<td>%1.2f</td>"), prcTriangles * 100.0f );
					sourceCode += wxString::Format( wxT("<td>%1.2f</td>"), prcArea * 100.0f );
					sourceCode += wxT("</tr>");					
				}

				// End of table
				sourceCode += wxT("</table><br>");
			}

			// Find duplicated chunks ( chunks with the same shader and texture(s) )
			Int32 chunksToBeDeleted = mesh->MergeChunks( true );
			if ( chunksToBeDeleted > 0 )
			{
				problemList.EmitProblem( wxT("Found %i chunk(s) using the same shader and texture(s). <a href=\"MergeChunks\">Fix!</a>"), chunksToBeDeleted );
			}

			// Chunk info
			{
				sourceCode += wxT("<h3>Chunks in the LOD</h3><br>");
				sourceCode += wxT("<table border=1>");

				// Header
				sourceCode += wxT("<tr><th width=50 align=center><i>ID</i></th>");
				sourceCode += wxT("<th align=center><i>Vertex Type</i></th>");
				sourceCode += wxT("<th align=center><i>Material</i></th>");
				sourceCode += wxT("<th align=center><i>BPV</i></th>");
				sourceCode += wxT("<th align=center><i>Vertices</i></th>");
				sourceCode += wxT("<th align=center><i>Indices</i></th>");
				sourceCode += wxT("<th align=center><i>Vert/tri ratio</i></th>");

				// Skinning histogram
				if ( isSkinned )
				{
					sourceCode += wxT("<th align=center><i>S1</i></th>");
					sourceCode += wxT("<th align=center><i>S2</i></th>");
					sourceCode += wxT("<th align=center><i>S3</i></th>");
					sourceCode += wxT("<th align=center><i>S4</i></th>");
				}

				sourceCode += wxT("</tr>");

				// Add row for each chunk
				for ( Uint32 j=0; j<lod.m_chunks.Size(); j++ )
				{
					const SMeshChunk& chunk = chunks[ lod.m_chunks[j] ];

					sourceCode += wxT("<tr>");
					sourceCode += wxString::Format( wxT("<td>%i</td>"), j );

					sourceCode += wxT("<td>");
					sourceCode += MeshVertexTypeToName( chunk.m_vertexType );
					sourceCode += wxT("</td>");

					sourceCode += wxT("<td>");
					const String materialName = m_mesh->GetMaterialNames()[ chunk.m_materialID ];
					sourceCode += wxString::Format( wxT("<a href=\"mat:%s\">%s</a>"), materialName.AsChar(), materialName.AsChar() );
					sourceCode += wxT("</td>");

					sourceCode += wxString::Format( wxT("<td>%i</td>"), chunk.m_numBonesPerVertex );

					sourceCode += wxString::Format( wxT("<td>%i</td>"), chunk.m_numVertices );

					sourceCode += wxString::Format( wxT("<td>%i</td>"), chunk.m_numIndices );

					Uint32 numTriangles = chunk.m_numIndices / 3;
					Float ratio = (Float)chunk.m_numVertices / numTriangles;
					sourceCode += wxString::Format( wxT("<td>%1.2f</td>"), ratio );

					if ( ratio > 2.0f )
					{
						problemList.EmitWarning( wxT("Vert / tri ratio (%.2f) is very high for chunk %d in lod %d"), ratio, j, i );
					}

					if ( isSkinned )
					{
						// Calculate skinning histogram
						Uint32 s1=0, s2=0, s3=0, s4=0;
						CalcChunkSkinningHistogram( chunk, s1, s2, s3, s4 );

						// Extract used bones indices
						if ( !allUsedBones.Empty() )
						{
							CalcChunkSkinningUsedBones( chunk, allUsedBones );
						}

						// Skinning data
						sourceCode += wxString::Format( wxT("<td>%i</td>"), s1 );
						sourceCode += wxString::Format( wxT("<td>%i</td>"), s2 );
						sourceCode += wxString::Format( wxT("<td>%i</td>"), s3 );
						sourceCode += wxString::Format( wxT("<td>%i</td>"), s4 );
					}

					sourceCode += wxT("</tr>");
				}

				// End of table
				sourceCode += wxT("</table><br>");
			}
		}
	}

	// Skinning issues
	if ( isAnythingSkinned )
	{
		// Count used bones
		Uint32 numUsedBones = 0;
		for ( Uint32 i=0; i<allUsedBones.Size(); i++ )
		{
			if ( allUsedBones[i] )
			{
				numUsedBones++;
			}
		}

		if ( numUsedBones <= 1 )
		{
			problemList.EmitProblem( wxT( "%s Skinning is not needed. <a href=\"RemoveSkinningData\">Fix!</a>" ), numUsedBones == 1 ? wxT( "Mesh uses only 1 bone." ) : wxT( "Mesh doesn't use any bones." ) );	
		}
		else if ( mesh->GetCollisionMesh() )
		{
			problemList.EmitProblem( wxT("Skinned mesh should not have collision! <a href=\"RemoveCollision\">Fix!</a>") );	
		}
	}

	{		
		CHTMLTag tagP( sourceCode, wxT("p") );
		sourceCode += wxString::Format( wxT( "<h1>Texture Groups</h1>") );

		// Gather texture group info
		TDynArray< MeshTextureGroupInfo* > groupInfos;
		for ( Uint32 j=0; j<usedTextures.Size(); j++ )
		{
			MeshTextureInfo* info = usedTextures[j];
			CName textureGroupName = info->m_texture->GetTextureGroupName();

			// Find group info
			MeshTextureGroupInfo* group = NULL;
			for ( Uint32 i=0; i<groupInfos.Size(); i++ )
			{
				MeshTextureGroupInfo* groupInfo = groupInfos[i];
				if ( textureGroupName == groupInfo->m_name )
				{
					group = groupInfo;
					break;
				}
			}

			// Create new group
			if ( !group )
			{
				group = new MeshTextureGroupInfo;
				group->m_dataSize = 0;
				group->m_numTextures = 0;
				group->m_name = textureGroupName;
				groupInfos.PushBack( group );
			}

			// Report textures that are not assigned to any group
			if ( textureGroupName == TXT("Default") || !textureGroupName )
			{
				if ( info->m_texture->GetFile() )
				{
					String texturePath = info->m_texture->GetFile()->GetDepotPath();
					CFilePath filePath( texturePath );
					problemList.EmitWarning( wxT("Texture <a href=\"%s\">%s</a> is not assigned to any texture group."), texturePath.AsChar(), filePath.GetFileName().AsChar() );
				}
			}

			// Accumulate stats
			group->m_numTextures++;
			group->m_dataSize += info->m_dataSize;
		}

		// Sort group
		qsort( groupInfos.TypedData(), groupInfos.Size(), sizeof( MeshTextureGroupInfo* ), &MeshTextureGroupInfo::CmpFumc );

		// Texture group table
		{
			sourceCode += wxT("<table border=1>");

			// Header
			sourceCode += wxT("<tr><th width=50 align=center><i>Name</i></th>");
			sourceCode += wxT("<th align=center><i>Num Textures</i></th>");
			sourceCode += wxT("<th align=center><i>Data Size</i></th></tr>");

			// Add row for each texture group
			for ( Uint32 j=0; j<groupInfos.Size(); j++ )
			{
				const MeshTextureGroupInfo* group = groupInfos[j];

				sourceCode += wxT("<tr>");

				// Name
				sourceCode += wxT("<td>");
				sourceCode += group->m_name.AsString().AsChar();
				sourceCode += wxT("</td>");

				// Count
				sourceCode += wxT("<td>");
				sourceCode += wxString::Format( wxT("%i"), group->m_numTextures );
				sourceCode += wxT("</td>");

				// Memory size
				sourceCode += wxT("<td>");
				sourceCode += MemSizeToText( group->m_dataSize );
				sourceCode += wxT("</td>");

				sourceCode += wxT("</tr>");
			}

			// End of table
			sourceCode += wxT("</table><br>");
		}

		// Cleanup
		groupInfos.ClearPtr();
	}

	{		
		CHTMLTag tagP( sourceCode, wxT("p") );
		sourceCode += wxString::Format( wxT( "<h1>Textures</h1>") );

		sourceCode += wxT("<table border=1>");

		// Header
		sourceCode += wxT("<tr><th width=130 align=center><i>Texture</i></th>");
		sourceCode += wxT("<th align=center><i>Extra data</i></th></tr>");

		// Sort group
		qsort( usedTextures.TypedData(), usedTextures.Size(), sizeof( MeshTextureInfo* ), &MeshTextureInfo::CmpFuncByDataSize );

		// Add row for each chunk
		for ( Uint32 j=0; j<usedTextures.Size(); j++ )
		{
			MeshTextureInfo* info = usedTextures[j];

			sourceCode += wxT("<tr>");

			// The icon
			String texturePath = TXT("Internal");
			if ( info->m_texture->GetFile() )
			{
				texturePath = info->m_texture->GetFile()->GetDepotPath();
				sourceCode += wxString::Format( wxT("<td><a href=\"%s\"><img src=\"%s\"/></a></td>"), texturePath.AsChar(), info->m_thumbnailFile.wc_str() );
			}
			else
			{
				sourceCode += wxString::Format( wxT("<td><img src=\"%s\"/></td>"), info->m_thumbnailFile.wc_str() );
			}

			sourceCode += wxT("<td valign=top>");

			// Name
			sourceCode += wxString::Format( wxT("<a href=\"%s\">%s</a>"), texturePath.AsChar(), texturePath.AsChar() );
			sourceCode += wxT("<br><br>");

			// Data
			sourceCode += wxString::Format( wxT("%i x %i, %i mipmap(s)<br>"), info->m_texture->GetWidth(), info->m_texture->GetHeight(), info->m_texture->GetMipCount() );
			sourceCode += wxString::Format( wxT("%s<br>"), info->m_texture->GetTextureGroupName().AsString().AsChar() );
			sourceCode += wxT("Mem: <b>");
			sourceCode += MemSizeToText( info->m_dataSize );
			sourceCode += wxT("</b><br><br>");

			// List of used materials
			if ( !info->m_usedByMaterials.Empty() )
			{
				// Print list
				sourceCode += wxT("Used in materials: ");
				for ( Uint32 k=0; k<info->m_usedByMaterials.Size(); k++ )
				{
					const Uint32 materialIndex = info->m_usedByMaterials[k];
					const String& name = m_mesh->GetMaterialNames()[ materialIndex ];

					// Add to list
					if ( k > 0 ) sourceCode += wxT(", ");
					sourceCode += wxString::Format( wxT("<a href=\"mat:%s\">"), name.AsChar() ); 
					sourceCode += m_mesh->GetMaterialNames()[ materialIndex ].AsChar();
					sourceCode += wxT("</a>");
				}
				sourceCode += wxT("<br><br>");

				// Scan for chunks that uses this texture
				TDynArray< Uint32 > usedByChunks;
				for ( Uint32 k=0; k<info->m_usedByMaterials.Size(); k++ )
				{
					const Uint32 materialIndex = info->m_usedByMaterials[k];
					for ( Uint32 z=0; z<chunks.Size(); z++ )
					{
						const SMeshChunk& chunk = chunks[z];
						if ( chunk.m_materialID == materialIndex )
						{
							usedByChunks.PushBackUnique( z );
						}
					}
				}

				// Print texture triangle usage for each LOD
				for ( Uint32 k=0; k<lods.Size(); k++ )
				{
					const CMesh::LODLevel& lod = lods[k];

					Uint32 texturedTriangles = 0;
					Double texturedArea = 0.0;
					for ( Uint32 z=0; z<lod.m_chunks.Size(); z++ )
					{
						const Uint32 chunkIndex = lod.m_chunks[z];
						if ( usedByChunks.Exist( chunkIndex ) )
						{
							texturedTriangles += chunks[ chunkIndex ].m_numIndices / 3;
							texturedArea += chunkAreas[ chunkIndex ];
						}
					}

					// Print stats
					const Float prcTriangles = (Float) texturedTriangles / (Float) lodTriangles[ k ];
					const Float prcArea = (Float) texturedArea / (Float) lodArea[ k ];
					sourceCode += wxString::Format( wxT("LOD%i: %i triangles ( %1.2f%% ), %1.2f%% of Area<br>"), k, texturedTriangles, prcTriangles * 100.0f, prcArea * 100.0f );
				}
			}

			sourceCode += wxT("</td>");
			sourceCode += wxT("</tr>");
		}

		// End of table
		sourceCode += wxT("</table><br>");
	}

	// Collision
	Bool nonOptimalCollisionFound = false;
	const CCollisionMesh* colMesh = mesh->GetCollisionMesh();
	if ( colMesh )
	{	
		Uint32 vert, indices;
		CHTMLTag tagP( sourceCode, wxT("p") );
		sourceCode += wxString::Format( wxT( "<h1>Collision</h1>") );

		// Shapes count
		sourceCode += wxT( "Number of shapes: <b>");
		sourceCode += RawCountToText( colMesh->GetShapes().Size() );
		sourceCode += wxT( "</b><br>");

		// Chunk info
		{
			sourceCode += wxT("<h3>Shapes in collider</h3><br>");
			sourceCode += wxT("<table border=1>");

			// Header
			sourceCode += wxT("<tr><th width=50 align=center><i>ID</i></th>");
			sourceCode += wxT("<th align=center><i>Havok Class</i></th>");
			sourceCode += wxT("<th align=center><i>Vertices</i></th>");
			sourceCode += wxT("<th align=center><i>Triangles</i></th>");
			sourceCode += wxT("<th align=center><i>Material</i></th></tr>");

			// Add row for each shape
			const TDynArray< ICollisionShape* >& shapes = colMesh->GetShapes();
			for ( Uint32 j=0; j<shapes.Size(); j++ )
			{
				const ICollisionShape* shape = shapes[j];
				sourceCode += wxT("<tr>");
				sourceCode += wxString::Format( wxT("<td>%i</td>"), j );

				// Split the string to its parts, removing the CCLASS name which the user is not interested of
				String cName = shape->GetLocalClass()->GetName().AsChar();
				TDynArray< String > parts = cName.Split( TXT("CCollisionShape") );
				if ( !parts.Empty() )
				{
					sourceCode += wxString::Format( wxT("<td>%s</td>"), parts[0].AsChar());

					shape->GetStats( vert,indices );
					sourceCode += wxString::Format( wxT("<td>%d</td>"), vert );
					// Dividing it by three since it is showing indices insted of triangle count
					sourceCode += wxString::Format( wxT("<td>%d</td>"), indices/3 );

					Uint32 numMaterials = shape->GetNumPhysicalMaterials();
					TDynArray<String> mtlNames;
					for ( Uint32 i = 0; i < numMaterials; ++i )
					{
						mtlNames.PushBackUnique( shapes[j]->GetPhysicalMaterial( i ).AsString() );
					}

					String materialName = String::Join( mtlNames, TXT("; ") );
					if ( materialName == TXT("default") )
					{
						problemList.EmitProblem( wxT("Default Material found on collision shape %i."), j );
					}

					sourceCode += wxString::Format( wxT("<td>%s</td>"), materialName.AsChar() );
				}				
			}

			// End of table
			sourceCode += wxT("</table><br>");
		}
	}
	else
	{
		problemList.EmitWarning( wxT("No Collision Mesh Found") );
	}

	// Skeleton
	Uint32 numMeshBones = mesh->GetBoneCount();
	if ( numMeshBones > 0 )
	{	
		CHTMLTag tagP( sourceCode, wxT("p") );
		sourceCode += wxString::Format( wxT( "<h1>Skeleton</h1>") );

		// Vertex count
		sourceCode += wxT( "Number of bones: <b>");
		sourceCode += RawCountToText( numMeshBones );
		sourceCode += wxT( "</b><br><i>");

		for ( Uint32 i=0; i<lods.Size(); i++ )
		{
			const Uint32 lodIndex = i;
			const Uint32 numBones = mesh->CountLODBones( lodIndex );

			sourceCode += wxString::Format( wxT( "LOD %i - "), lodIndex );
			sourceCode += RawCountToText( numBones );
			sourceCode += wxT( " bone(s)<br>");
		}

		sourceCode += wxT( "</i><br>");

		// Create table
		sourceCode += wxT("<table border=1>");

		// Header
		sourceCode += wxT("<tr><th width=50 align=center><i>ID</i></th>");
		sourceCode += wxT("<th align=center><i>Name</i></th>");
		sourceCode += wxT("<th align=center><i>Vertices</i></th></tr>");
		sourceCode += wxT("<th align=center><i>Vertex epsilon</i></th>");

		// Add bone info
		Uint32 notUsedBonesCount = 0;
		const Uint32 maxNotUsedBonesReported = 10;
		const CName* meshBoneNames = mesh->GetBoneNames();
		const Float* meshVertexEpsilons = mesh->GetBoneVertexEpsilons();
		for ( Uint32 i=0; i<numMeshBones; i++ )
		{
			sourceCode += wxT("<tr>");

			// ID
			sourceCode += wxString::Format( wxT("<td>%i</td>"), i );

			// Name
			sourceCode += wxT("<td>");
			sourceCode += meshBoneNames[i].AsString().AsChar();
			sourceCode += wxT("</td>");

			// Mapped vertices
			sourceCode += wxT("<td>");
			sourceCode += RawCountToText( allUsedBones[i] );
			sourceCode += wxT("</td>");

			// Vertex epsilon
			sourceCode += wxT("<td>");
			sourceCode += wxString::Format( wxT("%f"), meshVertexEpsilons[i] );
			sourceCode += wxT("</td>");

			// End
			sourceCode += wxT("</tr>");

			// Not used bone !
			if ( !allUsedBones[i] )
			{
				if ( notUsedBonesCount < maxNotUsedBonesReported )
				{
					problemList.EmitWarning( wxT("Bone '%s' is not used by skinning. <a href=\"RemoveUnusedBones\">Fix!</a>"), meshBoneNames[i].AsString().AsChar() );
				}

				notUsedBonesCount++;
			}
		}

		// Final error
		if ( notUsedBonesCount > maxNotUsedBonesReported )
		{
			const Uint32 moreBones = notUsedBonesCount - maxNotUsedBonesReported;
			problemList.EmitWarning( wxT("%i more bones are not used by skinning..."), moreBones );
		}

		// End of table
		sourceCode += wxT("</table><br>");
	}

	// Create header
	wxString finalCode;
	{
		CHTMLTag tagHTML( finalCode, wxT("html") );
		CHTMLTag tagBody( finalCode, wxT("body") );

		// Totals
		{
			CHTMLTag tagP( sourceCode, wxT("p") );
			finalCode += wxT( "<h1>Totals</h1>");

			// Render data
			finalCode += wxT( "Render data: <b>");
			finalCode += MemSizeToText( CalcMeshRenderDataSize( mesh ) );
			finalCode += wxT( "</b><br>");

			// Collision data
			finalCode += wxT( "Collision data: <b>");
			finalCode += MemSizeToText( CalcMeshCollisionDataSize( mesh ) );
			finalCode += wxT( "</b><br>");

			// Mesh data
			finalCode += wxT( "Engine data: <b>");
			finalCode += MemSizeToText( CalcMeshRawDataSize( mesh ) );
			finalCode += wxT( "</b><br>" );

			// Mesh data
			finalCode += wxT( "Texture data: <b>");
			finalCode += MemSizeToText( textureDataSize );
			if ( textureDataSize > 0 )
			{
				finalCode += wxString::Format( wxT(" in %i texture(s)"), usedTextures.Size() );
			}
			finalCode += wxT( "</b><br>" );

			// Autohide distance
			finalCode += wxT( "Autohide Distance data: <b>");
			finalCode += wxString::Format( wxT("%1.2f"), mesh->GetAutoHideDistance() );
			finalCode += wxT( "</b><br>" );
		}

		// Problems
		if ( problemList.problemsCode.Length() || problemList.warningCode.Length() )
		{
			CHTMLTag tagP( sourceCode, wxT("p") );
			finalCode += wxT( "<h1>Problems</h1>");
			finalCode += problemList.problemsCode;
			finalCode += problemList.warningCode;
		}

		// Sub stats
		finalCode += sourceCode;
	}

	// Update content
	wxHtmlWindow* html = XRCCTRL( *this, "Stats", wxHtmlWindow );
	html->SetPage( finalCode );
}
