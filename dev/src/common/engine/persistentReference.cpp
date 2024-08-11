/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "persistentReference.h"
#include "../core/scriptStackFrame.h"
#include "entity.h"
IMPLEMENT_ENGINE_CLASS( PersistentRef );

void PersistentRef::Set( CNode* node )
{
	ASSERT( node );

	CEntity* entity = Cast< CEntity >( node );

	if( entity )
	{
		m_entityHandle.Set( entity ); 
	}

	m_position = node->GetWorldPosition();
	m_rotation = node->GetWorldRotation();	
}

//! Set using orientation
void PersistentRef::Set( const Vector& pos, const EulerAngles& rot )
{
	// Clear handle
	m_entityHandle.Set( NULL );

	// Set values
	m_position = pos;
	m_rotation = rot;
}

//! Get entity
CEntity* PersistentRef::GetEntity()
{
	return m_entityHandle.Get();
}

//! Get entity
const CEntity* PersistentRef::GetEntity() const
{
	return m_entityHandle.Get();
}

//! Get full world position
Vector PersistentRef::GetWorldPosition() const
{
	const CEntity* entity = m_entityHandle.Get();
	if( entity )
	{
		return entity->GetWorldPosition();
	}

	return m_position;
}

//! Get full world rotation
EulerAngles PersistentRef::GetWorldRotation() const
{
	const CEntity* entity = m_entityHandle.Get();
	if( entity )
	{
		return entity->GetWorldRotation();
	}

	return m_rotation;
}

//! Get full world orientation
void PersistentRef::GetWorldOrientation( Vector& position, EulerAngles& rotation ) const
{
	const CEntity* entity = m_entityHandle.Get();
	if( entity )
	{
		position = entity->GetWorldPosition();
		rotation = entity->GetWorldRotation();
	}
	else
	{
		position = m_position;
		rotation = m_rotation;
	}
}

static void funcPersistentRefSetNode( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	GET_PARAMETER( THandle< CNode >, nodeHandle, NULL );	
	FINISH_PARAMETERS;
	CNode* node = nodeHandle.Get();
	if( node )
	{
		persistentRef.Set( node );
	}
	else
	{
		persistentRef.Set( Vector::ZEROS, EulerAngles::ZEROS );
	}
}

static void funcPersistentRefSetOrientation( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	GET_PARAMETER( Vector, pos, Vector::ZEROS );
	GET_PARAMETER( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;
	persistentRef.Set( pos, rot );
}

static void funcPersistentRefGetEntity( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	FINISH_PARAMETERS;
	RETURN_OBJECT( persistentRef.GetEntity() );
}

static void funcPersistentRefGetWorldPosition( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	FINISH_PARAMETERS;
	RETURN_STRUCT( Vector, persistentRef.GetWorldPosition() );
}

static void funcPersistentRefGetWorldRotation( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	FINISH_PARAMETERS;
	RETURN_STRUCT( EulerAngles, persistentRef.GetWorldRotation() );
}

static void funcPersistentRefGetWorldOrientation( IScriptable* context, CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( PersistentRef, persistentRef, PersistentRef() );
	GET_PARAMETER_REF( Vector, pos, Vector::ZEROS );
	GET_PARAMETER_REF( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;
	persistentRef.GetWorldOrientation( pos, rot );
}

void ExportEnginePersistentRefNatives()
{
	NATIVE_GLOBAL_FUNCTION( "PersistentRefSetNode",	funcPersistentRefSetNode );
	NATIVE_GLOBAL_FUNCTION( "PersistentRefSetOrientation",	funcPersistentRefSetOrientation );

	NATIVE_GLOBAL_FUNCTION( "PersistentRefGetEntity", funcPersistentRefGetEntity );
	NATIVE_GLOBAL_FUNCTION( "PersistentRefGetWorldPosition", funcPersistentRefGetWorldPosition );
	NATIVE_GLOBAL_FUNCTION( "PersistentRefGetWorldRotation", funcPersistentRefGetWorldRotation );
	NATIVE_GLOBAL_FUNCTION( "PersistentRefGetWorldOrientation", funcPersistentRefGetWorldOrientation );
}