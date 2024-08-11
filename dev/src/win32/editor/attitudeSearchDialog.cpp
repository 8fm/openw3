/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "Build.h"
#include "attitudeSearchDialog.h"
#include "../../common/core/gatheredResource.h"

BEGIN_EVENT_TABLE( CEdAttitudeSearchlDialog, wxDialog )
	EVT_BUTTON(		XRCID("m_search"),		CEdAttitudeSearchlDialog::OnSearchClicked )
END_EVENT_TABLE()

CEdAttitudeSearchlDialog::CEdAttitudeSearchlDialog( wxCommandEvent& event )
	 : m_event( event )
{
	wxXmlResource::Get()->LoadDialog( this, wxTheFrame->GetAssetBrowser(), wxT("AttitudeGroupSearch") );

	m_attitudeList = XRCCTRL( *this, "attitudeList", wxChoice );

	const C2dArray& attitudeGroupsArray = SAttitudesResourcesManager::GetInstance().Get2dArray();

	const Uint32 &numRows = attitudeGroupsArray.GetNumberOfRows();
	wxArrayString choices;
	for ( Uint32 i = 0; i < numRows; ++i )
	{
		choices.Add( attitudeGroupsArray.GetValue( 0, i ).AsChar() );
	}

	m_attitudeList->Append( choices );

	ShowModal();
}

CEdAttitudeSearchlDialog::~CEdAttitudeSearchlDialog()
{
}

void CEdAttitudeSearchlDialog::OnSearchClicked( wxCommandEvent& event )
{
	String selectedAttitude = m_attitudeList->GetStringSelection();

	if ( selectedAttitude != TXT("") )
	{
		wxTheFrame->GetAssetBrowser()->SearchAttitudeGroup( m_event, CName(selectedAttitude) );
		EndModal( XRCID("m_search") );
	}
}
