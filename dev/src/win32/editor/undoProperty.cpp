#include "build.h"

#include "undoProperty.h"
#include "undoManager.h"

namespace
{
	CProperty* FindProperty( const STypedObject& typedObject, const CName& propertyName )
	{
		if ( ISerializable* obj = typedObject.AsSerializable() )
		{ // get the property from the _actual_ class (in case of a pointer/handle property, m_typedObject may point in fact to a base)
			return obj->GetClass()->FindProperty( propertyName );
		}
		else
		{
			return typedObject.m_class->FindProperty( propertyName );
		}
	}
}

IMPLEMENT_ENGINE_CLASS( CUndoProperty );

void CUndoProperty::PrepareStep( CEdUndoManager& undoManager, const STypedObject& typedObject, CName propertyName )
{
	if ( CProperty* property = FindProperty( typedObject, propertyName ) )
	{
		CUndoProperty *stepToAdd = undoManager.SafeGetStepToAdd< CUndoProperty >();
		if ( !stepToAdd )
		{
			undoManager.SetStepToAdd( stepToAdd = new CUndoProperty( undoManager ) );
		}

		ASSERT( !stepToAdd->m_initFinished );

		for ( const SHistoryEntry& entry : stepToAdd->m_history )
		{
			if ( entry.m_typedObject.m_object == typedObject.m_object && entry.m_propertyName == propertyName )
			{
				return;
			}
		}

		SHistoryEntry entry( typedObject, property );

		property->Get( entry.m_typedObject.m_object, entry.m_valuePre.Data() );

		stepToAdd->m_history.PushBack( entry );
	}
}

void CUndoProperty::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoProperty *stepToAdd = undoManager.SafeGetStepToAdd< CUndoProperty >() )
	{
		stepToAdd->m_initFinished = true;
		
		// store post-change value
		for ( SHistoryEntry& entry : stepToAdd->m_history )
		{
			if ( CProperty* property = FindProperty( entry.m_typedObject, entry.m_propertyName ) )
			{
				property->Get( entry.m_typedObject.m_object, entry.m_valuePost.Data() );
			}
		}

		stepToAdd->PushStep();
	}
}

CUndoProperty::CUndoProperty( CEdUndoManager& undoManager )
	: IUndoStep( undoManager )
	, m_initFinished( false )
{}

void CUndoProperty::DoStep( Bool undo )
{
	for ( const SHistoryEntry& entry : m_history )
	{
		if ( CProperty* property = FindProperty( entry.m_typedObject, entry.m_propertyName ) )
		{
			if ( CObject* obj = entry.m_typedObject.AsObject() )
			{
				obj->OnPropertyPreChange( property );
			}

			property->Set( entry.m_typedObject.m_object, undo ? entry.m_valuePre.Data() : entry.m_valuePost.Data() );

			if ( CObject* obj = entry.m_typedObject.AsObject() )
			{
				obj->OnPropertyPostChange( property );
			}
		}
	}
}

void CUndoProperty::DoUndo()
{
	DoStep( true );
}

void CUndoProperty::DoRedo()
{
	DoStep( false );
}

void CUndoProperty::OnObjectRemoved( CObject *object )
{
	m_history.Erase(
		RemoveIf( m_history.Begin(), m_history.End(), 
			[ object ]( const SHistoryEntry &h ) { return h.m_typedObject.m_object == object; } ),
		m_history.End()
		);

	if ( m_history.Empty() )
	{
		PopStep();
	}
}

void CUndoProperty::RemoveInvalidProperties()
{
	m_history.Erase(
		RemoveIf( m_history.Begin(), m_history.End(), 
			[]( const SHistoryEntry &h ) { return FindProperty( h.m_typedObject, h.m_propertyName ) == nullptr; } ),
		m_history.End()
		);

	if ( m_history.Empty() )
	{
		PopStep();
	}
}

String CUndoProperty::GetName()
{
	TDynArray<String> names;

	for ( const SHistoryEntry& entry : m_history )
	{
		names.PushBackUnique( entry.m_propertyName.AsString() );
	}

	String name = String::Join( names, TXT(", ") );;
	if ( name.GetLength() > 0 )
		name += TXT(" changed");

	return name;
}

String CUndoProperty::GetTarget()
{
	if ( m_targetString.Empty() )
	{
		THashSet< CObject* > mentionedObjects;

		for ( const SHistoryEntry& entry : m_history )
		{
			// We can only show CObjects
			if ( CObject* obj = entry.m_typedObject.AsObject() )
			{
				// Make sure we mention objects once
				if ( !mentionedObjects.Exist( obj ) )
				{
					mentionedObjects.Insert( obj );
				
					// Insert a comma for the second, third, etc object
					if ( !m_targetString.Empty() )
					{
						m_targetString += TXT(", ");
					}

					// Add the object's name to the string
					m_targetString += obj->GetFriendlyName();
				}
			}
		}
	}

	return m_targetString;
}
