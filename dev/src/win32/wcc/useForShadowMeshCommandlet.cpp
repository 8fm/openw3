/**
* Copyright (c) 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/commandlet.h"
#include "../../common/core/depot.h"
#include "../../common/engine/mesh.h"
#include "wccVersionControl.h"

/// Sets useForShadowMesh flag for the last LOD in selected meshes
class CUseForShadowMeshCommandlet : public ICommandlet
{
	DECLARE_ENGINE_CLASS( CUseForShadowMeshCommandlet, ICommandlet, 0 );

public:
	CUseForShadowMeshCommandlet()
	{
		m_commandletName = CName( TXT("useForShadowMesh") );
	}

	~CUseForShadowMeshCommandlet()
	{
	}

	// ICommandlet interface
	virtual const Char* GetOneLiner() const 
	{ 
		return TXT("Set useForShadowMesh flag for the last LOD in selected meshes"); 
	}

	virtual void PrintHelp() const
	{
		LOG_WCC( TXT("Usage:") );
		LOG_WCC( TXT("  useForShadowMesh -dir=<path> (-force) (-nonRecursive)") );
		LOG_WCC( TXT("  -force : reset all shadowmesh data and set for the last LOD (skip meshes with already set shadowmesh data if flag is not present)") );
	}

	virtual bool Execute( const CommandletOptions& options )
	{
		String dirPath;
		if ( !options.GetSingleOptionValue( TXT("dir"), dirPath ) )
		{
			ERR_WCC( TXT("Missing required parameter 'dir=<path>'") );
			return false;
		}

		if ( !dirPath.EndsWith( TXT("\\") ) )
		{
			dirPath += TXT("\\");
		}

		Bool force = false;
		if ( options.HasOption( TXT("force") ) )
		{
			force = true;
		}

		Bool isRecursive = true;
		if ( options.HasOption( TXT("nonRecursive") ) )
		{
			isRecursive = false;
		}

		SChangelist shadowmeshChangelist = SChangelist::DEFAULT;
		if ( InitializeWCCVersionControl( options ) )
		{
			// Get changelist, if specified
			Uint32 clNumber = 0;
			String clString;
			if ( !options.GetSingleOptionValue( TXT( "cl" ), clString ) || !::FromString( clString, clNumber ) )
			{
				clNumber = 0;
			}
			if( !static_cast< CSourceControlP4* >( GVersionControl )->CreateChangelistWithNumber( clNumber, shadowmeshChangelist ) )
			{
				shadowmeshChangelist = SChangelist::DEFAULT;
			}
		}
		else
		{
			WARN_WCC( TXT("Failed to initialize P4 version control. Commandlet will not checkout modified files.") );
		}

		String depotPath;
		GDepot->GetAbsolutePath( depotPath );

		TDynArray< String > filePaths;
		GFileManager->FindFiles( depotPath + dirPath, TXT("*.w2mesh"), filePaths, isRecursive );

		for ( String& path : filePaths )
		{
			String dirAbsolutePath = path.StringBefore( TXT("\\"), true ).ToLower() + TXT("\\");
			CDirectory* dir = GDepot->FindPath( dirAbsolutePath.RightString( dirAbsolutePath.Size() - depotPath.Size() ) );
			if ( !dir->IsPopulated() )
			{
				dir->ForceRepopulate();
			}
			CDiskFile* file = dir->FindLocalFile( path.StringAfter( TXT("\\"), true ) );
			THandle< CResource > resource = file->Load();
			if ( !file->LoadThumbnail() )
			{
				WARN_WCC( TXT("Couldn't load thumbnail for %ls (either it will be lost or it wasn't there in the first place)"), file->GetAbsolutePath().AsChar() );
			}
			
			CMesh* mesh = Cast< CMesh >( resource );

			Bool isSetUseForShadowMesh = false;
			for ( SMeshChunkPacked& chunk : mesh->m_chunks )
			{
				if ( chunk.m_useForShadowmesh )
				{
					isSetUseForShadowMesh = true;
				}
			}

			if ( !force && isSetUseForShadowMesh ) // if not forcing, skip meshes which already have shadow mesh metadata
			{
				continue;
			}

			mesh->m_mergeInGlobalShadowMesh = true;

			for ( Uint16 i = 0; i < mesh->m_lodLevelInfo.Size() - 1; ++i ) // reset chunks that do not belong to the last LOD
			{
				CMesh::TChunkArray& lodChunkArray = mesh->m_lodLevelInfo[i].m_chunks;
				for ( Uint16 chunkIndex : lodChunkArray )
				{
					mesh->m_chunks[ chunkIndex ].m_useForShadowmesh = false;
				}
			}

			CMesh::TChunkArray& lodChunkArray = mesh->m_lodLevelInfo[ mesh->m_lodLevelInfo.Size() - 1 ].m_chunks;
			for ( Uint16 chunkIndex : lodChunkArray ) // mark only chunks from the last LOD
			{
				mesh->m_chunks[ chunkIndex ].m_useForShadowmesh = true;
			}

			if ( GVersionControl->IsSourceControlDisabled() )
			{
				file->SetForcedOverwriteFlag( ESaveReadonlyFileBehaviour::eOverwrite );
			}
			else
			{
				file->SetChangelist( shadowmeshChangelist );
				file->CheckOut();
			}

			if ( !mesh->Save() )
			{
				ERR_WCC( TXT("Couldn't save changes to %ls"), path.AsChar() );
			}

			file->Unload();
		}

		return true;
	}
};


BEGIN_CLASS_RTTI( CUseForShadowMeshCommandlet )
	PARENT_CLASS( ICommandlet )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CUseForShadowMeshCommandlet );
