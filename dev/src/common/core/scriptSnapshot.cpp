/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "scriptSnapshot.h"
#include "scriptingSystem.h"
#include "profiler.h"
#include "scriptableStateMachine.h"
#include "scriptableState.h"

//#define DUMP_CAPTURED_STATE

#ifndef TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE
extern void EntityHandleDataGetObjectHandle( const void *handleData, THandle< CObject >& objectHandle );
extern void EntityHandleDataSetObjectHandle( void *handleData, THandle< CObject >& objectHandle );
#endif
RED_DEFINE_STATIC_NAMED_NAME( _EntityHandle, "EntityHandle" );

CScriptSnapshot::CScriptSnapshot()
{

}

CScriptSnapshot::~CScriptSnapshot()
{
	m_scriptedObjects.ClearPtr();
}

void CScriptSnapshot::CaptureScriptData( const TDynArray< THandle< IScriptable > >& allScriptableObjects )
{
	CTimeCounter timeCounter;

	// Delete current snapshots
	m_scriptedObjects.ClearPtr();

	// Process all scriptables
	Uint32 numPropertySnapshots = 0;
	for ( Uint32 i=0; i<allScriptableObjects.Size(); ++i )
	{
		IScriptable* scriptable = allScriptableObjects[i].Get();
		if ( nullptr != scriptable )
		{
			// Inform object
			scriptable->OnScriptPreCaptureSnapshot();

			// Build snapshots from object
			ScriptableSnapshot* snapshot = BuildObjectSnapshot( scriptable );
			if ( nullptr != snapshot )
			{
				numPropertySnapshots += snapshot->m_properties.Size();
				m_scriptedObjects.PushBack( snapshot );
			}

			// Inform object
			scriptable->OnScriptPostCaptureSnapshot();
		}
	}

	// Show stats
	LOG_CORE( TXT("Snapshot from %i scriptables ( %i properties ) built in %1.2fs"), allScriptableObjects.Size(), numPropertySnapshots, timeCounter.GetTimePeriod() );
}

void CScriptSnapshot::RestoreObjectPropertiesDefaults( IScriptable* scriptedObject )
{
	// Get object class
	const CClass* objectClass = scriptedObject->GetClass();
	const void* defaultObject = objectClass->GetDefaultObjectImp();

	objectClass->ApplyDefaultValues( scriptedObject, scriptedObject != defaultObject, true );
}

void CScriptSnapshot::RestorePropertySnapshot( IScriptable* scriptedObject, void* data, const IRTTIType* type, const PropertySnapshot* snapshot )
{
	// Simple type
	ERTTITypeType typeType = type->GetType();

	if( type->GetName() == CNAME( _EntityHandle ) )
	{
#ifndef TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE
		EntityHandleDataSetObjectHandle( data, (THandle< CObject >&) snapshot->m_valueHandle );
#else
		RED_HALT( "TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE is defined" );
#endif
	}
	else if ( typeType == RT_Enum || typeType == RT_Simple || typeType == RT_BitField || typeType == RT_Fundamental )
	{
		{
			Bool res = type->FromString( data, snapshot->m_valueString );
			if( !res )
			{
				WARN_CORE( TXT("RestorePropertySnapshot FromString error for property %ls"), 
					snapshot->m_name.AsChar() );
			}
		}
	}

	// Arrays
	else if ( typeType == RT_Array )
	{
		// Get the array type
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( type );
		const IRTTIType* innerType = arrayType->GetInnerType();

		// Clean current array
		arrayType->Clean( data );

		// Create array
		const Uint32 numElements = snapshot->m_subValues.Size();
		if( numElements > 0 )
		{
			arrayType->AddArrayElement( data, numElements );

			// Add elements
			for ( Uint32 i=0; i<numElements; i++ )
			{
				void* elementData = arrayType->GetArrayElement( data, i );
				RestorePropertySnapshot( scriptedObject, elementData, innerType, snapshot->m_subValues[i] );
			}
		}
	}

	// Structure
	else if ( typeType == RT_Class )
	{
		const CClass* pointedClass = static_cast< const CClass* >( type );

		// Restore properties
		for ( Uint32 i=0; i<snapshot->m_subValues.Size(); i++ )
		{
			// Find property to restore
			PropertySnapshot* subSnapshot = snapshot->m_subValues[i];			
			CProperty* prop = pointedClass->FindProperty( subSnapshot->m_name );
			if ( prop )
			{
				// Get the target data
				void* propData = prop->GetOffsetPtr( data );
				RestorePropertySnapshot( scriptedObject, propData, prop->GetType(), subSnapshot );
			}
		}
	}

	// Handle
	else if ( typeType == RT_Handle )
	{
#ifdef DUMP_CAPTURED_STATE
		LOG_CORE( TXT("Restoring property '%ls' in object 0x%llX '%ls', prop ptr = 0x%llX."),  snapshot->m_name.AsChar(), (Uint64)scriptedObject, scriptedObject->GetClass()->GetName().AsChar (), (Uint64)data );

		BaseSafeHandle tempHandle;
		type->Copy( &tempHandle, data );
		LOG_CORE( TXT("  Prev value = 0x%llX '%ls'."), (Uint64)tempHandle.Get(), tempHandle.Get() ? tempHandle.Get()->GetClass()->GetName().AsChar() : TXT("None") );

		type->Copy( data, &snapshot->m_valueHandle );

		LOG_CORE( TXT("  New value = 0x%llX '%ls'."), (Uint64)snapshot->m_valueHandle.Get(), snapshot->m_valueHandle.Get() ? snapshot->m_valueHandle.Get()->GetClass()->GetName().AsChar() : TXT("None") );
#else
		type->Copy( data, &snapshot->m_valueHandle );
#endif
	}
}

void CScriptSnapshot::RestoreScriptData( const TDynArray< THandle< IScriptable > >& allScriptableObjects )
{
	// Restore default values in all scriptable object that we originally stored
	for ( Uint32 i=0; i<allScriptableObjects.Size(); ++i )
	{
		IScriptable* scriptable = allScriptableObjects[i].Get();
		if ( nullptr != scriptable )
		{
			RestoreObjectPropertiesDefaults( scriptable );
		}
	}

	// Restore snapshot of property values
	for ( Uint32 i = 0; i < m_scriptedObjects.Size(); ++i )
	{
		ScriptableSnapshot* snapshot = m_scriptedObjects[ i ];
		RestoreObjectSnapshot( snapshot );

		// If we have a state machine, we will need to restore each of it's individual states in turn
		IScriptable* object = snapshot->m_scriptable.Get();
		for ( Uint32 iState = 0; iState < snapshot->m_states.Size(); ++iState )
		{
			ScriptableSnapshot* stateSnapshot = snapshot->m_states[ iState ];

			// Restore the state from the snapshot
			RestoreObjectSnapshot( stateSnapshot );
			IScriptable* stateObject = stateSnapshot->m_scriptable.Get();

			ASSERT( stateObject );
			ASSERT( stateObject->IsA< CScriptableState >() );

			// Put the state back into the machine
			object->RestoreState( static_cast< CScriptableState* >( stateObject ), stateSnapshot == snapshot->m_activeState );
		}
	}
}

void CScriptSnapshot::RestoreObjectSnapshot( const ScriptableSnapshot* snapshot )
{
	// Get the snapshotted object
	IScriptable* scriptedObject = snapshot->m_scriptable.Get();
	if ( scriptedObject )
	{
		// Process properties
		for ( Uint32 j = 0; j < snapshot->m_properties.Size(); ++j )
		{
			const PropertySnapshot* propSnapshot = snapshot->m_properties[j];

			// Find property in the object
			const CClass* objectClass = scriptedObject->GetClass();
			const CProperty* prop = objectClass->FindProperty( propSnapshot->m_name );
			if ( !prop )
			{
				LOG_CORE( TXT("Unable to restore property '%ls' from snapshot of object '%ls'."), 
					propSnapshot->m_name.AsChar(), 
					scriptedObject->GetFriendlyName().AsChar() );
				continue;
			}

			// Property is no longer scripted
			if ( !prop->IsScripted() )
			{
				LOG_CORE( TXT("Snapshot property '%ls' from object '%ls' is no longer scripted."), 
					propSnapshot->m_name.AsChar(), 
					scriptedObject->GetFriendlyName().AsChar() );

				continue;
			}

			// Get the data and restore property
			void* data = prop->GetOffsetPtr( scriptedObject );
			RestorePropertySnapshot( scriptedObject, data, prop->GetType(), propSnapshot );
		}
	}
}

CScriptSnapshot::PropertySnapshot* CScriptSnapshot::BuildPropertySnapshot( const IScriptable* scriptedObject, const IRTTIType* type, const void* data )
{
	// Simple type
	ERTTITypeType typeType = type->GetType();

	if ( type->GetName() == CNAME( _EntityHandle ) )
	{
#ifndef TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE
		// Get handle value
		THandle< IScriptable > handle;
		EntityHandleDataGetObjectHandle( data, (THandle< CObject >&) handle );

		// Save as simple property snapshot
		PropertySnapshot* snapshot = new PropertySnapshot();
		snapshot->m_valueHandle = handle;
		return snapshot;
#else
		RED_HALT( "TMP_NO_ENTITY_HANDLE_FOR_CORE_LINKAGE is defined" );
		return nullptr;
#endif
	}
	else if ( typeType == RT_Enum || typeType == RT_Simple || typeType == RT_BitField || typeType == RT_Fundamental )
	{
		// Get the string value
		String valueString;
		if ( !type->ToString( data, valueString ) )
		{
			WARN_CORE( TXT("Unable to export snapshot of '%ls'"), type->GetName().AsString().AsChar() );
			return NULL;
		}
		
		// Save as simple property snapshot
		PropertySnapshot* snapshot = new PropertySnapshot();
		snapshot->m_valueString = valueString;
		return snapshot;
	}

	// Handles
	if ( typeType == RT_Handle )
	{
		// Get handle value
		THandle< IScriptable > handle;
		type->Copy( &handle, data );

		// Save as simple property snapshot
		PropertySnapshot* snapshot = new PropertySnapshot();
		snapshot->m_valueHandle = handle;
		return snapshot;
	}

	// Dynamic array
	if ( typeType == RT_Array )
	{
		// Get the inner type of array
		const CRTTIArrayType* arrayType = static_cast< const CRTTIArrayType* >( type );
		const IRTTIType* innerType = arrayType->GetInnerType();
		PropertySnapshot* snapshot = new PropertySnapshot();

		// Create snapshots of sub elements
		const Uint32 numElements = arrayType->GetArraySize( data );
		for ( Uint32 i=0; i<numElements; i++ )
		{
			// Get element data
			const void* elementData = arrayType->GetArrayElement( data, i );

			// Create element snapshot
			PropertySnapshot* elementSnapshot = BuildPropertySnapshot( scriptedObject, innerType, elementData );
			if ( !elementSnapshot )
			{
				WARN_CORE( TXT("Unable to export snapshot of #%i in '%ls'"), i, 
					type->GetName().AsChar() );

				delete snapshot;
				return NULL;
			}

			// Add to element snapshot
			snapshot->m_subValues.PushBack( elementSnapshot );
		}
		
		// Return created snapshot
		return snapshot;
	}

	// Structure
	if ( typeType == RT_Class )
	{
		// Get properties
		TDynArray< CProperty* > subProperties;
		(( CClass* ) type )->GetProperties( subProperties );

		// Create base snapshot
		PropertySnapshot* snapshot = new PropertySnapshot();

		// Create sub properties snapshots
		for ( Uint32 i = 0; i < subProperties.Size(); ++i ) 
		{
			CProperty* subProperty = subProperties[i];

			// Create sub-property snapshot
			const void* subPropertyData = subProperty->GetOffsetPtr( data );
			PropertySnapshot* subSnapshot = BuildPropertySnapshot( scriptedObject, subProperty->GetType(), subPropertyData );
			if ( !subSnapshot )
			{
				delete snapshot;
				return NULL;
			}

			// Name it and add to base property snapshot
			subSnapshot->m_name = subProperty->GetName();
			snapshot->m_subValues.PushBack( subSnapshot );
		}
		return snapshot;
	}

	// Invalid type
	HALT(  "Invalid type" );
	return NULL;
}

CScriptSnapshot::ScriptableSnapshot* CScriptSnapshot::BuildObjectSnapshot( const IScriptable* scriptedObject )
{
	// Get object class
	const CClass* objectClass = scriptedObject->GetClass();
	const void* defaultObject = objectClass->GetDefaultObjectImp();

	// Get properties
	TDynArray< CProperty* > properties;
	objectClass->GetProperties( properties );

	// Remember value of script properties that are different than from default value
	ScriptableSnapshot* objectSnapshot = NULL;
	for ( Uint32 i = 0; i < properties.Size(); ++i )
	{
		const CProperty* prop = properties[i];
		if ( prop->IsScripted() )
		{
			const void* baseValue = prop->GetOffsetPtr( scriptedObject );
			const void* defaultValue = prop->GetOffsetPtr( defaultObject );

			// Skip property if it has the same value as in the base object
			if ( prop->GetType()->Compare( baseValue, defaultValue, 0 ) )
			{
				continue;
			}

			// Build property snapshot
			PropertySnapshot* propSnapshot = BuildPropertySnapshot( scriptedObject, prop->GetType(), baseValue );
			if ( propSnapshot )
			{
				// Create the object snapshot if not already created
				if ( !objectSnapshot )
				{
					objectSnapshot = new ScriptableSnapshot( scriptedObject );
				}

				// Name it and add to the object snapshot
				propSnapshot->m_name = prop->GetName(); 
				objectSnapshot->m_properties.PushBack( propSnapshot );
			}
		}
	}

	// Grab state machine setting
	{
		// Create the object snapshot if not already created
		if ( !objectSnapshot )
		{
			objectSnapshot = new ScriptableSnapshot( scriptedObject );
		}

		TDynArray< CScriptableState* > states;
		scriptedObject->GetStates( states );

		objectSnapshot->m_states.Resize( states.Size() );

		for( Uint32 i = 0; i < states.Size(); ++i )
		{
			objectSnapshot->m_states[ i ] = BuildObjectSnapshot( states[ i ] );

			// BuildObjectSnapshot returns null if the state was identical to the default object
			if ( !objectSnapshot->m_states[ i ] )
			{
				const CClass* stateClass = states[ i ]->GetClass();
				const CScriptableState* defaultStateObject = const_cast< CClass* >( stateClass )->GetDefaultObject< CScriptableState >();

				objectSnapshot->m_states[ i ] = new ScriptableSnapshot( defaultStateObject );
			}

			// Remember which state is currently active
			if ( states[ i ] && states[ i ] == scriptedObject->GetCurrentState() )
			{
				objectSnapshot->m_activeState = objectSnapshot->m_states[ i ];
			}
		}
	}

	// Return created or not object snapshot
	return objectSnapshot;
}

CScriptSnapshot::ScriptableSnapshot* CScriptSnapshot::BuildEditorObjectSnapshot( const IScriptable* scriptableObject )
{
	// Get object class
	const CClass* objectClass = scriptableObject->GetClass();

	// Get properties
	TDynArray< CProperty* > properties;
	objectClass->GetProperties( properties );

	// Remember value of script properties that are different than from default value
	ScriptableSnapshot* objectSnapshot = NULL;
	for ( Uint32 i = 0; i < properties.Size(); ++i )
	{
		const CProperty* prop = properties[i];
		if ( prop->IsEditable() )
		{
			const void* baseValue = prop->GetOffsetPtr( scriptableObject );

			// Build property snapshot
			PropertySnapshot* propSnapshot = BuildPropertySnapshot( scriptableObject, prop->GetType(), baseValue );
			if ( propSnapshot )
			{
				// Create the object snapshot if not already created
				if ( !objectSnapshot )
				{
					objectSnapshot = new ScriptableSnapshot( scriptableObject );
				}

				// Name it and add to the object snapshot
				propSnapshot->m_name = prop->GetName(); 
				objectSnapshot->m_properties.PushBack( propSnapshot );
			}
		}
	}

	// Return created or not object snapshot
	return objectSnapshot;
}

void CScriptSnapshot::RestoreEditorObjectSnapshot( IScriptable* scriptableObject, const ScriptableSnapshot* snapshot )
{
	// Process properties
	for ( Uint32 j=0; j<snapshot->m_properties.Size(); j++ )
	{
		const PropertySnapshot* propSnapshot = snapshot->m_properties[j];

		// Find property in the object
		const CClass* objectClass = scriptableObject->GetClass();
		const CProperty* prop = objectClass->FindProperty( propSnapshot->m_name );
		if ( !prop )
		{
			LOG_CORE( TXT("Unable to restore property '%ls' from snapshot of object '%ls'."), 
				propSnapshot->m_name.AsChar(), 
				scriptableObject->GetFriendlyName().AsChar() );

			continue;
		}

		// Property is no longer scripted ? WTF
		if ( !prop->IsEditable() )
		{
			LOG_CORE( TXT("Snapshot property '%ls' from object '%ls' is no longer editable."), 
				propSnapshot->m_name.AsChar(), 
				scriptableObject->GetFriendlyName().AsChar() );

			continue;
		}

		// Get the data and restore property
		void* data = prop->GetOffsetPtr( scriptableObject );
		RestorePropertySnapshot( scriptableObject, data, prop->GetType(), propSnapshot );
	}
}
