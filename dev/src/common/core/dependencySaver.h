/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dependencyMapper.h"
#include "dynarray.h"
#include "string.h"
#include "resourcePath.h"
#include "dependencyLinker.h"
#include "diskFile.h"
#include "deferredDataBuffer.h"

/******************************/
/* Dependency saver context   */
/******************************/
/// Resource saving context
class DependencySavingContext : public DependencyMappingContext
{
public:
	Bool						m_dumpStats;				//!< Dump file stats after saving file
	Bool						m_useFeedback;				//!< Use feedback to show progress during lengty operation
	Bool						m_hashPaths;				//!< While saving hash the paths instead of storing them fully
	Bool						m_zeroNonDeterministicData;	//!< Don't save the time stamp or any other build-varying stuff
	String						m_taskName;					//!< Name of the operation (for feedback)

public:
	// Constructor for a case when you are saving only one object
	DependencySavingContext( ISerializable* objectToSave );

	// Constructor for a case when you are saving more than one object
	DependencySavingContext( const TObjectsToMap& objectsToSave );

	// Constructor for a case when you are saving more than one object
	DependencySavingContext( const TSerializablesToMap& objectsToSave );
};

/******************************/
/* Dependency saver interface */
/******************************/
class IDependencySaver : public IDependencyLinker
{
	friend class CDependencyLinkerFactory;

public:
	IDependencySaver( IFile& file ) : IDependencyLinker( file ) {}
	virtual ~IDependencySaver() {}

	// Save objects, returns true on success
	virtual Bool SaveObjects( DependencySavingContext& context ) = 0;
};

/********************/
/* Dependency saver */
/********************/

class CDependencySaver : public IDependencySaver
{
	friend class CDependencyLinkerFactory;
	friend class CEdBehaviorGraphEditor; // this is a hack way but i don't want to expose the guts of linker to anyone, this way it's limited

public:
	CDependencySaver( IFile& file, const CDiskFile* fileBeingSaved = nullptr );
	virtual ~CDependencySaver();

	// Save objects, returns true on success
	virtual Bool SaveObjects( DependencySavingContext& context );

public:
	// IFile magic mapping interface
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer ) override;
	virtual void SerializeName( class CName& name ) override;  
	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle ) override;
	virtual void SerializeTypeReference( class IRTTIType*& type ) override;
	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer & buffer ) override;
	virtual void SerializePropertyReference( const class CProperty*& prop ) override;

	// Create latent loading token for current file position
	virtual IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 currentOffset ) override;

	// Create direct memory access to the file being saved
	virtual IFileDirectMemoryAccess* QueryDirectMemoryAccess() override;

private:
	struct DeferredBufferToPatch
	{
		Red::TAtomicSharedPtr< DeferredDataAccessPatchable >		m_access;
	};

	typedef TDynArray< DeferredBufferToPatch, MC_Linker >					TBuffersToPatch;
	typedef THashMap< void*, Int32, DefaultHashFunc<void*>, DefaultEqualFunc<void*>, MC_Linker >	TPointerMap;
	typedef THashMap< CName, Int32, DefaultHashFunc<CName>, DefaultEqualFunc<CName>, MC_Linker >	TNameMap;
	typedef THashMap< const CProperty*, Int32, DefaultHashFunc<const CProperty*>, DefaultEqualFunc<const CProperty*>, MC_Linker >	TPropertyMap;
	typedef THashMap< String, Int32, DefaultHashFunc<String>, DefaultEqualFunc<String>, MC_Linker >	TSoftDependencyMap;
	typedef TDynArray< Uint32, MC_Linker >									TExportMap;
	typedef THashMap< Int32, Int32, DefaultHashFunc<Int32>, DefaultEqualFunc<Int32>, MC_Linker >	TBufferMap;

	const CDiskFile*				m_savedFile;			// File being saved
#ifndef NO_RESOURCE_COOKING
	DeferredDataBufferExtractor*	m_bufferExtractor;		// Buffer extractor - allows to move the buffers outside of the file
#endif

	TPointerMap			m_objectIndices;			// Mapped object indices
	TNameMap			m_nameIndices;				// Mapped name indices
	TPropertyMap		m_propertyIndices;			// Mapped property indices
	TSoftDependencyMap	m_softDependeciesMap;		// Soft dependencies of file (map)
	TBufferMap			m_bufferMap;				// Mapped buffers to save
	TBuffersToPatch		m_bufferPatchTable;			// Patching table for buffers

	static Bool SetupNames( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TNameMap& outNames );
	static Bool SetupImports( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TSoftDependencyMap& outSoftDependencies, TPointerMap& outObjectMap, const Bool hashPaths );
	static Bool SetupExports( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TPointerMap& outObjectMap, TExportMap& outExportMap );
	static Bool SetupBuffers( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TBufferMap& outBufferMap );
	static Bool SetupProperties( CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, TPropertyMap& outProperties );

	Bool WriteExports( DependencySavingContext& context, const CDependencyMapper &mapper, const TExportMap& exportMap, class CDependencyFileDataBuilder& builder, const Uint64 headerOffset );
	Bool WriteBuffers( DependencySavingContext& context, const CDependencyMapper &mapper, class CDependencyFileDataBuilder& builder, const Uint64 headerOffset );

	Int32 MapObject( const CPointer& pointer );
	Int32 MapName( const CName& name );
	Int32 MapSoftDependency( const class BaseSoftHandle& handle );
	Int32 MapDeferredDataBuffer( DeferredDataBuffer& buffer );
	Int32 MapProperty( const CProperty* prop );
};
