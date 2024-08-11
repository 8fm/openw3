#include "build.h"
#include "assetBrowser.h"
#include "../../common/core/depot.h"
#include <shellapi.h>
#include "meshStats.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/apexResource.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/graphBlock.h"
#include "../../common/engine/materialParameter.h"


void CreateArrayResource( C2dArray** debugDumpInfo, const TDynArray< String >& columnNames )
{
	CResource::FactoryInfo< C2dArray > info;
	*debugDumpInfo = info.CreateResource();

	for ( const String& colName : columnNames )
	{
		(*debugDumpInfo)->AddColumn( colName, TXT("") );
	}

	(*debugDumpInfo)->AddRow();
	for ( Uint32 i = 0; i < columnNames.Size(); ++i )
	{
		(*debugDumpInfo)->SetValue( columnNames[i], i, 0 );
	}
}

void SaveArrayResource( C2dArray* debugDumpInfo, CDirectory* dir, const String& filename )
{
	String temp;
	dir->GetDepotPath( temp );
	temp = String::Printf( TXT("%ls\\%ls"), temp.AsChar(), filename.AsChar() );

	if ( CDiskFile* diskFile = GDepot->FindFile( temp ) )
	{
		String message = String::Printf( TXT("File '%ls' already exists.\nDo you want to replace it?"), temp.AsChar() );
		if ( wxMessageBox( message.AsChar(), TXT("Confirm file replace"), wxYES_NO | wxCENTER | wxICON_WARNING ) != wxYES )
		{
			return;
		}
	}

	debugDumpInfo->SaveAs( dir, CFilePath( temp ).GetFileName(), true );
}

// dump debug info


String CollectEffectsUsedResources( CFXDefinition* effect )
{
	TDynArray< CResource* > resources;
	effect->CollectUsedResources( resources, true );

	String str = TXT("");
	for ( auto* res : resources )
	{
		if ( res->GetFile() )
		{
			if ( res->GetFile()->GetDepotPath() != TXT("") )
			{
				str += res->GetFile()->GetDepotPath() + TXT(";");
			}
		}
	}
	return str;
}

void CEdAssetBrowser::OnDumpDebugInfoEntityTemplatesEffects( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	if ( contextMenuDir->GetDirs().Empty() )
	{
		GFeedback->ShowError( TXT("Sorry, this batcher works only on directories for now") );
		// it needs to be rewritten using CResourceIteratorAdaptor to work also on files
		return;
	}

	TDynArray< String > columnNames;
	columnNames.PushBack( TXT("EntityPath") );
	columnNames.PushBack( TXT("EffectName") );
	columnNames.PushBack( TXT("Effects") );
	
	GFeedback->BeginTask( TXT("Generating debug dump for Entity Templates" ), false );
	TDynArray< CDiskFile* > diskFiles;
	const Uint32 size = contextMenuDir->GetDirs().Size();

	// gathering entity templates
	for ( Uint32 i = 0; i < size; i++ )
	{
		diskFiles.Clear();
		CDirectory* dir = contextMenuDir->GetDirs()[i];
		GetAllResourceFilesInDirectory< CEntityTemplate >( dir, diskFiles );

		const Uint32 diskFileSize = diskFiles.Size();

		if( diskFileSize > 0 )
		{
			C2dArray* debugDumpInfo = nullptr;
			CreateArrayResource( &debugDumpInfo, columnNames );

			Uint32 totalRowsPrinted = debugDumpInfo->GetNumberOfRows();

			for( Uint32 f = 0; f < diskFileSize; ++f )
			{
				if( diskFiles[ f ]->Load() )
				{
					// If this is a Entity Template
					if ( CEntityTemplate* entTemp = Cast< CEntityTemplate >( diskFiles[ f ]->GetResource() ) )
					{						
						debugDumpInfo->AddRow();

						// Add all entity effects
						TDynArray< CFXDefinition* > effects;
						entTemp->GetAllEffects( effects );
						for ( Uint32 i=0; i<effects.Size(); i++ )
						{
							CFXDefinition* effect = effects[i];

							debugDumpInfo->AddRow();
							debugDumpInfo->SetValue( diskFiles[ f ]->GetDepotPath(), columnNames[0], totalRowsPrinted );
							debugDumpInfo->SetValue( effect->GetName().AsString(), columnNames[1], totalRowsPrinted );
							debugDumpInfo->SetValue( CollectEffectsUsedResources( effect ), columnNames[2], totalRowsPrinted );
							++totalRowsPrinted;
						}
					}
				}
				GFeedback->UpdateTaskProgress( f, diskFileSize );
			}

			SaveArrayResource( debugDumpInfo, contextMenuDir->GetDirs()[i]->GetParent(), String::Printf( TXT("%s_entity_templates"), contextMenuDir->GetDirs()[i]->GetName().AsChar() ) );

			debugDumpInfo->Discard();
		}
	}

	GFeedback->EndTask();

	OnRefreshTab( event );

	::ShellExecute( NULL, TXT("explore"), contextMenuDir->GetDirs()[0]->GetParent()->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );
}

void DumpEntityInfo( C2dArray* infoArray, const CEntity* entity )
{
	// Calculate avd streaming distance for streamed components
	Float avgCompDist = 0;
	Uint32 countComp = 0;
	Bool hasDismembermentComponent = false;

	Float texDataSize = 0.0f;
	Float texArrayDataSize = 0.0f;
	Float meshDataSize = 0.0f;
	TDynArray< MeshTextureInfo* > usedTextures;
	TDynArray< SMeshTextureArrayInfo* > usedTextureArrays;

	const TDynArray< CComponent* >& components = entity->GetComponents();
	for ( auto it = components.Begin(); it != components.End(); ++it )
	{
		CComponent* comp = *it;
		if ( comp->IsStreamed() )
		{
			avgCompDist += comp->GetMinimumStreamingDistance();
			++countComp;
		}

		if ( !hasDismembermentComponent && comp->GetClass()->GetName() == CName( TXT("CDismembermentComponent") ) )
		{
			hasDismembermentComponent = true;
		}

		if( CMeshComponent* cmc = Cast< CMeshComponent > ( comp ) )
		{
			// generate approx. resource memory alloc size
			if( CMesh* mesh = cmc->GetMeshNow() )
			{
				for ( Uint32 i = 0; i < mesh->GetNumLODLevels(); i++ )
				{
					meshDataSize += MeshStatsNamespace::CalcMeshLodRenderDataSize( mesh, i );
				}

				// Gather used texture arrays
				TDynArray< SMeshTextureArrayInfo* > usedChunkTextureArrays;
				MeshStatsNamespace::GatherTextureArraysUsedByMesh( mesh, usedChunkTextureArrays );

				// Fill the global mesh texture arrays
				if( usedTextureArrays.Empty() )
				{
					usedTextureArrays = usedChunkTextureArrays;
				}
				else
				{
					for( Uint32 j=0; j<usedChunkTextureArrays.Size(); ++j )
					{
						for( Uint32 k=0; k<usedTextureArrays.Size(); ++k )
						{
							if( SMeshTextureArrayInfo::CmpFuncByDepotPath( usedTextureArrays[k], usedChunkTextureArrays[j] ) != 0 )
							{
								usedTextureArrays.PushBack( usedChunkTextureArrays[j] );
								break;
							}
						}
					}
				}
				usedChunkTextureArrays.Clear();

				// Gather used textures
				TDynArray< MeshTextureInfo* > usedChunkTextures;
				MeshStatsNamespace::GatherTexturesUsedByMesh( mesh, usedChunkTextures );

				// Fill the global mesh textures
				if( usedTextures.Empty() )
				{
					usedTextures = usedChunkTextures;
				}
				else
				{	
					for ( Uint32 i=0; i<usedChunkTextures.Size(); i++ )
					{
						if ( MeshTextureInfo* usedChunkTexture = usedChunkTextures[i] )
						{
							auto it = FindIf( usedTextures.Begin(), usedTextures.End(), 
								[ usedChunkTexture ]( MeshTextureInfo* tex )
							{
								return tex && MeshTextureInfo::CmpFuncByDepotPath( &usedChunkTexture, &tex ) == 0;
							} );

							if ( it == usedTextures.End() )
							{
								usedTextures.PushBack( usedChunkTexture );
							}
						}
					}
				}
				usedChunkTextures.Clear();
			}
			else // mesh not found?
			{
				meshDataSize = -1.0f;
			}
		}		
	}

	for ( Uint32 i=0; i<usedTextures.Size(); i++ )
	{
		MeshTextureInfo* texInfo = usedTextures[i];
		texDataSize += texInfo->m_dataSize;
	}

	// calculate data size for all gathered texture arrays
	for( Uint32 i = 0; i < usedTextureArrays.Size(); ++i )
	{
		const CTextureArray* textureArray = usedTextureArrays[i]->m_textureArray.Get();

		TDynArray< CBitmapTexture* > arrayTextures;
		textureArray->GetTextures( arrayTextures );
		const Uint32 textureCount = arrayTextures.Size();

		for( Uint32 j = 0; j < textureCount; ++j )
		{
			texArrayDataSize += MeshStatsNamespace::CalcTextureDataSize( arrayTextures[j] );
		}
	}

	Uint32 row = infoArray->GetNumberOfRows();

	infoArray->AddRow();
	infoArray->SetValue( entity->GetEntityTemplate()->GetFile()->GetDepotPath(), 0, row );
	infoArray->SetValue( String::Printf( TXT("%d"), entity->GetStreamingDistance() ), 1, row );
	infoArray->SetValue( String::Printf( TXT("%d"), countComp > 0 ? (Uint32)( avgCompDist / (Float)countComp ) : 0 ), 2, row );
	infoArray->SetValue( String::Printf( TXT("%d"), countComp ), 3, row );
	infoArray->SetValue( hasDismembermentComponent ? TXT("Yes") : TXT("No"), 4, row );

	infoArray->SetValue( (String) MeshStatsNamespace::MemSizeToText(texDataSize), 5, row );
	infoArray->SetValue( (String) MeshStatsNamespace::MemSizeToText(texArrayDataSize), 6, row );
	infoArray->SetValue( (String) MeshStatsNamespace::MemSizeToText(meshDataSize), 7, row );

}

void CEdAssetBrowser::OnDumpDebugInfoEntityTemplates( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );

	TDynArray< String > columnNames;
	columnNames.PushBack( TXT("Path") );
	columnNames.PushBack( TXT("StreamingDistance") );
	columnNames.PushBack( TXT("AvgDistance") );
	columnNames.PushBack( TXT("ComponentsCount") );
	columnNames.PushBack( TXT("HasDismembermentComponent") );
	columnNames.PushBack( TXT("Texture data") );
	columnNames.PushBack( TXT("Texture array data") );
	columnNames.PushBack( TXT("Mesh data") );

	C2dArray* infoArray = nullptr;
	CreateArrayResource( &infoArray, columnNames );

	for ( CResourceIteratorAdapter< CEntityTemplate > entTemplate( *contextMenuDir, TXT("Dumping entity templates debug info"), RIF_ReadOnly ); entTemplate; ++entTemplate )
	{
		if ( CEntity* entity = entTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() ) )
		{
			SEntityStreamingState state;
			entity->PrepareStreamingComponentsEnumeration( state, false, SWN_DoNotNotifyWorld );
			entity->ForceFinishAsyncResourceLoads();

			DumpEntityInfo( infoArray, entity );
			
			entity->FinishStreamingComponentsEnumeration( state );
			entity->Discard();
		}
	}

	CDirectory* saveDir = GDepot->FindLocalDirectory( TXT("qa") );
	if ( !contextMenuDir->GetDirs().Empty() )
	{
		saveDir = contextMenuDir->GetDirs()[0]->GetParent();
	}
	else if ( !contextMenuDir->GetFiles().Empty() )
	{
		saveDir = contextMenuDir->GetFiles()[0]->GetDirectory();
	}

	SaveArrayResource( infoArray, saveDir, TXT("entityTemplatesDebugInfo.csv") );
	::ShellExecute( NULL, TXT("explore"), saveDir->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );
}

void CollectMaterialsWithNullTextures( const CMeshTypeResource* meshTypeRes, TDynArray< String >& nullDiffuse, TDynArray< String >& nullNormal )
{
	const CMeshTypeResource::TMaterials& materials = meshTypeRes->GetMaterials();
	for ( Uint32 i = 0; i < materials.Size(); ++i )
	{
		IMaterial* mat = materials[i];
		CMaterialInstance* matInstance = Cast< CMaterialInstance >( mat );

		Bool gotDiffuse = true, gotNormal = true, gotParameters = false;

		CMaterialInstance* tmp = matInstance;
		while ( tmp )
		{
			if ( !matInstance->GetParameters().Empty() )
			{
				gotParameters = true;
			}
			tmp = Cast< CMaterialInstance >( tmp->GetBaseMaterial() );
		}

		if ( matInstance )
		{
			THandle<ITexture> texHandle = 0;
			THandle<CTextureArray> texArrayHandle = 0;
			if ( matInstance->ReadParameter( CName( TXT("Diffuse") ), texHandle ) && !texHandle.IsValid() )
			{
				gotDiffuse = false;
			}
			else if ( matInstance->ReadParameter( CName( TXT("DiffuseArray") ), texArrayHandle ) && !texArrayHandle.IsValid() )
			{
				gotDiffuse = false;
			}

			texHandle = 0;
			texArrayHandle = 0;
			if ( matInstance->ReadParameter( CName( TXT("Normal") ), texHandle ) && !texHandle.IsValid() )
			{
				gotNormal = false;
			}
			else if ( matInstance->ReadParameter( CName( TXT("NormalArray") ), texArrayHandle ) && !texArrayHandle.IsValid() )
			{
				gotNormal = false;
			}
		}

		if ( gotParameters )
		{
			if ( !gotDiffuse )
			{
				nullDiffuse.PushBack( meshTypeRes->GetMaterialNames()[i] );
			}
			if ( !gotNormal )
			{
				nullNormal.PushBack( meshTypeRes->GetMaterialNames()[i] );
			}
		}
	}
}

void DumpMeshTypeResInfo( C2dArray* debugDumpInfo, CMeshTypeResource* meshTypeRes, Uint32 maxLODcount, Bool fromEntity = false )
{
	const Float autohide = meshTypeRes->GetAutoHideDistance();
	Uint32 lodCount = meshTypeRes->GetNumLODLevels();

	Uint32 row = debugDumpInfo->GetNumberOfRows();
	debugDumpInfo->AddRow();
	debugDumpInfo->SetValue( meshTypeRes->GetFile()->GetFileName(), TXT( "Name" ), row );
	debugDumpInfo->SetValue( ToString( fromEntity ), TXT( "From entity" ), row );
	debugDumpInfo->SetValue( meshTypeRes->GetClass()->GetName().AsChar(), TXT( "Type" ), row );
	debugDumpInfo->SetValue( ToString( autohide ), TXT( "Autohide" ), row );

	for( Uint32 k = 0; k < maxLODcount ; ++k )
	{
		debugDumpInfo->SetValue( TXT(""), String::Printf( TXT("LOD%i V/T ratio"), k ), row );
		debugDumpInfo->SetValue( TXT(""), String::Printf( TXT("LOD%i vertices"), k ), row );
		debugDumpInfo->SetValue( TXT(""), String::Printf( TXT("LOD%i triangles"), k ), row );
		debugDumpInfo->SetValue( TXT(""), String::Printf( TXT("LOD%i"), k ), row );
		debugDumpInfo->SetValue( TXT(""), String::Printf( TXT("LOD%i Number Chunks"), k ), row );
	}

	if ( CApexResource* apexTypeRes = Cast< CApexResource >( meshTypeRes ) )
	{
		apexTypeRes->FillLODStatistics( debugDumpInfo );
	}
	else if ( CMesh* meshRes = Cast< CMesh >( meshTypeRes ) )
	{
		Uint32 meshDataSize = 0;
		for ( Uint32 j = 0; j < meshRes->GetNumLODLevels(); j++ )
		{
			meshDataSize += MeshStatsNamespace::CalcMeshLodRenderDataSize( meshRes, j );
		}
		debugDumpInfo->SetValue( String::Printf( TXT("%1.4f"), (Float)meshDataSize / (1024.f * 1024.f) ), TXT( "Size MB" ), row );

		for( Uint32 j=0; j < maxLODcount ; ++j )
		{
			if ( meshRes && j < lodCount )
			{
				Uint32 numc = meshRes->GetMeshLODLevels()[j].m_chunks.Size();
				Uint32 triCount = meshTypeRes->CountLODTriangles( j );
				Uint32 verticesCount = meshTypeRes->CountLODVertices( j );
				const TDynArray<String> & mats = meshTypeRes->GetMaterialNames();	

				String str = ToString(triCount) + TXT(" (");

				Uint32 k;
				for( k=0;k<numc;++k)
				{
					Int32 chid = meshRes->GetMeshLODLevels()[j].m_chunks[k];
					Int32 id = meshRes->GetChunks()[chid].m_materialID;
					Int32 nu = meshRes->GetChunks()[chid].m_numIndices/3;

					str += String::Printf( TXT( " %i " ), nu ) + mats[id];
					if( k<numc-1 )
					{
						str += TXT(",");
					}
				}
				str += TXT(" )");
				debugDumpInfo->SetValue( String::Printf( TXT("%1.2f"), (Float)verticesCount / triCount ), String::Printf( TXT("LOD%i V/T ratio"), j ), row );
				debugDumpInfo->SetValue( String::Printf( TXT("%i"), verticesCount ), String::Printf( TXT("LOD%i vertices"), j ), row );
				debugDumpInfo->SetValue( String::Printf( TXT("%i"), triCount ), String::Printf( TXT("LOD%i triangles"), j ), row );

				debugDumpInfo->SetValue( str ,  String::Printf( TXT( "LOD%i" ), j ), row );
				debugDumpInfo->SetValue( String::Printf( TXT( " %i " ), numc ) ,  String::Printf( TXT( "LOD%i Number Chunks" ), j ), row );
			}
		}
	}

	debugDumpInfo->SetValue(  meshTypeRes->GetDepotPath() ,  String ( TXT( "Path" ) ), row );
	debugDumpInfo->SetValue(  meshTypeRes->GetAuthorName() ,  String ( TXT( "Author" ) ), row );

	TDynArray< String > nullDiffuse, nullNormal;
	CollectMaterialsWithNullTextures( meshTypeRes, nullDiffuse, nullNormal );

	debugDumpInfo->SetValue( ( nullDiffuse.Empty() ? String::EMPTY : String::Printf( TXT("Diffuse: %ls "),	String::Join( nullDiffuse, TXT(",") ).AsChar() ) )
		+ ( nullNormal.Empty() ? String::EMPTY : String::Printf( TXT("Normal: %ls"), String::Join( nullNormal, TXT(",") ).AsChar() ) ), String ( TXT( "Null textures" ) ), row );

}

void CEdAssetBrowser::OnDumpDebugInfoMeshes( wxCommandEvent& event )
{
	CContextMenuDir *contextMenuDir = wxCheckCast< CContextMenuDir >( event.m_callbackUserData );
	const Uint32 maxLODcount = 3;

	//GFeedback->BeginTask( TXT("Generating debug dump" ), false );
	TDynArray< CDiskFile* > alreadyProcessed;

	TDynArray< String > columnNames;
	columnNames.PushBack( TXT("Name") );
	columnNames.PushBack( TXT("From entity") );
	columnNames.PushBack( TXT("Type") );
	columnNames.PushBack( TXT("Autohide") );
	columnNames.PushBack( TXT("Size MB") );
	for( Uint32 k=0; k < maxLODcount ; ++k )
	{
		columnNames.PushBack( String::Printf( TXT("LOD%i V/T ratio"), k ) );
		columnNames.PushBack( String::Printf( TXT("LOD%i vertices"), k ) );
		columnNames.PushBack( String::Printf( TXT("LOD%i triangles"), k ) );
		columnNames.PushBack( String::Printf( TXT("LOD%i"), k ) );
		columnNames.PushBack( String::Printf( TXT("LOD%i Number Chunks"), k ) );
	}
	columnNames.PushBack( TXT("Path") );
	columnNames.PushBack( TXT("Author") );
	columnNames.PushBack( TXT("Null textures") );


	C2dArray* debugDumpInfo = nullptr;
	CreateArrayResource( &debugDumpInfo, columnNames );

	// gathering meshes
	for ( CResourceIteratorAdapter< CResource > res( *contextMenuDir, TXT("Resaving resources..."), RIF_ReadOnly ); res; ++res )
	{
		if ( CMeshTypeResource* meshTypeRes = Cast< CMeshTypeResource >( res.Get() ) )
		{				
			alreadyProcessed.PushBack( meshTypeRes->GetFile() );
			DumpMeshTypeResInfo( debugDumpInfo, meshTypeRes, maxLODcount );
		}
		else if ( CEntityTemplate* entTemplate = Cast< CEntityTemplate >( res.Get() ) )
		{				
			if ( CEntity* entity = entTemplate->CreateInstance( nullptr, EntityTemplateInstancingInfo() ) )
			{
				SEntityStreamingState state;
				entity->PrepareStreamingComponentsEnumeration( state, false, SWN_DoNotNotifyWorld );
				entity->ForceFinishAsyncResourceLoads();

				for ( const CComponent* component : entity->GetComponents() )
				{
					const CMeshTypeComponent* meshComponent = Cast< CMeshTypeComponent >( component );
					CMeshTypeResource* meshTypeRes;
					if ( meshComponent && ( meshTypeRes = meshComponent->GetMeshTypeResource() ) != nullptr && !alreadyProcessed.Exist( meshTypeRes->GetFile() ) )
					{
						alreadyProcessed.PushBack( meshTypeRes->GetFile() );
						DumpMeshTypeResInfo( debugDumpInfo, meshTypeRes, maxLODcount, true );
					}
				}

				entity->FinishStreamingComponentsEnumeration( state );
				entity->Discard();
			}
			//GFeedback->UpdateTaskProgress( f, diskFileSize );
		}
	}
	
	CDirectory* parentDir = contextMenuDir->GetDirs().Size() != 0 ? contextMenuDir->GetDirs()[0]->GetParent() : contextMenuDir->GetFiles()[0]->GetDirectory();
	SaveArrayResource( debugDumpInfo, parentDir, parentDir->GetName().AsChar() );
	debugDumpInfo->Discard();

	//GFeedback->EndTask();
	OnRefreshTab( event );

	::ShellExecute( NULL, TXT("explore"), parentDir->GetAbsolutePath().AsChar(), NULL, NULL, SW_SHOWNORMAL );
}