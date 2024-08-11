/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "object.h"
#include "memoryFileWriter.h"
#include "dependencySaver.h"
#include "memoryFileReader.h"
#include "dependencyLoader.h"
#include "configVarSystem.h"
#include "configVarLegacyWrapper.h"
#include "namesRegistry.h"
#include "events.h"
#include "resource.h"
#include "objectMap.h"
#include "objectRootSet.h"
#include "objectMap.h"
#include "objectDiscardList.h"
#include "objectAllocationTracer.h"
#include "objectGC.h"
#include "cooker.h"
#include "../redSystem/unitTestMode.h"

//#define DEBUG_OBJECTS

#ifdef DEBUG_OBJECTS
#pragma optimize("",off)
#endif

#define PARENT_INDEX_MASK ( 0x7FFFFFFF )

/************************************************************************/
/* Objects RTTI implementation                                          */
/************************************************************************/

IMPLEMENT_ENGINE_CLASS( CObject );

Red::Threads::CMutex GTheFuckingChildListMutex;

#ifdef DEBUG_OBJECTS
	static Uint32 GObjectAllocationBreak = 0;
#endif

CObject::CObject()
	: m_parent( nullptr )
	, m_previousChild( nullptr )
	, m_nextChild( nullptr )
	, m_children( nullptr )
	, m_index( 0 )
	, m_objectFlags( 0 )
	// m_class( NULL ) - this cannot be overriden in constructor because is set in the allocator
{
	if ( !GetClass() )
	{
		RED_HALT("CObject created without Class. Probably zero initialization occurred!");
//		__debugbreak();
	}

	// Handle GC case
	if ( GObjectGC->IsDoingGC() )
	{
		RED_HALT( "CObject::CObject called when in GC on another thread." );
	}

	// Allocate new index
	GObjectsMap->AllocateObject( this, m_index );

	// Tracer helper
#ifndef RED_FINAL_BUILD
	/*if( !Red::System::UnitTestMode() && m_index )
	{
		Helper::SObjectAllocationTracker::GetInstance().RegisterObject( m_index );
	}*/
#endif

#ifdef DEBUG_OBJECTS
	if ( GObjectAllocationBreak == m_index )
	{
		LOG_CORE( TXT("Object class: %ls"), GetClass()->GetName().AsChar() );
		LOG_CORE( TXT("Object ptr: 0x%llX"), (Uint64)this );

		RED_HALT( "CObject: debug!" );
	}
#endif
}

CObject::~CObject()
{
	// Tracer helper
#ifndef RED_FINAL_BUILD
	/*if( !Red::System::UnitTestMode() && m_index )
	{
		Helper::SObjectAllocationTracker::GetInstance().UnregisterObject( m_index );
	}*/
#endif

	// Unregister
	if ( m_index != 0 )
	{
		GObjectsMap->DeallocateObject( this, m_index );
		m_index = 0;
	}

	// Make sure it made to the CObject::OnFinalize
	if ( !HasFlag( OF_Finalized ) )
	{
		RED_FATAL( "Destructor called on non finalized CObject! Use Discard() instead of delete. Debug now!" );
	}
}

void CObject::InitializeSystem()
{
	static Bool isInitialized = false;

	if ( !isInitialized )
	{
		isInitialized = true;

		GObjectsMap = new CObjectsMap();
		GObjectsDiscardList = new CObjectsDiscardList();
		GObjectsRootSet = new CObjectsRootSet();
		GObjectGC = new CObjectGC();

		LOG_CORE( TXT("Object system intiialized") );
	}
}

void CObject::OnPropertyPostChange( IProperty* )
{
	MarkModified();
}

const void* CObject::GetBaseObjectData() const
{
	CObject *objTemplate = GetTemplate();
	if ( objTemplate != NULL )
	{
		return objTemplate->GetTemplateInstance();
	}

	// default
	return GetClass()->GetDefaultObjectImp();
}

CClass* CObject::GetBaseObjectClass() const
{
	// why do we need this ? for abstract or scripted objects the default class can exist only for the base classes not for the actual class
	// in a normal class it's the same class
	return GetClass();
}

void CObject::OnFinalize()
{
	RED_FATAL_ASSERT( !HasFlag( OF_Discarded ), "OnFinalize called on already discarded object" );
	RED_FATAL_ASSERT( !HasFlag( OF_Finalized ), "OnFinalize should be called only once" );

	// This flag is set here to indicate that object was fully finalized
	m_objectFlags |= OF_Finalized;
}

CObject *CObject::CreateObjectStatic( CClass *baseClass, CObject *parent, Uint16 flags, Bool )
{
	// Register object in the object system
	CObject *object = baseClass->CreateObject< CObject >();

	// Setup object params
	object->m_objectFlags = flags;

	// Bind object parent
	object->SetParent( parent );

	// Done
	return object;
}

Bool CObject::MarkModified()
{
	// Go up
	if ( m_parent && IsTransient() == false )
	{
		return m_parent->MarkModified();
	}

	// Not a resource, allow to modify
	return true;
}

Bool CObject::CanModify()
{
	// Go up
	if ( m_parent && IsTransient() == false )
	{
		return m_parent->CanModify();
	}

	// Not a resource, allow to modify
	return true;
}

void CObject::Discard()
{
	// We cannot discard objects that are in the root set
	if ( HasFlag( OF_Root ) )
	{
		HALT( "Trying to discard object '%ls' that is still in the root set. This is a serious programmer's fuckup! Kill the bastard.", GetFriendlyName().AsChar() );
		return;
	}

	// We cannot directly discard invalid objects
	if ( m_index == INVALID_OBJECT_INDEX )
	{
		HALT( "Unable to directly Discard invalid object" );
		return;
	}

	// Discard, only once
	if ( !HasFlag( OF_Discarded ) )
	{
		// Unlink this object from child list - only if we have parent and also parent wasn't 
		UnlinkFromChildList();

		// Unlink from object mapping
		UnlinkFromObjectMap();

		// Remove the editor references to this object
		UnlinkEditorReferenced();	

		// Discard children
		CObject *cur, *next;
		for ( cur=m_children; cur; cur=next )
		{
			next = cur->m_nextChild;
			ASSERT( cur->GetParent() == this );
			cur->Discard();
		}

		// Finalize - high level cleanup just before children will be destroyed
		if ( !HasFlag( OF_Finalized ))
		{
			OnFinalize();
			SetFlag( OF_Finalized );
		}

		// Invalid any handles pointing to this object
		DiscardHandles();

		// Add to the discard list 
		GObjectsDiscardList->Add( this );
		RED_ASSERT( m_objectFlags & OF_Discarded );	
	}
}

void CObject::DiscardNoLock()
{
	// We cannot discard objects that are in the root set
	if ( HasFlag( OF_Root ) )
	{
		HALT( "Trying to discard object '%ls' that is still in the root set. This is a serious programmer's fuckup! Kill the bastard.", GetFriendlyName().AsChar() );
		return;
	}

	// We cannot directly discard invalid objects
	if ( m_index == INVALID_OBJECT_INDEX )
	{
		HALT( "Unable to directly Discard invalid object" );
		return;
	}

	// Discard, only once
	if ( !HasFlag( OF_Discarded ) )
	{
		// Unlink this object from child list - only if we have parent and also parent wasn't 
		UnlinkFromChildListNoLock();

		// Unlink from object mapping
		UnlinkFromObjectMapNoLock();

		// Remove the editor references to this object
		UnlinkEditorReferenced();	

		// Discard children
		CObject *cur, *next;
		for ( cur=m_children; cur; cur=next )
		{
			next = cur->m_nextChild;
			ASSERT( cur->GetParent() == this );
			cur->DiscardNoLock();
		}

		// Finalize - high level cleanup just before children will be destroyed
		if ( !HasFlag( OF_Finalized ))
		{
			OnFinalize();
			SetFlag( OF_Finalized );
		}

		// Invalid any handles pointing to this object
		DiscardHandles();

		// Add to the discard list 
		GObjectsDiscardList->AddNoLock( this );
		RED_ASSERT( m_objectFlags & OF_Discarded );	
	}
}

void CObject::AddToRootSet()
{
	GObjectsRootSet->Add( this );
}

void CObject::SetParent( CObject* object )
{
	// Cannot create objects in unlinked hierarchies
	if ( object && object->GetObjectIndex() == INVALID_OBJECT_INDEX )
	{
		HALT( "Cannot create objects in unlinked object hierarchies" );
		return;
	}

	// Unlink from current parent list
	UnlinkFromChildList();

	// Change parent
	m_parent = object;

	// Link to new child list
	if ( m_parent )
	{
		LinkToChildList( m_parent->m_children );
	}
}

void CObject::RemoveFromRootSet()
{
	GObjectsRootSet->Remove( this );
}

void CObject::GetChildren( TDynArray< CObject* >& children ) const
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GTheFuckingChildListMutex );

	for ( CObject* child=m_children; child != nullptr; child=child->m_nextChild )
	{
		children.PushBack( child );
	}
}

Int32 CObject::GetClassIndex() const
{
	CClass* theClass = GetClass();
	while ( theClass )
	{
		Int32 index = theClass->GetClassIndex();
		if ( index != -1 )
		{
			return index;
		}

		// Go to base class
		theClass = theClass->GetBaseClass();
	}

	// Not determined
	return -1;
}

CObject* CObject::Clone( CObject* newParent, Bool cloneWithTransientObjects /*=true*/, Bool cloneWithReferencedObjects /*=true*/ ) const
{
	TDynArray< Uint8 > buffer;
	// Reserve some memory for object - 1k is safe and should be enough
	buffer.Reserve( 1024 );

	// Save object to memory
	{
		// Setup writer stream
		CMemoryFileWriter writer( buffer );
		CDependencySaver saver( writer, NULL );

		// Setup save context
		DependencySavingContext context( const_cast< CObject* >( this ) );
		context.m_saveReferenced = cloneWithReferencedObjects;
		context.m_saveTransient = cloneWithTransientObjects;
		if ( !saver.SaveObjects( context ) )
		{
			return NULL;
		}
	}

	// Load new copy from memory
	{
		CMemoryFileReader reader( buffer, 0 );
		CDependencyLoader loader( reader, NULL );

		// Load objects
		DependencyLoadingContext loadingContext;
		loadingContext.m_parent = newParent;
		if ( !loader.LoadObjects( loadingContext ) )
		{
			return NULL;
		}

		// Post load cloned objects
		loader.PostLoad();

		// return loaded object
		return loadingContext.m_loadedRootObjects[0];
	}
}

ISerializable* CObject::GetSerializationParent() const
{
	return m_parent;
}

void CObject::RestoreSerializationParent( ISerializable* parent )
{
	RED_UNUSED( parent );
	RED_ASSERT( m_parent == parent, TXT("Restored object parent is different" ) );
}

CObject* CObject::GetTemplate() const
{
	return NULL;
}

const CObject* CObject::GetTemplateInstance() const
{
	return NULL;
}

CObject* CObject::CreateTemplateInstance( CObject* ) const
{
	HALT(  "CreateTemplateInstance called on invalid object" );
	return NULL;
}

// Collectes used resources
class CResourceCollector : public IFile
{
public:
	CObject*					m_baseObject;
	TDynArray< CResource* >		m_resources;
	TDynArray< void* >			m_visited;
	Bool						m_loadSoftHandles;

public:
	RED_INLINE CResourceCollector( CObject* baseObject, Bool loadSoftHandles )
		: IFile( FF_Writer | FF_FileBased | FF_ResourceCollector | ( GCNameAsNumberSerialization ? FF_HashNames : 0 ) )
		, m_baseObject( baseObject )
		, m_loadSoftHandles( loadSoftHandles )
	{
	};

	// Pointer serialization
	virtual void SerializePointer( const class CClass* pointerClass, void*& pointer )
	{
		// Visit objects only once
		if ( pointer && !m_visited.Exist( pointer ) )
		{
			m_visited.PushBack( pointer );

			// objects
			if ( pointerClass->IsObject() )
			{
				CObject* obj = static_cast< CObject* >( pointer );

				// Object is a resource
				if ( obj->IsA< CResource >() )
				{
					m_resources.PushBackUnique( SafeCast< CResource >( obj ) );
				}

				// Object is in base object tree, recurse
				if ( !obj->IsContained( m_baseObject ) )
				{
					return;
				}
			}

			// recursion
			if ( pointerClass->IsSerializable() )
			{
				ISerializable* obj = static_cast< ISerializable* >( pointer );
				obj->OnSerialize( *this );
			}
		}
	}

	virtual void SerializeSoftHandle( class BaseSoftHandle& softHandle )
	{
		TSoftHandle< CResource > *accessibleSoftHandle = reinterpret_cast< TSoftHandle< CResource > * >( &softHandle );
		if ( accessibleSoftHandle != nullptr && !m_visited.Exist( accessibleSoftHandle ) )
		{
			if ( m_loadSoftHandles )
			{
				accessibleSoftHandle->Load();
			}

			if ( accessibleSoftHandle->IsLoaded() )
			{
				m_visited.PushBack( accessibleSoftHandle );
				m_resources.PushBackUnique( accessibleSoftHandle->Get() );
			}
		}
	}

protected:
	// Fake IFile interface
	virtual void Serialize( void*, size_t ) {}
	virtual Uint64 GetOffset() const { return 0; };
	virtual Uint64 GetSize() const { return 0; }
	virtual void Seek( Int64 ) {};
};

void CObject::CollectUsedResources( TDynArray< CResource* >& resources, Bool loadSoftHandles /* = false */ ) const
{
	CResourceCollector collector( ( CObject* ) this, loadSoftHandles );
	collector.m_baseObject->OnSerialize( collector );
	resources.PushBackUnique( collector.m_resources );
}

void CObject::OnDebugValidate()
{
	// Destroyed/Unreachable objects cannot be in the root set
	if ( HasFlag( OF_Discarded ) ) 
	{
		ASSERT( ! IsInRootSet() );
	}

	// Check object mapping
	ASSERT( m_index != 0 );
//	ASSERT( SObjectsMap::GetInstance().m_allObjects[ m_index ] == this );

	// Make sure object's class is valid
	ASSERT( GetClass() != NULL );
}

void CObject::OnScriptPreCaptureSnapshot()
{
}

void CObject::OnScriptPostCaptureSnapshot()
{
}

void CObject::OnScriptReloaded()
{
}

#ifndef NO_RESOURCE_COOKING

void CObject::OnCook( class ICookerFramework& cooker )
{
	// just make sure we don't recook something that's already cooked
	if ( HasFlag( OF_WasCooked ) )
	{
		cooker.CookingError( this, TXT("Tried to cook the same object twice!") );
	}
}

#endif

void AddDefaultObjectToRootSet( CObject *obj )
{
	ASSERT( obj );
	obj->SetFlag( OF_DefaultObject );
	obj->AddToRootSet();
}

void RemoveFromRootSet( CObject *obj )
{
	ASSERT( obj );
	obj->RemoveFromRootSet();
}

Bool CObject::LoadObjectConfig( const Char* category, const Char* section /*=NULL*/ )
{
	// Get file
	Config::Legacy::CConfigLegacyFile* file = SConfig::GetInstance().GetLegacy().GetFile( category );
	if ( !file )
	{
		WARN_CORE( TXT("Object's '%ls' class '%ls' has invalid configuration category '%ls'"), GetFriendlyName().AsChar(), GetClass()->GetName().AsString().AsChar(), category );
		return false;
	}

	// Get section
	const String sectionName = section ? section : GetClass()->GetName().AsString();
	Config::Legacy::CConfigLegacySection* configSection = file->GetSection( sectionName );
	if ( !configSection )
	{
		WARN_CORE( TXT("Object's '%ls' class '%ls' configuration section '%ls' not found in category '%ls'"), GetFriendlyName().AsChar(), GetClass()->GetName().AsString().AsChar(), sectionName.AsChar(), category );
		return false;
	}

	// Load values from config
	const THashMap< String, TDynArray< String > >& items = configSection->GetItems();
	for ( THashMap< String, TDynArray< String > >::const_iterator i=items.Begin(); i!=items.End(); ++i )
	{
		// Get property
		const CName propName( i->m_first.AsChar() );
		CProperty* prop = GetClass()->FindProperty( propName );
		if ( !prop )
		{
			WARN_CORE( TXT("Object's '%ls' has no configuration property '%ls'"), GetFriendlyName().AsChar(), propName.AsString().AsChar() );
			continue;
		}

		// Is config property ?
		if ( !prop->IsConfig() )
		{
			WARN_CORE( TXT("Object's '%ls' property '%ls' is not configuration property"), GetFriendlyName().AsChar(), propName.AsString().AsChar() );
			continue;
		}

		// Import value
		void* propData = prop->GetOffsetPtr( this );
		if ( i->m_second.Empty() )
		{
			// No value, clean
			prop->GetType()->Clean( propData );
		}
		else
		{
			if ( prop->GetType()->GetType() == RT_Array )
			{
				// Clear array
				prop->GetType()->Clean( propData );

				// Add elements
				CRTTIArrayType* arrayType = ( CRTTIArrayType* ) prop->GetType();
				arrayType->AddArrayElement( propData, i->m_second.Size() );
				for ( Uint32 j=0; j<i->m_second.Size(); j++ )
				{
					void* elementData = arrayType->GetArrayElement( propData, j );
					arrayType->GetInnerType()->FromString( elementData, i->m_second[j] );
				}
			}
			else
			{
				// Import single element
				prop->GetType()->FromString( propData, i->m_second[0] );
			}
		}
	}

	// Config read
	return true;
}

Bool CObject::SaveObjectConfig( const Char* category, const Char* section /*=NULL*/ ) const
{
	// Get file
	Config::Legacy::CConfigLegacyFile* file = SConfig::GetInstance().GetLegacy().GetFile( category );
	if ( !file )
	{
		WARN_CORE( TXT("Object's '%ls' class '%ls' has invalid configuration category '%ls'"), GetFriendlyName().AsChar(), GetClass()->GetName().AsString().AsChar(), category );
		return false;
	}

	// Get section
	const String sectionName = section ? GetClass()->GetName().AsString() : section;
	Config::Legacy::CConfigLegacySection* configSection = file->GetSection( sectionName, true );
	if ( !configSection )
	{
		WARN_CORE( TXT("Object's '%ls' class '%ls' configuration section '%ls' not found in category '%ls'"), GetFriendlyName().AsChar(), GetClass()->GetName().AsString().AsChar(), sectionName.AsChar(), category );
		return false;
	}

	// Clear section content
	configSection->Clear();

	// Save config properties
	TDynArray< CProperty* > properties;
	GetClass()->GetProperties( properties );

	// Save properties
	for ( Uint32 i=0; i<properties.Size(); i++ )
	{
		CProperty* prop = properties[i];
		if ( prop->IsConfig() )
		{
			const void* propData = prop->GetOffsetPtr( this );
			if ( prop->GetType()->GetType() == RT_Array )
			{
				// Clear current value list
				configSection->RemoveValues( prop->GetName().AsString().AsChar() );

				// Export elements
				CRTTIArrayType* arrayType = ( CRTTIArrayType* ) prop->GetType();
				const Uint32 numElements = arrayType->GetArraySize( propData );
				for ( Uint32 j=0; j<numElements; j++ )
				{
					// Export single value
					String value;
					const void* arrayElementData = arrayType->GetArrayElement( propData, j );
					if ( arrayType->GetInnerType()->ToString( arrayElementData, value ) )
					{
						configSection->WriteValue( prop->GetName().AsString().AsChar(), value, true );
					}
				}
			}
			else
			{
				// Export single value
				String value;
				if ( prop->GetType()->ToString( propData, value ) )
				{
					configSection->WriteValue( prop->GetName().AsString().AsChar(), value, false );
				}
			}
		}
	}

	// Flush config
	file->Write();
	return true;
}

void CObject::LinkToChildList( CObject*& list )
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GTheFuckingChildListMutex );

	RED_FATAL_ASSERT( !m_nextChild, "Already linked" );
	RED_FATAL_ASSERT( !m_previousChild, "Already linked" );

	if( list )
	{
		RED_FATAL_ASSERT( !list->m_previousChild, "Management of CObject child assume we are always pushing front. Debug." );
		list->m_previousChild = this;
		m_nextChild = list;
	}

	list = this;
}

void CObject::UnlinkFromChildList()
{
	Red::Threads::CScopedLock< Red::Threads::CMutex > lock( GTheFuckingChildListMutex );
	UnlinkFromChildListNoLock();
}

void CObject::UnlinkFromChildListNoLock()
{
	if ( m_parent && m_parent->m_children == this )
	{
		m_parent->m_children = m_nextChild; 
	}

	CObject * previous = m_previousChild;
	CObject * next = m_nextChild;
	if( next )
	{
		next->m_previousChild = previous;
	}

	if( previous )
	{
		previous->m_nextChild = next;
	}

	m_nextChild = nullptr;
	m_previousChild = nullptr;
}

void CObject::UnlinkFromObjectMap()
{
	if ( m_index )
	{
		GObjectsMap->DeallocateObject( this, m_index );
		m_index = 0;
	}
}

void CObject::UnlinkFromObjectMapNoLock()
{
	if ( m_index )
	{
		GObjectsMap->DeallocateObjectNoLock( this, m_index );
		m_index = 0;
	}
}

void CObject::UnlinkEditorReferenced()
{
#if defined( RED_PLATFORM_WINPC ) && !defined( RED_FINAL_BUILD )
	EDITOR_DISPATCH_EVENT( CNAME( OnObjectDiscarded ), CreateEventData( this ) );
#endif
}

void CObject::OnAllHandlesReleased()
{
	// objects that are no longer referenced get's discarded
	Discard();
}

Bool CObject::OnValidateHandleCreation() const
{
	if ( HasFlag( OF_Discarded ) )	
	{
		RED_HALT( "Trying to create a handle to a discarded object '%ls', class '%ls', 0x%llX. What a beautiful idea.", 
			GetFriendlyName().AsChar(), GetClass()->GetName().AsChar(), (Uint64) this );

		return false;
	}

	return true;
}

