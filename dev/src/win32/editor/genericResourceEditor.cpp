/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "genericResourceEditor.h"

CEdGenericResourceEditor::CEdGenericResourceEditor( wxWindow* parent, CResource* resource )
	: CEdPropertiesFrame( parent, resource->GetFriendlyName(), nullptr )
	, m_resource( resource )
{
	SetObject( resource );
	resource->AddToRootSet();

	m_browser->Get().Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdGenericResourceEditor::OnPropertiesChanged ), NULL, this );
}

CEdGenericResourceEditor::~CEdGenericResourceEditor()
{
	if ( m_resource->IsModified() )
	{
		if ( wxMessageBox( TXT("Resource has been modified. Do you wish to save it?"), TXT("Modified resource"), wxYES_NO | wxCENTRE ) == wxYES )
		{
			m_resource->Save();
		}
	}

	m_resource->RemoveFromRootSet();
}

void CEdGenericResourceEditor::OnPropertiesChanged( wxCommandEvent& event )
{
	m_resource->MarkModified();
}


