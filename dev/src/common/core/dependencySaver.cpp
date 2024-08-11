/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "dependencySaver.h"
#include "depot.h"
#include "resource.h"
#include "profiler.h"
#include "feedback.h"
#include "fileShadowWriter.h"
#include "deferredDataBuffer.h"
#include "cooker.h"
#include "deferredDataBufferExtractor.h"
#include "dependencyFileTables.h"
#include "dependencyFileTablesBuilder.h"
#include "../redSystem/crc.h"

DependencySavingContext::DependencySavingContext( ISerializable* objectToSave )
	: DependencyMappingContext( objectToSave )
	, m_dumpStats( false )
	, m_useFeedback( false )
	, m_taskName( TXT("Saving dependencies") )
	, m_hashPaths( false )
	, m_zeroNonDeterministicData( false )
{};

DependencySavingContext::DependencySavingContext( const TObjectsToMap& objectsToSave )
	: DependencyMappingContext( objectsToSave )
	, m_dumpStats( false )
	, m_useFeedback( false )
	, m_taskName( TXT("Saving dependencies") )
	, m_hashPaths( false )
	, m_zeroNonDeterministicData( false )
{};

DependencySavingContext::DependencySavingContext( const TSerializablesToMap& objectsToSave )
	: DependencyMappingContext( objectsToSave )
	, m_dumpStats( false )
	, m_useFeedback( false )
	, m_taskName( TXT("Saving dependencies") )
	, m_hashPaths( false )
	, m_zeroNonDeterministicData( false )
{};

CDependencySaver::CDependencySaver( IFile& file, const CDiskFile* fileBeingSaved )
	: IDependencySaver( file )
	, m_savedFile( fileBeingSaved )
#ifndef NO_RESOURCE_COOKING
	, m_bufferExtractor( nullptr )
#endif
{
}

CDependencySaver::~CDependencySaver()
{
}

Bool CDependencySaver::SetupNames( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TNameMap& outNames )
{
	// Emit names into the file tables
	for ( Uint32 i=0; i<mapper.m_names.Size(); ++i )
	{
		const Uint32 nameIndex = builder.MapName( mapper.m_names[i] );
		RED_ASSERT( nameIndex == i, TXT("Name mapping inconsistency (%d!=%d)"), i, nameIndex );
		if ( nameIndex != i )
			return false;
	}

	// for mapping use the same map
	outNames = Move( mapper.m_nameIndices );

	// names are ok
	return true;
}

Bool CDependencySaver::SetupProperties( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TPropertyMap& outProperties )
{
	// Emit properties into the file tables
	for ( Uint32 i=0; i<mapper.m_properties.Size(); ++i )
	{
		const Uint32 propIndex = builder.MapProperty( mapper.m_properties[i] );
		RED_ASSERT( propIndex == i, TXT("Property mapping inconsistency (%d!=%d)"), i, propIndex );
		if ( propIndex != i )
			return false;
	}

	// use the same map for mapping
	outProperties = Move( mapper.m_propertyIndices );

	// properties are OK
	return true;
}

Bool CDependencySaver::SetupImports( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TSoftDependencyMap& outSoftDependencies, TPointerMap& outObjectMap, const Bool hashPaths )
{
	// Emit imports into the file tables
	for ( Uint32 i=0; i<mapper.m_imports.Size(); ++i )
	{
		const auto& import = mapper.m_imports[i];
		RED_FATAL_ASSERT( import.m_resource != nullptr, "Import with no resource" );

		CDependencyFileDataBuilder::ImportInfo importBuilder;
		importBuilder.m_isSoft = false;
		importBuilder.m_isObligatory = false; // for now
		importBuilder.m_isTemplate = import.m_usedAsTemplate;
		importBuilder.m_className = import.m_resource->GetClass()->GetName();
		importBuilder.m_path = UNICODE_TO_ANSI( import.m_resource->GetDepotPath().AsChar() );

		const Uint32 index = builder.MapImport( importBuilder, hashPaths );
		RED_ASSERT( index == i, TXT("Import mapping inconsistency (%d!=%d)"), i, index );
		if ( index != i )
			return false;
	}

	// Emit soft imports into the file tables
	// NOTE: it may happen that soft import is already reported as hard import, in such case we do nothing
	outSoftDependencies.Reserve( mapper.m_softDependencies.Size() );
	for ( Uint32 i=1; i<mapper.m_softDependencies.Size(); ++i )
	{
		const auto& softImport = mapper.m_softDependencies[i];
		RED_FATAL_ASSERT( !softImport.Empty(), "Soft import without a path" );

		// setup import info
		CDependencyFileDataBuilder::ImportInfo importBuilder;
		importBuilder.m_isSoft = true;
		importBuilder.m_isObligatory = false;
		importBuilder.m_isTemplate = false;
		importBuilder.m_path = UNICODE_TO_ANSI( softImport.AsChar() );

		// extract resource class
		CDiskFile* file = GDepot->FindFileUseLinks( softImport, 0 );
		if ( file )
		{
			const CClass* resourceClass = file->GetResourceClass();
			if ( resourceClass )
			{
				importBuilder.m_className = resourceClass->GetName();
			}

			// force resolved path to be used
			importBuilder.m_path = UNICODE_TO_ANSI( file->GetDepotPath().AsChar() );
		}

		const Uint32 index = builder.MapImport( importBuilder, false ); // NOTE: we never hash soft import paths
		outSoftDependencies.Insert( softImport, index );
	}

	// Imports seems to be valid
	return true;
}

Bool CDependencySaver::SetupExports( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TPointerMap& outObjectMap, TExportMap& outExportMap )
{
	// Emit export data
	outExportMap.Resize( mapper.m_exports.Size() );
	for ( Uint32 i=0; i<mapper.m_exports.Size(); ++i )
	{
		const auto& exp = mapper.m_exports[i];

		// setup export info
		CDependencyFileDataBuilder::ExportInfo exportBuilder;
		exportBuilder.m_className = exp.GetRuntimeClass()->GetName();

		// CObject
		CObject* obj = exp.GetObjectPtr();
		if ( NULL != obj )
		{
			exportBuilder.m_objectFlags = obj->GetFlags() & ( OF_SerializationMask );

			// Map parent
			Int32 parent = -1;
			mapper.m_objectIndices.Find( obj->GetParent(), parent );
			ASSERT( parent >= 0 );
			exportBuilder.m_parent = parent;

			// Map template
			if ( obj->GetTemplate() )
			{
				Int32 templateIndex = 0;
				mapper.m_objectIndices.Find( obj->GetTemplate(), templateIndex );
				exportBuilder.m_template = templateIndex;
				ASSERT( templateIndex != 0 );
			}

			// Sanity check
			ASSERT( exportBuilder.m_parent < i+1 );
		}

		// Register in output data, for now we do not support re indexing
		const Uint32 exportIndex = builder.MapExport( exportBuilder );
		RED_ASSERT( exportIndex == i, TXT("Export mapping inconsistency (%d!=%d)"), i, exportIndex );
		if ( exportIndex != i )
			return false;

		// Keep in the map
		outExportMap[ i ] = exportIndex;
	}

	// Use the same object mapping table (saves time)
	outObjectMap = Move( mapper.m_objectIndices );

	// Exports seems to be valid
	return true;
}

Bool CDependencySaver::SetupBuffers( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TBufferMap& outBufferMap )
{
	// Emit buffer data
	outBufferMap.Reserve( mapper.m_bufferIndices.Size() );
	for ( Uint32 i=0; i<mapper.m_buffers.Size(); ++i )
	{
		const auto& buf = mapper.m_buffers[i];
		RED_FATAL_ASSERT( buf.m_id != 0, "Found mapped buffer with invalid ID" );
		RED_FATAL_ASSERT( buf.m_size != 0, "Found mapped buffer with zero size" );

		// empty buffers should not be mapped
		if ( !buf.m_size )
		{
			ERR_CORE( TXT("Defered data buffer is empty and yet it got mapped. File cannot be saved like that. Check your save related logic.") );
			return false;
		}

		// buffer lost the data
		if ( !buf.m_handle->GetData() )
		{
			ERR_CORE( TXT("Defered data buffer got lost before it got saved. File cannot be saved like that. Check your save related logic.") );
			return false;
		}

		// buffer size has changed
		if ( buf.m_handle->GetSize() != buf.m_size )
		{
			ERR_CORE( TXT("Defered data buffer's size has changed (%d->%d) before it got saved. File cannot be saved like that. Check your save related logic."), buf.m_size, buf.m_handle->GetSize() );
			return false;
		}

		// setup buffer info and add it to the builder
		CDependencyFileDataBuilder::BufferInfo bufferBuilder;
		bufferBuilder.m_dataSizeInMemory = buf.m_size; // the only thing we know right now is the size in memory :)

		// add to buffer table
		builder.MapBuffer( bufferBuilder );
	}

	// Use the same buffer mapping table (saves time)
	outBufferMap = Move( mapper.m_bufferIndices );
	return true;
}

Bool CDependencySaver::SaveObjects( DependencySavingContext& context )
{
#ifndef NO_RESOURCE_COOKING
	// Setup buffer extractor
	m_bufferExtractor = context.m_bufferExtractor;
#endif

	// when saving use the current version always
	m_version = VER_CURRENT;
	m_file->m_version = VER_CURRENT;

	// Resave flag
	if ( context.m_isResourceResave && m_savedFile )
	{
		m_flags |= FF_ResourceResave;
	}
	else
	{
		m_flags &= ~FF_ResourceResave;
	}

	// Filtering
	if ( context.m_filterScriptedProps )
	{
		m_flags |= FF_FilterScriptProps;
	}

	// Map object dependencies creating import and export tables
	CDependencyMapper mapper( context );
	SetCooker( mapper.IsCooker() );

	// Prepare file tables
	CDependencyFileData fileData;
	CDependencyFileDataBuilder builder( fileData );

	// Copy data from mapper to file tables
	TExportMap exportMap;
	if ( !SetupNames( mapper, builder, m_nameIndices ) )
		return false;
	if ( !SetupImports( mapper, builder, m_softDependeciesMap, m_objectIndices, context.m_hashPaths ) )
		return false;
	if ( !SetupExports( mapper, builder, m_objectIndices, exportMap ) )
		return false;
	if ( !SetupBuffers( mapper, builder, m_bufferMap ) )
		return false;
	if ( !SetupProperties( mapper, builder, m_propertyIndices ) )
		return false;

	// Write the initial header, all offsets are RELATIVE to the header, not to the file start
	const Uint64 headerOffset = GetOffset();
	fileData.Save( *m_file, headerOffset, context.m_zeroNonDeterministicData );

	// In case of a resave to the same file the buffers will have to be patched, create a patch table to store the new access for each buffer
	if ( IsResourceResave() )
	{
		m_bufferPatchTable.Resize( mapper.m_buffers.Size()+1 );
	}

	// Write exports
	if ( !WriteExports( context, mapper, exportMap, builder, headerOffset ) )
		return false;

	// Write buffers, NOTE: buffers can be extracted here
	if ( !WriteBuffers( context, mapper, builder, headerOffset ) )
		return false;

#ifndef NO_RESOURCE_COOKING
	// Report dependencies
	if ( context.m_cooker )
	{
		// report all imports
		for ( Uint32 i=0; i<mapper.m_imports.Size(); ++i )
		{
			if ( mapper.m_imports[i].m_resource )
			{
				const String path = mapper.m_imports[i].m_resource->GetDepotPath();
				context.m_cooker->ReportHardDependency( path );
			}
		}

		// report all soft imports
		for ( Uint32 i=0; i<mapper.m_softDependencies.Size(); ++i )
		{
			if ( !mapper.m_softDependencies[i].Empty() )
			{
				context.m_cooker->ReportSoftDependency( mapper.m_softDependencies[i] );
			}
		}
	}
#endif

	// Write header, now with valid data
	const Uint64 prevOffset = m_file->GetOffset();
	m_file->Seek( headerOffset );
	fileData.Save( *m_file, headerOffset, context.m_zeroNonDeterministicData );
	m_file->Seek( prevOffset );

	// Dependencies saved
	return true;
}

namespace Helper
{
	class CDependencySaverCRCCalculator
	{
	public:
		CDependencySaverCRCCalculator( IFile*& file, Uint32& crcResult )
			: m_baseFile( file )
			, m_saveFile( file )
			, m_baseFilePtr( &file )
			, m_crcResult( &crcResult )
			, m_baseOffset( file->GetOffset() )
		{
			m_memory = file->QueryDirectMemoryAccess();
			if ( !m_memory )
			{
				// create a temporary shadow file that will save the same data into a memory buffer
				// this way we can compute the CRC without the need to open the file again
				m_saveFile = new CShadowFileWriter( m_baseFile, m_baseOffset );
				m_memory = m_saveFile->QueryDirectMemoryAccess(); 
				m_memoryBuferOffset = 0;

				// substitute file
				file = m_saveFile;
			}
			else
			{
				// the original file itself is mapped to memory so the object's data is placed somewhere in the, not at the offset 0
				m_memoryBuferOffset = m_baseOffset;
			}
		}

		~CDependencySaverCRCCalculator()
		{
			// restore file pointer
			*m_baseFilePtr = m_baseFile;

			// save CRC
			*m_crcResult = ComputeDataCRC();
				
			// delete the shadow file copy, if there
			if ( m_saveFile != m_baseFile )
			{
				delete m_saveFile;
				m_saveFile = nullptr;
			}
		}

	private:
		const Uint32 ComputeDataCRC() const
		{
			RED_FATAL_ASSERT( m_memory != nullptr, "Invalid internal state" );
			RED_FATAL_ASSERT( m_baseFile->GetOffset() >= m_baseOffset, "Invalid object placement" );

			// object data is placed somewhere in the target file
			const Uint8* objectBase = m_memory->GetBufferBase() + m_memoryBuferOffset;
			const Uint32 objectSize = (Uint32)( m_baseFile->GetOffset() - m_baseOffset );
			if ( objectSize > m_memory->GetBufferSize() )
			{
				ERR_CORE( TXT("CRC calculation failed, object size=%d, buffer size=%d, fileoffset=%d, memoffset=%d, base=%d, file=%ls"),
					objectSize, m_memory->GetBufferSize(), 
					m_baseFile->GetOffset(), m_memoryBuferOffset, m_baseOffset,
					m_baseFile->GetFileNameForDebug() );
				return 0;
			}

			// compute data size
			static Red::CRC32 crcCalculator;
			return crcCalculator.Calculate( objectBase, objectSize );
		}

		TDynArray< Uint8 >			m_tempMemory;
		Uint32*						m_crcResult;
		IFileDirectMemoryAccess*	m_memory;
		Uint64						m_baseOffset;
		Uint64						m_memoryBuferOffset;
		IFile*						m_saveFile;
		IFile*						m_baseFile;
		IFile**						m_baseFilePtr;
	};
} // Helper

Bool CDependencySaver::WriteExports( DependencySavingContext &context, const CDependencyMapper &mapper, const TExportMap& exportMap, class CDependencyFileDataBuilder& builder, const Uint64 headerOffset )
{
	for ( Uint32 i=0; i<mapper.m_exports.Size(); i++ )
	{
		const auto& exp = mapper.m_exports[i];

		// Get export offset
		const Uint32 exportDataOffset = (Uint32)( GetOffset() - headerOffset );

		// Save block
		Uint32 exportCRC = 0;
		{
#ifndef RED_FINAL_BUILD
			// When saving in a normal build compute the CRC of the saved data
			Helper::CDependencySaverCRCCalculator crcCalculator( m_file, exportCRC );
#endif

			// Prepare for save
			ISerializable* serializable = exp.GetSerializablePtr();
			if ( NULL != serializable )
			{
				// Save using the serialization interface
				serializable->OnSerialize( *this );
			}
			else
			{
				// Save using general RTTI
				exp.GetClass()->Serialize( *this, exp.GetPointer() );
			}

		}

		// Calculate data size
		const Uint32 exportIndex = exportMap[i];
		const Uint32 exportDataSize = (Uint32)( GetOffset() - headerOffset ) - exportDataOffset;
		if ( exportDataSize == 0 )
		{
			ERR_CORE( TXT("Export #%d in %ls has no data, class %ls"), 
				i, m_file->GetFileNameForDebug(), exp.GetRuntimeClass()->GetName().AsChar() );
			return false;
		}

		// Patch the export data in the file tables
		builder.PatchExport( exportIndex, exportDataOffset, exportDataSize, exportCRC );
	}	

	// exports saved
	return true;
}

Bool CDependencySaver::WriteBuffers( DependencySavingContext& context, const CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, const Uint64 headerOffset )
{
	for ( Uint32 i=0; i<mapper.m_buffers.Size(); ++i )
	{
		const auto& buf = mapper.m_buffers[i];

		// Sanity checks
		RED_FATAL_ASSERT( buf.m_size > 0, "Mapped buffers should never be empty" );
		RED_FATAL_ASSERT( buf.m_handle->GetSize() > 0, "Mapped buffers should never be empty" );
		RED_FATAL_ASSERT( buf.m_handle->GetData() != nullptr, "Mapped buffers should never be empty" );
		RED_FATAL_ASSERT( buf.m_id != 0, "Mapped buffer with invalid ID" );

		// Extract the buffer if we want to
#ifndef NO_RESOURCE_COOKING
		if ( context.m_bufferExtractor )
		{
			Uint32 bufferSizeOnDisk = 0;
			Uint32 bufferDiskCRC = 0;
			String bufferExtractedPath;
			if ( context.m_bufferExtractor->Extract( buf.m_id, buf.m_size, buf.m_handle->GetData(), /*out*/ bufferSizeOnDisk, /*out*/ bufferDiskCRC, /*out*/ bufferExtractedPath ) )
			{
				// patch the buffer table with information about extracted buffer
				// basically, all of the extracted buffers have the diskOffset=0
				builder.PatchBuffer( buf.m_id, 0, bufferSizeOnDisk, bufferDiskCRC );

				// for cooking we also need to report the newly extracted buffer as a dependency so the file gets carried along
				if ( context.m_cooker )
				{
					RED_ASSERT( !bufferExtractedPath.Empty(), TXT("Buffer extracted but no file path specified - data will be lost for sure") );
					context.m_cooker->ReportSoftDependency( bufferExtractedPath );
				}

				// no more work to be done
				continue; 
			}
		}
#endif

		// TODO: in here we can EASILY compress the buffers if we want to :)
		// NOTE: CRC should be calculated from the data actually saved not from the data in memory - this way it's easier to verify that the file is OK

		// Buffer is not extracted, save it at the end of the file
		const Uint32 bufferDataOffset = (Uint32)( m_file->GetOffset() - headerOffset );

		// patch the buffer
		if ( !m_bufferPatchTable.Empty() )
		{
			if ( m_bufferPatchTable[ buf.m_id ].m_access )
			{
				IFileLatentLoadingToken* token = CreateLatentLoadingToken( bufferDataOffset );
				m_bufferPatchTable[ buf.m_id ].m_access->PatchAccess( token );
			}
		}

		// no compression case
		{
			// Save data
			void* bufferData = buf.m_handle->GetData();
			m_file->Serialize( bufferData, buf.m_size );

			// Make sure we advanced properly
			const Uint32 bufferDataEndOffset = (Uint32)( m_file->GetOffset() - headerOffset );
			if ( bufferDataEndOffset != (bufferDataOffset + buf.m_size) )
			{
				ERR_CORE( TXT("Failed to save buffer data of size %d to %ls - OUT OF DISK SPACE ?"), buf.m_size, m_file->GetFileNameForDebug() );
				return false;
			}

			// Compute CRC
			static Red::System::CRC32 crcCalculator;
			const Uint32 bufferCRC = crcCalculator.Calculate( bufferData, buf.m_size );

			// Patch up the buffer data in the file tables
			const Uint32 bufferSizeOnDisk = buf.m_size; // no compression
			builder.PatchBuffer( buf.m_id, bufferDataOffset, bufferSizeOnDisk, bufferCRC );
		}
	}

	return true;
}

void CDependencySaver::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	CPointer enginePointer( pointer, const_cast< CClass* >( pointerClass ) );
	Int32 index = MapObject( enginePointer );
	*m_file << index;
}

void CDependencySaver::SerializeName( class CName& name )
{
	Uint16 index = (Uint16)MapName( name );
	*m_file << index;
}

void CDependencySaver::SerializeSoftHandle( class BaseSoftHandle& softHandle )
{
	Uint16 index = (Uint16)MapSoftDependency( softHandle );
	*m_file << index;
}

void CDependencySaver::SerializeTypeReference( class IRTTIType*& type )
{
	Uint16 index = type ? (Uint16)MapName( type->GetName() ) : 0;
	*m_file << index;
}

void CDependencySaver::SerializePropertyReference( const class CProperty*& prop )
{
	Uint16 index = (Uint16) MapProperty( prop );
	*m_file << index;
}

void CDependencySaver::SerializeDeferredDataBuffer( DeferredDataBuffer & buffer )
{
	Uint16 index = (Uint16)MapDeferredDataBuffer( buffer );
	*m_file << index;

	// we are resaving the buffer - update the access 
	// the problem is that we don't know the final offset yet, so create a access that will patch later
	if ( IsResourceResave() && index )
	{
		RED_FATAL_ASSERT( index < m_bufferPatchTable.Size(), "Buffer patch table not created or invalid size" );
		RED_FATAL_ASSERT( m_bufferPatchTable[ index ].m_access == nullptr, "Buffer with ID %d already mapped in %ls", index, m_file->GetFileNameForDebug() );
		m_bufferPatchTable[ index ].m_access = buffer.SetAccessFromLatentTokenForLaterPatching();
		RED_ASSERT( m_bufferPatchTable[ index ].m_access != nullptr, TXT("Buffer with ID %d could not create patch acccess in %ls"), index, m_file->GetFileNameForDebug() );
	}
}

Int32 CDependencySaver::MapObject( const CPointer& pointer )
{
	// Null object
	if ( pointer.IsNull() )
	{
		return 0;
	}

	// Use object mapping to index objects
	Int32 objectIndex = 0;
	if ( m_objectIndices.Find( pointer.GetPointer(), objectIndex ) )
	{
		return objectIndex;
	}

	// Not mapped
	RED_HALT( "Unmapped object" );
	return 0;
}

Int32 CDependencySaver::MapName( const CName& name )
{
	// Empty name
	if ( !name )
	{
		return 0;
	}

	// Use name mapping to index objects
	Int32 nameIndex = 0;
	if ( m_nameIndices.Find( name, nameIndex ) )
	{
		return nameIndex;
	}

	// Not mapped
	RED_HALT( "Unmapped name '%ls'", name.AsString().AsChar() );
	return 0;
}

Int32 CDependencySaver::MapProperty( const CProperty* prop )
{
	// No property or property not from class
	if ( !prop || !prop->GetParent() )
		return 0;

	// Use the existing property mapping
	Int32 propertyIndex = 0;
	if ( m_propertyIndices.Find( prop, propertyIndex ) )
	{
		return propertyIndex;
	}

	// Not mapped
	RED_HALT( "Unmapped property '%ls' in '%ls'", prop->GetName().AsChar(), prop->GetParent()->GetName().AsChar() );
	return 0;
}

Int32 CDependencySaver::MapSoftDependency( const class BaseSoftHandle& handle )
{
	// Empty soft dependency
	if ( handle.GetPath().Empty() )
		return 0;

	// Fast cache
	const String lowerPath = handle.GetPath().ToLower();
	const Int32* existingIndex = m_softDependeciesMap.FindPtr( lowerPath );
	if ( existingIndex != nullptr )
	{
		return (*existingIndex + 1); // 0 has special meaning
	}

	// Not mapped
	RED_HALT( "Unmapped soft dependency to '%ls'", handle.GetPath().AsChar() );
	return 0;
}

Int32 CDependencySaver::MapDeferredDataBuffer( DeferredDataBuffer& buffer )
{
	// empty buffers always map to zero
	if ( !buffer.GetSize() )
		return 0;

	// map by data pointer - we assume it's not reallocated
	const Int32 bufferKey = buffer.GetSaveID();
	if ( bufferKey != -1 )
	{
		// find buffer ID
		Int32 mappedIndex = 0;
		if ( m_bufferMap.Find( bufferKey, mappedIndex ) )
			return mappedIndex;
	}

	// Not mapped
	RED_HALT( "Unmapped defered data buffer of size %d in '%ls' (key=%d)", buffer.GetSize(), m_file->GetFileNameForDebug(), bufferKey );
	return 0;
}

IFileDirectMemoryAccess* CDependencySaver::QueryDirectMemoryAccess()
{
	return m_file->QueryDirectMemoryAccess();
}
