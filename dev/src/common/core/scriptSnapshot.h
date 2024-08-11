/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "names.h"
#include "string.h"
#include "handleMap.h"
#include "object.h"

/// Snapshot of the scripting state
class CScriptSnapshot
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );

public:
	//! Snapshot of a scripted field
	struct PropertySnapshot
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );

		typedef TDynArray< PropertySnapshot*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > SubProperties;

		CName							m_name;				//!< Name of the cached property
		String							m_valueString;		//!< Value - in case of simple types
		THandle< IScriptable >			m_valueHandle;		//!< Value - in case of object pointer
		SubProperties					m_subValues;		//!< Sub values - arrays and structures

		PropertySnapshot()
		{
		}

		~PropertySnapshot()
		{
			m_subValues.ClearPtr();
		}
	};

	//! Snapshot of a scripted object
	struct ScriptableSnapshot
	{
		DECLARE_STRUCT_MEMORY_POOL( MemoryPool_ScriptCompilation, MC_ScriptCompilation );

		typedef TDynArray< ScriptableSnapshot*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > States;

		THandle< IScriptable >				m_scriptable;		//!< Object being snapshotted
		PropertySnapshot::SubProperties		m_properties;		//!< Properties

		States								m_states;
		ScriptableSnapshot*					m_activeState;

		const void*							m_debugOrgAddress;
		CName								m_debugOrgClass;

		ScriptableSnapshot( const IScriptable* object )
		:	m_scriptable( object )
		,	m_debugOrgAddress( object )
		,	m_debugOrgClass( object->GetClass()->GetName() )
		{
		}

		~ScriptableSnapshot()
		{
			m_properties.ClearPtr();
		}
	};

protected:
	typedef TDynArray< ScriptableSnapshot*, MC_ScriptCompilation, MemoryPool_ScriptCompilation > TObjectList;

	TObjectList m_scriptedObjects;		//!< Snapshots of scripted objects

public:
	CScriptSnapshot();
	~CScriptSnapshot();

	//! Create a snapshot, will clear all scripting data
	void CaptureScriptData( const TDynArray< THandle< IScriptable > >& allScriptables );

	//! Restore scripting data
	void RestoreScriptData( const TDynArray< THandle< IScriptable > >& allScriptables );

public:
	//! Build single object snapshot of editable properties
	ScriptableSnapshot* BuildEditorObjectSnapshot( const IScriptable* scriptedObject );

	//! Restore snapshot of single an object
	void RestoreEditorObjectSnapshot( IScriptable* scriptedObject, const ScriptableSnapshot* snapshot );

protected:
	//! Build a snapshot for a property
	PropertySnapshot* BuildPropertySnapshot( const IScriptable* scriptedObject, const IRTTIType* type, const void* data );

	//! Build a script snapshot for an object
	ScriptableSnapshot* BuildObjectSnapshot( const IScriptable* scriptedObject );

	//! Restore property default values
	void RestoreObjectPropertiesDefaults( IScriptable* scriptedObject );

	//! Restore property snapshot
	void RestorePropertySnapshot( IScriptable* object, void* data, const IRTTIType* type, const PropertySnapshot* snapshot );

	//! Restore properties from object snapshot
	void RestoreObjectSnapshot( const ScriptableSnapshot* snapshot );	
};
