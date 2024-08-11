/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "dependencyMapper.h"
#include "object.h"
#include "resource.h"
#include "cooker.h"
#include "deferredDataBufferExtractor.h"

#define TO_EXPORT_INDEX( x ) ( ((Int32)x)+1 )
#define TO_IMPORT_INDEX( x ) ( -(((Int32)x) + 1 ) )
#define FROM_EXPORT_INDEX( x ) ( ((Int32)x) - 1 )
#define FROM_IMPORT_INDEX( x ) ( -((Int32)x) - 1 )

DependencyMappingContext::DependencyMappingContext( const DependencyMappingContext::TObjectsToMap& objects )
	: m_saveTransient( false )
	, m_saveReferenced( false )
	, m_isResourceResave( false )
	, m_initialExports( objects.Size() )
	, m_filterScriptedProps( false )
#ifndef NO_RESOURCE_COOKING
	, m_cooker( nullptr )
	, m_bufferExtractor( nullptr )
	, m_speechCollector( nullptr )
#endif
{
	for ( Uint32 i=0; i<objects.Size(); ++i )
	{
		m_initialExports[i] = objects[i];
	}
}

DependencyMappingContext::DependencyMappingContext( const DependencyMappingContext::TSerializablesToMap& objects )
	: m_saveTransient( false )
	, m_saveReferenced( false )
	, m_isResourceResave( false )
	, m_initialExports( objects )
	, m_filterScriptedProps( false )
#ifndef NO_RESOURCE_COOKING
	, m_cooker( nullptr )
	, m_bufferExtractor( nullptr )
	, m_speechCollector( nullptr )
#endif
{}

DependencyMappingContext::DependencyMappingContext( ISerializable* object )
	: m_saveTransient( false )
	, m_saveReferenced( false )
	, m_isResourceResave( false )
	, m_filterScriptedProps( false )
#ifndef NO_RESOURCE_COOKING
	, m_cooker( NULL )
	, m_bufferExtractor( nullptr )
	, m_speechCollector( nullptr )
#endif
{
	ASSERT( object != NULL );

	if ( NULL != object )
	{
		m_initialExports.Resize( 1 );
		m_initialExports[0] = object;
	}
}

CDependencyMapper::CDependencyMapper( const CDependencyMapper& other )
	: IFile( 0 )
	, m_context( other.m_context )
{
}

CDependencyMapper::CDependencyMapper( DependencyMappingContext& context )
	: IFile( FF_Mapper | FF_FileBased | FF_Writer | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
	, m_context( context )
{
	// Cooking ?
	// TODO: we should try to remove this flag if possible, right now it's used only for Curve object AFAIK, the rest of the system is using OnCook
#ifndef NO_RESOURCE_COOKING
	if ( context.m_cooker )
	{
		m_flags |= FF_Cooker;
	}
#endif

	// Filtering
	if ( context.m_filterScriptedProps )
	{
		m_flags |= FF_FilterScriptProps;
	}

	// Map empty name
	CName emptyName;
	MapName( emptyName );

	// Map empty soft dependency
	MapSoftDependency( String::EMPTY );

	// Map empty property
	m_properties.PushBack( 0 );

	// Map dependencies of initial objects as forced exports
	const Uint32 numInitialObjects = m_context.m_initialExports.Size();
	for ( Uint32 i=0; i<numInitialObjects; i++ )
	{
		ISerializable* object = m_context.m_initialExports[i];

#ifdef RED_ASSERTS_ENABLED
		if ( object->IsA< CObject >() )
		{
			const CObject* fullObject = static_cast< const CObject* >( object );
			ASSERT( !fullObject->HasFlag( OF_Discarded ) );
			if ( !context.m_saveTransient )
			{
				RED_ASSERT( !fullObject->HasFlag( OF_Transient ), TXT("One of the top level objects you specified for saving is transient and the m_saveTransient is not enabled.") );
			}
		}
#endif

		MapObject( object, false );
	}

	// Sort mapping tables
	for ( Uint32 i=0; i<m_tempExports.Size(); i++ )
	{
		MapFinalObject( m_tempExports[i] );
	}

	// Sanity check
	ASSERT( m_tempExports.Size() == m_exports.Size() );
	ASSERT( m_objectIndices.Size() == m_tempObjectIndices.Size() );
}

Int32 CDependencyMapper::MapFinalObject( const CPointer& object )
{
	// NULL pointer
	if ( object.IsNull() )
	{
		return 0;
	}

	// Do not map already mapped objects, this also handles imports
	Int32 objectIndex = 0;
	if ( m_objectIndices.Find( object.GetPointer(), objectIndex ) )
	{
		return objectIndex;
	}

	// Make sure object was initially mapped
	Int32 unsortedObjectIndex = 0;
	if ( !m_tempObjectIndices.Find( object.GetPointer(), unsortedObjectIndex ) )
	{
		HALT(  "Unmapped object reached when sorting dependencies" );
		return 0;
	}

	// ISerializable parent
	if ( object.IsSerializable() )
	{
		ISerializable* serializableObject = object.GetSerializablePtr();

		ISerializable* serializableParent = serializableObject->GetSerializationParent();
		if ( serializableParent != nullptr && serializableParent != ISerializable::ANY_PARENT )
		{
			MapFinalObject( serializableParent );
		}
	}

	// CObject specific stuff - template
	if ( object.IsObject() )
	{
		CObject* realObject = object.GetObjectPtr();
		MapFinalObject( realObject->GetTemplate() );
	}

	// Add to final table
	objectIndex = TO_EXPORT_INDEX( m_exports.Size() );
	m_exports.PushBack( object );

	// Map it to an index
	m_objectIndices.Insert( object.GetPointer(), objectIndex );
	return objectIndex;
}

Int32 CDependencyMapper::MapName( const CName& name )
{
	// Do not map already mapped objects
	Int32 nameIndex = 0;
	if ( m_nameIndices.Find( name, nameIndex ) )
	{
		return nameIndex;
	}

	// Map
	nameIndex = m_names.Size();
	m_names.PushBack( name );

	// Map it to an index
	m_nameIndices.Insert( name, nameIndex );
#ifndef NO_ASSERTS
	Int32 foundNameIndex = 0;
	ASSERT( m_nameIndices.Find( name, foundNameIndex ) );
	ASSERT( foundNameIndex == nameIndex );
#endif
	return nameIndex;
}

Int32 CDependencyMapper::MapProperty( const CProperty* prop )
{
	// do not map properties that are not from classes
	if ( !prop || !prop->GetParent() )
		return 0;

	// already mapped
	const Int32* existingIndex = m_propertyIndices.FindPtr( prop );
	if ( existingIndex != nullptr )
	{
		ASSERT( m_properties[ *existingIndex ] == prop );
		return *existingIndex;
	}

	// map the names
	MapName( prop->GetParent()->GetName() );
	MapName( prop->GetName() );

	// request index
	const Int32 propIndex = m_properties.Size();
	m_properties.PushBack( prop );
	m_propertyIndices.Insert( prop, propIndex );
	return propIndex;
}

Int32 CDependencyMapper::MapSoftDependency( const String& resourcePath )
{
	// already seen ?
	const String lowerPath = resourcePath.ToLower();
	const Int32* existingIndex = m_softDependenciesMap.FindPtr( lowerPath );
	if ( existingIndex != nullptr )
	{
		ASSERT( m_softDependencies[ *existingIndex ] == lowerPath );
		return *existingIndex;
	}

	// Add to map
	const Int32 index = m_softDependencies.Size();
	m_softDependencies.PushBack( lowerPath );
	m_softDependenciesMap[ lowerPath ] = index;
	return index;
}

namespace Helper
{
	static Red::Threads::AtomicOps::TAtomic32 GBufferSaveID = 1;

	RED_FORCE_INLINE static const Int32 AllocSaveID()
	{
		return Red::Threads::AtomicOps::Increment32(&GBufferSaveID);
	}
}

Int32 CDependencyMapper::MapDataBuffer( DeferredDataBuffer& dataBuffer )
{
	// load the buffer when initially mapped
	BufferHandle handle = dataBuffer.AcquireBufferHandleSync();
	if ( !handle || !handle->GetSize() )
		return 0; // empty buffer

	// allocate ID
	Int32 bufferKey = dataBuffer.GetSaveID();
	if ( bufferKey == -1 )
	{
		bufferKey = Helper::AllocSaveID();
		dataBuffer.SetSaveID( bufferKey );
	}

	// already mapped ?
	Int32 bufferIndex = -1;
	if ( m_bufferIndices.Find( bufferKey, bufferIndex ) )
	{
		RED_HALT( "Deffered data buffer is already mapped - do you try to save the SAME buffer twice ?" );
		return bufferIndex;
	}

	// add to table of buffers
	BufferInfo info;
	info.m_handle = handle;
	info.m_saveID = bufferKey;
	info.m_id = m_buffers.Size() + 1;
	info.m_size = handle->GetSize();
	m_buffers.PushBack( info );
	m_bufferIndices.Insert( bufferKey, info.m_id );

	// return mapped index
	return info.m_id;
}

Int32 CDependencyMapper::MapObject( const CPointer& pointer, Bool usedAsTemplate )
{
	// Some objects should not be mapped
	if ( !ShouldMapObject( pointer ) )
	{
		// Map it to NULL
		m_objectIndices.Insert( pointer.GetPointer(), 0 );
		m_tempObjectIndices.Insert( pointer.GetPointer(), 0 );
		return 0;
	}

	// Do not map objects that are already mapped
	Int32 objectIndex = 0;
	if ( m_tempObjectIndices.Find( pointer.GetPointer(), objectIndex ) )
	{
		// Well, it's an import, mark it as used as template if needed
		if ( usedAsTemplate && objectIndex < 0 )
		{
			ImportInfo& info = m_imports[ FROM_IMPORT_INDEX( objectIndex ) ];
			info.m_usedAsTemplate = true;
		}

		// Return index
		return objectIndex;
	}

	// Add to tables
	if ( ShouldExportObject( pointer ) )
	{
		// Add object to export table
		objectIndex = TO_EXPORT_INDEX( m_tempExports.Size() );
		m_tempExports.PushBack( pointer );

		// Map it to an index
		m_tempObjectIndices.Insert( pointer.GetPointer(), objectIndex );

		// Map class name
		MapName( pointer.GetRuntimeClass()->GetName() );

		// Serializable stuff
		if ( pointer.IsSerializable() )
		{
			ISerializable* serializable = pointer.GetSerializablePtr();
			serializable->OnPreDependencyMap( *this );

			// Map parent object, prevents missing spots and keeps the export list parent<child ordered
			ISerializable* serializableParent = serializable->GetSerializationParent();
			if ( serializableParent != nullptr && serializableParent != ISerializable::ANY_PARENT )
			{
				MapObject( serializableParent, false );
			}

			// Special CObject only stuff
			if ( pointer.IsObject() )
			{
				// Map template
				CObject* realObject = static_cast< CObject* >( serializable );
				MapObject( realObject->GetTemplate(), true );

#ifndef NO_RESOURCE_COOKING
				// Cooking magic
				if ( m_context.m_cooker )
				{
					if ( !realObject->HasFlag( OF_WasCooked ) )
					{
						CTimeCounter timer;

						realObject->OnCook( *m_context.m_cooker );
						realObject->SetFlag( OF_WasCooked );

						// HACK: make sure the Cleanup function is called after cooking!
						CResource* res = Cast< CResource >( realObject );
						if ( res )
							res->CleanupSourceData();

						// report cooking time for cooked object
						m_context.m_cooker->ReportCookingTime( realObject->GetClass(), timer.GetTimePeriod() );
					}
				}
#endif
			}

			// Notify object that it's going to be saved
			serializable->OnPreSave();

			// Serialize (recursive)
			serializable->OnSerialize( *this );
		}
		else
		{
			// Non ISerializable object (a normal structure)
			pointer.GetClass()->Serialize( *this, pointer.GetPointer() );
		}
	}
	else
	{
		// Every import should be a resource
		CResource* resource = Cast< CResource >( pointer.GetObjectPtr() );
		if ( NULL != resource )
		{
			// Do not, do not ever map resources that are not from files
			if ( resource->GetFile() == nullptr )
			{
				// do not map his object
				m_objectIndices.Insert( pointer.GetPointer(), 0 );
				m_tempObjectIndices.Insert( pointer.GetPointer(), 0 );
				return 0;
			}

			// Map class name
			const CName className = resource->GetClass()->GetName();
			MapName( className );

			// Add object to import table
			objectIndex = TO_IMPORT_INDEX( m_imports.Size() );
			m_imports.PushBack( ImportInfo( resource, usedAsTemplate, false ) );

			// Map it to an index
			m_tempObjectIndices.Insert( resource, objectIndex );
		
			// Also map in a final table
			m_objectIndices.Insert( resource, objectIndex );
		}
	}

	// Return allocated index
	return objectIndex;
}

namespace
{
	const Bool IsContained( const ISerializable* object, const ISerializable* root )
	{
		while ( object != nullptr )
		{
			if ( (object == root) || (object == ISerializable::ANY_PARENT) )
			{
				return true;
			}

			object = object->GetSerializationParent();
		}

		return false;
	}
}

Bool CDependencyMapper::ShouldMapObject( const CPointer& pointer )
{
	// Don't map NULL objects
	if ( pointer.IsNull() )
	{
		return false;
	}

	// Do not map objects of transient classes
	if ( pointer.GetRuntimeClass()->IsAlwaysTransient() )
	{
		return false;
	}

	// CObject only stuff
	if ( pointer.IsObject() )
	{
		// Don't map transient objects
		CObject* realObject = pointer.GetObjectPtr();
		if ( realObject->IsTransient() && !m_context.m_saveTransient )
		{
			return false;
		}

		// Don't map referenced objects
		if ( realObject->HasFlag( OF_Referenced ) && !m_context.m_saveReferenced )
		{
			return false;
		}

		// Map other objects only if they are contained inside initial export set
		const Uint32 numInitialExports = m_context.m_initialExports.Size();
		for ( Uint32 i=0; i<numInitialExports; i++ )
		{
			if ( IsContained( realObject, m_context.m_initialExports[i] ) )
			{
				return true;
			}
		}

		// In force export cases we need to check if we are contained in existing export
		for ( Uint32 i=0; i<m_tempExports.Size(); i++ )
		{
			CObject* tempExportObject = m_tempExports[i].GetObjectPtr();
			if ( (NULL != tempExportObject) && realObject->IsContained( tempExportObject ) )
			{
				return true;
			}
		}

		// Always map resources
		if ( realObject->IsA( ClassID<CResource>() ) )
		{
			// Do not include links to resources that parent us
			const Uint32 numInitialExports = m_context.m_initialExports.Size();
			for ( Uint32 i=0; i<numInitialExports; i++ )
			{
				if ( IsContained( m_context.m_initialExports[i], realObject ) )
				{
					return false;
				}
			}

			// Include
			return true;
		}

		// Do not map this object, it is not a resource and lies outside the scope of initial export objects
		return false;
	}
	else
	{
		// This is a non CObject pointer, we have no choice but to map it since we have no control over where it comes from
		// TODO: we can add a method ISerializable::CanSaveFor(CObject* root) where it would be possible to filter the scope of the object saved
		return true;
	}
}

Bool CDependencyMapper::ShouldExportObject( const CPointer& pointer )
{
	// Resources are usually not exported
	CResource* resource = Cast< CResource >( pointer.GetObjectPtr() );
	if ( NULL != resource )
	{
		// Special case for embedded resources
		const Uint32 numInitialExports = m_context.m_initialExports.Size();
		for ( Uint32 i=0; i<numInitialExports; i++ )
		{
			CObject* realObject = Cast< CObject >( m_context.m_initialExports[i] );
			if ( realObject && resource->IsContained( realObject ) )
			{
				return true;
			}
		}

		// In force export cases we need to check if we are contained in existing export
		for ( Uint32 i=0; i<m_tempExports.Size(); i++ )
		{
			CObject* tempExportObject = m_tempExports[i].GetObjectPtr();
			if ( (NULL != tempExportObject) && resource->IsContained( tempExportObject ) )
			{
				return true;
			}
		}

		// Import resources
		return false;
	}

	// Rest of the objects usually is exported
	// This includes ISerializable objects as well
	return true;
}

void CDependencyMapper::SerializePointer( const class CClass* pointerClass, void*& pointer )
{
	CPointer enginePointer( pointer, const_cast<CClass*>( pointerClass ) );
	MapObject( enginePointer, false );
}

void CDependencyMapper::SerializeName( class CName& name )
{
	MapName( name );
}

void CDependencyMapper::SerializeSoftHandle( class BaseSoftHandle& softHandle )
{
	MapSoftDependency( softHandle.GetPath() );
}

void CDependencyMapper::SerializeTypeReference( class IRTTIType*& type )
{
	if ( NULL != type )
	{
		MapName( type->GetName() );
	}
}

void CDependencyMapper::SerializePropertyReference( const class CProperty*& prop )
{
	MapProperty( prop );
}

void CDependencyMapper::SerializeDeferredDataBuffer( DeferredDataBuffer & buffer )
{
	MapDataBuffer( buffer );
}

