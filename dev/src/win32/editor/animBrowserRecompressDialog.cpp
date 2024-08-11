#include "build.h"
#include "animBrowserRecompressDialog.h"


BEGIN_EVENT_TABLE( CAnimBrowserRecompressDialog, wxDialog )
	EVT_BUTTON( XRCID("RecompressBtn"), CAnimBrowserRecompressDialog::OnRecompress )
	EVT_BUTTON( XRCID("CancelBtn"), CAnimBrowserRecompressDialog::OnCancel )
	EVT_LISTBOX_DCLICK( XRCID( "RecompressionSettingsList"), CAnimBrowserRecompressDialog::OnRecompress )
END_EVENT_TABLE()

CAnimBrowserRecompressDialog::CAnimBrowserRecompressDialog( wxWindow* parent, SAnimationBufferBitwiseCompressionPreset preset )
: m_preset(preset)
, m_recompress(false)
{
	wxXmlResource::Get()->LoadDialog( this, parent, wxT("ChooseRecompressionPresetDialog") );

	m_presetList = XRCCTRL( *this, "RecompressionSettingsList", wxListBox );

	SetMinSize( wxSize( 300, 200 ) );
	SetSize( wxSize( 300, 200 ) );

	FillList();

	Layout();
}

CAnimBrowserRecompressDialog::~CAnimBrowserRecompressDialog()
{
}

void CAnimBrowserRecompressDialog::OnRecompress( wxCommandEvent& event )
{
	Int32 selection = m_presetList->GetSelection();
	if ( selection != wxNOT_FOUND )
	{
		m_recompress = true;
		m_preset = (SAnimationBufferBitwiseCompressionPreset)( (Int32)( m_presetList->GetClientData( selection ) ) );
		EndDialog(1);
	}
}

void CAnimBrowserRecompressDialog::OnCancel( wxCommandEvent& event )
{
	m_recompress = false;

	EndDialog(0);
}

void CAnimBrowserRecompressDialog::FillList()
{
	m_presetList->Freeze();
	m_presetList->Clear();

	Int32 selected = -1;
	if ( CEnum* presets = SRTTI::GetInstance().FindEnum(CName(TXT("SAnimationBufferBitwiseCompressionPreset"))) )
	{
		const TDynArray< CName >& options = presets->GetOptions();
		for ( auto iOption = options.Begin(); iOption != options.End(); ++ iOption )
		{
			Int32 value;
			if ( presets->FindValue( *iOption, value ) )
			{
				if ( value != ABBCP_Custom )
				{
					Int32 appended = m_presetList->Append( iOption->AsChar(), (void*)value );
					if ( value == m_preset )
					{
						selected = appended;
					}
				}
			}
		}
	}

	m_presetList->Thaw();

	if ( selected != -1 )
	{
		m_presetList->Select( selected );
	}
}
