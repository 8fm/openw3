/**
* Copyright (c) 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "bundleMetadataStoreCommandlet.h"
#include "../../common/core/bundleMetadataStore.h"
#include "../../common/core/bundleMetadataStoreBuilder.h"
#include "../../common/core/asyncIO.h"

RED_DEFINE_STATIC_NAME( metadatastore )

IMPLEMENT_ENGINE_CLASS( CBundleMetadataStoreCommandlet );

CBundleMetadataStoreCommandlet::CBundleMetadataStoreCommandlet()
{
	m_commandletName = CNAME( metadatastore );
}

CBundleMetadataStoreCommandlet::~CBundleMetadataStoreCommandlet()
{

}

Bool CBundleMetadataStoreCommandlet::Execute( const CommandletOptions& options )  
{
	CTimeCounter timer;

	Red::Core::Bundle::CBundleMetadataStoreBuilder::Settings metadataStoreSettings;

	// Paths
	metadataStoreSettings.m_rootPath = GFileManager->GetBundleDirectory();
	options.GetSingleOptionValue( TXT( "path" ), metadataStoreSettings.m_rootPath );
	if( !metadataStoreSettings.m_rootPath.EndsWith(TXT("\\")) && !metadataStoreSettings.m_rootPath.EndsWith(TXT("/")) )
	{
		metadataStoreSettings.m_rootPath += TXT( "\\" );
	}
	LOG_WCC( TXT("Bundle input directory: '%ls'"), metadataStoreSettings.m_rootPath.AsChar() );

	// Patch bundles path
	TList< String > patchRootDirs;
	if ( options.HasOption( TXT( "patch" ) ) )
	{
		patchRootDirs = options.GetOptionValues( TXT( "patch" ) );
	}

	// Output path
	String storePath = metadataStoreSettings.m_rootPath + TXT( "metadata.store" );
	options.GetSingleOptionValue( TXT("out"), storePath );
	LOG_WCC( TXT("Store output path: '%ls'"), storePath.AsChar() );

	// Censorshit level
	if ( options.HasOption( TXT("censorship") ) )
	{
		metadataStoreSettings.m_isCensored = true;
	}

	// Delete existing file
	GFileManager->DeleteFile( storePath );

	// Create new store object
	Red::Core::Bundle::CBundleMetadataStore store( metadataStoreSettings.m_rootPath );
	Red::Core::Bundle::CBundleMetadataStoreBuilder storeBuilder( *GDeprecatedIO );

	// Get a list of every bundle in the supplied path
	GFileManager->FindFilesRelative( metadataStoreSettings.m_rootPath, TXT(""), TXT( "*.bundle" ), metadataStoreSettings.m_bundlesPaths, true );

	// Get a list of patch bundles
	for ( auto it = patchRootDirs.Begin(); it != patchRootDirs.End(); ++it )
	{
		String patchRootDir = *it;
		LOG_WCC( TXT( "Bundle patch directory: '%ls'" ), patchRootDir.AsChar() );
		if ( !patchRootDir.Empty() )
		{
			if( !patchRootDir.EndsWith(TXT("\\")) && !patchRootDir.EndsWith(TXT("/")) )
			{
				patchRootDir += TXT( "\\" );
			}
			Red::Core::Bundle::CBundleMetadataStoreBuilder::PatchGroup patchGroup;

			patchGroup.m_patchRootPath = patchRootDir;
			GFileManager->FindFilesRelative( patchRootDir, TXT(""), TXT( "*.bundle" ), patchGroup.m_patchBundlesPaths, true );
			Sort( patchGroup.m_patchBundlesPaths.Begin(), patchGroup.m_patchBundlesPaths.End() );

			for ( Uint32 i=0; i<patchGroup.m_patchBundlesPaths.Size(); ++i )
			{
				LOG_WCC( TXT("Patch bundle: '%ls'"), patchGroup.m_patchBundlesPaths[i].AsChar() );
			}

			metadataStoreSettings.m_patchGroups.PushBack( patchGroup );
		}
	}

	// To make the resulting file deterministic
	Sort( metadataStoreSettings.m_bundlesPaths.Begin(), metadataStoreSettings.m_bundlesPaths.End() );

	// Construct the store from the bundles!
	if ( !storeBuilder.ProcessBundles( metadataStoreSettings ) )
	{
		ERR_WCC( TXT("Failed to build metadata store from bundles. Something's wrong with the data.") );
		return false;
	}

	// Copy data
	store.InitializeFromBuilder( storeBuilder );
	LOG_WCC( TXT( "Building the metadata store took %1.1f sec ( %u bundles, %u file entries )" ), timer.GetTimePeriod(), store.BundleCount(), store.ItemCount() );

	// Set the header bit to reflect the censorship flag
	store.SetCensored( metadataStoreSettings.m_isCensored );

	// Validate store
	if ( !store.Validate() )
	{
		ERR_WCC( TXT("Store generation failed - data is corrupted") );
		return false;
	}

	// Save the store to disk
	IFile* writer = GFileManager->CreateFileWriter( storePath, FOF_Buffered | FOF_AbsolutePath );
	if( writer )
	{
		store.Save( *writer );
		delete writer;
		writer = nullptr;

		return true;
	}

	return false;
}

const Char* CBundleMetadataStoreCommandlet::GetOneLiner() const  
{
	return TXT( "Generates a metadata.store file for the specified directory" );
}

void CBundleMetadataStoreCommandlet::PrintHelp() const
{

}
