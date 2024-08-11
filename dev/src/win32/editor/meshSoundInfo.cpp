/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "meshSoundInfo.h"
#include "meshPreviewPanel.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/indexed2dArray.h"
#include "../../common/core/gatheredResource.h"

extern CGatheredResource resSoundInfos;

CEdMeshSoundInfo::CEdMeshSoundInfo(wxWindow* parent, CEdMeshPreviewPanel* preview, CMesh* mesh)
	: m_parent( parent )
	, m_preview( preview )
	, m_mesh( mesh )
	, m_default( CName( TXT( "default" ) ) )
{
	m_soundBoneMappingInfo = XRCCTRL( *parent, "SoundAdditionalInfoChoice", wxChoice );
	ASSERT( m_soundBoneMappingInfo );

	m_soundBoneMappingInfo->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdMeshSoundInfo::OnSoundAdditionalInfoChanged ), NULL, this );

	m_createSoundInfo = XRCCTRL( *parent, "CreateSoundInfoButton", wxButton );
	m_createSoundInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshSoundInfo::OnMeshSoundInfoAdd ), NULL, this );

	m_removeSoundInfo = XRCCTRL( *parent, "RemoveSoundInfoButton", wxButton );
	m_removeSoundInfo->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdMeshSoundInfo::OnMeshSoundInfoRemove ), NULL, this );

	m_soundTypeIdentification = XRCCTRL( *parent, "SoundTypeIdentificationChoice", wxTextCtrl );
	m_soundTypeIdentification->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdMeshSoundInfo::OnMeshSoundTypeIdentificationChanged ), NULL, this );

	m_soundSizeIdentification = XRCCTRL( *parent, "SoundSizeIdentificationChoice", wxTextCtrl );
	m_soundSizeIdentification->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdMeshSoundInfo::OnMeshSoundSizeIdentificationChanged ), NULL, this );

	Refresh();
}


CEdMeshSoundInfo::~CEdMeshSoundInfo(void)
{
}

void CEdMeshSoundInfo::Refresh()
{
	if( m_soundBoneMappingInfo )
	{
		m_soundBoneMappingInfo->Clear();
	}
	else
	{
		return;
	}

	if( m_mesh )
	{
		const SMeshSoundInfo* meshSoundInfo = m_mesh->GetMeshSoundInfo();

		m_soundBoneMappingInfo->Enable( meshSoundInfo != 0 );
		m_soundSizeIdentification->Enable( meshSoundInfo != 0 );
		m_soundTypeIdentification->Enable( meshSoundInfo != 0 );
		m_createSoundInfo->Enable( meshSoundInfo == 0 );
		m_removeSoundInfo->Enable( meshSoundInfo != 0 );

		wxString stringNone( "None"  );
		m_soundBoneMappingInfo->Insert(1, &stringNone, 0);
		m_soundBoneMappingInfo->SetSelection( 0 );

		if( !meshSoundInfo )
		{
			wxString stringSSI( m_default.AsChar() );
			m_soundSizeIdentification->SetValue( stringSSI );
			m_soundTypeIdentification->SetValue( stringSSI );
			return;
		}

		THandle< CIndexed2dArray > arr = resSoundInfos.LoadAndGet< CIndexed2dArray >();	

		if (arr)
		{
			Uint32 colNum, rowNum;
			arr->GetSize(colNum,rowNum);
			TDynArray< String > uniqueChoices;

			for ( Uint32 j = 0; j < rowNum; j++ )
			{
				const String& currValue = arr->GetValue( 0, j );
				wxString string( currValue.AsChar() );
				m_soundBoneMappingInfo->Insert( 1, &string, j + 1 ); 
				if( CName( currValue ) == meshSoundInfo->m_soundBoneMappingInfo )
				{
					m_soundBoneMappingInfo->SetSelection( j + 1 );
				}
			}
		}

		wxString stringSSI( meshSoundInfo->m_soundSizeIdentification != CName::NONE ? meshSoundInfo->m_soundSizeIdentification.AsChar() : m_default.AsChar() );
		m_soundSizeIdentification->SetValue( stringSSI );

		wxString stringSTI( meshSoundInfo->m_soundTypeIdentification != CName::NONE ? meshSoundInfo->m_soundTypeIdentification.AsChar() : m_default.AsChar() );
		m_soundTypeIdentification->SetValue( stringSTI );
	}
}


void CEdMeshSoundInfo::OnSoundAdditionalInfoChanged( wxCommandEvent& event )
{
	if( m_mesh )
	{
		SMeshSoundInfo* meshSoundInfo = m_mesh->GetMeshSoundInfo();
		if( meshSoundInfo )
		{
			wxString selectedString = m_soundBoneMappingInfo->GetStringSelection();

			meshSoundInfo->m_soundBoneMappingInfo = selectedString != "" ? CName( selectedString.wc_str( ) ) : CName::NONE ;
		}
	}
}

void CEdMeshSoundInfo::OnMeshSoundTypeIdentificationChanged( wxCommandEvent& event )
{
	if( m_mesh )
	{
		SMeshSoundInfo* meshSoundInfo = m_mesh->GetMeshSoundInfo();
		if( meshSoundInfo )
		{
			wxString selectedString = m_soundTypeIdentification->GetValue();

			meshSoundInfo->m_soundTypeIdentification = selectedString != "" ? CName( selectedString.wc_str( ) ) : m_default ;
		}
	}
}

void CEdMeshSoundInfo::OnMeshSoundSizeIdentificationChanged( wxCommandEvent& event )
{
	if( m_mesh )
	{
		SMeshSoundInfo* meshSoundInfo = m_mesh->GetMeshSoundInfo();
		if( meshSoundInfo )
		{
			wxString selectedString = m_soundSizeIdentification->GetValue();

			meshSoundInfo->m_soundSizeIdentification = selectedString != "" ? CName( selectedString.wc_str( ) ) : m_default ;
		}
	}
}

void CEdMeshSoundInfo::OnMeshSoundInfoAdd( wxCommandEvent& event )
{
	if( m_mesh )
	{
		SMeshSoundInfo* meshSoundInfo = new SMeshSoundInfo();
		m_mesh->SetMeshSoundInfo( meshSoundInfo );

		Refresh();
	}
}

void CEdMeshSoundInfo::OnMeshSoundInfoRemove( wxCommandEvent& event )
{
	if( m_mesh )
	{
		m_mesh->SetMeshSoundInfo( nullptr );

		Refresh();
	}
}