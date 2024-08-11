/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "dynarray.h"
#include "pointer.h"
#include "hashmap.h"
#include "hashmap.h"
#include "names.h"
#include "deferredDataBuffer.h"

class ICookerFramework;
class CResource;
class DeferredDataBufferExtractor;
class CSpeechCollector;

// Cooking platform
enum ECookingPlatform : Int32
{
	PLATFORM_None,			//!< No cooking
	PLATFORM_Null,			//!< Null cooking (special case - no files output - just gather asset dependencies)
	PLATFORM_Resave,		//!< Resave cooking (special case - resave file - no cooking)
	PLATFORM_PC,			//!< PC cooking
	PLATFORM_XboxOne,		//!< XboxOne cooking
	PLATFORM_PS4,			//!< PS4 cooking
};

/******************************/
/* Resource mapping context   */
/******************************/
class DependencyMappingContext
{
public:
	typedef TDynArray< ISerializable* >	TSerializablesToMap;
	typedef TDynArray< CObject* >	TObjectsToMap;

	TSerializablesToMap			m_initialExports;		//!< Roots of the object trees to save
	Bool						m_saveTransient;		//!< Save event transient objects
	Bool						m_saveReferenced;		//!< Save referenced objects
	Bool						m_isResourceResave;		//!< We are resaving existing resource to the same file
	Bool						m_filterScriptedProps;	//!< Filter out non-editable scripted properties

#ifndef NO_RESOURCE_COOKING
	ICookerFramework*				m_cooker;			//!< Resource cooker framework - cooking only
	DeferredDataBufferExtractor*	m_bufferExtractor;	//!< Buffer extraction helper - cooking only
	CSpeechCollector*				m_speechCollector;	//!< Speeach collector helper - cooking only
#endif

public:
	// If you are saving just one object use this constructor
	DependencyMappingContext( ISerializable* object );

	// If you want more than one object save use this constructor
	DependencyMappingContext( const TObjectsToMap& object );

	// Generic interface accepting pointer to ISerializable objects
	DependencyMappingContext( const TSerializablesToMap& object );
};

/****************************/
/* Dependency mapper		*/
/****************************/
class CDependencyMapper : public IFile
{
public:
	struct ImportInfo
	{
		CResource*		m_resource;					//!< Imported resource
		Bool			m_fromBaseResGroup;			//!< Imported from base resource group
		Bool			m_usedAsTemplate;			//!< Resource used as template

		//! Default constructor
		RED_FORCE_INLINE ImportInfo()
			: m_resource( NULL )
			, m_fromBaseResGroup( false )
			, m_usedAsTemplate( false )
		{};

		//! Constructor
		RED_FORCE_INLINE ImportInfo( CResource* resource, Bool usedAsTemplate, Bool fromBaseResGroup )
			: m_resource( resource )
			, m_fromBaseResGroup( fromBaseResGroup )
			, m_usedAsTemplate( usedAsTemplate )
		{};
	};

	struct BufferInfo
	{
		BufferHandle	m_handle;	//!< Buffer's data
		Int32			m_saveID;	//!< Buffer's save ID
		Uint32			m_size;		//!< Buffer's size
		Uint32			m_id;		//!< Buffer ID

		RED_FORCE_INLINE BufferInfo()
			: m_id(0) // invalid id
			, m_size(0) // invalid size
		{}
	};

public:
	typedef TDynArray< CPointer, MC_Linker >								TExportTable;
	typedef TDynArray< ImportInfo, MC_Linker >								TImportTable;
	typedef TDynArray< CName, MC_Linker >									TNameTable;
	typedef TDynArray< String, MC_Linker >									TSoftDepsTable;
	typedef TDynArray< const CProperty*, MC_Linker >						TPropertyTable;
	typedef TDynArray< BufferInfo, MC_Linker >								TBufferTable;

	typedef THashMap< void*, Int32, DefaultHashFunc<void*>, DefaultEqualFunc<void*>, MC_Linker >	TPointerMap;
	typedef THashMap< CName, Int32, DefaultHashFunc<CName>, DefaultEqualFunc<CName>, MC_Linker >	TNameMap;
	typedef THashMap< String, Int32, DefaultHashFunc<String>, DefaultEqualFunc<String>, MC_Linker >	TSoftDepsMap;
	typedef THashMap< Int32, Int32, DefaultHashFunc<Int32>, DefaultEqualFunc<Int32>, MC_Linker >	TBufferMap;
	typedef THashMap< const CProperty*, Int32, DefaultHashFunc<const CProperty*>, DefaultEqualFunc<const CProperty*>, MC_Linker >	TPropertyMap;
		
	TExportTable	m_exports;				// List of contained object
	TImportTable	m_imports;				// List of used external resources
	TNameTable		m_names;				// List of used names
	TPropertyTable	m_properties;			// List of used properties
	TBufferTable	m_buffers;				// List of used buffers
	TPointerMap		m_objectIndices;		// Object remapping indices
	TNameMap		m_nameIndices;			// Name remapping indices
	TBufferMap		m_bufferIndices;		// Buffer remapping indices
	TPropertyMap	m_propertyIndices;		// Property remapping indices
	TSoftDepsTable	m_softDependencies;		// Soft resource dependencies
	TSoftDepsMap	m_softDependenciesMap;  // Map of soft dependencies

	TExportTable	m_tempExports;			// Unsorted list of contained object
	TPointerMap		m_tempObjectIndices;	// Unsorted remapping indices

	// Settings
	DependencyMappingContext&				m_context;

public:
	// Map dependencies of given objects
	CDependencyMapper( DependencyMappingContext& context );

	// Map dependency on given object to either being an internal export or imported external resource
	Int32 MapObject( const CPointer& object, Bool usedAsTemplate );

	// Map name
	Int32 MapName( const CName& name );

	// Map soft dependency
	Int32 MapSoftDependency( const String& path );

	// Map a data buffer
	Int32 MapDataBuffer( DeferredDataBuffer& dataBuffer );

	// Determine if we should map given object at all
	Bool ShouldMapObject( const CPointer& object );

	// Determine if we should import or export given object
	Bool ShouldExportObject( const CPointer& object );

	// Map final object, creates sorted export table from unsorted one
	Int32 MapFinalObject( const CPointer& object );

	// Map property to index
	Int32 MapProperty( const CProperty* prop );

protected:
	// Unimplemented IFile interface
	virtual void Serialize( void*, size_t ) {}
	virtual Uint64 GetOffset() const { return 0; }
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {}

	// IFile magic mapping interface
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer ) override;
	virtual void SerializeName( class CName& name ) override;
	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle ) override;
	virtual void SerializeTypeReference( class IRTTIType*& type ) override;
	virtual void SerializeDeferredDataBuffer( DeferredDataBuffer & buffer ) override;
	virtual void SerializePropertyReference( const class CProperty*& prop ) override;

#ifndef NO_RESOURCE_COOKING
	// IFile magic interface
	virtual CSpeechCollector* QuerySpeechCollector( ) const override { return m_context.m_speechCollector; }
#endif

private:
	CDependencyMapper( const CDependencyMapper& other );
	CDependencyMapper& operator=( const CDependencyMapper& ) { return *this; };
};
