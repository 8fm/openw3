/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "dependencyLinker.h"
#include "dependencyFileTables.h"
#include "scopedPtr.h"
#include "diskFile.h"
#include "fileLoadingStats.h"

class CDependencyFileData;

/// Dependency between files
class FileDependency
{
public:
	String		m_depotPath;			//!< Path to the resource
	CName		m_resourceClass;		//!< Class of the resource
	Bool		m_isSoftDepdencency;	//!< Soft handle dependency

	//! Default constructor
	RED_INLINE FileDependency()
		: m_isSoftDepdencency( false )
	{};

	//! Initialize from direct string
	FileDependency( CName resourceClass, const String& depotFilePath, const Bool isSoft = false );
};

/// Premapped dependency data (to speed up loading of the tables)
class DependencyLoadingPrecachedTables
{
public:
	TDynArray< CName, MC_Linker >				m_mappedNames;			// Mapped names (runtime data only)
	TDynArray< IRTTIType*, MC_Linker >			m_mappedTypes;			// Mapped RTTI types (runtime data only)
	TDynArray< const CProperty*, MC_Linker >	m_mappedProperties;		// Mapped file properties (runtime data only)
	TDynArray< Int32, MC_Linker >				m_mappedPropertyOffsets;// Mapped RTTI properties offsets (runtime data only)
	TDynArray< CDiskFile*, MC_Linker >			m_mappedImports;		// Mapped imported resources

public:
	DependencyLoadingPrecachedTables();
};

/// Callback interface used to load imports of the loaded object
class IDependencyImportLoader
{
public:
	virtual ~IDependencyImportLoader() {};

	/// Load file as dependency
	virtual BaseSafeHandle LoadDependency( const CClass* resourceClass, const AnsiChar* path, const CDependencyFileData::Import& importData, const Bool silent ) = 0;

	// Set default import loader (use with care), returns current one
	static IDependencyImportLoader* SetDefault( IDependencyImportLoader* newImportLoader );

	// Get default import loader (that loads stuff through the GDepot)
	static IDependencyImportLoader* GetDefault();
};

/// Dependency loading context
class DependencyLoadingContext
{
public:
	typedef TDynArray< THandle< CResource > >	TLoadedResources;
	typedef TDynArray< CObject* >				TLoadedRoots;
	typedef TDynArray< CPointer >				TLoadedObjects;
	typedef DependencyLoadingPrecachedTables	TTables;

	ISerializable*				m_parent;					//!< Parent root to attach all loaded objects to
	TLoadedRoots				m_loadedRootObjects;		//!< Root objects loaded from file (not a ISerializable ones)
	TLoadedObjects				m_loadedObjects;			//!< List of all loaded objects
	TLoadedResources			m_loadedResources;			//!< Loaded root resources
	Bool						m_isAsyncLoader;			//!< We are loading in async loader ( other thread )
	Bool						m_getAllLoadedObjects;		//!< Get all loaded objects regardless of everything (good for copy&pase and editor stuff)
	Bool						m_doNotLoadImports;			//!< Do not load imported objects, very fast, WARNING: do not save objects loaded like that
	Bool						m_validateHeader;			//!< Do header validation - NOTE - loading will fail if header validation fails
	Bool						m_loadSoftDependencies;		//!< Load also all of the soft dependenecies of this object
	CFileLoadingStats*			m_stats;					//!< Loading stats
	CObject*					m_firstExportMemory;		//!< Preallocated memory (always at least CObject*) to be used for first export
	Uint32						m_firstExportMemorySize;	//!< Size of the preallocated memory to be used for first export
	CClass*						m_rootClassOverride;		//!< Custom override for the root class
	const TTables*				m_precachedTables;			//!< Precached tables - to speed up data deserialization
	IDependencyImportLoader*	m_importLoader;				//!< Loader interface to chain loading any imported dependencies

public:
	DependencyLoadingContext();
};

/*******************************/
/* Dependency loader interface */
/*******************************/
class IDependencyLoader : public IDependencyLinker
{
	friend class CDependencyLinkerFactory;

protected:
	const CDiskFile*				m_loadedFile;			// Resource file being loaded

public:
	IDependencyLoader( IFile& file, const CDiskFile* loadedFile ) : IDependencyLinker( file ), m_loadedFile( loadedFile ) {}
	virtual ~IDependencyLoader() {}

	// Get root class of the resource (with minimal loading)
	virtual const CClass* GetRootClass() { return nullptr; }

	// Deserialize objects with dependencies
	virtual Bool LoadObjects( DependencyLoadingContext& context ) = 0;

	// Load the list of dependencies of this file
	// If you include soft handles in the list of dependencies returned from this function, please be aware
	// that you might stumble upon a circular dependency (which will cause infinite recursion if you're also
	// calling this function for all the returned dependencies as well)
	virtual Bool LoadDependencies( TDynArray< FileDependency >& dependencies, Bool includeSoftHandles = false ) = 0;

	// Initialize loaded objects by calling OnPostLoad on them
	virtual Bool PostLoad() { return false; }
};

/*********************/
/* Dependency loader */
/*********************/
class CDependencyLoader : public IDependencyLoader, public IFileDirectMemoryAccess
{
	friend class CDependencyLinkerFactory;
	friend class CEdBehaviorGraphEditor; // this is a hack way but i don't want to expose the guts of linker to anyone, this way it's limited

public:
	CDependencyLoader( IFile& file, const CDiskFile* loadedFile );
	~CDependencyLoader();

	//! Validate exports to find missing templates used on layers
	Bool SearchForExportsMissingTemplates( TDynArray< String >& missingFiles );

	//! Precache data for faster deserialization
	Bool PrecacheResolvedTables( DependencyLoadingPrecachedTables& outTables );

	// Deserialize objects with dependencies
	virtual Bool LoadObjects( DependencyLoadingContext& context );

	// Initialize loaded objects by calling OnPostLoad on them
	virtual Bool PostLoad();

	//! Load file tables
	virtual Bool LoadTables();

	//! Load the list of dependencies of this file
	virtual Bool LoadDependencies( TDynArray< FileDependency >& dependencies, Bool includeSoftHandles );

	// Get root class of the resource (with minimal loading)
	virtual const CClass* GetRootClass();

	// Get file tables
	RED_INLINE const CDependencyFileData* GetFileTables() const { return m_data.Get(); }

public:
	//! Version & Magic
	static const Uint32 FILE_VERSION;
	static const Uint32 FILE_MAGIC;

	//! Set version of file
	void SetVersion( Uint32 version );

	// Dependency serialization interface
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer ) override;
	virtual void SerializeName( class CName& name ) override;
	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle ) override;
	virtual void SerializeTypeReference( class IRTTIType*& type ) override;
	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer& buffer ) override;
	virtual void SerializePropertyReference( const class CProperty*& prop ) override;

private:
	DependencyLoadingContext*					m_context;				// Loading context
	Red::TScopedPtr< CDependencyFileData >		m_data;					// File tables

	struct ResolvedImport
	{
		CClass*						m_class;
		const AnsiChar*				m_path;
		BaseSafeHandle				m_resource;
	};

	struct ResolvedExport
	{
		CClass*						m_class;
		BaseSafeHandle				m_object;
		void*						m_rawPointer;
		Bool						m_skip;
	};

	TDynArray< CName, MC_Linker >				m_mappedNames;			// Mapped names (runtime data only)
	TDynArray< IRTTIType*, MC_Linker >			m_mappedTypes;			// Mapped RTTI types (runtime data only)
	TDynArray< const CProperty*, MC_Linker >	m_mappedProperties;		// Mapped file properties (runtime data only)
	TDynArray< Int32, MC_Linker >				m_mappedPropertyOffsets;// Mapped RTTI properties offsets (runtime data only)
	TDynArray< ResolvedImport, MC_Linker >		m_mappedImports;		// Mapped file imports (runtime data only)
	TDynArray< ResolvedExport, MC_Linker >		m_mappedExports;		// Mapped file exports (runtime data only)

	// Inplace loading
	CFileDirectSerializationTables				m_fileTables;			// Mapped file tables (for data stream deserialiation)
	const Uint8*								m_fileMappedMemory;		// Mapped file memory
	Uint32										m_fileMappedSize;		// Size of mapped file memory

	//! Map loaded data to runtime data
	Bool MapRuntimeData();

	//! Load data from inplace files
	Bool LoadInplaceFiles();

	//! Load all included files
	Bool CreateImports();

	//! Create all objects from this file
	Bool CreateExports();

	//! Load all objects
	Bool LoadExports( Uint64 baseOffset );	

	//! Map index to a name
	CName MapName( Uint16 index ) const;

	//! Map index to a type
	IRTTIType* MapType( Uint16 index );

	//! Map index to a property
	const CProperty* MapProperty( Uint16 index ) const;

	//! Map index to object
	CPointer MapObject( Int32 index );

	//! Map index to a pointer
	void* MapPointer( Int32 index, const class CClass* pointerClass );

	//! Map index to a soft handle
	BaseSoftHandle MapSoftHandle( Uint16 index );

	// private part of IFile interface - should not be directly accessible
	virtual IFileLatentLoadingToken* CreateLatentLoadingToken( Uint64 currentOffset );
	virtual CClass* ChangeExportClass( CClass* exportClass, const DependencyLoadingContext* context ) { return exportClass; }

	// interface for IFileDirectMemoryAccess - needed for data stream deserialization
	virtual Uint8* GetBufferBase() const override;
	virtual Uint32 GetBufferSize() const override;

	// inplace loading bullshit :( :( :( I cried writing this
	virtual const class CFileDirectSerializationTables* QuerySerializationTables() const override;
	virtual class IFileDirectMemoryAccess* QueryDirectMemoryAccess() override { return static_cast< IFileDirectMemoryAccess* >( this ); }

	// Generate file for deferred data buffer extracted from this file with given ID
	void GenerateBufferFileName( const Uint32 bufferId, String& outBufferPath ) const;

	//
	Red::Core::Bundle::FileID FindBufferFileID( const Uint32 bufferId ) const;

	// legacy handling for deferred data buffers
	void LoadLegacyDeferedDataBuffer( DeferredDataBuffer& buffer ) const;
};
