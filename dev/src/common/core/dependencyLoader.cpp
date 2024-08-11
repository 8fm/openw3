/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "../redSystem/crc.h"

#include "dependencyLoader.h"
#include "handleMap.h"
#include "softHandle.h"
#include "profiler.h"
#include "depot.h"
#include "feedback.h"
#include "fileLatentLoadingToken.h"
#include "dependencySaver.h"
#include "filePath.h"
#include "deferredDataBufferExtractor.h"
#include "dependencyFileTables.h"
#include "fileSystemProfilerWrapper.h"
#include "depotBundles.h"
#include "messagePump.h"

namespace detail_maps
{
#include "detail_maps.texarray.h"
}
namespace henrietta_emb_detail_01
{
#include "henrietta_emb_detail_01.xbm.h"
}
namespace pattern_cloth_008
{
#include "pattern_cloth_008.xbm.h"
}
/*
namespace detailmap_cloth_01
{
#include "detailmap_cloth_01.xbm.h"
}
namespace detailmap_cloth_02
{
#include "detailmap_cloth_02.xbm.h"
}
namespace detailmap_cloth_03
{
#include "detailmap_cloth_03.xbm.h"
}
namespace detailmap_cloth_08
{
#include "detailmap_cloth_08.xbm.h"
}
namespace detailmap_rope_01
{
#include "detailmap_rope_01.xbm.h"
}
*/

namespace
{
	struct BlobMapEntry
	{
		const Char* m_fileName;
		const Uint8* m_blob;
		Uint32 m_blobSize;
	};

#define BLOBDEF(x) { MACRO_TXT(RED_STRINGIFY(x)), x::Data, sizeof(x::Data) }
	static BlobMapEntry BlobMap[] = {
		//BLOBDEF( pbr_bob_pattern2 ),
		BLOBDEF( detail_maps ),
		BLOBDEF( pattern_cloth_008 ),
		BLOBDEF( henrietta_emb_detail_01 ),

		/*
		BLOBDEF( detailmap_cloth_01 ),
		BLOBDEF( detailmap_cloth_02 ),
		BLOBDEF( detailmap_cloth_03 ),
		BLOBDEF( detailmap_cloth_08 ),
		BLOBDEF( detailmap_rope_01 ),
		*/
	};
#undef BLOBDEF

	static Bool FindBlob( Uint32 index, BlobMapEntry& outEntry )
	{
		outEntry = BlobMap[ index ];
		return true;
	}
}

#define FROM_EXPORT_INDEX( x ) ( ((Int32)x) - 1 )
#define FROM_IMPORT_INDEX( x ) ( -((Int32)x) - 1 )

const Uint32 CDependencyLoader::FILE_VERSION = CDependencyFileData::FILE_VERSION;
const Uint32 CDependencyLoader::FILE_MAGIC = CDependencyFileData::FILE_MAGIC;

//---------------------------------------

class CDependencyImportLoaderDepot : public IDependencyImportLoader
{
public:
	/// Load file as dependency
	virtual BaseSafeHandle LoadDependency( const CClass* objectClass, const AnsiChar* path, const CDependencyFileData::Import& importData, const Bool silent ) override final
	{
		// Get the disk file to load
		CDiskFile* depotFile = nullptr;

		// In read only file systems we can use hashed paths
		if ( GFileManager->IsReadOnly() )
		{
			// get (or compute) the file path hash
			TDepotFileHash fileHash;
			if ( importData.m_flags & CDependencyFileData::eImportFlags_HashedPath )
			{
				RED_FATAL( "Hashed paths are not supported yet" );
				fileHash = importData.m_path;
			}
			else
			{
				fileHash = Red::System::CalculatePathHash64( path );
			}

			// find file
			depotFile = GDepot->FindFile( fileHash );

			//if ( !depotFile )
			//{
			//	// first try remapping the material
			//	static const TDepotFileHash missingMaterialHash = Red::System::CalculatePathHash64( "dlc\\bob\\data\\engine\\materials\\graphs\\pbr_bob_pattern2.w2mg" );
			//	static const TDepotFileHash existingMaterialHash = Red::System::CalculatePathHash64( "engine\\materials\\graphs\\pbr_std_tint_mask_det_pattern.w2mg" );
			//	if ( fileHash == missingMaterialHash )
			//	{
			//		depotFile = GDepot->FindFile( existingMaterialHash );
			//	}
			//}

			if ( !depotFile )
			{
				// try loading resources from their binarized versions
				static TDepotFileHash binaryResourcesArray[] = 
				{
					Red::System::CalculatePathHash64( "dlc\\bob\\data\\characters\\models\\common\\textures\\detailmaps_pbr\\detail_maps.texarray" ),
					Red::System::CalculatePathHash64( "dlc\\bob\\data\\characters\\models\\common\\textures\\patterns\\pattern_cloth_008.xbm" ),
					Red::System::CalculatePathHash64( "dlc\\bob\\data\\characters\\models\\main_npc\\anna_henrietta\\model\\henrietta_emb_detail_01.xbm" ),

					/*
					Red::System::CalculatePathHash64( "characters\\models\\common\\textures\\detailmaps_pbr\\detailmap_cloth_01.xbm" ),
					Red::System::CalculatePathHash64( "characters\\models\\common\\textures\\detailmaps_pbr\\detailmap_cloth_02.xbm" ),
					Red::System::CalculatePathHash64( "characters\\models\\common\\textures\\detailmaps_pbr\\detailmap_cloth_03.xbm" ),
					Red::System::CalculatePathHash64( "characters\\models\\common\\textures\\detailmaps_pbr\\detailmap_cloth_04.xbm" ),
					Red::System::CalculatePathHash64( "characters\\models\\common\\textures\\detailmaps_pbr\\detailmap_rope_01.xbm" )
					*/
				};
				static const Uint32 binaryResourcesArraySize = ARRAY_COUNT( binaryResourcesArray );
				for ( Uint32 i = 0; i < binaryResourcesArraySize; ++i )
				{
					if ( fileHash == binaryResourcesArray[ i ] )
					{
						BlobMapEntry blob;
						if ( FindBlob( i, blob ) )
						{
							Red::TScopedPtr< IFile > reader( new CMemoryFileReaderExternalBuffer( blob.m_blob, blob.m_blobSize ) );

							// Create loader
							CDiskFile* outFile = nullptr;
							CDependencyLoader loader( *reader.Get(), outFile );

							DependencyLoadingContext loadingContext;
							loadingContext.m_parent = nullptr;
							loadingContext.m_validateHeader = true;
							loadingContext.m_getAllLoadedObjects = true;

							if ( loader.LoadObjects( loadingContext ) )
							{
								RED_ASSERT( loadingContext.m_loadedResources.Size() == 1 );

								LOG_CORE( TXT("Asset '%ls' of class '%ls' mapped in a hacky way from binary file."),
									ANSI_TO_UNICODE( path ), objectClass->GetName().AsChar() );

								THandle< CResource > resource = loadingContext.m_loadedResources[0];
								return (const BaseSafeHandle&) resource;
							}
						}
					}
				}
			}
		}
		else
		{
			// use slow path in non read only depot
			depotFile = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( path ), 0 );
		}

		// report missing imports
		if ( !depotFile )
		{
			if ( !silent )
			{
				ERR_CORE( TXT("!!! MISSING IMPORT !!! Failed to find '%ls' of class '%ls' using hashed path. Trying normaly."), 
					ANSI_TO_UNICODE( path ), objectClass->GetName().AsChar() );
			}

			// use slow path in non read only depot
			depotFile = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( path ), 0 );
			if ( !depotFile )
			{
				if ( !silent )
				{
					ERR_CORE( TXT("!!! MISSING IMPORT !!! Failed to find depot file '%ls'. File not in the depot."), 
						ANSI_TO_UNICODE( path ) );
				}

				return BaseSafeHandle();
			}
		}

		// if it's an inplace file it should be already loaded
		// it's not fatal but this will cause a really slow (recursive) load
		if ( importData.m_flags & CDependencyFileData::eImportFlags_Inplace )
		{
			if ( !depotFile->IsLoaded() )
			{
				if ( !silent )
				{
					ERR_CORE( TXT("!!! MISSING INPLACE !!! Inplace file '%ls' of class '%ls' is still not loaded."), 
						ANSI_TO_UNICODE( path), objectClass->GetName().AsChar() );
				}

				return BaseSafeHandle();
			}

			return depotFile->GetResource();
		}

		// Load the resource SYNCHRONUSLY
		THandle< CResource > resource = depotFile->Load();
		if ( !resource )
		{
			if ( !silent )
			{
				ERR_CORE( TXT("!!! MISSING IMPORT !!! Failed resource import '%ls' of class '%ls'"), 
					ANSI_TO_UNICODE( path ), objectClass->GetName().AsChar() );
			}

			return BaseSafeHandle();
		}

		// Return loaded resource
		return (const BaseSafeHandle&) resource;
	}
};

static CDependencyImportLoaderDepot theClassicLoader;
static IDependencyImportLoader* theDefaultLoader = &theClassicLoader;

IDependencyImportLoader* IDependencyImportLoader::SetDefault( IDependencyImportLoader* newImportLoader )
{
	IDependencyImportLoader* prev = theDefaultLoader;
	theDefaultLoader = newImportLoader;
	return prev;
}

IDependencyImportLoader* IDependencyImportLoader::GetDefault()
{
	return theDefaultLoader;
}

//---------------------------------------

DependencyLoadingContext::DependencyLoadingContext()
	: m_parent( NULL )
	, m_isAsyncLoader( false )
	, m_getAllLoadedObjects( false )
	, m_doNotLoadImports( false )
	, m_stats( NULL )
	, m_firstExportMemory( NULL )
	, m_firstExportMemorySize( 0 )
	, m_validateHeader( false )
	, m_rootClassOverride( nullptr )
	, m_precachedTables( nullptr )
	, m_importLoader( IDependencyImportLoader::GetDefault() )
{
}

//---------------------------------------

FileDependency::FileDependency( CName className, const String& depotFilePath, const Bool isSoft /*= false*/ )
	: m_depotPath( depotFilePath )
	, m_resourceClass( className  )
	, m_isSoftDepdencency( isSoft )
{
}

CDependencyLoader::CDependencyLoader( IFile& file, const CDiskFile* loadedFile )
	: IDependencyLoader( file, loadedFile )
	, m_context( NULL )
	, m_data( nullptr )
{
}

CDependencyLoader::~CDependencyLoader()
{
}

void CDependencyLoader::SetVersion( Uint32 version )
{
	m_file->m_version = version;
	m_version = version;
}

const CClass* CDependencyLoader::GetRootClass()
{
	// Load the header and dependency tables
	if ( !LoadTables() )
		return nullptr;

	// No export
	if ( m_data->m_exports.Empty() )
		return nullptr;

	// Get the class of export 0 (the resource class)
	const auto& rootExport = m_data->m_exports[0];
	const Uint32 rootClassIndex = m_data->m_names[ rootExport.m_className ].m_string;
	const AnsiChar* rootClassName = &m_data->m_strings[ rootClassIndex ];
	if ( !*rootClassName )
		return nullptr;

	// find class in RTTI system
	return SRTTI::GetInstance().FindClass( CName( ANSI_TO_UNICODE( rootClassName ) ) );
}

Bool CDependencyLoader::LoadDependencies( TDynArray< FileDependency >& outDependencies, Bool includeSoftHandles )
{
	// Load the header and dependency tables
	if ( !LoadTables() )
		return false;

	// Generate the dependencies
	outDependencies.Reserve( outDependencies.Size() + m_data->m_imports.Size() );
	for ( Uint32 i = 0; i < m_data->m_imports.Size(); ++i )
	{
		const auto& importInfo = m_data->m_imports[ i ];

		// skip over soft dependencies
		const Bool isSoft = (importInfo.m_flags & CDependencyFileData::eImportFlags_Soft) != 0;
		if ( isSoft && !includeSoftHandles )
			continue;

		// get import class name
		const Uint32 importClassIndex = m_data->m_names[ importInfo.m_className ].m_string;
		const CName importClassName( ANSI_TO_UNICODE( &m_data->m_strings[ importClassIndex ] ) );

		// get import path
		const AnsiChar* importPath = &m_data->m_strings[ importInfo.m_path ];

		// Collect referenced resources
		outDependencies.PushBack( FileDependency( importClassName, ANSI_TO_UNICODE( importPath ), isSoft ) );
	}

	// Return true to indicate valid file
	return true;
}

Bool CDependencyLoader::LoadObjects( DependencyLoadingContext& context )
{
	// HACK moradin we have to be able to suspend even while loading so we have to pump the messages
	if ( GMessagePump && SIsMainThread() )
	{
		GMessagePump->PumpMessages();
	}

	// Cleanup context
	m_context = &context;
	m_context->m_loadedRootObjects.Clear();

	// Load the header and dependency tables
	const Uint64 baseOffset = GetOffset();
	if ( !LoadTables() )
		return false;

	// Map disk data to runtime data
	if ( !MapRuntimeData() )
		return false;

	// Load all inplace files
	if ( !LoadInplaceFiles() )
		return false;

	// Resolve all references to other imports
	if ( !CreateImports() )
		return false;	

	// Create all exported objects
	if ( !CreateExports() )
		return false;

	// Load exports
	if ( !LoadExports( baseOffset ) )
		return false;

	// Loading was successful
	return true;
}

void CDependencyLoader::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	Int32 index = 0;
	*m_file << index;

	pointer = MapPointer( index, pointerClass );
}

void CDependencyLoader::SerializeName( class CName& name )
{
	Uint16 index = 0;
	*m_file << index;

	name = MapName( index );
}

void CDependencyLoader::SerializeSoftHandle( class BaseSoftHandle& softHandle )
{
	Uint16 index = 0;
	*m_file << index;

	softHandle = MapSoftHandle( index );
}

void CDependencyLoader::SerializeTypeReference( class IRTTIType*& type )
{
	Uint16 index = 0;
	*m_file << index;

	type = MapType( index );
}

void CDependencyLoader::SerializePropertyReference( const class CProperty*& prop )
{
	Uint16 index = 0;
	*m_file << index;

	prop = MapProperty( index );
}

void CDependencyLoader::LoadLegacyDeferedDataBuffer( DeferredDataBuffer& buffer ) const
{
	Uint16 version=0;
	*m_file << version;
	RED_FATAL_ASSERT( version == 0 || version == 1, "Fucked up cook" );

	Uint16 alignment=0;
	*m_file << alignment;
	RED_FATAL_ASSERT( alignment == 16, "Fucked up cook" );

	Uint32 size=0;
	*m_file << size;

	Int32 fileId = -1;
	if ( version == 0 /*deferredDataBufferStringFilename*/ ) 
	{
		String dummy;
		*m_file << dummy;
	}
	else if ( version == 1 /*deferredDataBufferIdFilename*/ )
	{
		*m_file << fileId;
	}

	Uint64 dataOffset = 0;
	*m_file << dataOffset;

	if ( size == 0 )
	{
		// buffer is empty
		buffer.Clear();
	}
	else
	{
		if ( GFileManager->IsReadOnly() )
		{
			if ( fileId != -1 )
			{
				ERR_CORE( TXT("Trying to use legacy cooked DDB ID %d (size %d) for file '%ls' with no cooked data"),
					fileId, size, m_file->GetFileNameForDebug() );
			}
			else
			{
				// well, we know the size and position, initialize the latent token offset
				IFileLatentLoadingToken* token = const_cast< CDependencyLoader* >(this)->CreateLatentLoadingToken( dataOffset );
				RED_ASSERT( token != nullptr, TXT("Failed to restore data for DeferedDataBuffer in file '%ls'"), m_file->GetFileNameForDebug() );

				//LOG_CORE( TXT("Found legacy cooked DDB size %d, offset %d in file '%ls'"), size, dataOffset, m_file->GetFileNameForDebug() );
				buffer.SetAccessFromLatentToken( size, token );
			}
		}
		else
		{
			RED_ASSERT( dataOffset >= 16 && dataOffset < m_file->GetSize(), TXT("Invalid offset to data of DeferedDataBuffer in file '%ls'"), m_file->GetFileNameForDebug() );

			// well, we know the size and position, initialize the latent token offset
			IFileLatentLoadingToken* token = const_cast< CDependencyLoader* >(this)->CreateLatentLoadingToken( dataOffset );
			RED_ASSERT( token != nullptr, TXT("Failed to restore data for DeferedDataBuffer in file '%ls'"), m_file->GetFileNameForDebug() );

			if ( token )
			{
				//LOG_CORE( TXT("Found legacy DDB size %d, offset %d in file '%ls'"), size, dataOffset, m_file->GetFileNameForDebug() );
				buffer.SetAccessFromLatentToken( size, token );
			}
			else
			{
				LOG_CORE( TXT("Found legacy DDB size %d, offset %d in file '%ls' - NO TOKEN"), size, dataOffset, m_file->GetFileNameForDebug() );
				buffer.SetAccessFromFileData( size, dataOffset, *m_file );
			}
		}
	}

	// HACK++ - mark the buffer as properly deserialized from file, even if it's empty
	buffer.HACK_SetWasDeserializedFromFile();
	// HACK--
}

void CDependencyLoader::SerializeDeferredDataBuffer( DeferredDataBuffer& buffer )
{
	// OLD VERSION - buffer offset was saved directly
	if ( m_file->GetVersion() < VER_UPDATED_RESOURCE_FORMAT )
	{
		LoadLegacyDeferedDataBuffer( buffer );
		return;
	}

	// new way - only the buffer ID is saved
	Uint16 bufferIndex = 0;
	*m_file << bufferIndex;

	// empty buffer
	if ( bufferIndex == 0 )
	{
		buffer.Clear();
		return;
	}

#ifndef RED_FINAL_BUILD
	// validate buffer index
	if ( bufferIndex > m_data->m_buffers.Size() )
	{
		ERR_CORE( TXT("Invalid buffer index %d (of %d) in file '%ls'. Mapping to None."), 
			bufferIndex, m_data->m_buffers.Size(), m_file->GetFileNameForDebug() );
		return;
	}
#endif

	// get buffer description from buffer table
	const auto& bufferInfo = m_data->m_buffers[ bufferIndex-1 ];

	// buffer is empty - this should not happen
	RED_ASSERT( bufferInfo.m_dataSizeOnDisk > 0, TXT("Buffer %d with a valid index has no size, file '%ls'"), bufferInfo, m_file->GetFileNameForDebug() );
	if ( !bufferInfo.m_dataSizeOnDisk || !bufferInfo.m_dataSizeInMemory )
	{
		buffer.Clear();
		return;
	}

	// is this buffer stored within this file ?
	if ( bufferInfo.m_dataOffset != 0 )
	{
		IFileLatentLoadingToken* token = CreateLatentLoadingToken( bufferInfo.m_dataOffset );
		RED_ASSERT( token != nullptr, TXT("Unable to create latent access for buffer ID %d (size %d, pos %d) in file '%ls'"), 
			bufferIndex, bufferInfo.m_dataSizeOnDisk, bufferInfo.m_dataOffset, m_file->GetFileNameForDebug() );

		// create latent access point
		if ( token )
		{
			buffer.SetAccessFromLatentToken( bufferInfo.m_dataSizeOnDisk, token );
		}
		else
		{
			// try to load buffer data directly, without latent token
			buffer.SetAccessFromFileData( bufferInfo.m_dataSizeOnDisk, bufferInfo.m_dataOffset, *m_file );
		}
	}
	else
	{
		// is this a file from bundles ?
		Red::Core::Bundle::FileID bundleFileID = 0;
		if ( GDepot->GetBundles() && m_loadedFile )
		{
			bundleFileID = m_loadedFile->GetBufferFileID( bufferIndex );
			if ( bundleFileID )
			{
				// use bundle file
				buffer.SetAccessFromBundleFile( bufferInfo.m_dataSizeOnDisk, bundleFileID );
			}
			else
			{
				CBundleDiskFile* originalFile = GDepot->GetBundles()->GetOriginalDLCFile(*((const CBundleDiskFile*)m_loadedFile));
				bundleFileID = originalFile != nullptr ? originalFile->GetBufferFileID( bufferIndex ) : 0;
				if ( bundleFileID )
				{
					WARN_CORE( TXT("Using buffer file %d from original bundle instead from patch for %ls"), bufferIndex, m_loadedFile->GetDepotPath().AsChar() );
					// use bundle file
					buffer.SetAccessFromBundleFile( bufferInfo.m_dataSizeOnDisk, bundleFileID );
				}
				else
				{
					WARN_CORE( TXT("Missing buffer file %d for '%ls' in bundles"), 
						bufferIndex, m_loadedFile->GetDepotPath().AsChar() );
				}
			}
		}
#if !defined( RED_CONFIGURATION_RELEASE )
		else
		{
			WARN_CORE( TXT("Unable to determine access for buffer file %d for '%ls'"), 
				bufferIndex, m_loadedFile->GetDepotPath().AsChar() );
		}
#endif
	}
}

namespace Helper
{
	class CScopedLoadTablesProfile
	{
	public:
		CScopedLoadTablesProfile( const CDiskFile* file )
			: m_file( file )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			if ( m_file ) RedIOProfiler::ProfileDiskFileLoadTablesStart( m_file->GetDepotPath().AsChar() );
#endif
		}

		~CScopedLoadTablesProfile()
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			if ( m_file ) RedIOProfiler::ProfileDiskFileLoadTablesEnd( m_file->GetDepotPath().AsChar() );
#endif
		}

	private:
		const CDiskFile* m_file;
	};
} // Helper

Bool GSkipCRCVerification = true;

Bool CDependencyLoader::PrecacheResolvedTables( DependencyLoadingPrecachedTables& outTables )
{
	// load file tables
	if ( !LoadTables() )
		return false;

	// map runtime data
	if ( !MapRuntimeData() )
		return false;

	// map the referenced types from exports
	for ( Uint32 i=0; i<m_data->m_exports.Size(); ++i )
		MapType( m_data->m_exports[i].m_className );

	// map the referenced types from imports
	for ( Uint32 i=0; i<m_data->m_imports.Size(); ++i )
		MapType( m_data->m_imports[i].m_className );

	// copy resolved data
	outTables.m_mappedNames = std::move( m_mappedNames );
	outTables.m_mappedProperties = std::move( m_mappedProperties );
	outTables.m_mappedPropertyOffsets = std::move( m_mappedPropertyOffsets );
	outTables.m_mappedTypes = std::move( m_mappedTypes );

	// resolve depot files for imports
	outTables.m_mappedImports.Resize( m_data->m_imports.Size() );
	for ( Uint32 i=0; i<m_data->m_imports.Size(); ++i )
	{
		const auto& import = m_data->m_imports[i];
		outTables.m_mappedImports[i] = nullptr;

		if ( import.m_flags & CDependencyFileData::eImportFlags_Soft )
			continue;

		const AnsiChar* path = &m_data->m_strings[ import.m_path ];
		outTables.m_mappedImports[i] = GDepot->FindFile( ANSI_TO_UNICODE( path ) );
	}

	// make sure array sizes are optimal
	outTables.m_mappedNames.Shrink();
	outTables.m_mappedProperties.Shrink();
	outTables.m_mappedPropertyOffsets .Shrink();
	outTables.m_mappedTypes.Shrink();
	return true;	
}

Bool CDependencyLoader::LoadTables()
{
	Helper::CScopedLoadTablesProfile profiler( m_loadedFile );

	// Remember the base offset for the whole file structure
	const Uint64 baseOffset = GetOffset();

	// Load data from file tables
	Uint32 fileVersion = 0;
	CDependencyFileData* data = new CDependencyFileData();
	if ( !data->Load( *m_file, fileVersion ) )
	{
		delete data;
		return false;
	}

	// Make sure file version is valid
	RED_FATAL_ASSERT( fileVersion >= VER_MINIMAL && fileVersion <= VER_CURRENT, "Invalid file version for '%ls' (%d)", m_file->GetFileNameForDebug(), fileVersion );
	SetVersion( fileVersion );

	// Setup data
	m_data.Reset( data );

	// Validate CRC of the objects
#ifndef RED_FINAL_BUILD
	if ( !GFileManager->IsReadOnly() && !GIsGame && !GSkipCRCVerification )
	{
		static Red::CRC32 crcCalculator;
		Uint8 readBuffer[ 16*1024 ];

		// validate exports
		for ( Uint32 i=0; i<m_data->m_exports.Size(); ++i )
		{
			const auto& ex = m_data->m_exports[i];
			if ( !ex.m_crc )
				continue;

			// move to object position
			Uint32 objectSize = ex.m_dataSize;
			m_file->Seek( baseOffset + ex.m_dataOffset );

			// read data and compute CRC
			Uint32 crc = 0;
			while ( objectSize > 0 )
			{
				// read chunk
				Uint32 maxRead = Min<Uint32>( ARRAY_COUNT_U32( readBuffer ), objectSize );
				m_file->Serialize( readBuffer, maxRead );
				objectSize -= maxRead;

				// compute CRC
				crc = crcCalculator.Calculate( readBuffer, maxRead, crc );
			}

			// compare it
			if ( ex.m_crc != crc )
			{
				const AnsiChar* className = &m_data->m_strings[ ex.m_className ];

				ERR_CORE( TXT("!!! FILE CORRUPTION !!!" ) );
				ERR_CORE( TXT("Object %d of class '%ls' at offset %d, size %d in '%ls' is corrupted (CRC mismatch)."), 
					i, ANSI_TO_UNICODE(className),
					ex.m_dataOffset, ex.m_dataSize, m_file->GetFileNameForDebug() );

				return false;
			}
		}

		// validate buffers
		for ( Uint32 i=0; i<m_data->m_buffers.Size(); ++i )
		{
			const auto& ex = m_data->m_buffers[i];
			if ( !ex.m_crc )
				continue;

			// move to object position
			Uint32 objectSize = ex.m_dataSizeOnDisk;
			m_file->Seek( baseOffset + ex.m_dataOffset );

			// read data and compute CRC
			Uint32 crc = 0;
			while ( objectSize > 0 )
			{
				// read chunk
				Uint32 maxRead = Min<Uint32>( ARRAY_COUNT_U32( readBuffer ), objectSize );
				m_file->Serialize( readBuffer, maxRead );
				objectSize -= maxRead;

				// compute CRC
				crc = crcCalculator.Calculate( readBuffer, maxRead, crc );
			}

			// compare it
			if ( ex.m_crc != crc )
			{
				ERR_CORE( TXT("!!! FILE CORRUPTION !!!" ) );
				ERR_CORE( TXT("Buffer %d at offset %d, size %d in '%ls' is corrupted (CRC mismatch)."), 
					i, ex.m_dataOffset, ex.m_dataSizeOnDisk, m_file->GetFileNameForDebug() );

				return false;
			}
		}
	}
#endif
	
	// Header loaded
	return true;
}

namespace Helper
{
	class CScopedMapTableProfile
	{
	public:
		CScopedMapTableProfile( const CDiskFile* file )
			: m_file( file )
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			if ( m_file ) RedIOProfiler::ProfileDiskFileMapTablesStart( m_file->GetDepotPath().AsChar() );
#endif
		}

		~CScopedMapTableProfile()
		{
#ifdef RED_PROFILE_FILE_SYSTEM
			if ( m_file ) RedIOProfiler::ProfileDiskFileMapTablesEnd( m_file->GetDepotPath().AsChar() );
#endif
		}

	private:
		const CDiskFile* m_file;
	};
} // Helper

Bool CDependencyLoader::MapRuntimeData()
{
	Helper::CScopedMapTableProfile profiler( m_loadedFile );

	// Create runtime data
	m_mappedNames.Resize( m_data->m_names.Size() );
	m_mappedTypes.Resize( m_data->m_names.Size() );
	m_mappedProperties.Resize( m_data->m_properties.Size() );
	m_mappedPropertyOffsets.Resize( m_data->m_properties.Size() );
	m_mappedImports.Resize( m_data->m_imports.Size() );
	m_mappedExports.Resize( m_data->m_exports.Size() );

	// Map names
	for ( Uint32 i=0; i<m_mappedNames.Size(); ++i )
	{
		// get text
		const AnsiChar* string = &m_data->m_strings[ m_data->m_names[i].m_string ];
		m_mappedNames[i] = CName( ANSI_TO_UNICODE( string ) );

		// types are not mapped until requested
		m_mappedTypes[i] = nullptr;
	}

	// Map imports
	for ( Uint32 i=0; i<m_mappedImports.Size(); ++i )
	{
		const auto& data = m_data->m_imports[i];

		// resolve path - only when we don't have the hash
		const AnsiChar* string = "<hashed path>";
		if ( !(data.m_flags & CDependencyFileData::eImportFlags_HashedPath) )
		{
			// resource path
			string = &m_data->m_strings[ data.m_path ];
		}

		// set resolved path
		m_mappedImports[i].m_path = string;

		// map type name
		IRTTIType* type = MapType( data.m_className );
#ifndef RED_FINAL_BUILD
		if ( type && type->GetType() != RT_Class )
		{
			ERR_CORE( TXT("Type for import '%ls' is '%ls' which is not a class"), 
				ANSI_TO_UNICODE( string ), type->GetName().AsChar() );
			return false;
		}
#endif

		// mapped type should be a resource
		CClass* importClass = static_cast< CClass* >( type );
#ifndef RED_FINAL_BUILD
		if ( type && !importClass->IsA< CResource >() )
		{
			ERR_CORE( TXT("Type for import '%ls' is '%ls' which is not a CResource"), 
				ANSI_TO_UNICODE( string ), type->GetName().AsChar() );
			return false;
		}
#endif

		m_mappedImports[i].m_class = importClass;
	}

	// Map properties
	if ( !m_mappedProperties.Empty() )
	{
		m_mappedProperties[0] = nullptr;
		m_mappedPropertyOffsets[0] = -1;

		// map properties
		for ( Uint32 i=1; i<m_mappedProperties.Size(); ++i )
		{
			const auto& data = m_data->m_properties[i];

			// Lookup in the RTTI cache, type must match
			//const CName typeName = MapName( data.m_typeName );
			const CProperty* prop = SRTTI::GetInstance().FindProperty( data.m_hash );
			if ( prop )//&& prop->GetType()->GetName() == typeName )
			{
				m_mappedProperties[i] = prop;
				m_mappedPropertyOffsets[i] = prop ? prop->GetDataOffset() : -1;
			}
			else
			{
				CClass* classType = SRTTI::GetInstance().FindClass( MapName( data.m_className ) );
				if ( classType )
				{
					CProperty* prop = classType->FindProperty( MapName( data.m_propertyName ) );
					if ( prop )
					{
						WARN_CORE( TXT("Unhashed property '%ls' in '%ls': property should be in global cache but it's not, cur hash = 0x%16llX, saved hash = 0x%16llX"),
							MapName( data.m_propertyName ).AsChar(), MapName( data.m_className ).AsChar(),
							prop->GetHash(), data.m_hash );

						m_mappedProperties[i] = prop;
						m_mappedPropertyOffsets[i] = prop->GetDataOffset();
					}
					else
					{
						WARN_CORE( TXT("Missing property '%ls' in '%ls': not found in class"),
							MapName( data.m_propertyName ).AsChar(), MapName( data.m_className ).AsChar() );

						m_mappedProperties[i] = nullptr;
						m_mappedPropertyOffsets[i] = -1;
					}
				}
				else
				{
					WARN_CORE( TXT("Missing property '%ls' in '%ls': no class"),
						MapName( data.m_propertyName ).AsChar(), MapName( data.m_className ).AsChar() );

					m_mappedProperties[i] = nullptr;
					m_mappedPropertyOffsets[i] = -1;
				}
			}
		}
	}

	// Map exports
	for ( Uint32 i=0; i<m_mappedExports.Size(); ++i )
	{
		const auto& data = m_data->m_exports[i];

		// map export type
		IRTTIType* type = MapType( data.m_className );
#ifndef RED_FINAL_BUILD
		if ( !type )
		{
			WARN_CORE( TXT("Unknown type for export %d ('%ls') in file '%ls'"), 
				i, MapName( data.m_className ).AsChar(), m_file->GetFileNameForDebug() );			
		}
		else if ( type->GetType() != RT_Class )
		{
			WARN_CORE( TXT("Non class type for export %d ('%ls') is file '%ls'"), 
				i, type->GetName().AsChar(), m_file->GetFileNameForDebug() );
			type = nullptr;
		}
#endif

		// allow the export class to be changed
		CClass* exportClass = static_cast< CClass* >( type );
		CClass* changedClass = ChangeExportClass( exportClass, m_context );
		if ( changedClass && (changedClass != exportClass) )
		{
			WARN_CORE( TXT("Export class for #%d changed from '%ls' to '%ls' (file %ls)"), 
				i, exportClass->GetName().AsChar(), changedClass->GetName().AsChar(), 
				m_file->GetFileNameForDebug() );

			exportClass = changedClass;
		}

		// store
		m_mappedExports[i].m_class = exportClass;
		m_mappedExports[i].m_rawPointer = nullptr;
	}

	// Prepare file tables
	m_fileTables.m_numMappedNames = m_mappedNames.Size();
	m_fileTables.m_mappedNames = m_mappedNames.TypedData();
	m_fileTables.m_numMappedTypes = m_mappedTypes.Size();
	m_fileTables.m_mappedTypes = (const IRTTIType**) m_mappedTypes.TypedData();
	m_fileTables.m_numMappedProperties = m_mappedProperties.Size();
	m_fileTables.m_mappedProperties = (const CProperty**) m_mappedProperties.TypedData();
	m_fileTables.m_mappedPropertyOffsets = m_mappedPropertyOffsets.TypedData();

	// Runtime data was mapped
	return true;
}

namespace Helper
{
	/// Helper class for accessing file data at modified offset
	class COffsetFileReader : public IFile, public IFileDirectMemoryAccess
	{
	public:
		COffsetFileReader( IFile* baseFile, const Uint32 offset, const Uint32 size )
			: IFile( baseFile->GetFlags() )
			, m_file( baseFile )
			, m_offset( offset )
			, m_size( size )
		{
			// get base file DMA
			m_baseDMA = baseFile->QueryDirectMemoryAccess();

			// move to the start of the file
			baseFile->Seek( offset );
		}

		virtual void Serialize( void* buffer, size_t size ) override
		{
			m_file->Serialize( buffer, size );
		}

		virtual Uint64 GetOffset() const override
		{
			const Uint64 baseOffset = m_file->GetOffset();
			RED_FATAL_ASSERT( baseOffset >= m_offset, "Out of bounds file access for inplace data (%d < %d). Very unsafe.", baseOffset, m_offset );
			RED_FATAL_ASSERT( baseOffset <= m_offset + m_size, "Out of bounds file access for inplace data (%d > %d). Very unsafe.", baseOffset, m_offset + m_size );
			return m_file->GetOffset() - m_offset;
		}

		virtual Uint64 GetSize() const override
		{
			return m_size;
		}

		virtual void Seek( Int64 offset ) override
		{
			RED_FATAL_ASSERT( offset >= 0, "Out of bounds file access for inplace data (%d < %d). Very unsafe.", (Uint32)offset, 0 );
			RED_FATAL_ASSERT( offset <= (Int64)m_size, "Out of bounds file access for inplace data (%d > %d). Very unsafe.", (Uint32)offset, m_size );
			m_file->Seek( offset + m_offset );
		}

		virtual const Char *GetFileNameForDebug() const override
		{
			return m_file->GetFileNameForDebug();
		}

		virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override
		{
			return m_baseDMA ? this : nullptr;
		}

		virtual Uint8* GetBufferBase() const override
		{
			RED_FATAL_ASSERT( m_baseDMA != nullptr, "No DMA access in base file yet DMA access for offseted file requested" );
			return m_baseDMA->GetBufferBase() + m_offset;
		}

		virtual Uint32 GetBufferSize() const override
		{
			return m_size;
		}

	private:
		IFile*						m_file;
		IFileDirectMemoryAccess*	m_baseDMA;
		Uint32						m_offset;
		Uint32						m_size;
	};
}

Bool CDependencyLoader::LoadInplaceFiles()
{
	// no inplace files to load
	if ( m_data->m_inplace.Empty() )
		return true;

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadInplaceStart( m_loadedFile->GetDepotPath().AsChar(), m_data->m_inplace.Size() );
#endif

	// Load inplace buffers
	for ( Uint32 inplaceIndex=0; inplaceIndex<m_data->m_inplace.Size(); ++inplaceIndex )
	{
		const auto& inpl = m_data->m_inplace[ inplaceIndex ];

		// find file in the depot using hashed path
		CDiskFile* depotFile = nullptr;
		if ( GFileManager->IsReadOnly() )
		{
			// hashed paths are only aviable in read only dept
			depotFile = GDepot->FindFile( inpl.m_pathHash );
		}
		
		// try normal path
		if ( !depotFile )
		{
			const AnsiChar* path = &m_data->m_strings[ inpl.m_path ];
			depotFile = GDepot->FindFile( ANSI_TO_UNICODE( path ) );
			if ( !depotFile )
			{
				ERR_CORE( TXT("!!! MISSING INPLACE !!! Failed to find depot file for hash 0x%016llX (%hs) in file '%ls'. Not loading."), 
					inpl.m_pathHash, path, m_file->GetFileNameForDebug() );
				continue;
			}
		}

		// DO NOT INPLACE LOAD THE OVERRIDDEN FILES
		if ( depotFile->IsOverriden() )
		{
			const AnsiChar* path = &m_data->m_strings[ inpl.m_path ];
			LOG_CORE( TXT("Ignoring overridden inplace file '%hs'"), path );
			continue;
		}

		// begin loading
		if ( !depotFile->IsLoading() && depotFile->BeingLoading() )
		{
			// load from data that's contained within this file
			Helper::COffsetFileReader offsetReader( m_file, inpl.m_dataOffset, inpl.m_dataSize );

			SDiskFilePostLoadList postLoadList;
			depotFile->InternalDeserialize( &offsetReader, m_context->m_importLoader, postLoadList );

			if ( !depotFile->IsFailed() )
			{
				depotFile->InternalPostLoad( postLoadList );
				RED_FATAL_ASSERT( depotFile->GetResource(), "Resource not finalized properly" );
			}

			RED_FATAL_ASSERT( !depotFile->IsLoading(), "Resource not finalized properly" );
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadInplaceEnd( m_loadedFile->GetDepotPath().AsChar() );
#endif

	// Even if we have some missing imports allow to continue
	return true;
}

Bool CDependencyLoader::CreateImports()
{
	// no imports to load
	if ( m_data->m_imports.Empty() )
		return true;

	// import loading not enabled
	if ( !m_context->m_importLoader )
		return true;

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadImportsStart( m_loadedFile->GetDepotPath().AsChar(), m_data->m_imports.Size() );
#endif

	// Load imports
	for ( Uint32 importIndex=0; importIndex<m_mappedImports.Size(); importIndex++ )
	{
		const auto& imp = m_data->m_imports[ importIndex ];
		auto& runtime = m_mappedImports[ importIndex ];

		// Soft import
		if ( imp.m_flags & CDependencyFileData::eImportFlags_Soft )
			continue;

		// Empty import path
		if ( !runtime.m_path || !runtime.m_path[0] )
			continue;

		// Get object class
		CClass* objectClass = runtime.m_class;
		if ( !objectClass )
		{
			const CName className = MapName( imp.m_className );

			ERR_CORE( TXT("!!! MISSING IMPORT !!! Unknown class '%ls' for import '%ls' in '%ls'. Skipping import."), 
				className.AsChar(), ANSI_TO_UNICODE( runtime.m_path ), GetFileNameForDebug() );
			continue;
		}

		// Already loaded
		if ( runtime.m_resource.IsValid() )
			continue;

		// bind
		runtime.m_resource = m_context->m_importLoader->LoadDependency( objectClass, runtime.m_path, imp, false /*silent*/ );
		if ( !runtime.m_resource.IsValid() )
		{
			ERR_CORE( TXT("!!! MISSING IMPORT !!! Missing resource import '%ls' of class '%ls' in file '%ls'."), 
				ANSI_TO_UNICODE( runtime.m_path ), objectClass->GetName().AsChar(), GetFileNameForDebug() );
			continue;
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadImportsEnd( m_loadedFile->GetDepotPath().AsChar() );
#endif

	// Even if we have some missing imports allow to continue
	return true;
}

Bool CDependencyLoader::CreateExports()
{
	Bool ret = true;

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileCreateExportsStart( m_loadedFile->GetDepotPath().AsChar(), m_data->m_exports.Size() );
#endif

	for ( Uint32 exportIndex=0; exportIndex<m_mappedExports.Size(); ++exportIndex )
	{
		const auto& ex = m_data->m_exports[ exportIndex ];
		auto& runtime = m_mappedExports[ exportIndex ];

		// Get object class
		CClass* objectClass = runtime.m_class;
		if ( !objectClass )
		{
			runtime.m_skip = true;
			continue;
		}

		// If parent is skipped skip this object also
		if ( ex.m_parent && m_mappedExports[ ex.m_parent-1 ].m_skip )
		{
			runtime.m_skip = true;
			continue;
		}

		// Determine parent object 
		ISerializable* objectParent = m_context->m_parent;
		if ( ex.m_parent )
		{
			const auto& parent = m_mappedExports[ ex.m_parent-1 ];
			RED_FATAL_ASSERT( parent.m_class != nullptr, "Parent object with no class" ); // should be handled by the skip logic

			if ( parent.m_class->IsSerializable() )
			{
				objectParent = static_cast< ISerializable* >( parent.m_rawPointer );
				if ( !objectParent )
				{
					continue;
				}
			}
		}

		// CObjects are more complicated
		if ( objectClass->IsObject() )
		{
			// Parent object should be a CObject
			if ( objectParent && !objectParent->IsA< CObject >() )
			{
				ERR_CORE( TXT("Parent of '%ls' is not a CObject but '%ls', in file '%ls'"), 
					objectClass->GetName().AsChar(), objectParent->GetClass()->GetName().AsChar(), m_file->GetFileNameForDebug() );
				continue;
			}

			// Real parent
			CObject* realObjectParent = Cast< CObject >( objectParent );

			// Class is transient, skip loading
			if ( objectClass->IsAlwaysTransient() )
			{
				WARN_CORE( TXT("Skipping loading of transient class '%ls'."), objectClass->GetName().AsChar() );
				runtime.m_skip = true;
				continue;
			}

			// Object is created from a template
			if ( ex.m_template )
			{
				// Get template
				CObject* templateObject = MapObject( ex.m_template ).GetObjectPtr();
				if ( !templateObject )
				{
					if ( ex.m_template < 0 )
					{
						WARN_CORE( TXT("Unable to instance template %ls. Missing template on path: %ls. PLEASE DEBUG."), 
							realObjectParent->GetFriendlyName().AsChar(), ANSI_TO_UNICODE( m_mappedImports[ -(ex.m_template+1) ].m_path ) );
					}
					else
					{
						WARN_CORE( TXT("Unable to instance template %ls. Missing template. PLEASE DEBUG."), 
							realObjectParent->GetFriendlyName().AsChar() );
					}

					// just skip the object
					runtime.m_skip = true;
					continue;
				}

				// Create object
				runtime.m_rawPointer = templateObject->CreateTemplateInstance( realObjectParent );
				if ( !runtime.m_rawPointer )
				{
					WARN_CORE( TXT("Unable to create instance of template '%ls'"), templateObject->GetFriendlyName().AsChar() );
				}
			}

			// Create from class anyways
			if ( !runtime.m_rawPointer )
			{
				Uint16 objectFlags = ex.m_objectFlags;

				// Use preallocated memory
				CObject* createdObject = nullptr;
				if ( m_context->m_firstExportMemory && exportIndex == 0 )
				{
					// make sure we have enough memory
					RED_FATAL_ASSERT( objectClass->GetSize() <= m_context->m_firstExportMemorySize, "Preallocated memory for object is to small" );
					//RED_FATAL_ASSERT( m_context->m_firstExportMemory->IsA( objectClass ), "Invalid resource class passed to dependency loader" );

					// construct the object using the target class
					createdObject = m_context->m_firstExportMemory;

					// recreate the data if required
					CClass* currentClass = createdObject->GetClass();
					if ( currentClass != objectClass )
					{
						CDiskFile* file = nullptr;
						if ( objectClass->IsA< CResource >() )
						{
							file = ((CResource*) createdObject)->GetFile();							
							if ( file )
							{
								((CResource*) createdObject)->m_file = nullptr;
								file->m_resource = nullptr;
							}
						}

						// destroy old class data, create new class data
						createdObject->SetFlag( OF_Finalized );
						currentClass->DestroyObject( createdObject, /* call dtor*/ true, /* free mem */ false );

						// create new object in place of the already allocated memory
						objectClass->CreateObject( objectClass->GetSize(), true, createdObject );

						if ( file )
						{
							file->m_resource = ((CResource*) createdObject);
							((CResource*) createdObject)->m_file = file;
						}
					}
				}
				else
				{
					// Create new empty export
					createdObject = objectClass->CreateObject< CObject >();;

					// Bail if no export was created
					if ( NULL == createdObject )
					{
						WARN_CORE( TXT("Unable to create export of class '%ls'"), objectClass->GetName().AsChar() );
						runtime.m_skip = true;
						continue;
					}
				}

				createdObject->SetParent( realObjectParent );
				createdObject->ClearFlag( 0xFFFF );
				createdObject->SetFlag( objectFlags & OF_SerializationMask );

				runtime.m_rawPointer = createdObject;

				// extract created resources
				if ( createdObject && !realObjectParent && createdObject->IsA< CResource >() )
				{
					m_context->m_loadedResources.PushBack( static_cast< CResource* >( createdObject ) );
				}
			}
		}
		else
		{
			// Simple object, memory is allocated from memory pool specified in the class
			void* object = objectClass->CreateObject( objectClass->GetSize() );
			if ( NULL == object )
			{
				WARN_CORE( TXT("Unable to create export of class '%ls'"), objectClass->GetName().AsChar() );
				runtime.m_skip = true;
				continue;
			}

			// Setup object pointer using the class that was saved
			runtime.m_rawPointer = object;
		}

		// If created object is of IReferencable type keep the handle around - 
		// this will allow us to handle cases when loaded objects are deleted (for example in PostLoad)
		if ( objectClass->IsA< IReferencable >() )
		{
			runtime.m_object = BaseSafeHandle( static_cast< IReferencable* >( runtime.m_rawPointer ) );
		}

		// Restore object parent
		if ( objectClass->IsSerializable() && objectParent != nullptr )
		{
			ISerializable* serializable = static_cast< ISerializable* >( runtime.m_rawPointer );
			serializable->RestoreSerializationParent( objectParent );
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileCreateExportsEnd( m_loadedFile->GetDepotPath().AsChar() );
#endif

	// Return error flag
	return ret;
}

Bool CDependencyLoader::LoadExports( Uint64 baseOffset )
{
	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadExportsStart( m_loadedFile->GetDepotPath().AsChar(), m_data->m_exports.Size() );
#endif

	// get the mapped memory
	IFileDirectMemoryAccess* fileDirectMemoryAccess = m_file->QueryDirectMemoryAccess();
	m_fileMappedMemory = fileDirectMemoryAccess ? fileDirectMemoryAccess->GetBufferBase() : nullptr;
	m_fileMappedSize = fileDirectMemoryAccess ? fileDirectMemoryAccess->GetBufferSize() : 0;

	// Load exports
	for ( Uint32 exportIndex=0; exportIndex<m_mappedExports.Size(); ++exportIndex )
	{
		const auto& ex = m_data->m_exports[ exportIndex ];
		auto& runtime = m_mappedExports[ exportIndex ];

		// No data, skip the object
		if ( !runtime.m_rawPointer )
			continue;

		// Sanity checks
		RED_FATAL_ASSERT( ex.m_dataSize > 0, "Export %d in %ls has no data", exportIndex, m_file->GetFileNameForDebug() );

		// Detect case when object was deleted before it got loaded
		if ( runtime.m_object.IsLost() )
		{
			// anoying
			/*WARN_CORE( TXT("Export %d ('%ls') from '%ls' deleted before it was loaded"),
				exportIndex, runtime.m_class->GetName().AsChar(), m_file->GetFileNameForDebug() );*/
			continue;
		}

		// Move to file position
		const Uint64 objectStart = baseOffset + ex.m_dataOffset;
		m_file->Seek( objectStart );
		m_file->ClearError();

		// Load object - 3 different methods
		// Method 1 (classic): Load the full CObject, this has support for all of the features
		// Method 2 (lighter): Load the object as ISerializable - no GC stuff, no parent object stuff, etc, NO TEMEPLATES
		// Method 3 (ultra light): Load the object as a pointer to something (even a structure) - unsafe, tricky memory managment but it works!
		// TODO: those ifs are killing me, there should be a way to get rid of them at least in the cooked data
		if ( runtime.m_class->IsObject() )
		{
			// Load as object
			CObject* object = static_cast< CObject* >( runtime.m_rawPointer );
			object->OnSerialize( *this );

			// Emit exported root
			if ( ex.m_parent == 0 )
			{
				m_context->m_loadedRootObjects.PushBack( object );
			}
		}
		else if ( runtime.m_class->IsSerializable() )
		{
			// Load as serializable
			ISerializable* serializable = static_cast< ISerializable* >( runtime.m_rawPointer );
			serializable->OnSerialize( *this );
		}
		else
		{
			// Load object via the class RTTI (there's no virtual method in the object itself)
			runtime.m_class->Serialize( *this, runtime.m_rawPointer );
		}

#ifndef RED_FINAL_BUILD
		// Seeked past the end of the file ?
		const Uint64 objectEnd = baseOffset + ex.m_dataOffset + ex.m_dataSize;
		if ( m_file->HasErrors() )
		{
			ERR_CORE( TXT("Export %d ('%ls') from '%ls' caused file IO errors when reading (start=%d, size=%d)"),
				exportIndex, runtime.m_class->GetName().AsChar(), m_file->GetFileNameForDebug(),
				objectStart, ex.m_dataSize );
		}
		if ( m_file->GetOffset() > objectEnd )
		{
			ERR_CORE( TXT("Export %d ('%ls') from '%ls' accessed data beyond it's location in file (offset=%d, start=%d, stop=%d)"),
				exportIndex, runtime.m_class->GetName().AsChar(), m_file->GetFileNameForDebug(),
				m_file->GetOffset(), objectStart, objectEnd );
		}
#endif

		// All object export
		if ( m_context->m_getAllLoadedObjects )
		{
			m_context->m_loadedObjects.PushBack( CPointer( runtime.m_rawPointer, runtime.m_class ) );
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFileLoadExportsEnd( m_loadedFile->GetDepotPath().AsChar() );
#endif

	// Assume no errors
	return true;
}

Bool CDependencyLoader::SearchForExportsMissingTemplates( TDynArray< String >& missingFiles )
{
	missingFiles.Clear();
#if !defined( NO_EDITOR )
	if ( LoadTables() && MapRuntimeData() )
	{
		for ( Uint32 exportIndex = 0; exportIndex < m_mappedExports.Size(); ++exportIndex )
		{
			const auto& ex = m_data->m_exports[ exportIndex ];

			if ( ex.m_template )
			{
				auto& runtime = m_mappedImports[ -( ex.m_template + 1 ) ];

				// Get template
				CDiskFile* depotFile = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( runtime.m_path ), 0 );
				CObject* templateObject = depotFile ? depotFile->Load() : nullptr;
				if ( !templateObject )
				{
					if ( SIsMainThread() )
					{
						if ( ex.m_template < 0 )
						{
							missingFiles.PushBack( ANSI_TO_UNICODE( m_mappedImports[ -(ex.m_template+1) ].m_path ) );
						}
						else
						{
							missingFiles.PushBack( String::EMPTY );
						}
					}
				}
			}
		}
	}
#endif
	return !missingFiles.Empty();
}

Bool CDependencyLoader::PostLoad()
{
	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFilePostLoadStart( m_loadedFile->GetDepotPath().AsChar() );
#endif

	for ( Uint32 exportIndex=0; exportIndex<m_mappedExports.Size(); ++exportIndex )
	{
		auto& runtime = m_mappedExports[ exportIndex ];

		// No data, skip the object
		if ( !runtime.m_rawPointer )
			continue;

		// Detect case when object was deleted before it got loaded
		if ( runtime.m_object.IsLost() )
		{
			// anoying
			/*WARN_CORE( TXT("Export %d ('%ls') from '%ls' deleted before it was postloaded"),
				exportIndex, runtime.m_class->GetName().AsChar(), m_file->GetFileNameForDebug() );*/
			continue;
		}
	
		// Let the object know it was loaded, this only works on ISerializable objects
		if ( runtime.m_class->IsSerializable() )
		{
			ISerializable* serializable = static_cast< ISerializable* >( runtime.m_rawPointer );
			serializable->OnPostLoad();
		}
	}

	// profiling
#ifdef RED_PROFILE_FILE_SYSTEM
	if ( m_loadedFile )
		RedIOProfiler::ProfileDiskFilePostLoadEnd( m_loadedFile->GetDepotPath().AsChar() );
#endif

	// Assume no errors
	return true;
}

CPointer CDependencyLoader::MapObject( Int32 index )
{
	if ( index < 0 )
	{
		const Int32 importIndex = -(index+1);
#ifndef RED_FINAL_BUILD
		if ( importIndex >= (Int32)m_mappedImports.Size() )
		{
			ERR_CORE( TXT("Invalid import index %d (of %d) in file '%ls'. Mapping to NULL."), 
				importIndex, m_mappedImports.Size(), m_file->GetFileNameForDebug() );

			return nullptr;
		}
#endif

		const auto& data = m_mappedImports[ importIndex ];

		// it's illegal to directly map soft imports
		const Bool isSoft = 0 != (m_data->m_imports[ importIndex ].m_flags & CDependencyFileData::eImportFlags_Soft);
		if ( isSoft )
		{
			ERR_CORE( TXT("Trying to directly map soft import at index %d (of %d) in file '%ls'. Mapping to NULL."), 
				importIndex, m_mappedImports.Size(), m_file->GetFileNameForDebug() );

			return nullptr;
		}

		return (CResource*) data.m_resource.Get();
	}
	else if ( index > 0 )
	{
		const Int32 exportIndex = (index-1);
#ifndef RED_FINAL_BUILD
		if ( exportIndex >= (Int32)m_mappedExports.Size() )
		{
			ERR_CORE( TXT("Invalid export index %d (of %d) in file '%ls'. Mapping to NULL."), 
				exportIndex, m_mappedExports.Size(), m_file->GetFileNameForDebug() );

			return nullptr;
		}
#endif

		const auto& data = m_mappedExports[ exportIndex ];
		return CPointer( data.m_rawPointer, data.m_class );
	}

	// NULL (index=0)
	return nullptr;
}

void* CDependencyLoader::MapPointer( Int32 index, const class CClass* pointerClass )
{
	const CPointer mappedPointer = MapObject( index );
	if ( !mappedPointer.IsNull() )
	{
		if ( mappedPointer.GetRuntimeClass()->IsA( pointerClass ) )
		{
			return mappedPointer.GetPointer();
		}
		else
		{
			WARN_CORE( TXT("Destination pointer type has changed. Unable to store '%ls' in a pointer for '%ls'"),
				mappedPointer.GetRuntimeClass()->GetName().AsChar(),
				pointerClass->GetName().AsChar() );

			return nullptr;
		}
	}
	else 
	{
		return nullptr;
	}
}

CName CDependencyLoader::MapName( Uint16 index ) const
{
	if ( index >= (Uint16) m_mappedNames.Size() )
	{
		ERR_CORE( TXT("Invalid name index %d (of %d) in file '%ls'. Mapping to None."), 
			index, m_mappedNames.Size(), m_file->GetFileNameForDebug() );
		return CName::NONE;
	}

	return m_mappedNames[ index ];
}

IRTTIType* CDependencyLoader::MapType( Uint16 index )
{
	IRTTIType* type = nullptr;

#ifndef RED_FINAL_BUILD
	if ( index >= (Uint16) m_mappedTypes.Size() )
	{
		ERR_CORE( TXT("Invalid type index %d (of %d) in file '%ls'. Mapping to None."), 
			index, m_mappedTypes.Size(), m_file->GetFileNameForDebug() );
		return nullptr;
	}
#endif

	if ( index != 0 )
	{
		// Use already mapped type
		type = m_mappedTypes[ index ];
		if ( !type )
		{
			// Map type name and find matching type
			const CName mappedName = MapName( index );
			type = SRTTI::GetInstance().FindType( mappedName );
			m_mappedTypes[ index ] = type;
		}
	}

	return type;
}

const CProperty* CDependencyLoader::MapProperty( Uint16 index ) const
{
	const CProperty* prop = nullptr;

#ifndef RED_FINAL_BUILD
	if ( index >= (Uint16) m_mappedProperties.Size() )
	{
		ERR_CORE( TXT("Invalid property index %d (of %d) in file '%ls'. Mapping to None."), 
			index, m_mappedProperties.Size(), m_file->GetFileNameForDebug() );
		return nullptr;
	}
#endif

	if ( index != 0 )
	{
		prop = m_mappedProperties[ index ];
	}

	return prop;
}

BaseSoftHandle CDependencyLoader::MapSoftHandle( Uint16 index )
{
	if ( !index )
		return BaseSoftHandle();

	const Uint32 softHandleIndex = m_data->m_softImportBase + (index-1);
#ifndef RED_FINAL_BUILD	
	if ( softHandleIndex >= m_mappedImports.Size() )
	{
		ERR_CORE( TXT("Invalid soft import index %d (of %d) in file '%ls'. Mapping to None."), 
			index, m_mappedImports.Size() - m_data->m_softImportBase, m_file->GetFileNameForDebug() );
			return String::EMPTY;
	}
#endif

	// in legacy code it's illegal to use hard import as soft dependency
	if ( m_data->m_softImportBase > 0 )
	{
		const Bool isSoft = (m_data->m_imports[ softHandleIndex ].m_flags & CDependencyFileData::eImportFlags_Soft) != 0;
		if ( !isSoft )
		{
			ERR_CORE( TXT("TryinInvalid soft import index %d (of %d) in file '%ls' - it's a hard dependency. Mapping to None."), 
				index, m_mappedImports.Size() - m_data->m_softImportBase, m_file->GetFileNameForDebug() );
			return String::EMPTY;
		}
	}

	// use the path
	const AnsiChar* path = m_mappedImports[ softHandleIndex ].m_path;

#ifndef RED_FINAL_BUILD
	// make double sure that paths inside soft handles are always resolved
	if ( !GFileManager->IsReadOnly() )
	{
		CDiskFile* file = GDepot->FindFileUseLinks( ANSI_TO_UNICODE( path ), 0 );
		if ( file != nullptr )
		{
			return BaseSoftHandle( file->GetDepotPath() );
		}
	}
#endif

	return BaseSoftHandle( ANSI_TO_UNICODE( path ) );
}


/// Reading token
class LoaderLatentLoadingToken : public IFileLatentLoadingToken
{
protected:
	const CDiskFile*	m_file;
	Uint32				m_version;

public:
	//! Constructor
	LoaderLatentLoadingToken( const CDiskFile* file, Uint64 offset, Uint32 version )
		: IFileLatentLoadingToken( offset)
		, m_file( file )
		, m_version( version )
	{}

	virtual IFileLatentLoadingToken* Clone() const
	{
		return new LoaderLatentLoadingToken( m_file, m_offset, m_version );
	}

	//! Describe loading token
	virtual String GetDebugInfo() const
	{
		return String::Printf( TXT("File %s, offset %") RED_PRIWu64 TXT(", version %i"), m_file->GetDepotPath().AsChar(), m_offset, m_version );
	}

	//! Resume loading
	virtual IFile* Resume( Uint64 relativeOffset )
	{
		// Open file at position
		IFile* file = m_file->CreateReader();
		if ( !file )
		{
			GFeedback->ShowError( TXT("File '%ls' cannot be found"), m_file->GetDepotPath().AsChar() );
			return NULL;
		}

		// Restore version
		file->m_version = m_version;

		// Set offset
		file->Seek( m_offset + relativeOffset );
		return file;
	}
};

IFileLatentLoadingToken* CDependencyLoader::CreateLatentLoadingToken( Uint64 currentOffset )
{	
	// We are loading a legit file
	if ( m_loadedFile )
	{
		// Create loading token
		return new LoaderLatentLoadingToken( m_loadedFile, currentOffset, GetVersion() );
	}

	// No real file being loaded, not able to resume
	return NULL;
}

IFileLatentLoadingToken* CDependencySaver::CreateLatentLoadingToken( Uint64 currentOffset )
{
	// We are saving to a disk file, we can resume loading from there
	if ( m_savedFile )
	{
		// Create loading token
		return new LoaderLatentLoadingToken( m_savedFile, currentOffset, GetVersion() );
	}

	// We cannot resume loading
	return NULL;
}

void CDependencyLoader::GenerateBufferFileName( const Uint32 bufferId, String& outBufferPath ) const
{
	DeferredDataBufferExtractor::GenerateFileName( m_loadedFile->GetDepotPath(), bufferId, outBufferPath );
}

Uint8* CDependencyLoader::GetBufferBase() const
{
	return const_cast< Uint8* >( m_fileMappedMemory );
}

Uint32 CDependencyLoader::GetBufferSize() const
{
	return m_fileMappedSize;
}

const class CFileDirectSerializationTables* CDependencyLoader::QuerySerializationTables() const
{
	return &m_fileTables;
}
