
#include "Build.h"
#include "meshMaterialRemapper.h"

#include "../../common/core/depot.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/material.h"
#include "../../common/engine/materialInstance.h"
#include "../../common/engine/materialDefinition.h"
#include "../../common/engine/meshTypeResource.h"
#include "../../common/engine/materialParameterInstance.h"
#include "../../common/engine/texture.h"
#include "undoMeshEditor.h"
#include "resourceIterator.h"
#include "remappingDialog.h"

CEdMeshMaterialRemapper::CEdMeshMaterialRemapper( wxWindow* parent, CEdUndoManager* undoManager )
	: m_parent( parent )
	, m_undoManager( undoManager )
{
	GDepot->GetAbsolutePath( m_initialMatPath );
	m_initialMatPath += TXT("environment\\textures_tileable\\common_materials\\");
}

Bool CEdMeshMaterialRemapper::Execute( CMeshTypeResource* mesh )
{
	ASSERT( mesh->GetFile()->IsLoaded() );
	m_initialTemplatePath = mesh->GetFile()->GetDirectory()->GetAbsolutePath();

	if ( !mesh->MarkModified() )
	{
		return false;
	}

	CollectMeshMaterialInfo( mesh );

	return DoShit();
}

Bool CEdMeshMaterialRemapper::Execute( const TDynArray< CDiskFile* >& meshFiles )
{
	if ( !meshFiles.Empty() )
	{
		m_initialTemplatePath = meshFiles[0]->GetDirectory()->GetAbsolutePath();
	}

	for ( CResourceIterator< CMeshTypeResource > mesh( meshFiles, TXT("Collecting materials from meshes"), RIF_ReadOnly ); mesh; ++mesh )
	{
		CollectMeshMaterialInfo( mesh.Get() );
	}

	return DoShit();
}

Bool CEdMeshMaterialRemapper::Execute( const TDynArray< CDirectory* >& dirs )
{
	if ( !dirs.Empty() )
	{
		m_initialTemplatePath = dirs[0]->GetParent() ? dirs[0]->GetParent()->GetAbsolutePath() : dirs[0]->GetAbsolutePath();
	}

	for ( CResourceIterator< CMeshTypeResource > mesh( dirs, TXT("Collecting materials from meshes"), RIF_ReadOnly ); mesh; ++mesh )
	{
		CollectMeshMaterialInfo( mesh.Get() );
	}

	return DoShit();
}

void CEdMeshMaterialRemapper::CollectMeshMaterialInfo( const CMeshTypeResource* mesh )
{
	String meshDir = GetPathWithLastDirOnly( mesh->GetDepotPath() );
	Uint32 materialInd = 0;

	for ( const String& meshMatName : mesh->GetMaterialNames() )
	{
		IMaterial* material = mesh->GetMaterials()[ materialInd++ ];
		Bool localInstance = false;
		String baseMaterialName = String::EMPTY;
		String shaderName = String::EMPTY;
		String diffuseTex = String::EMPTY;

		if ( material )
		{
			localInstance = material->GetParent() == mesh;

			if ( localInstance )
			{
				if ( const IMaterial* baseMaterial = material->GetBaseMaterial() )
				{
					baseMaterialName = baseMaterial->GetFile() ? GetPathWithLastDirOnly( baseMaterial->GetFile()->GetDepotPath() ) : String::EMPTY;
				}
			}
			else
			{
				if ( const CDiskFile* matFile = material->GetFile() )
				{
					baseMaterialName = GetPathWithLastDirOnly( matFile->GetDepotPath() );
				}
			}

			if ( const IMaterialDefinition* matDef = material->GetMaterialDefinition() )
			{
				CDiskFile* shaderFile = matDef->GetFile();
				shaderName = shaderFile ? GetPathWithLastDirOnly( shaderFile->GetFileName() ) : String::EMPTY;
			}

			if ( const CMaterialInstance* matInstance = Cast< CMaterialInstance >( material ) )
			{
				diffuseTex = FindDiffuseTextureName( matInstance );
			}
		}

		MaterialInfo key( meshMatName, diffuseTex, shaderName, localInstance );
		if ( TDynArray< MeshDesc >* meshes = m_allMeshMaterials.FindPtr( key ) )
		{
			meshes->PushBack( MeshDesc( meshDir, baseMaterialName ) );
		}
		else
		{
			TDynArray< MeshDesc > mmiArr;
			mmiArr.PushBack( MeshDesc( meshDir, baseMaterialName ) );
			m_allMeshMaterials.Insert( key, mmiArr );
		}
	}

	m_meshFiles.PushBack( mesh->GetFile() );
}

Bool CEdMeshMaterialRemapper::DoShit()
{
	String matPath = m_initialMatPath;
	matPath = wxDirSelector( wxT("Select materials source directory"), matPath.AsChar() );
	if ( matPath.Empty() )
	{
		return false;
	}

	matPath = matPath.RightString( matPath.Size() - GDepot->GetRootDataPath().Size() );
	CDirectory* matDir = GDepot->FindPath( matPath + L"\\" );

	if ( !matDir )
	{
		return false;
	}

	m_allMatFiles.Clear();
	GetAllResourceFilesInDirectory< IMaterial >( matDir, m_allMatFiles );

	TDynArray< String > allMaterialPaths;
	for ( CDiskFile* matFile : m_allMatFiles )
	{
		allMaterialPaths.PushBack( GetPathWithLastDirOnly( matFile->GetDepotPath() ) );
	}
	Sort( allMaterialPaths.Begin(), allMaterialPaths.End() );

	CEdRemappingDialog::Mappings mappings;

	for ( const auto& mashMatEntry : m_allMeshMaterials )
	{
		const String& meshMatName = mashMatEntry.m_first.m_materialName;
		const String& diffuseTexName = mashMatEntry.m_first.m_diffuseTexture;
		const TDynArray< MeshDesc >& meshesForMat = mashMatEntry.m_second;
		Bool localInstance = mashMatEntry.m_first.m_localInstance;

		// find a match by name
		Int32 idxFound = -1;
		if ( localInstance )
		{
			if ( !FindMaterialMatch( allMaterialPaths, meshMatName, idxFound ) )
			{
				Int32 tmpIdx = idxFound;
				// find a match by diffuse texture name
				if ( !FindMaterialMatch( allMaterialPaths, CFilePath( diffuseTexName ).GetFileName(), idxFound ) && tmpIdx != -1 )
				{
					// if no perfect match found by diffuse texture name and we found a decent match by material name, retrieve data from previous search
					idxFound = tmpIdx;
				}
			}
		}
		else
		{
			// if it's not local instance, choose previous remapping - find base material in given possibilities
			// if it's not found, we probably used different directory
			FindMaterialMatch( allMaterialPaths, CFilePath( meshesForMat[0].m_baseMaterial ).GetFileName(), idxFound );
		}

		String tooltip = String::Printf( TXT("SHADER: %ls\nLocal Instance: %ls\n"),
			mashMatEntry.m_first.m_shaderName.AsChar(), mashMatEntry.m_first.m_localInstance ? TXT("Yes") : TXT("No") );

		for ( const MeshDesc& meshInfo : meshesForMat )
		{
			tooltip += String::Printf( TXT( "%ls (base: %ls)\n" ), meshInfo.m_meshName.AsChar(), meshInfo.m_baseMaterial.AsChar() );
		}

		String arrowText = TXT(" -->");
		// add info about match kind ( by diffuse or by material name)
		if ( localInstance && idxFound != -1 )
		{
			String matchedBy = CFilePath( allMaterialPaths[ idxFound ] ).GetFileName() == meshMatName ? TXT("Mat") : TXT("Diff");
			arrowText = matchedBy + arrowText;
		}

		CEdRemappingDialog::MappingEntry mapping( GenerateMappingLabel( meshMatName, diffuseTexName), allMaterialPaths, idxFound, tooltip );
		mapping.m_iconResource = localInstance ? TXT("IMG_STAR16_GRAY") : TXT("IMG_STAR16_YELLOW");
		mapping.m_arrowText = arrowText;
		mappings.PushBack( mapping );
	}

	CEdRemappingDialog remappingDlg( m_parent, TXT("material mapper") );
	remappingDlg.SetupSpecialActionButton( TXT("Apply template"), std::bind( &CEdMeshMaterialRemapper::ApplyTemplate, this, std::placeholders::_1 ) );
	if ( !remappingDlg.Execute( mappings ) )
	{
		return false;
	}

	// remap materials
	for ( CResourceIterator< CMeshTypeResource > mesh( m_meshFiles, TXT("Remapping materials") ); mesh; ++mesh )
	{
		if ( m_undoManager )
		{
			CUndoMeshMaterialsChanged::CreateStep( *m_undoManager, mesh.Get(), TXT("remap materials") );
		}

		TDynArray< String >& meshMatNames  = mesh->GetMaterialNames();
		CMeshTypeResource::TMaterials&   meshMaterials = mesh->GetMaterials();
		
		for ( Int32 meshMatIdx = 0; meshMatIdx < meshMaterials.SizeInt(); ++meshMatIdx )
		{
			const String diffusePath = FindDiffuseTextureName( Cast< CMaterialInstance >( meshMaterials[meshMatIdx] ) );
			const String& mappingLabel = GenerateMappingLabel( meshMatNames[meshMatIdx], CFilePath( diffusePath ).GetFileName() );

			auto it = FindIf( mappings.Begin(), mappings.End(), 
							  [&mappingLabel]( const CEdRemappingDialog::MappingEntry& e ) { return mappingLabel == e.m_original; }
							);
			ASSERT ( it != mappings.End() );
			
			if ( it->m_selectedIdx != -1 ) // -1 means "do not remap this"
			{
				const String& chosenMaterial = it->m_possibilities[ it->m_selectedIdx ];
				auto matFile = FindIf( m_allMatFiles.Begin(), m_allMatFiles.End(),
					[&chosenMaterial]( const CDiskFile* f ){ return GetPathWithLastDirOnly( f->GetDepotPath() ) == chosenMaterial; }
				);

				if ( matFile != m_allMatFiles.End() && *matFile ) // use material instance from disk
				{
					THandle< IMaterial > mat = Cast< IMaterial >( (*matFile)->Load() );
					ASSERT ( mat );
					meshMaterials[ meshMatIdx ] = mat;
				}
				else
				{
					// check if we didn't use template with local material instance for this material
					String indexString, name;
					Int32 localTemplateMatInd = -1; 
					if ( chosenMaterial.Split( TXT(":"), &indexString, &name ) && FromString( indexString, localTemplateMatInd )  )
					{
						// material to remap starts with a number - that's a name of the local instance, try to use it
						if ( localTemplateMatInd >= 0 && localTemplateMatInd < (Int32)m_templateLocalMaterials.Size() )
						{
							// create new material instance with the same base material and parameters
							CMaterialInstance* mi = new CMaterialInstance( mesh.Get(), m_templateLocalMaterials[ localTemplateMatInd ] );
							mi->SetBaseMaterial( m_templateLocalMaterials[ localTemplateMatInd ]->GetBaseMaterial() );
							
							if ( CMaterialInstance* templateMat = Cast< CMaterialInstance >( m_templateLocalMaterials[ localTemplateMatInd ] ) )
							{
								mi->ClearInstanceParameters();
								for ( const MaterialParameterInstance& mpi : templateMat->GetParameters() )
								{
									mi->WriteParameterRaw( mpi.GetName(), mpi.GetData() );
								}
							}

							meshMaterials[ meshMatIdx ] = mi;
						}
					}
				}
			}
		}

		mesh->CreateRenderResource();
	}

	return true;
}

Bool CEdMeshMaterialRemapper::FindMaterialMatch( const TDynArray< String >& allMaterialPaths, const String& matchName, Int32& idxFound )
{
	idxFound = -1;
	if ( matchName.Empty() )
	{
		return false;
	}

	for ( Int32 i = 0; i < allMaterialPaths.SizeInt(); ++i )
	{
		String cleanMatName = CFilePath( allMaterialPaths[i] ).GetFileName();

		if ( cleanMatName == matchName )
		{
			idxFound = i; // perfect fit - end
			return true;
		}

		if ( cleanMatName.BeginsWith( matchName ) || matchName.BeginsWith( cleanMatName ) )
		{
			idxFound = i; // good enough, but keep looking for possible perfect fit
		}
	}
	// couldn't find perfect fit - return false
	return false;
}

String CEdMeshMaterialRemapper::FindDiffuseTextureName( const CMaterialInstance* matInstance )
{
	while ( matInstance )
	{
		for ( const MaterialParameterInstance& param : matInstance->GetParameters() )
		{
			if ( param.GetName() == TXT( "Diffuse" ) )
			{
				THandle<ITexture> diffuseTexHandle = 0;
				matInstance->ReadParameter( param.GetName(), diffuseTexHandle );
				if ( diffuseTexHandle.IsValid() )
				{
					return CFilePath( diffuseTexHandle->GetDepotPath() ).GetFileName();
				}
				break;
			}
		}
		matInstance = Cast< CMaterialInstance >( matInstance->GetBaseMaterial() );
	}
	return String::EMPTY;
}

String CEdMeshMaterialRemapper::GenerateMappingLabel( const String& meshMatName, const String& diffuseTexName )
{
	String mappingLabel = meshMatName;
	mappingLabel += String::Printf( TXT(" (%ls)"), diffuseTexName.Empty() ? TXT("standard") : diffuseTexName.AsChar() );

	if ( meshMatName != diffuseTexName )
	{
		mappingLabel = TXT("M != D ") + mappingLabel;
	}
	return mappingLabel;
}

void CEdMeshMaterialRemapper::ApplyTemplate( CEdRemappingDialog* remappingDlg )
{
	if ( !remappingDlg )
	{
		return;
	}

	String meshPath = m_initialTemplatePath;
	meshPath = wxFileSelector( wxT("Select template mesh"), meshPath.AsChar() );
	remappingDlg->SetFocus();

	if ( meshPath.Empty() )
	{
		return;
	}

	meshPath = meshPath.RightString( meshPath.Size() - GDepot->GetRootDataPath().Size() );
	CMeshTypeResource* mesh = Cast< CMeshTypeResource >( GDepot->LoadResource( meshPath) );

	if ( !mesh )
	{
		return;
	}

	Uint32 matIdx = 0;
	THashMap< String, String > templateMappings;
	for ( const String& meshMatName : mesh->GetMaterialNames() )
	{
		IMaterial* material = mesh->GetMaterials()[ matIdx++ ];
		if ( material )
		{
			const String depotPath = material->GetDepotPath();
			const String diffusePath = FindDiffuseTextureName( Cast< CMaterialInstance >( material ) );
			
			if ( depotPath.Empty() ) // it's a local material instance
			{
				templateMappings.Insert( meshMatName, ToString( m_templateLocalMaterials.Size() ) + TXT(": ") + meshMatName );
				m_templateLocalMaterials.PushBack( material );
			}
			else
			{
				templateMappings.Insert( meshMatName, GetPathWithLastDirOnly( depotPath ) );
				m_allMatFiles.PushBackUnique( material->GetFile() );
			}
		}
	}

	// update selection
	TDynArray< String > originals = remappingDlg->GetOriginals();
	for ( const String& original : originals )
	{
		TDynArray< String > originalParts = original.Split( TXT(" ") );
		if ( originalParts.Size() > 1 )
		{
			if ( const String* newValue = templateMappings.FindPtr( originalParts[ originalParts.Size() - 2 ] ) )
			{
				remappingDlg->UpdateSelection( original, *newValue, true );
			}
		}
	}
}
