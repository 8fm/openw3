
#include "build.h"
#include "undoTreeEditor.h"

IMPLEMENT_ENGINE_CLASS( CUndoTreeBlockMove );

CUndoTreeBlockMove::CUndoTreeBlockMove( CEdUndoManager& undoManager, CEdTreeEditor* editor )
	: IUndoStep( undoManager )
	, m_editor( editor )
{
}

/*static*/
void CUndoTreeBlockMove::PrepareStep( CEdUndoManager& undoManager, CEdTreeEditor* editor, IScriptable* owner, const wxPoint& offset, Bool alternate )
{
	CUndoTreeBlockMove* step = undoManager.SafeGetStepToAdd< CUndoTreeBlockMove >();
	if ( !step )
	{
		step = new CUndoTreeBlockMove( undoManager, editor );
		undoManager.SetStepToAdd( step );
	}

	if ( Info* info = step->m_infos.FindPtr( owner ) )
	{
		ASSERT ( info->m_alternate == alternate );
		info->m_offset += offset;
	}
	else
	{
		step->m_infos.Insert( owner, Info( offset, alternate ) );
	}
}

/*static*/
void CUndoTreeBlockMove::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoTreeBlockMove* step = undoManager.SafeGetStepToAdd< CUndoTreeBlockMove >() )
	{
		step->PushStep();
	}
}

void CUndoTreeBlockMove::DoStep()
{
	for ( auto infoIt = m_infos.Begin(); infoIt != m_infos.End(); ++infoIt )
	{
		CEdTreeEditor::LayoutInfo* layout;
		if ( m_editor->m_layout.Find( infoIt->m_first, layout ) )
		{
			infoIt->m_second.m_offset = -infoIt->m_second.m_offset;
			m_editor->MoveBlock( layout, infoIt->m_second.m_offset, infoIt->m_second.m_alternate );
		}
		else
		{
			ASSERT( false, TXT("Layout not found") );
		}
	}
}

/*virtual*/
void CUndoTreeBlockMove::DoUndo()
{
	DoStep();
}

/*virtual*/
void CUndoTreeBlockMove::DoRedo()
{
	DoStep();
}

String CUndoTreeBlockMove::GetName()
{
	return TXT("move block");
}

//-------------------

IMPLEMENT_ENGINE_CLASS( CUndoTreeBlockExistance );

CUndoTreeBlockExistance::CUndoTreeBlockExistance( CEdUndoManager& undoManager )
	: IUndoStep( undoManager ) // <- here the UndoManager becomes the parent of the step
{
}

void CUndoTreeBlockExistance::DoPrepareCreationStep( CObject* object )
{
	m_infos.PushBack( Info( object, object->GetParent(), true ) );
}

void CUndoTreeBlockExistance::DoPrepareDeletionStep( CObject* object )
{
	Info info( object, object->GetParent(), false );
	object->SetParent( this );
	m_infos.PushBack( info );
}

void CUndoTreeBlockExistance::SetNameOverride( const String& name )
{
	m_nameOverride = name;
}

void CUndoTreeBlockExistance::DoBlock( Info& info, Bool backInTime )
{
	if ( info.m_created ^ backInTime )
	{
		info.m_object->SetParent( info.m_parent.Get() );
		SetupCustomRelationship( info.m_object, true );
	}
	else
	{
		SetupCustomRelationship( info.m_object, false );
		info.m_object->SetParent( this );
	}
}


void CUndoTreeBlockExistance::DoStep( Bool backInTime )
{
	if ( backInTime )
	{
		for ( Int32 i = static_cast< Int32 >( m_infos.Size()-1 ); i>=0; --i )
		{
			DoBlock( m_infos[i], backInTime );
		}
	}
	else
	{
		for ( Uint32 i=0; i<m_infos.Size(); ++i )
		{
			DoBlock( m_infos[i], backInTime );
		}
	}
}

/*virtual*/
void CUndoTreeBlockExistance::DoUndo()
{
	DoStep( true );
}

/*virtual*/
void CUndoTreeBlockExistance::DoRedo()
{
	DoStep( false );
}

/*virtual*/
String CUndoTreeBlockExistance::GetName()
{
	if ( !m_nameOverride.Empty() )
	{
		return m_nameOverride;
	}
	else
	{
		if ( m_infos.Size() == 1 )
		{
			String name = GetFriendlyBlockName( m_infos[0].m_object );
			if ( name.Empty() )
			{
				name = TXT("node");
			}

			return ( m_infos[0].m_created ? TXT("create ") : TXT("remove ") ) + name;
		}
		else
		{
			Bool hasCreation = false;
			Bool hasDeletion = false;

			for ( Uint32 i=0; i<m_infos.Size(); ++i )
			{
				if ( m_infos[i].m_created )
				{
					hasCreation = true;
				}
				else
				{
					hasDeletion = true;
				}
			}

			if ( hasCreation )
			{
				if ( hasDeletion )
				{
					return TXT("create and remove nodes");
				}
				else
				{
					return TXT("create nodes");
				}
			}
			else
			{
				return TXT("remove nodes");
			}
		}
	}
}

void CUndoTreeBlockExistance::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( Info& info : m_infos )
		{
			file << info.m_object;
		}
	}
}

// ----------------

IMPLEMENT_ENGINE_CLASS( CUndoTreeBlockDecorate );

CUndoTreeBlockDecorate::CUndoTreeBlockDecorate( CEdUndoManager& undoManager, CObject* object, CObject* decorated )
	: IUndoStep( undoManager )
	, m_object( object )
	, m_parent( object->GetParent() )
	, m_decorated( decorated )
{
}

/*virtual*/ 
void CUndoTreeBlockDecorate::DoUndo() /*override*/
{
	SetupCustomRelationship( m_object, false );
	m_object->SetParent( this );

	SetupCustomRelationship( m_decorated, false );
	m_decorated->SetParent( m_parent );
	SetupCustomRelationship( m_decorated, true );
}

/*virtual*/ 
void CUndoTreeBlockDecorate::DoRedo() /*override*/
{
	m_object->SetParent( m_parent );
	SetupCustomRelationship( m_object, true );

	SetupCustomRelationship( m_decorated, false );
	m_decorated->SetParent( m_object );
	SetupCustomRelationship( m_decorated, true );
}

/*virtual*/ 
String CUndoTreeBlockDecorate::GetName() /*override*/
{
	return TXT("decorate");
}

/*virtual*/ 
void CUndoTreeBlockDecorate::OnSerialize( IFile& file ) /*override*/
{
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		file << m_object;
	}
}
