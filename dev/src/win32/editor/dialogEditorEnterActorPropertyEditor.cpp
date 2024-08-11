/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "dialogEditorEnterActorPropertyEditor.h"

CEdDialogEditorEnterActorPropertyEditor::CEdDialogEditorEnterActorPropertyEditor( CPropertyItem* propertyItem )
	: ISelectionEditor( propertyItem )
{

}

CEdDialogEditorEnterActorPropertyEditor::~CEdDialogEditorEnterActorPropertyEditor()
{

}

void CEdDialogEditorEnterActorPropertyEditor::FillChoices()
{
	// Fill tree with entries from dialog timeline
	m_ctrlChoice->AppendString( TXT( "auto" ) );
	m_ctrlChoice->AppendString( TXT( "enter_behind" ) );
	m_ctrlChoice->AppendString( TXT( "enter_right" ) );
	m_ctrlChoice->AppendString( TXT( "enter_left" ) );
	m_ctrlChoice->AppendString( TXT( "run_behind" ) );
}

///////////////////////////////////////////////////////////////////////////////////

CEdDialogEditorExitActorPropertyEditor::CEdDialogEditorExitActorPropertyEditor( CPropertyItem* propertyItem )
: ISelectionEditor( propertyItem )
{

}

CEdDialogEditorExitActorPropertyEditor::~CEdDialogEditorExitActorPropertyEditor()
{

}

void CEdDialogEditorExitActorPropertyEditor::FillChoices()
{
	// Fill tree with entries from dialog timeline
	m_ctrlChoice->AppendString( TXT( "auto" ) );
	m_ctrlChoice->AppendString( TXT( "exit_behind" ) );
	m_ctrlChoice->AppendString( TXT( "exit_right" ) );
	m_ctrlChoice->AppendString( TXT( "exit_left" ) );
}
