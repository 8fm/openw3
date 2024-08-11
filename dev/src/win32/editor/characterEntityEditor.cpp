/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/tokenzr.h>
#include "characterEntityEditor.h"
#include "../../common/engine/appearanceComponent.h"
#include "../../common/engine/mesh.h"
#include "../../common/core/depot.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/hitProxyMap.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/meshComponent.h"
#include "../../common/engine/characterEntityTemplate.h"
#include "../../common/engine/worldTick.h"
#include "../../common/engine/renderFrame.h"
#include "../../common/engine/renderProxy.h"
#include "../../common/engine/renderCommands.h"
#include "meshStats.h"


enum {
	ID_POPUP_SET_COLOR = 1000,
	ID_POPUP_COLLAPSE,
	ID_POPUP_OPEN_ENTITY,
	ID_POPUP_OPEN_MESH,
	ID_POPUP_DEFAULT_LOD,
	ID_POPUP_FORCE_LOD0,
	ID_POPUP_FORCE_LOD1,
	ID_POPUP_FORCE_LOD2,
	ID_POPUP_FORCE_LOD3,
	ID_POPUP_REFRESH_CHARACTER,
	ID_POPUP_SET_BASE_ENTITY_OVERRIDE,
	ID_POPUP_CLEAR_BASE_ENTITY_OVERRIDE,
	ID_POPUP_EXPORT_ENTITY_TEMPLATE
};

BEGIN_EVENT_TABLE(CEdCharacterEntityEditor, wxSmartLayoutPanel)
	EVT_MENU( XRCID("fileSaveMenuItem"), CEdCharacterEntityEditor::OnFileSaveMenuItem )
	EVT_MENU( XRCID("fileReloadMenuItem"), CEdCharacterEntityEditor::OnFileReloadMenuItem )
	EVT_MENU( XRCID("fileExportEntityTemplateMenuItem"), CEdCharacterEntityEditor::OnFileExportEntityTemplateMenuItem )
	EVT_MENU( XRCID("fileCloseMenuItem"), CEdCharacterEntityEditor::OnFileCloseMenuItem )
END_EVENT_TABLE()

class CEdCharacterEntityEditorConfiguration
{
	struct Type
	{
		String name;
		String tplname;
	};

	struct Base
	{
		String name;
		String group;
		TDynArray<String> paths;
		THashMap<Char,String> parts;
		THashMap<String,Type> types, nametypes;
	};

	TDynArray<Base>			m_bases;
	THashMap<String,Uint32>	m_basemap;

	void LoadXMLBases( wxXmlNode* bases );
	void LoadXMLGroups( wxXmlNode* groups );
	void LoadXML( const wxString& filename );

public:
	CEdCharacterEntityEditorConfiguration();

	void Reload();

	String TranslateBodypart( Uint32 baseIndex, Char ch ) const;
	const String& TranslateType( Uint32 baseIndex, const String& code ) const;
	const String& GetTypeTemplateName( Uint32 baseIndex, const String& name ) const;

	Uint32 GetBaseCount() const;
	const String& GetBaseName( Uint32 index ) const;
	const TDynArray<String>& GetBasePaths( Uint32 index ) const;
	Uint32 FindBase( const String& name ) const;
};

CEdCharacterEntityEditorConfiguration::CEdCharacterEntityEditorConfiguration()
{
	Reload();
}

void CEdCharacterEntityEditorConfiguration::LoadXMLBases( wxXmlNode* bases )
{
	for ( wxXmlNode* child=bases->GetChildren(); child; child=child->GetNext() )
	{
		if ( child->GetName() != wxT("Base") ) continue;

		Base base;
		base.name = child->GetAttribute( wxT("Name") ).wc_str();

		for ( wxXmlNode* sub=child->GetChildren(); sub; sub=sub->GetNext() )
		{
			if ( sub->GetName() == wxT("Dir") )
			{
				if ( sub->GetAttribute( wxT("Scan") ) == wxT("yes") )
				{
					CDirectory* dir = GDepot->FindPath( sub->GetAttribute( wxT("Path") ) );
					if ( dir )
					{
						for ( CDirectory* subdir : dir->GetDirectories() )
						{
							base.paths.PushBack( subdir->GetDepotPath() );
						}
					}
				}
				else
				{
					base.paths.PushBack( sub->GetAttribute( wxT("Path") ).wc_str() );
				}
			}
			else if ( sub->GetName() == wxT("Part") )
			{
				base.parts.Insert( sub->GetAttribute( wxT("Character") )[0], sub->GetAttribute( wxT("Name") ).wc_str() );
			}
			else if ( sub->GetName() == wxT("Type") )
			{
				Type type;
				type.name = sub->GetAttribute( wxT("Name") ).wc_str();
				type.tplname = sub->GetAttribute( wxT("Template") ).wc_str();
				base.types.Insert( sub->GetAttribute( wxT("Code") ).wc_str(), type );
				base.nametypes.Insert( type.name, type );
			}
		}

		m_basemap[base.name] = m_bases.Size();
		m_bases.PushBack( base );
	}
}

void CEdCharacterEntityEditorConfiguration::LoadXML( const wxString& filename )
{
	wxXmlDocument doc( filename );

	m_bases.Clear();
	
	wxXmlNode* root = doc.GetRoot();
	for ( wxXmlNode* rootChild=root->GetChildren(); rootChild; rootChild=rootChild->GetNext() )
	{
		if ( rootChild->GetName() == wxT("Bases") )
		{
			LoadXMLBases( rootChild );
		}
	}
}

void CEdCharacterEntityEditorConfiguration::Reload()
{
	CDiskFile* xmlfile = GDepot->FindFile( TXT("engine\\templates\\editor\\charactereditorconfig.xml") );
	if ( xmlfile )
	{
		String fullpath = xmlfile->GetAbsolutePath();
		LoadXML( fullpath.AsChar() );
	}
}

String CEdCharacterEntityEditorConfiguration::TranslateBodypart( Uint32 baseIndex, Char ch ) const
{
	const Base& base = m_bases[baseIndex];
	if ( base.parts.KeyExist( ch ) )
	{
		return base.parts[ ch ];
	}
	return String::Chr( ch );
}

const String& CEdCharacterEntityEditorConfiguration::TranslateType( Uint32 baseIndex, const String& code ) const
{
	const Base& base = m_bases[baseIndex];
	if ( base.types.KeyExist( code ) )
	{
		return base.types[ code ].name;
	}
	return code;
}

const String& CEdCharacterEntityEditorConfiguration::GetTypeTemplateName( Uint32 baseIndex, const String& name ) const
{
	const Base& base = m_bases[baseIndex];
	if ( base.nametypes.KeyExist( name ) )
	{
		return base.nametypes[ name ].tplname;
	}
	return String::EMPTY;
}

Uint32 CEdCharacterEntityEditorConfiguration::GetBaseCount() const
{
	return m_bases.Size();
}

const String& CEdCharacterEntityEditorConfiguration::GetBaseName( Uint32 index ) const
{
	return m_bases[ index ].name;
}

const TDynArray<String>& CEdCharacterEntityEditorConfiguration::GetBasePaths( Uint32 index ) const
{
	return m_bases[ index ].paths;
}

Uint32 CEdCharacterEntityEditorConfiguration::FindBase( const String& name ) const
{
	if ( m_basemap.KeyExist( name ) )
	{
		return m_basemap[name];
	}
	return UINT_MAX;
}

//////////////////////////////////////////////////////////////////////////

CEdColorShiftPairEditorPopup::CEdColorShiftPairEditorPopup( wxWindow* parent, const wxString& title, const CColorShift& colorShift1, const CColorShift& colorShift2 )
	: wxDialog( parent, wxID_ANY, title )
{
	wxBoxSizer* sizer;
	wxButton* btn;

	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxFlexGridSizer* fgSizer1;
	fgSizer1 = new wxFlexGridSizer( 0, 2, 0, 0 );
	fgSizer1->AddGrowableCol( 1 );
	fgSizer1->SetFlexibleDirection( wxBOTH );
	fgSizer1->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );

	m_primaryColorLabel = new wxStaticText( this, wxID_ANY, wxT("Primary Color [Red channel]:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_primaryColorLabel->Wrap( -1 );
	m_primaryColorLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

	fgSizer1->Add( m_primaryColorLabel, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 10 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	sizer = new wxBoxSizer( wxHORIZONTAL );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Copy"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnCopy1Click ), NULL, this );
	sizer->Add( btn, 0, wxTOP|wxBOTTOM, 10 );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Paste"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnPaste1Click ), NULL, this );
	sizer->Add( btn, 0, wxALL, 10 );
	m_panel1->Layout();
	m_panel1->SetSizer( sizer );
	sizer->Fit( m_panel1 );
	fgSizer1->Add( m_panel1, 1, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );


	m_primaryColorHueShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Hue Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_primaryColorHueShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_primaryColorHueShiftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_primaryColorHueShiftSlider = new wxSlider( this, wxID_ANY, 0, 0, 360, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_primaryColorHueShiftSlider, 0, wxALIGN_CENTER_VERTICAL|wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_primaryColorSaturationShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Saturation Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_primaryColorSaturationShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_primaryColorSaturationShiftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_primaryColorSaturationShiftSlider = new wxSlider( this, wxID_ANY, 0, -100, 100, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_primaryColorSaturationShiftSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_primaryColorLuminanceShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Luminance Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_primaryColorLuminanceShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_primaryColorLuminanceShiftLabel, 0, wxLEFT|wxALIGN_CENTER_VERTICAL, 20 );

	m_primaryColorLuminanceShiftSlider = new wxSlider( this, wxID_ANY, 0, -100, 100, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_primaryColorLuminanceShiftSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_secondaryColorLabel = new wxStaticText( this, wxID_ANY, wxT("Secondary Color [Blue channel]:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_secondaryColorLabel->Wrap( -1 );
	m_secondaryColorLabel->SetFont( wxFont( wxNORMAL_FONT->GetPointSize(), 70, 90, 92, false, wxEmptyString ) );

	fgSizer1->Add( m_secondaryColorLabel, 0, wxALL|wxALIGN_CENTRE_VERTICAL, 10 );

	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	sizer = new wxBoxSizer( wxHORIZONTAL );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Copy"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnCopy2Click ), NULL, this );
	sizer->Add( btn, 0, wxTOP|wxBOTTOM, 10 );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Paste"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnPaste2Click ), NULL, this );
	sizer->Add( btn, 0, wxALL, 10 );
	m_panel1->Layout();
	m_panel1->SetSizer( sizer );
	sizer->Fit( m_panel1 );
	fgSizer1->Add( m_panel1, 1, wxALIGN_RIGHT|wxALIGN_CENTRE_VERTICAL|wxRIGHT|wxTOP|wxBOTTOM, 5 );

	m_secondaryColorHueShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Hue Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_secondaryColorHueShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_secondaryColorHueShiftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_secondaryColorHueShiftSlider = new wxSlider( this, wxID_ANY, 0, 0, 360, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_secondaryColorHueShiftSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_secondaryColorSaturationShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Saturation Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_secondaryColorSaturationShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_secondaryColorSaturationShiftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_secondaryColorSaturationShiftSlider = new wxSlider( this, wxID_ANY, 0, -100, 100, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_secondaryColorSaturationShiftSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	m_secondaryColorLuminanceShiftLabel = new wxStaticText( this, wxID_ANY, wxT("Luminance Shift:"), wxDefaultPosition, wxDefaultSize, 0 );
	m_secondaryColorLuminanceShiftLabel->Wrap( -1 );
	fgSizer1->Add( m_secondaryColorLuminanceShiftLabel, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 20 );

	m_secondaryColorLuminanceShiftSlider = new wxSlider( this, wxID_ANY, 0, -100, 100, wxDefaultPosition, wxSize( 200, -1 ), wxSL_BOTH|wxSL_HORIZONTAL );
	fgSizer1->Add( m_secondaryColorLuminanceShiftSlider, 0, wxEXPAND|wxLEFT|wxRIGHT, 5 );

	
	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	sizer = new wxBoxSizer( wxHORIZONTAL );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Copy"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnCopyAllClick ), NULL, this );
	sizer->Add( btn, 0, wxALL, 10 );
	btn = new wxButton( m_panel1, wxID_ANY, wxT("Paste"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnPasteAllClick ), NULL, this );
	sizer->Add( btn, 0, wxTOP|wxBOTTOM, 10 );
	m_panel1->Layout();
	m_panel1->SetSizer( sizer );
	sizer->Fit( m_panel1 );
	fgSizer1->Add( m_panel1, 1, wxALIGN_LEFT|wxLEFT|wxTOP|wxBOTTOM|wxEXPAND, 5 );


	m_panel1 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );


	btn = new wxButton( m_panel1, wxID_ANY, wxT("Swap"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSwapClick ), NULL, this );
	bSizer2->Add( btn, 0, wxTOP|wxBOTTOM, 10 );
	m_restoreButton = new wxButton( m_panel1, wxID_ANY, wxT("Restore"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_restoreButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnRestoreClick ), NULL, this );
	bSizer2->Add( m_restoreButton, 0, wxTOP|wxLEFT|wxBOTTOM, 10 );
	m_resetButton = new wxButton( m_panel1, wxID_ANY, wxT("Reset"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	m_resetButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnResetClick ), NULL, this );
	bSizer2->Add( m_resetButton, 0, wxALL, 10 );

	m_panel1->SetSizer( bSizer2 );
	m_panel1->Layout();
	bSizer2->Fit( m_panel1 );
	fgSizer1->Add( m_panel1, 1, wxALIGN_RIGHT|wxALL, 5 );
	
	SetColorShifts( colorShift1, colorShift2 );

	StoreInitial();
	
	m_primaryColorHueShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 
	m_primaryColorSaturationShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 
	m_primaryColorLuminanceShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 
	m_secondaryColorHueShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 
	m_secondaryColorSaturationShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 
	m_secondaryColorLuminanceShiftSlider->Connect( wxEVT_SCROLL_THUMBTRACK, wxCommandEventHandler( CEdColorShiftPairEditorPopup::OnSliderChange ), NULL, this ); 

	SetSizer( fgSizer1 );
	SetMinClientSize( wxSize( 450, 100 ) );
	Fit();
	Layout();
	Centre( wxBOTH );

	LayoutRecursively( this );
}

wxString CEdColorShiftPairEditorPopup::ColorShiftToString( const CColorShift& colorShift )
{
	return wxString::Format( wxT("%d,%d,%d"), colorShift.m_hue, colorShift.m_saturation, colorShift.m_luminance );
}

CColorShift CEdColorShiftPairEditorPopup::StringToColorShift( const wxString& string )
{
	wxArrayString tokens = wxStringTokenize( string, TXT(",") );
	CColorShift shift;
	long l;
	if ( tokens.size() > 0 && tokens[0].ToLong( &l ) ) shift.m_hue = l;
	if ( tokens.size() > 1 && tokens[1].ToLong( &l ) ) shift.m_saturation = l;
	if ( tokens.size() > 2 && tokens[2].ToLong( &l ) ) shift.m_luminance = l;
	return shift;
}

void CEdColorShiftPairEditorPopup::ExtractBothColorShiftsFromString( const wxString& string, CColorShift& colorShift1, CColorShift& colorShift2 )
{
	wxArrayString tokens = wxStringTokenize( string, TXT("|") );
	if ( tokens.size() > 0 ) colorShift1 = StringToColorShift( tokens[0] );
	if ( tokens.size() > 1 ) colorShift2 = StringToColorShift( tokens[1] );
}

void CEdColorShiftPairEditorPopup::DoCopyColorShiftsToClipboard( CColorShift* color1, CColorShift* color2, wxWindow* parent )
{
	wxString code;
	if ( color1 && color2 )
	{
		code = wxString( wxT("ColorShifts:B=") ) + ColorShiftToString( *color1 ) + wxT("|") + ColorShiftToString( *color2 );
	}
	else if ( color1 )
	{
		code = wxString( wxT("ColorShifts:P=") ) + ColorShiftToString( *color1 );
	}
	else if ( color2 )
	{
		code = wxString( wxT("ColorShifts:S=") ) + ColorShiftToString( *color2 );
	}

	if ( wxTheClipboard->Open() )
	{
		wxTheClipboard->SetData( new wxTextDataObject( code ) );
		wxTheClipboard->Close();
		wxTheClipboard->Flush();
	}
	else
	{
		wxMessageBox( wxT("Failed to copy the color shifts to clipboard"), wxT("Cannot open the clipboard"), wxICON_ERROR|wxCENTRE|wxOK, parent );
	}
}

void CEdColorShiftPairEditorPopup::CopyColorShiftsToClipboard( bool copy1, bool copy2 )
{
	CColorShift shift1, shift2;
	ASSERT( copy1 || copy2, TXT("At least one color shift must be copied") );

	GetColorShifts( shift1, shift2 );

	if ( copy1 && copy2 )
	{
		DoCopyColorShiftsToClipboard( &shift1, &shift2, this );
	}
	else if ( copy1)
	{
		DoCopyColorShiftsToClipboard( &shift1, nullptr, this );
	}
	else
	{
		DoCopyColorShiftsToClipboard( nullptr, &shift2, this );
	}
}

bool CEdColorShiftPairEditorPopup::PasteColorShiftsFromClipboard( CColorShift& colorShift1, CColorShift& colorShift2, bool& pasted1, bool& pasted2, wxWindow* parent )
{
	wxTextDataObject data;
	wxString code;

	if ( !wxTheClipboard->Open() )
	{
		wxMessageBox( wxT("Failed to paste the color shifts from clipboard"), wxT("Cannot open the clipboard"), wxICON_ERROR|wxCENTRE|wxOK, parent );
		return false;
	}
	if ( !wxTheClipboard->GetData( data ) )
	{
		wxTheClipboard->Close();
		wxMessageBox( wxT("Failed to get the color shifts data from clipboard"), wxT("Cannot get clipboard data"), wxICON_ERROR|wxCENTRE|wxOK, parent );
		return false;
	}
	code = data.GetText();
	wxTheClipboard->Close();

	if ( code.Left( 14 ) != wxT("ColorShifts:P=") &&
		 code.Left( 14 ) != wxT("ColorShifts:S=") &&
		 code.Left( 14 ) != wxT("ColorShifts:B=") )
	{
		wxMessageBox( wxT("Invalid data in the clipboard"), wxT("Invalid data"), wxICON_ERROR|wxCENTRE|wxOK, parent );
		return false;
	}

	if ( code[12] == L'P' || code[12] == L'S' )
	{
		CColorShift shift = StringToColorShift( code.SubString( 14, code.Length() ) );
		pasted1 = code[12] == L'P';
		colorShift1 = code[12] == L'P' ? shift : CColorShift();
		pasted2 = code[12] == L'S';
		colorShift2 = code[12] == L'S' ? shift : CColorShift();
	}
	else
	{
		ExtractBothColorShiftsFromString( code.SubString( 14, code.Length() ), colorShift1, colorShift2 );
		pasted1 = pasted2 = true;
	}

	return pasted1 || pasted2;
}

void CEdColorShiftPairEditorPopup::OnSliderChange( wxCommandEvent& event )
{
	if ( m_hook )
	{
		CColorShift shift1, shift2;
		GetColorShifts( shift1, shift2 );
		m_hook->OnColorShiftPairModify( this, shift1, shift2 );
	}
}

void CEdColorShiftPairEditorPopup::OnResetClick( wxCommandEvent& event )
{
	SetColorShifts( CColorShift(), CColorShift() );
}

void CEdColorShiftPairEditorPopup::OnRestoreClick( wxCommandEvent& event )
{
	SetColorShifts( m_initialShift1, m_initialShift2 );
}

void CEdColorShiftPairEditorPopup::OnSwapClick( wxCommandEvent& event )
{
	CColorShift shift1, shift2;
	GetColorShifts( shift1, shift2 );
	SetColorShifts( shift2, shift1 );
}

void CEdColorShiftPairEditorPopup::OnCopy1Click( wxCommandEvent& event )
{
	CopyColorShiftsToClipboard( true, false );
}

void CEdColorShiftPairEditorPopup::OnPaste1Click( wxCommandEvent& event )
{
	CColorShift shift1, shift2, pshift1, pshift2;
	bool pasted1, pasted2;

	GetColorShifts( shift1, shift2 );

	if ( PasteColorShiftsFromClipboard( pshift1, pshift2, pasted1, pasted2 ) )
	{
		shift1 = pasted1 ? pshift1 : pshift2;
		SetColorShifts( shift1, shift2 );
	}
}

void CEdColorShiftPairEditorPopup::OnCopy2Click( wxCommandEvent& event )
{
	CopyColorShiftsToClipboard( false, true );
}

void CEdColorShiftPairEditorPopup::OnPaste2Click( wxCommandEvent& event )
{
	CColorShift shift1, shift2, pshift1, pshift2;
	bool pasted1, pasted2;

	GetColorShifts( shift1, shift2 );

	if ( PasteColorShiftsFromClipboard( pshift1, pshift2, pasted1, pasted2 ) )
	{
		shift2 = pasted2 ? pshift2 : pshift1;
		SetColorShifts( shift1, shift2 );
	}
}

void CEdColorShiftPairEditorPopup::OnCopyAllClick( wxCommandEvent& event )
{
	CopyColorShiftsToClipboard( true, true );
}

void CEdColorShiftPairEditorPopup::OnPasteAllClick( wxCommandEvent& event )
{
	CColorShift shift1, shift2, pshift1, pshift2;
	bool pasted1, pasted2;

	GetColorShifts( shift1, shift2 );

	if ( PasteColorShiftsFromClipboard( pshift1, pshift2, pasted1, pasted2 ) )
	{
		if ( pasted1 ) shift1 = pshift1;
		if ( pasted2 ) shift2 = pshift2;
		SetColorShifts( shift1, shift2 );
	}
}

void CEdColorShiftPairEditorPopup::StoreInitial()
{
	GetColorShifts( m_initialShift1, m_initialShift2 );
}

void CEdColorShiftPairEditorPopup::SetColorShifts( const CColorShift& colorShift1, const CColorShift& colorShift2 )
{
	m_primaryColorHueShiftSlider->SetValue( colorShift1.m_hue );
	m_primaryColorSaturationShiftSlider->SetValue( colorShift1.m_saturation );
	m_primaryColorLuminanceShiftSlider->SetValue( colorShift1.m_luminance );
	m_secondaryColorHueShiftSlider->SetValue( colorShift2.m_hue );
	m_secondaryColorSaturationShiftSlider->SetValue( colorShift2.m_saturation );
	m_secondaryColorLuminanceShiftSlider->SetValue( colorShift2.m_luminance );
	if ( m_hook )
	{
		m_hook->OnColorShiftPairModify( this, colorShift1, colorShift2 );
	}
}

void CEdColorShiftPairEditorPopup::GetColorShifts( CColorShift& colorShift1, CColorShift& colorShift2 ) const
{
	colorShift1.m_hue = m_primaryColorHueShiftSlider->GetValue();
	colorShift1.m_saturation = m_primaryColorSaturationShiftSlider->GetValue();
	colorShift1.m_luminance = m_primaryColorLuminanceShiftSlider->GetValue();
	colorShift2.m_hue = m_secondaryColorHueShiftSlider->GetValue();
	colorShift2.m_saturation = m_secondaryColorSaturationShiftSlider->GetValue();
	colorShift2.m_luminance = m_secondaryColorLuminanceShiftSlider->GetValue();
}

void CEdColorShiftPairEditorPopup::SetHook( IEdColorShiftPairEditorPopupHook* hook )
{
	m_hook = hook;
}

//////////////////////////////////////////////////////////////////////////

class CEdCharacterEntityEditorGridHook : public IEdIconGridHook
{
	CEdCharacterEntityEditor* m_editor;

public:
	CEdCharacterEntityEditorGridHook( CEdCharacterEntityEditor* editor )
		: m_editor( editor )
	{
	}

	virtual void OnIconGridSelectionChange( class CEdIconGrid* grid, Bool primary )
	{
		m_editor->OnIconGridSelectionChange();
	}
};


//////////////////////////////////////////////////////////////////////////

class CEdCharacterEntityEditorPreviewPanel : public CEdPreviewPanel
{
	CEdCharacterEntityEditor*	m_editor;

protected:
	virtual void HandleSelection( const TDynArray< CHitProxyObject* >& object )
	{
		if ( object.Size() < 1 ) return;

		CMeshComponent* component = Cast<CMeshComponent>( object[0]->GetHitObject() );
		if ( component )
		{
			m_editor->HandleSelectionOfMeshComponent( component );
		}
	}
	
	virtual void HandleContextMenu( Int32 x, Int32 y )
	{
		CHitProxyMap map;
		CHitProxyObject* object = GetHitProxyAtPoint( map, x, y );

		wxMenu menu;
		
		if ( object && object->GetHitObject() && object->GetHitObject()->IsA<CMeshTypeComponent>() )
		{
			m_editor->HandleSelectionOfMeshComponent( (CMeshTypeComponent*)object->GetHitObject() );
			menu.SetClientData( object->GetHitObject() );
			menu.Append( ID_POPUP_SET_COLOR, wxT("Set part color...") );
			menu.Append( ID_POPUP_COLLAPSE, wxT("Toggle collapse") );
			menu.AppendSeparator();
			menu.Append( ID_POPUP_OPEN_ENTITY, wxT("Open part entity...") );
			menu.Append( ID_POPUP_OPEN_MESH, wxT("Open part mesh...") );
			menu.AppendSeparator();
		}
		else
		{
			menu.SetClientData( NULL );
		}

		menu.Append( ID_POPUP_DEFAULT_LOD, wxT("Default LOD") );
		menu.Append( ID_POPUP_FORCE_LOD0, wxT("Force LOD 0") );
		menu.Append( ID_POPUP_FORCE_LOD1, wxT("Force LOD 1") );
		menu.Append( ID_POPUP_FORCE_LOD2, wxT("Force LOD 2") );
		menu.Append( ID_POPUP_FORCE_LOD3, wxT("Force LOD 3") );
		menu.AppendSeparator();

		menu.Append( ID_POPUP_REFRESH_CHARACTER, wxT("Refresh character") );
		menu.AppendSeparator();

		menu.Append( ID_POPUP_SET_BASE_ENTITY_OVERRIDE, wxT("Set base entity override") );
		menu.Append( ID_POPUP_CLEAR_BASE_ENTITY_OVERRIDE, wxT("Clear base entity override") );
		menu.AppendSeparator();

		menu.Append( ID_POPUP_EXPORT_ENTITY_TEMPLATE, wxT("Export entity template") );
		menu.Connect( wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&CEdCharacterEntityEditor::OnPreviewPopupMenuSelected, NULL, m_editor );
		PopupMenu( &menu, x, y );
	}

	
	virtual void OnViewportGenerateFragments( IViewport *view, CRenderFrame *frame )
	{
		if ( ShouldDrawGrid() )
		{
			m_renderingWindow->GetViewport()->SetRenderingMask( SHOW_Grid );
		}
		else
		{
			m_renderingWindow->GetViewport()->ClearRenderingMask( SHOW_Grid );
		}

		IRenderScene* scene = m_previewWorld->GetRenderSceneEx();
		if ( scene )
		{
			SceneRenderingStats stats = scene->GetRenderStats();

			Uint32 x = 30;

			// Draw rendering stats
			if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_TriangleStats ) )
			{
				// Start at bottom
				Uint32 y = frame->GetFrameOverlayInfo().m_height - 130;

				//dex++:	slightly changed
				//kk		even more slightly changed
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene triangles: %i"), stats.m_numSceneTriangles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene chunks: %i"), stats.m_numSceneChunks ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Scene vertices: %i"), stats.m_numSceneVerts ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;			
				y += 20;

				frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle emitters: %i"), stats.m_numParticleEmitters ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle count: %i"), stats.m_numParticles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle mesh chunks: %i"), stats.m_numParticleMeshChunks ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				frame->AddDebugScreenText( x, y, String::Printf( TXT("Particle mesh triangles: %i"), stats.m_numParticleMeshTriangles ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
				//dex--

				x += 170;
			}

			if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_ApexStats ) )
			{
				Uint32 y = frame->GetFrameOverlayInfo().m_height - 130;

				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex cloths: %i updated / %i rendered"), stats.m_numApexClothsUpdated, stats.m_numApexClothsRendered ); y += 15;
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex destruction: %i updated / %i rendered"), stats.m_numApexDestructiblesUpdated, stats.m_numApexDestructiblesRendered ); y += 15;
				y += 20;

				// Render resources
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex resources rendered: %d"), stats.m_numApexResourcesRendered ); y += 15;
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex buffers updated (non fast path): %d VB / %d IB / %d BB"), stats.m_numApexVBUpdated, stats.m_numApexIBUpdated, stats.m_numApexBBUpdated ); y += 15;
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB semantics updated: %d"), stats.m_numApexVBSemanticsUpdated ); y += 15;
				frame->AddDebugScreenFormatedText( x, y, Color::WHITE, TXT("Apex VB fast path update: %d"), stats.m_numApexVBUpdatedFastPath ); y += 15;
			}
			
			CEntity* ent = m_editor->m_entity;

			if ( ent )
			{
				//if ( !m_editor->m_alreadyCalculatedTexStats )
				//{
					m_editor->m_textureStats = m_editor->CollectTextureUsage( ent );
				//}	
				Uint32 x = 200;
				Uint32 y = frame->GetFrameOverlayInfo().m_height - 130;

				frame->AddDebugScreenText( x, y, String::Printf( TXT("Texture Cost: %1.2f MB"), m_editor->m_textureStats ), 0, false, Color(255,255,255), Color(0,0,0) );  y += 15;
			}
		}

		// Generate base fragments
		CEdRenderingPanel::OnViewportGenerateFragments( view, frame );
	}

public:
	CEdCharacterEntityEditorPreviewPanel( wxWindow* parent, CEdCharacterEntityEditor* editor )
		: CEdPreviewPanel( parent, true )
		, m_editor( editor )
	{
		GetViewport()->SetRenderingMode( RM_Shaded );
		GetViewport()->SetRenderingMask( SHOW_Selection );
	}

	
};

//////////////////////////////////////////////////////////////////////////

class CEdCharacterEntityEditorPickPointClient : public CEdWorldPickPointClient
{
	CEdCharacterEntityEditor*	m_editor;

public:
	CEdCharacterEntityEditorPickPointClient( CEdCharacterEntityEditor* editor )
		: m_editor( editor )
	{}
	
	//! Called when any world pick point action occurs
	virtual void OnWorldPickPointAction( EWorldPickPointAction action ) override
	{
		switch ( action )
		{
		case WPPA_SET:
			m_editor->CreateWorldPreviewAt( wxTheFrame->GetWorldEditPanel()->GetPickPoint() );
			break;
		case WPPA_CLEAR:
			m_editor->DestroyWorldPreview();
			break;
		}
	}
};


//////////////////////////////////////////////////////////////////////////

CEdCharacterEntityEditor::CEdCharacterEntityEditor( wxWindow* parent, CCharacterEntityTemplate* ce_template )
	: wxSmartLayoutPanel( parent, TXT("CharacterEntityEditor"), false )
	, m_template( ce_template )
	, m_entity( NULL )
	, m_doNotUpdateCharacter( false )
	, m_baseIndex( UINT_MAX )
	, m_hasWorldPreview( false ) 
{
	m_alreadyCalculatedTexStats = false;
	Uint32 usedBaseIndex;
	THashMap<String,TDynArray<String>/* omg, c++ */> usedPartFilenames;
	m_config = new CEdCharacterEntityEditorConfiguration();
	CollectPartsFromEntity( usedBaseIndex, usedPartFilenames );
	m_appearances.Clear();
	m_template->AddToRootSet();

	m_searchTimer.SetOwner( this );
	Connect( m_searchTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( CEdCharacterEntityEditor::OnSearchTimer ), NULL, this );

	m_applyAppearanceTimer.SetOwner( this );
	Connect( m_applyAppearanceTimer.GetId(),wxEVT_TIMER, wxTimerEventHandler( CEdCharacterEntityEditor::OnApplyAppearanceTimer ), NULL, this );

	SetSizer( new wxBoxSizer( wxVERTICAL ) );
	wxSplitterWindow* splitter = new wxSplitterWindow( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE );
	splitter->SetSashGravity( 0.25 );
	((wxBoxSizer*)GetSizer())->Add( splitter, 1, wxEXPAND, 0 );

	wxPanel* leftPanel = new wxPanel( splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize );
	leftPanel->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	m_appearancesPanel = new wxPanel( leftPanel, wxID_ANY, wxDefaultPosition, wxDefaultSize );
	((wxBoxSizer*)leftPanel->GetSizer())->Add( m_appearancesPanel, 0, wxEXPAND|wxTOP|wxBOTTOM|wxLEFT, 10 );

	m_appearancesPanel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	((wxBoxSizer*)m_appearancesPanel->GetSizer())->Add( new wxStaticText( m_appearancesPanel, wxID_ANY, wxT("Appearance:") ), 0, wxALIGN_CENTER_VERTICAL|wxRIGHT, 5 );

	m_appearanceChoice = new CEdChoice( m_appearancesPanel, wxDefaultPosition, wxDefaultSize, true );
	m_appearanceChoice->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdCharacterEntityEditor::OnAppearanceChoiceChange ), NULL, this );
	m_appearanceChoice->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCharacterEntityEditor::OnAppearanceChoiceChange ), NULL, this );
	m_appearanceChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCharacterEntityEditor::OnAppearanceChoiceChange ), NULL, this );
	((wxBoxSizer*)m_appearancesPanel->GetSizer())->Add( m_appearanceChoice, 1, wxALIGN_CENTER_VERTICAL, 0 );

	wxButton* deleteAppearanceButton = new wxButton( m_appearancesPanel, wxID_ANY, wxT("Remove") );
	deleteAppearanceButton->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdCharacterEntityEditor::OnDeleteAppearanceClicked ), NULL, this );
	((wxBoxSizer*)m_appearancesPanel->GetSizer())->Add( deleteAppearanceButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );

	m_preview = new CEdCharacterEntityEditorPreviewPanel( leftPanel, this );
	((wxBoxSizer*)leftPanel->GetSizer())->Add( m_preview, 1, wxEXPAND, 0 );

	wxPanel* rightPanel = new wxPanel( splitter, wxID_ANY, wxDefaultPosition, wxDefaultSize );

	splitter->SplitVertically( leftPanel, rightPanel, 0 );

	wxBoxSizer* mainSizer = new wxBoxSizer( wxVERTICAL );
	rightPanel->SetSizer( mainSizer );
	wxBoxSizer* sizer;

	m_filterControls = new wxPanel( rightPanel, wxID_ANY );
	m_filterControls->SetSizer( sizer = new wxBoxSizer( wxHORIZONTAL ) );
	mainSizer->Add( m_filterControls, 0, wxEXPAND | wxLEFT | wxTOP | wxRIGHT, 10 );

	m_partButtons = new wxPanel( m_filterControls, wxID_ANY );
	m_partButtons->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
	sizer->Add( m_partButtons, 0, wxEXPAND, 0 );

	sizer->AddStretchSpacer( 1 );

	m_typesChoice = new wxChoice( m_filterControls, wxID_ANY );
	m_typesChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCharacterEntityEditor::OnChoiceSelected ), NULL, this );
	sizer->Add( m_typesChoice, 0, wxALIGN_CENTER_VERTICAL );

	sizer->AddStretchSpacer( 1 );

	m_partgroupChoice = new CEdChoice( m_filterControls, wxDefaultPosition, wxDefaultSize, true );
	m_partgroupChoice->Connect( wxEVT_COMMAND_TEXT_UPDATED, wxCommandEventHandler( CEdCharacterEntityEditor::OnPartgroupChoiceChange ), NULL, this );
	m_partgroupChoice->Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( CEdCharacterEntityEditor::OnPartgroupChoiceChange ), NULL, this );
	m_partgroupChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCharacterEntityEditor::OnChoiceSelected ), NULL, this );
	sizer->Add( m_partgroupChoice, 10, wxALIGN_CENTER_VERTICAL );

	sizer->AddStretchSpacer( 1 );

	m_usedOnly = new wxCheckBox( m_filterControls, wxID_ANY, wxT("Used only") );
	m_usedOnly->Connect( wxEVT_COMMAND_CHECKBOX_CLICKED, wxCommandEventHandler( CEdCharacterEntityEditor::OnChoiceSelected ), NULL, this );
	sizer->Add( m_usedOnly, 0, wxALIGN_CENTER_VERTICAL );

	sizer->AddStretchSpacer( 1 );

	m_baseChoice = new wxChoice( m_filterControls, wxID_ANY );
	m_baseChoice->Connect( wxEVT_COMMAND_CHOICE_SELECTED, wxCommandEventHandler( CEdCharacterEntityEditor::OnBaseChoiceSelected ), NULL, this );
	for ( Uint32 i=0; i<m_config->GetBaseCount(); ++i )
	{
		m_baseChoice->AppendString( m_config->GetBaseName( i ).AsChar() );
	}
	sizer->Add( m_baseChoice, 0, wxALIGN_CENTER_VERTICAL );

	m_gridhook = new CEdCharacterEntityEditorGridHook( this );
	
	m_grid = new CEdFileGrid( rightPanel, 0 );
	m_grid->SetHook( m_gridhook );
	mainSizer->Add( m_grid, 1, wxEXPAND | wxALL, 10 );

	AfterTemplateLoad( usedBaseIndex, usedPartFilenames );

	// Register the client
	m_pickPointClient = new CEdCharacterEntityEditorPickPointClient( this );
	RegisterPickPointClient();

	// Update and finalize layout
	Layout();
	Show();
}

CEdCharacterEntityEditor::~CEdCharacterEntityEditor()
{
	UnregisterPickPointClient();
	delete m_pickPointClient;

	DestroyWorldPreview();
	if ( m_entity )
	{
		m_entity->RemoveFromRootSet();
		m_entity = NULL;
	}
	RemoveParts();
	delete m_config;
	m_grid->SetHook( NULL );
	delete m_gridhook;
}

void CEdCharacterEntityEditor::CollectPartsFromEntity( Uint32& baseIndex, THashMap<String,TDynArray<String>/*c++ sux*/>& partfiles )
{
	const TDynArray<CEntityAppearance>& apps = m_template->GetAppearances();
	THashSet<String> allfiles;
	partfiles.Clear();
	for ( Uint32 i=0; i<apps.Size(); ++i )
	{
		const CEntityAppearance& app = apps[i];
		const TDynArray< THandle< CEntityTemplate > >& temps = app.GetIncludedTemplates();
		TDynArray<String> files;
		for ( Uint32 j=0; j<temps.Size(); ++j )
		{
			CEntityTemplate* temp = temps[j].Get();
			if ( temp && temp->GetFile() )
			{
				CDiskFile* file = temp->GetFile();
				if ( file )
				{
					allfiles.Insert( file->GetFileName() );
					files.PushBackUnique( file->GetFileName() );
				}
			}
		}
		partfiles.Insert( app.GetName().AsString(), files );
	}

	baseIndex = UINT_MAX;
	for ( Uint32 i=0; i<m_config->GetBaseCount() && baseIndex == UINT_MAX; ++i )
	{
		const TDynArray<String>& paths = m_config->GetBasePaths( i );
		for ( Uint32 j=0; j<paths.Size() && baseIndex == UINT_MAX; ++j )
		{
			CDirectory* dir = GDepot->FindPath( paths[j].AsChar() );
			if ( dir )
			{
				for ( CDirectory* sub : dir->GetDirectories() )
				{
					for ( CDiskFile* file : sub->GetFiles() )
					{
						if ( partfiles.KeyExist( file->GetFileName() ) )
						{
							baseIndex = i;
							break;
						}
					}

					if ( baseIndex != UINT_MAX )
						break;
				}
			}
		}
	}
}

void CEdCharacterEntityEditor::AfterTemplateLoad( Uint32 baseIndex, const THashMap<String,TDynArray<String>>& partfiles )
{
	if ( !m_template->MarkModified() )
	{
		wxMessageBox( wxT("Cannot edit a character template without it being able to modify the resource"), wxT("Error"), wxICON_ERROR|wxOK );
		DestroyLater( this );
		return;
	}

	TDynArray<SEntityTemplateColoringEntry> colorings = m_template->GetAllColoringEntries();
	THashMap<String,THashMap<String,SEntityTemplateColoringEntry*> > coloringsMap;

	m_baseEntityOverride = m_template->GetBaseEntityOverride();

	THashSet< TPair< CName, String > > collapsedComponents;
	TDynArray< CEntityAppearance* > templateAppearances;
	m_template->GetAllAppearances( templateAppearances );
	for ( auto it=templateAppearances.Begin(); it != templateAppearances.End(); ++it )
	{
		CEntityAppearance* ea = *it;
		const TDynArray< CName >& cc = ea->GetCollapsedComponents();
		for ( auto it2=cc.Begin(); it2 != cc.End(); ++it2 )
		{
			collapsedComponents.Insert( TPair< CName, String >( ea->GetName(), (*it2).AsString() ) );;
		}
	}

	for ( Uint32 i=0; i<colorings.Size(); ++i )
	{
		coloringsMap[colorings[i].m_appearance.AsString().AsChar()][colorings[i].m_componentName.AsString().AsChar()] = &colorings[i];
	}

	m_appearances.Clear();

	if ( baseIndex == UINT_MAX ) baseIndex = 0;
	m_baseChoice->SetSelection( baseIndex );
	m_baseIndex = UINT_MAX;
	SetBase( baseIndex );

	TDynArray<String> appearanceNames;
	partfiles.GetKeys( appearanceNames );
	for ( Uint32 i=0; i<appearanceNames.Size(); ++i )
	{
		const TDynArray<String>& appFiles = partfiles[appearanceNames[i]];
		Appearance app;
		app.name = appearanceNames[i];
		for ( Uint32 j=0; j<appFiles.Size(); ++j )
		{
			for ( Uint32 k=0; k<m_parts.Size(); ++k )
			{
				if ( m_parts[k]->file->GetFileName() == appFiles[j] )
				{
					PartRef pr( m_parts[k] );
					String compName = m_parts[k]->file->GetFileName();
					if ( compName.EndsWith( TXT(".w2ent") ) ) compName = compName.LeftString( compName.GetLength() - 6 );
					SEntityTemplateColoringEntry* coloringEntry = coloringsMap[app.name][compName];
					if ( coloringEntry )
					{
						pr.shift1 = coloringEntry->m_colorShift1;
						pr.shift2 = coloringEntry->m_colorShift2;
					}
					CEntityTemplate* tpl = Cast< CEntityTemplate >( m_parts[k]->file->GetResource() );
					if ( tpl != nullptr && tpl->GetEntityObject() != nullptr )
					{
						const TDynArray< CComponent* >& tc = tpl->GetEntityObject()->GetComponents();
						for ( auto it=tc.Begin(); !pr.collapse && it != tc.End(); ++it )
						{
							if ( collapsedComponents.Exist( TPair< CName, String >( CName( app.name ), (*it)->GetName() ) ) )
							{
								pr.collapse = true;
							}
						}
					}
					app.parts.PushBack( pr );
					break;
				}
			}
		}
		m_appearances.Insert( app.name, app );
	}

	UpdateAppearanceList();

	if ( m_appearances.Size() > 0 )
	{
		Appearance& firstAppearance = m_appearances[appearanceNames[0]];
		m_charparts.PushBackUnique( firstAppearance.parts );
		m_appearanceChoice->SetSelection( m_appearanceChoice->FindString( firstAppearance.name.AsChar() ) );
		m_appearanceChoice->SetValue( firstAppearance.name.AsChar() );
	}
	UpdateAppearance();
	UpdateCharacter();
}

String CEdCharacterEntityEditor::TranslateBodypart( Char ch )
{
	return m_config->TranslateBodypart( GetBase(), ch );
}

String CEdCharacterEntityEditor::TranslateType( const String& s )
{
	return m_config->TranslateType( GetBase(), s );
}

void CEdCharacterEntityEditor::ScanDirectory( CDirectory* dir )
{
	if ( !dir ) return;

	for ( CDirectory* sub : dir->GetDirectories() )
	{
		for ( CDiskFile* f : sub->GetFiles() )
		{
			ConsiderFile( f );
		}
	}
}

void CEdCharacterEntityEditor::ConsiderFile( CDiskFile* file )
{
	String name = file->GetFileName();
	if ( !name.EndsWith( TXT(".w2ent") ) ) return;

	// Get parts
	TDynArray<String> parts;
	name.Slice( parts, TXT("_") );

	String bodypart = parts.Size() > 0 ? parts[0] : TXT("?");
	String type = parts.Size() > 2 ? parts[2] : TXT("?");

	type = TranslateType( type );

	for ( Uint32 i=0; i<bodypart.GetLength(); ++i )
	{
		Char ch = bodypart[i];
		if ( !( ch >= L'0' && ch <= L'9' ) )
		{
			AddPart( file, TranslateBodypart( ch ), type );
		}
	}
}

void CEdCharacterEntityEditor::SetBase( Uint32 index )
{
	if ( index == m_baseIndex ) return;

	RemoveParts();

	m_baseIndex = index;

	const TDynArray<String>& paths = m_config->GetBasePaths( index );
	for ( Uint32 i=0; i<paths.Size(); ++i )
	{
		ScanDirectory( GDepot->FindPath( paths[i].AsChar() ) );
	}

	UseParts();
}

Uint32 CEdCharacterEntityEditor::GetBase() const
{
	return m_baseIndex;
}

void CEdCharacterEntityEditor::RemoveParts()
{
	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		delete m_parts[i];
	}
	m_parts.Clear();
	m_filePartMap.Clear();
	m_charparts.Clear();
	m_appearances.Clear();
	m_compsToEntitiesMap.Clear();
	static_cast<wxItemContainer*>( m_appearanceChoice )->Clear();
}

void CEdCharacterEntityEditor::AddPart( CDiskFile* file, const String& bodypart, const String& type )
{
	Part* part = new Part();
	part->file = file;
	part->bodypart = bodypart;
	part->type = type;
	part->partgroup = file->GetDirectory()->GetParent()->GetName();
	m_parts.PushBack( part );
	m_filePartMap[file].PushBack( part );
}

void CEdCharacterEntityEditor::UseParts()
{
	int index;
	THashMap<String,Bool> partsEnabled;
	bool usedOnly = m_usedOnly->GetValue();
	bool feedbackShown = false;

	// Collect parts, types, etc
	m_bodyparts.Clear();
	m_types.Clear();
	m_partgroups.Clear();

	for ( Uint32 i=0; i<m_parts.Size(); ++i )
	{
		m_bodyparts[m_parts[i]->bodypart].PushBack( m_parts[i] );
		m_types[m_parts[i]->type].PushBack( m_parts[i] );
		m_partgroups.Insert( m_parts[i]->partgroup );
	}

	// Fill entity grid using the existing (previous) button filters
	m_grid->Freeze();
	m_grid->Clear();
	const wxWindowList& buttons = m_partButtons->GetChildren();
	TDynArray<String> bodyparts;
	TDynArray<String> searchWords;
	if ( m_partgroupChoice->GetValue().Length() > 0 )
	{
		String( m_partgroupChoice->GetValue().wc_str() ).Slice( searchWords, TXT(" ") );
	}
	m_bodyparts.GetKeys( bodyparts );
	for ( Uint32 i=0; i<bodyparts.Size(); ++i )
	{
		Bool ok = true;
		for ( Uint32 j=0; j<buttons.GetCount(); ++j )
		{
			wxToggleButton* btn = (wxToggleButton*)buttons[j];
			if ( btn->GetLabelText() == wxString( bodyparts[i].AsChar() ) )
			{
				ok = btn->GetValue();
				partsEnabled[bodyparts[i]] = ok;
				break;
			}
		}
		if ( !ok ) continue;

		TDynArray<Part*> parts = m_bodyparts[ bodyparts[i] ];
		TDynArray<CDiskFile*> files;
		bool anyFileAdded = false;
		String selectedType = m_typesChoice->GetStringSelection();
		for ( Uint32 j=0; j<parts.Size(); ++j )
		{
			String filename = parts[j]->file->GetFileName();
			bool dontAdd = false;

			if ( searchWords.Size() )
			{
				for ( Uint32 k=0; k<searchWords.Size(); ++k )
				{
					if ( !filename.ContainsSubstring( searchWords[k] ) )
					{
						dontAdd = true;
						break;
					}
				}
			}

			if ( !dontAdd && m_typesChoice->GetSelection() > 0 )
			{
				if ( parts[j]->type != selectedType )
				{
					dontAdd = true;
				}
			}

			if ( !dontAdd && usedOnly )
			{
				if ( !m_usedparts.Exist( parts[j] ) )
				{
					dontAdd = true;
				}
				else
				{
					if ( m_grid->FindFile( parts[j]->file ) )
					{
						dontAdd = true;
					}
				}
			}

			if ( dontAdd ) continue;

			files.PushBack( parts[j]->file );
			anyFileAdded = true;
		}
		if ( anyFileAdded )
		{
			if ( !usedOnly ) m_grid->AddGroup( bodyparts[i] );
			m_grid->AddFiles( files );
		}
	}

	m_grid->Thaw();

	// Create filtering buttons for body parts and fill char parts
	m_partButtons->Freeze();
	m_partButtons->DestroyChildren();
	wxToggleButton* allButton = new wxToggleButton( m_partButtons, wxID_ANY, wxT("All"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
	((wxBoxSizer*)m_partButtons->GetSizer())->Add( allButton, 0 );
	allButton->SetValue( true );
	allButton->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdCharacterEntityEditor::OnAllPartsButtonClick ), NULL, this );
	for ( Uint32 i=0; i<bodyparts.Size(); ++i )
	{
		wxToggleButton* btn = new wxToggleButton( m_partButtons, wxID_ANY, bodyparts[i].AsChar(), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT );
		bool down;
		((wxBoxSizer*)m_partButtons->GetSizer())->Add( btn, 0 );
		btn->SetValue( down = ( partsEnabled.KeyExist(bodyparts[i]) ? partsEnabled[bodyparts[i]] : true ) );
		btn->Connect( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED, wxCommandEventHandler( CEdCharacterEntityEditor::OnPartButtonClick ), NULL, this );
		if ( !down )
		{
			allButton->SetValue( false );
		}

		// Fill character parts, making sure there are no preexisting
		// charparts that occupy the same bodyparts
		/*
		bool addPart = true;
		for ( Uint32 j=0; j<m_charparts.Size(); ++j )
		{
			Part* part = m_charparts[j];
			if ( part->bodypart == bodyparts[i] )
			{
				addPart = false;
				break;
			}
		}
		if ( addPart )
		{
			const TDynArray<Part*>& parts = m_bodyparts[bodyparts[i]];
			for ( Uint32 j=0; j<parts.Size(); ++j )
			{
				if ( m_filePartMap[parts[j]->file].Size() == 1)
				{
					m_charparts.PushBack( parts[j] );
					break;
				}
			}
		}
		*/
	}
	m_partButtons->Layout();
	m_partButtons->Thaw();

	// Fill types choice
	m_typesChoice->Freeze();
	wxString previousType = m_typesChoice->GetStringSelection();
	m_typesChoice->Clear();
	TDynArray<String> types;
	m_types.GetKeys( types );
	m_typesChoice->AppendString( wxT("All") );
	for ( Uint32 i=0; i<types.Size(); ++i )
	{
		m_typesChoice->AppendString( types[i].AsChar() );
	}
	index = m_typesChoice->FindString( previousType );
	if ( index == -1 ) index = 0;
	m_typesChoice->SetSelection( index );
	m_typesChoice->Thaw();

	// Fill partgroup choice
	m_partgroupChoice->Freeze();
	static_cast<wxItemContainer*>( m_partgroupChoice )->Clear();
	for ( THashSet<String>::const_iterator it=m_partgroups.Begin(); it != m_partgroups.End(); ++it )
	{
		m_partgroupChoice->AppendString( (*it).AsChar() );
	}
	m_partgroupChoice->Thaw();

	m_filterControls->Layout();

	// Guess
	UpdateAppearance();
	UpdateCharacter();
}

void CEdCharacterEntityEditor::SelectPart( Part* part )
{
	Int32 index = m_grid->FindFile( part->file );
	if ( index == -1 )
	{
		m_grid->Freeze();
		RemoveFilters();
		m_grid->Thaw();
		index = m_grid->FindFile( part->file );
	}

	if ( index != -1 )
	{
		m_grid->SetSelected( index );
	}
}

void CEdCharacterEntityEditor::UpdateAppearance()
{
	if ( m_appearanceChoice->GetValue().IsEmpty() ) return;
	
	String name = m_appearanceChoice->GetValue().wc_str();

	if ( !m_appearances.KeyExist( name ) )
	{
		m_appearances[name].name = name;
		UpdateAppearanceList();
	}

	Appearance& app = m_appearances[name];
	app.parts.Clear();
	app.parts.PushBack( m_charparts );
}

void CEdCharacterEntityEditor::ApplyAppearance( const Appearance& appearance )
{
	m_charparts.Clear();
	m_compsToEntitiesMap.Clear();
	m_charparts.PushBack( appearance.parts );
	if ( appearance.name != String( m_appearanceChoice->GetValue().wc_str() ) )
	{
		m_appearanceChoice->SetValue( appearance.name.AsChar() );
	}
	UpdateCharacter();
}

void CEdCharacterEntityEditor::CreateCharacterEntityTemplate( CEntityTemplate* target )
{
	// If we're updating this template, remove previous entity
	if ( target == m_template && m_entity )
	{
		m_entity->RemoveFromRootSet();
		m_preview->GetPreviewWorld()->DelayedActions();
		m_entity->Destroy();
		m_preview->GetPreviewWorld()->DelayedActions();
		m_entity = NULL;
	}

	// If we're creating a character entity template, set the base entity override
	if ( target->IsA< CCharacterEntityTemplate >() )
	{
		static_cast< CCharacterEntityTemplate* >( target )->SetBaseEntityOverride( m_baseEntityOverride );
	}

	// Remove existing appearances from the template
	while ( target->GetAppearances().Size() > 0 )
	{
		target->RemoveAppearance( target->GetAppearance( 0 ) );
	}
	target->RemoveAllColoringEntries();

	// Remove existing components from the template
	if ( target->GetEntityObject() )
	{
		target->GetEntityObject()->DestroyAllComponents();
	}

	// Create a new appearance for the entity
	String visibleAppearance = m_appearanceChoice->GetValue().wc_str();
	TDynArray<String> appearances;
	TDynArray<String> usedtypes;
	m_appearances.GetKeys( appearances );
	wxLongLong startTime = wxGetLocalTimeMillis();
	for ( Uint32 i=0; i<appearances.Size(); ++i )
	{
		const Appearance& app = m_appearances[appearances[i]];
		CName appCName( app.name );
		CEntityAppearance appearance( appCName );
		bool updateUsedParts = target == m_template && visibleAppearance == app.name;

		// Add the bodyparts to the appearance
		if ( updateUsedParts )
		{
			m_usedparts.Clear();
		}

		for ( Uint32 i=0; i<app.parts.Size(); ++i )
		{
			CEntityTemplate* tpl = LoadResource<CEntityTemplate>( app.parts[i].part->file->GetDepotPath() );
			bool addIt = false;
			CEntity* entity = tpl->CreateInstance( nullptr, EntityTemplateInstancingInfo() );
			entity->CreateStreamedComponents( SWN_DoNotNotifyWorld );
			ASSERT( tpl, TXT("Character part template not loaded") );
			const TDynArray<CComponent*>& comps = entity->GetComponents();
			for ( Uint32 j=0; j<comps.Size(); ++j )
			{
				CMeshComponent* meshComponent = Cast<CMeshComponent>( comps[j] );
				CClothComponent* clothComponent = Cast<CClothComponent>( comps[j] );
				if ( ( meshComponent && meshComponent->GetMeshNow() ) || clothComponent )
				{
					m_compsToEntitiesMap.Insert( CName( comps[j]->GetName() ), app.parts[i].part->file->GetFileName() );
					addIt = true;
				}
			}
			if ( addIt )
			{
				appearance.IncludeTemplate( tpl );
				if ( updateUsedParts )
				{
					m_usedparts.Insert( app.parts[i].part );
				}
				usedtypes.PushBackUnique( app.parts[i].part->type );
			}
			else
			{
				wxMessageBox( wxT("The entity ") + wxString( tpl->GetFile()->GetDepotPath().AsChar() ) + wxT(" does not contain a mesh!"), wxT("Cataclysmic Error"), wxOK|wxCENTRE|wxICON_ERROR, this );
			}
			entity->Discard();
		}

		for ( Uint32 i=0; i<app.parts.Size(); ++i )
		{
			const PartRef& pr = app.parts[i];
			CEntityTemplate* tpl = Cast< CEntityTemplate >( pr.part->file->GetResource() );

			if ( tpl != nullptr )
			{
				if ( pr.shift1.m_hue != 0 || pr.shift1.m_luminance != 0 || pr.shift1.m_saturation != 0 ||
					 pr.shift2.m_hue != 0 || pr.shift2.m_luminance != 0 || pr.shift2.m_saturation != 0 )
				{
					String name = pr.part->file->GetFileName();
					const TDynArray< CComponent* >& components = tpl->GetEntityObject()->GetComponents();
					
					for ( auto it=components.Begin(); it != components.End(); ++it )
					{
						CComponent* cmp = *it;
						if ( cmp->IsA< CMeshTypeComponent >() )
						{
							target->AddColoringEntry( appCName, CName( cmp->GetName() ), pr.shift1, pr.shift2 );
						}
					}
				}

				if ( pr.collapse )
				{
					appearance.CollapseTemplate( tpl );
				}
			}
		}

		target->AddAppearance( appearance );
	}

	// Create new includes list for the template using the current part slots
	TDynArray<THandle<CEntityTemplate>>& includes = target->GetIncludes();
	includes.Clear();

	// Include base entity template
	bool flipEntity = false;
	if ( m_baseEntityOverride.IsValid() ) // Is there an override specified?
	{
		includes.PushBack( m_baseEntityOverride );
		flipEntity = true;
	}
	else // No override, try to find base template from the parts
	{
		// If all parts use the same type, include its template (if any)
		if ( usedtypes.Size() > 0 )
		{
			String tplname = m_config->GetTypeTemplateName( GetBase(), usedtypes[0] );
			if ( !tplname.Empty() )
			{
				CEntityTemplate* tpl = LoadResource<CEntityTemplate>( tplname );
				if ( tpl )
				{
					flipEntity = true; // flip the entity since entity templates are pre-rotated
					includes.PushBack( tpl );
				}
			}
		}
	}

	// Remove stuff that ought to be removed
	m_preview->GetPreviewWorld()->DelayedActions();

	// Set entity class
	CName className = CName( TXT("CNewNPC") );
	target->SetEntityClass( SRTTI::GetInstance().FindClass( className ) );

	// Re/create the entity using the updated template
	CEntity* entity = NULL;
	EntitySpawnInfo info;
	info.m_template = target;
	info.m_spawnRotation = EulerAngles( 0, 0, flipEntity ? 180 : 0 ); // rotate the spawn if needed (see above)
	info.m_appearances.Clear();
	if ( target == m_template && m_appearances.KeyExist( visibleAppearance ) )
	{
		CName vaname( visibleAppearance );
		info.m_appearances.PushBack( vaname );
	}
	info.m_name = target == m_template ? TXT("PreviewEntity") : TXT("ExportEntity");
	info.m_detachTemplate = false;
	info.m_previewOnly = true;
	entity = m_preview->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( info );
	if( entity == nullptr )
	{
		return;
	}
	entity->AddToRootSet();

	TDynArray<CComponent*> comps;
	for ( Uint32 i=0; i<entity->GetComponents().Size(); ++i )
	{
		if ( entity->GetComponents()[i]->IsA<CAppearanceComponent>() )
		{
			comps.PushBack( entity->GetComponents()[i] );
		}
	}
	for ( Uint32 i=0; i<comps.Size(); ++i )
	{
		entity->RemoveComponent( comps[i] );
	}

	CAppearanceComponent* ac = (CAppearanceComponent*)entity->CreateComponent( CAppearanceComponent::GetStaticClass(), SComponentSpawnInfo() );
	target->GetEnabledAppearancesNames().Clear();
	if ( m_template != target )
	{
		for ( Uint32 i=0; i<appearances.Size(); ++i )
		{
			CName vaname( appearances[i] );
			target->GetEnabledAppearancesNames().PushBack( vaname );
		}
	}
	ac->ApplyInitialAppearance( info );

	entity->DetachTemplate();
	target->CaptureData( entity );

	// Destroy the entity, we do not need it anymore
	entity->RemoveFromRootSet();
	entity->Destroy();

	// Create a new preview entity with the template still attached to it
	// if we're working with our own template instance
	if ( target == m_template )
	{
		m_entity = m_preview->GetPreviewWorld()->GetDynamicLayer()->CreateEntitySync( info );
		m_entity->AddToRootSet();
		
		TDynArray<CComponent*> comps;
		for ( Uint32 i=0; i<m_entity->GetComponents().Size(); ++i )
		{
			if ( m_entity->GetComponents()[i]->IsA<CAppearanceComponent>() )
			{
				comps.PushBack( m_entity->GetComponents()[i] );
			}
		}
		for ( Uint32 i=0; i<comps.Size(); ++i )
		{
			m_entity->RemoveComponent( comps[i] );
		}

		CAppearanceComponent* ac = (CAppearanceComponent*)m_entity->CreateComponent( CAppearanceComponent::GetStaticClass(), SComponentSpawnInfo() );
		m_template->GetEnabledAppearancesNames().Clear();
		ac->ApplyInitialAppearance( info );
	}

	// tick the preview world to avoid the flickering from rotation (if any)
	CWorldTickInfo tinfo( m_preview->GetPreviewWorld(), 1 );
	m_preview->GetPreviewWorld()->Tick( tinfo );
}

void CEdCharacterEntityEditor::UpdateCharacter()
{
	if ( m_doNotUpdateCharacter ) return;

	CreateCharacterEntityTemplate( m_template );

	TDynArray<Part*> partsToDelete;
	TDynArray<Part*> parts;
	m_partColorEditorMap.GetKeys( parts );
	for ( Uint32 i=0; i<parts.Size(); ++i )
	{
		if ( !m_usedparts.Exist( parts[i] ) )
		{
			CEdColorShiftPairEditorPopup* popup = m_partColorEditorMap[parts[i]];
			if ( popup )
			{
				popup->Destroy();
			}
			m_partColorEditorMap.Erase( parts[i] );
		}
	}

	if ( m_hasWorldPreview )
	{
		UpdateWorldPreview();
	}

#if 0
	for ( Uint32 i=0; i<m_charparts.Size(); ++i )
	{
		// TODO: check for charpart including mesh
		CEntityTemplate* tpl = LoadResource<CEntityTemplate>( m_charparts[i]->file->GetDepotPath() );
		ASSERT( tpl );
		includes.PushBack( tpl );
		m_usedparts.Insert( m_charparts[i] );
	}
#endif

}

void CEdCharacterEntityEditor::RemoveFilters()
{
	wxCommandEvent fakeEvent( wxEVT_COMMAND_TOGGLEBUTTON_CLICKED );
	m_grid->Freeze();
	OnAllPartsButtonClick( fakeEvent );
	m_partgroupChoice->SetValue( wxT("") );
	if ( m_searchTimer.IsRunning() ) m_searchTimer.Stop();
	if ( m_applyAppearanceTimer.IsRunning() ) m_applyAppearanceTimer.Stop();
	m_typesChoice->SetSelection( 0 );
	m_usedOnly->SetValue( false );
	m_grid->Thaw();

	UseParts();
}

void CEdCharacterEntityEditor::ExportEntityTemplate()
{
	UpdateCharacter();
	
	bool reload = false;
	String filename = m_template->GetFile()->GetFileName();
	if ( filename.EndsWith( TXT(".w2cent") ) ) filename = filename.LeftString( filename.GetLength() - 7 );
	filename += TXT(".w2ent");
	CDirectory* dir = m_template->GetFile()->GetDirectory(); 
	CDiskFile* file = dir->FindLocalFile( filename );
	if ( file )
	{
		if ( wxTheFrame->GetAssetBrowser()->IsResourceOpenedInEditor( file->GetResource() ) )
		{
			wxMessageBox( wxT("Cannot export the entity template - an editor with an existing entity template with the same name is already open. Close it and try again."), wxT("Cannot export the template"), wxOK|wxCENTRE|wxICON_ERROR, this );
			return;
		}

		if ( !file->IsLocal() && !file->CheckOut() )
		{
			wxMessageBox( wxT("The entity template file already exists and cannot be checked out."), wxT("Cannot check out existing template"), wxOK|wxCENTRE|wxICON_ERROR, this );
			return;
		}

		if ( file->IsLoaded() )
		{
			reload = true;
			if ( wxMessageBox( wxT("The entity template file exists and is loaded. Re-exporting it might introduce unexpected side effects. Do you want to proceed with unloading the resource, exporting and reloading it back?"), wxT("The entity file about to export is loaded"), wxYES_NO|wxICON_WARNING|wxCENTRE, this ) != wxYES )
			{
				return;
			}
			file->Unload();
		}
	}
	CEntityTemplate* tpl = new CEntityTemplate();
	CreateCharacterEntityTemplate( tpl );
	if ( !tpl->SaveAs( dir, filename ) )
	{
		wxMessageBox( wxT("Failed to save the template file"), wxT("Entity exporting error"), wxOK|wxCENTRE|wxICON_ERROR, this );
	}
	tpl->Discard();
	if ( reload )
	{
		if ( !file->Load() )
		{
			wxMessageBox( wxT("Failed to reload the entity template after exporting it. Now might be a good idea to not save anything and restart the editor."), wxT("Expected potential failure"), wxICON_ERROR|wxCENTRE|wxOK, this );
		}
	}
	dir->Repopulate();
	wxTheFrame->GetAssetBrowser()->UpdateResourceList();
}

void CEdCharacterEntityEditor::RegisterPickPointClient()
{
	if ( wxTheFrame != nullptr && wxTheFrame->GetWorldEditPanel() != nullptr )
	{
		wxTheFrame->GetWorldEditPanel()->SetPickPointClient( m_pickPointClient, TXT("Spawn character entity here"), TXT("Remove character entity preview"), nullptr );
	}
}

void CEdCharacterEntityEditor::UnregisterPickPointClient()
{
	if ( wxTheFrame != nullptr && wxTheFrame->GetWorldEditPanel() != nullptr )
	{
		wxTheFrame->GetWorldEditPanel()->ClearPickPointClient( m_pickPointClient );
	}
}

void CEdCharacterEntityEditor::CreateWorldPreviewAt( const Vector& position )
{
	DestroyWorldPreview();

	if ( wxTheFrame->GetWorldEditPanel()->GetWorld() == nullptr )
	{
		wxMessageBox( wxT("No world in the main editor window"), wxT("Error"), wxOK );
		return;
	}

	m_hasWorldPreview = true;
	m_worldPreviewPoint = position;
	UpdateCharacter();
}

void CEdCharacterEntityEditor::DestroyWorldPreview()
{
	CEntity* entity = m_worldPreviewEntity.Get();
	if ( entity != nullptr )
	{
		entity->Destroy();
	}

	m_worldPreviewEntity = THandle< CEntity >::Null();
	m_hasWorldPreview = false;
}

void CEdCharacterEntityEditor::UpdateWorldPreview()
{
	CEntity* entity = m_worldPreviewEntity.Get();
	if ( entity != nullptr )
	{
		entity->Destroy();
	}

	if ( wxTheFrame->GetWorldEditPanel()->GetWorld() == nullptr )
	{
		return;
	}

	String visibleAppearance = m_appearanceChoice->GetValue().wc_str();
	EntitySpawnInfo info;
	info.m_appearances.PushBack( CName( visibleAppearance ) );
	info.m_previewOnly = true;
	info.m_spawnPosition = m_worldPreviewPoint;
	Matrix rotationMatrix;
	rotationMatrix.BuildFromDirectionVector( ( wxTheFrame->GetWorldEditPanel()->GetCameraPosition() - m_worldPreviewPoint ).Normalized3() );
	info.m_spawnRotation = rotationMatrix.ToEulerAngles();
	info.m_spawnRotation.Roll = info.m_spawnRotation.Pitch = 0;
	info.m_template = m_template;
	entity = wxTheFrame->GetWorldEditPanel()->GetWorld()->GetDynamicLayer()->CreateEntitySync( info );

	if ( entity != nullptr )
	{
		m_worldPreviewEntity = entity;
	}
	else
	{
		m_worldPreviewEntity = THandle< CEntity >::Null();
		m_hasWorldPreview = false;
	}
}

void CEdCharacterEntityEditor::OnPartButtonClick( wxCommandEvent& event )
{
	if ( !wxGetKeyState( WXK_CONTROL ) )
	{
		const wxWindowList& buttons = m_partButtons->GetChildren();
		for ( Uint32 i=0; i<buttons.GetCount(); ++i )
		{
			((wxToggleButton*)buttons[i])->SetValue( buttons[i] == event.GetEventObject() );
		}
	}
	UseParts();
}

void CEdCharacterEntityEditor::OnAllPartsButtonClick( wxCommandEvent& event )
{
	const wxWindowList& buttons = m_partButtons->GetChildren();
	for ( Uint32 i=0; i<buttons.GetCount(); ++i )
	{
		((wxToggleButton*)buttons[i])->SetValue( true );
	}
	UseParts();
}

void CEdCharacterEntityEditor::OnPreviewPopupMenuSelected( wxCommandEvent& event )
{
	CMeshTypeComponent* component = (CMeshTypeComponent*)(static_cast<wxMenu*>(event.GetEventObject())->GetClientData());
	TDynArray<CResource*> resources;

	switch ( event.GetId() )
	{
	case ID_POPUP_SET_COLOR:
		if ( component->GetEntity() )
		{
			for ( TDynArray<PartRef>::iterator it=m_charparts.Begin(); it != m_charparts.End(); ++it )
			{
				PartRef& part = *it;

 				String* parentEntityName = m_compsToEntitiesMap.FindPtr( CName( component->GetName() ) );
				if( parentEntityName == nullptr )
				{
					continue;
				}

				if ( part.part->file->GetFileName() == ( *parentEntityName ) )
				{
					CEdColorShiftPairEditorPopup* popup = m_partColorEditorMap[part.part];
					if ( !popup )
					{
						popup = new CEdColorShiftPairEditorPopup( this, wxT("Edit color shifts for ") + wxString( component->GetName().AsChar() ), part.shift1, part.shift2 );
						popup->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( CEdCharacterEntityEditor::OnCloseColorShiftPairEditor ), nullptr, this );
						popup->SetClientData( part.part );
						popup->SetHook( this );
						m_partColorEditorMap[part.part] = popup;
						popup->Show();
					}
					else
					{
						if ( !popup->IsVisible() )
						{
							popup->Show();
						}
						popup->SetFocus();
					}
					break;
				}
			}
		}
		break;
	case ID_POPUP_COLLAPSE:
		if ( component->GetEntity() )
		{
			for ( TDynArray<PartRef>::iterator it=m_charparts.Begin(); it != m_charparts.End(); ++it )
			{
				PartRef& part = *it;

				String* parentEntityName = m_compsToEntitiesMap.FindPtr( CName( component->GetName() ) );
				if( parentEntityName == nullptr )
				{
					continue;
				}

				if ( part.part->file->GetFileName() == ( *parentEntityName ) )
				{
					part.collapse = !part.collapse;
					UpdateAppearance();
					UpdateCharacter();
					break;
				}
			}
		}
		break;
	case ID_POPUP_OPEN_ENTITY:
		if ( component->GetEntity() )
		{
			for ( THashSet<Part*>::iterator it=m_usedparts.Begin(); it != m_usedparts.End(); ++it )
			{
				String* parentEntityName = m_compsToEntitiesMap.FindPtr( CName( component->GetName() ) );
				if( parentEntityName == nullptr )
				{
					continue;
				}

				if ( (*it)->file->GetFileName() == ( *parentEntityName ) )
				{
					wxTheFrame->GetAssetBrowser()->OpenFile( (*it)->file->GetDepotPath() );
					break;
				}
			}
		}
		break;
	case ID_POPUP_OPEN_MESH:
		if ( component->GetEntity() )
		{
			CMeshTypeResource* mesh = component->GetMeshTypeResource();
			CDiskFile* file = mesh ? mesh->GetFile() : NULL;
			if ( file )
			{
				wxTheFrame->GetAssetBrowser()->OpenFile( file->GetDepotPath() );
			}
		}
		break;
	case ID_POPUP_DEFAULT_LOD:
		( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetPreviewWorld()->GetRenderSceneEx(), -1 ) )->Commit();
		break;
	case ID_POPUP_FORCE_LOD0:
		( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetPreviewWorld()->GetRenderSceneEx(), 0 ) )->Commit();
		break;
	case ID_POPUP_FORCE_LOD1:
		( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetPreviewWorld()->GetRenderSceneEx(), 1 ) )->Commit();
		break;
	case ID_POPUP_FORCE_LOD2:
		( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetPreviewWorld()->GetRenderSceneEx(), 2 ) )->Commit();
		break;
	case ID_POPUP_FORCE_LOD3:
		( new CRenderCommand_ChangeSceneForcedLOD( m_preview->GetPreviewWorld()->GetRenderSceneEx(), 3 ) )->Commit();
		break;
	case ID_POPUP_REFRESH_CHARACTER:
		UpdateAppearance();
		UpdateCharacter();
		break;
	case ID_POPUP_SET_BASE_ENTITY_OVERRIDE:
		{
			String resourcePath;
			if ( GetActiveResource( resourcePath, CEntityTemplate::GetStaticClass() ) )
			{
				CResource* tplRes = GDepot->LoadResource( resourcePath, ResourceLoadingContext() );
				CEntityTemplate* tpl = Cast<CEntityTemplate>( tplRes );
				if ( tpl != nullptr )
				{
					// Make sure we're not trying to use the wrong template
					Bool ok = tpl != m_template;
					for ( auto part : m_parts )
					{
						if ( part->file == tpl->GetFile() )
						{
							ok = false;
							break;
						}
					}

					// Set override
					if ( ok )
					{
						m_baseEntityOverride = tpl;
						UpdateAppearance();
						UpdateCharacter();
					}
					else
					{
						wxMessageBox( wxT("You cannot use the selected entity template as a base!"),
							wxT("Invalid entity template"),
							wxICON_ERROR|wxCENTER,
							this );
					}
				}
				else
				{
					wxMessageBox( wxT("Please select an entity template in the asset browser"),
						wxT("Selected resource is not a template"),
						wxICON_ERROR|wxCENTER,
						this );
				}
			}
			else
			{
				wxMessageBox( wxT("Please select an entity template in the asset browser"),
					wxT("No selected template"),
					wxICON_ERROR|wxCENTER,
					this );
			}
		}
		break;
	case ID_POPUP_CLEAR_BASE_ENTITY_OVERRIDE:
		m_baseEntityOverride = THandle< CEntityTemplate >::Null();
		UpdateAppearance();
		UpdateCharacter();
		break;
	case ID_POPUP_EXPORT_ENTITY_TEMPLATE:
		ExportEntityTemplate();
		break;
	}
}

void CEdCharacterEntityEditor::OnChoiceSelected( wxCommandEvent& event )
{
	UseParts();
}

void CEdCharacterEntityEditor::OnBaseChoiceSelected( wxCommandEvent& event )
{
	if ( m_baseChoice->GetSelection() == m_baseIndex ) return;

	if ( !m_appearances.Empty() && wxMessageBox( wxT("If you change the base the existing appearances will be lost. Are you sure that you want to continue and lose the existing apperances?"), wxT("This might not be a good idea"), wxYES_NO|wxCENTRE|wxNO_DEFAULT|wxICON_WARNING, this ) != wxYES )
	{
		m_baseChoice->SetSelection( m_baseIndex );
		return;
	}

	SetBase( m_baseChoice->GetSelection() );
}

void CEdCharacterEntityEditor::OnPartgroupChoiceChange( wxCommandEvent& event )
{
	m_searchTimer.Start( event.GetEventType() == wxEVT_COMMAND_TEXT_ENTER ? 0 : 500, true );
}

void CEdCharacterEntityEditor::OnAppearanceChoiceChange( wxCommandEvent& event )
{
	String name = m_appearanceChoice->GetValue().wc_str();
	if ( m_appearances.KeyExist( name ) )
	{
		m_applyAppearanceTimer.Start( 300, true );
	}
	m_alreadyCalculatedTexStats = false;
}

void CEdCharacterEntityEditor::OnDeleteAppearanceClicked( wxCommandEvent& event )
{
	String appearanceName = m_appearanceChoice->GetValue().wc_str();
	if ( appearanceName.Empty() ) return;

	if ( m_appearances.Size() == 1 )
	{
		wxMessageBox( wxT("You cannot delete the last appearance"), wxT("There is only one appearance"), wxOK|wxCENTRE, this );
		return;
	}

	if ( m_appearances.KeyExist( appearanceName ) )
	{
		if ( wxMessageBox( wxT("Are you sure you want to delete the appearance '") + wxString( appearanceName.AsChar() ) + wxT("'? This is not reversible."), wxT("Delete appearance forever"), wxCENTRE|wxYES_NO|wxICON_QUESTION, this ) != wxYES )
		{
			return;
		}

		m_appearances.Erase( appearanceName );
		UpdateAppearanceList();
	}
}

void CEdCharacterEntityEditor::OnSearchTimer( wxTimerEvent& event )
{
	if ( !m_partgroupChoice->IsPopupShown() )
	{
		UseParts();
	}
}

void CEdCharacterEntityEditor::OnApplyAppearanceTimer( wxTimerEvent& event )
{
	String appearanceName = m_appearanceChoice->GetValue().wc_str();
	if ( appearanceName.Empty() ) return;

	if ( m_appearances.KeyExist( appearanceName ) )
	{
		ApplyAppearance( m_appearances[appearanceName] );
		if ( m_usedOnly->IsChecked() )
		{
			UseParts();
		}
	}
}

void CEdCharacterEntityEditor::OnIconGridSelectionChange()
{
	CDiskFile* file = m_grid->GetSelectedFile();
	TDynArray<Part*>& parts = m_filePartMap[file];
	bool toggleItem = wxGetKeyState( WXK_CONTROL );
	if ( parts.Size() == 0 || m_doNotUpdateCharacter ) return;

	// Check if the file is already used (so it needs to be removed if
	// WXK_CONTROL is pressed)
	bool fileUsed = false;
	for ( Uint32 i=0; i<m_charparts.Size(); ++i )
	{
		if ( m_charparts[i].part->file == file )
		{
			fileUsed = true;
			break;
		}
	}

	// Check if the file belong to the same type and ask for confirmation
	// if this is not the case
	if ( !fileUsed )
	{
		THashSet<String> usedTypes;
		String usedType;
		for ( auto it=m_usedparts.Begin(); it != m_usedparts.End(); ++it )
		{
			usedTypes.Insert( (*it)->type );
			usedType = (*it)->type;
		}
		if ( usedTypes.Size() == 1 && usedType != parts[0]->type )
		{
			if ( !YesNo( TXT("The part you want to add is of type '%s' but the current character is composed of '%s' parts. Are you sure you want to add this?"), parts[0]->type.AsChar(), usedType.AsChar() ) )
			{
				return;
			}
		}
	}
	
	// Remove all charparts that cover the same bodyparts as the bodyparts
	// the new charparts are covering if no toggling is to happen
	TDynArray<PartRef> toremove;
	if ( !toggleItem )
	{
		for ( Uint32 i=0; i<parts.Size(); ++i )
		{
			Part* part = parts[i];
			for ( Uint32 j=0; j<m_charparts.Size(); ++j )
			{
				Part* charpart = m_charparts[j].part;
				TDynArray<Part*> cpFileParts = m_filePartMap[charpart->file];
				for ( Uint32 k=0; k<cpFileParts.Size(); ++k )
				{
					if ( cpFileParts[k]->bodypart == part->bodypart )
					{
						toremove.PushBack( m_charparts[j] );
						break;
					}
				}
			}
		}
	}
	else  // when toggling, simply remove the charpart with the same file
	{
		for ( Uint32 i=0; i<m_charparts.Size(); ++i )
		{
			if ( m_charparts[i].part->file == file ) {
				toremove.PushBack( m_charparts[i] );
				break;
			}
		}
	}
	for ( Uint32 i=0; i<toremove.Size(); ++i )
	{
		m_charparts.Remove( toremove[i] );
	}

	if ( !fileUsed )
	{
		m_charparts.PushBackUnique( parts[0] );
	}

	// Make this depend on a flag or setting later
#if 0
	// Fill empty slots with default single-bodypart parts
	for ( Uint32 i=0; i<slotBodyparts.Size(); ++i )
	{
		if ( !m_slots[slotBodyparts[i]] )
		{
			TDynArray<Part*>& bparts = m_bodyparts[slotBodyparts[i]];
			int count = 0;
			for ( Uint32 j=0; j<bparts.Size(); j++ )
			{
				if ( m_filePartMap[bparts[j]->file].Size() == 1 )
				{
					m_slots[slotBodyparts[i]] = bparts[j];
					break;
				}
			}
		}
	}
#endif

	UpdateAppearance();
	UpdateCharacter();
}

void CEdCharacterEntityEditor::OnFileSaveMenuItem( wxCommandEvent& event )
{
	UpdateCharacter();
	if ( !m_template->Save() )
	{
		wxMessageBox( wxT("Failed to save the character entity template file"), wxT("Something is wrong"), wxOK|wxCENTRE|wxICON_ERROR, this );
	}
}

void CEdCharacterEntityEditor::OnFileReloadMenuItem( wxCommandEvent& event )
{
	wxString previousAppearance = m_appearanceChoice->GetValue();
	Uint32 usedBaseIndex;
	THashMap<String,TDynArray<String>/* omg, c++ */> usedPartFilenames;

	m_template->RemoveFromRootSet();
	CDiskFile* file = m_template->GetFile();
	file->Unload();
	ASSERT( file->Load(), TXT("The template should have been reloaded but it didn't" ) );
	m_appearanceChoice->SetValue( wxT("") );
	m_template = (CCharacterEntityTemplate*)file->GetResource();
	ASSERT( m_template, TXT("Something is wrong, the template shouldn't have been NULL if file->Load() above didn't assert") );
	m_template->AddToRootSet();

	CollectPartsFromEntity( usedBaseIndex, usedPartFilenames );
	AfterTemplateLoad( usedBaseIndex, usedPartFilenames );

	wxTheFrame->GetAssetBrowser()->OnEditorReload( m_template, this );

	int index = m_appearanceChoice->FindString( previousAppearance );
	if ( index != -1 )
	{
		m_appearanceChoice->SetSelection( index );
		m_appearanceChoice->SetValue( previousAppearance );

		// tick the preview world to avoid the flickering from changing
		CWorldTickInfo tinfo( m_preview->GetPreviewWorld(), 1 );
		m_preview->GetPreviewWorld()->Tick( tinfo );
	}
}

void CEdCharacterEntityEditor::OnFileExportEntityTemplateMenuItem( wxCommandEvent& event )
{
	ExportEntityTemplate();
}

void CEdCharacterEntityEditor::OnFileCloseMenuItem( wxCommandEvent& event )
{
	Close();
}

void CEdCharacterEntityEditor::HandleSelectionOfMeshComponent( CMeshTypeComponent* component )
{
	String name = component->GetName();

	for ( THashSet<Part*>::iterator it=m_usedparts.Begin(); it != m_usedparts.End(); ++it )
	{
		if ( (*it)->file->GetFileName() == component->GetName() + TXT(".w2ent") )
		{
			m_doNotUpdateCharacter = true;
			component->SetFlag( NF_Selected );
			SelectPart( *it );
			m_doNotUpdateCharacter = false;
			break;
		}
	}
}

void CEdCharacterEntityEditor::OnColorShiftPairModify( CEdColorShiftPairEditorPopup* sender, const CColorShift& shift1, const CColorShift& shift2 )
{
	// Find the part the sender editor of this notification is editing
	for ( Uint32 i=0; i<m_charparts.Size(); ++i )
	{
		PartRef& pr = m_charparts[i];

		// Part found, apply new color shifts
		if ( sender == m_partColorEditorMap[pr.part] )
		{
			sender->GetColorShifts( pr.shift1, pr.shift2 );
			UpdateAppearance();

			CEntityTemplate* tpl = Cast< CEntityTemplate >( pr.part->file->GetResource() );
			if ( tpl != nullptr )
			{
				const TDynArray< CComponent* >& components = tpl->GetEntityObject()->GetComponents();

				for ( auto it=components.Begin(); it != components.End(); ++it )
				{
					CComponent* cmp = *it;
					if ( cmp->IsA< CMeshTypeComponent >() )
					{
						m_entity->GetEntityTemplate()->AddColoringEntry( CName( m_appearanceChoice->GetValue().wc_str() ), CName( cmp->GetName() ), pr.shift1, pr.shift2 );
					}
				}
			}

			m_entity->ApplyMeshComponentColoring();
			return;
		}
	}

	// The part wasn't found so destroy the editor, it isn't valid anymore
	sender->Destroy();
}

IEditorPreviewCameraProvider::Info CEdCharacterEntityEditor::GetPreviewCameraInfo() const
{
	IEditorPreviewCameraProvider::Info res;
	res.m_cameraPostion  = m_preview->GetCameraPosition();
	res.m_cameraRotation = m_preview->GetCameraRotation();
	res.m_cameraFov      = m_preview->GetCameraFov();
	res.m_lightPosition  = m_preview->GetLightPosition();

	res.m_cameraPostion = Matrix().SetRotZ33( M_PI ).TransformPoint( res.m_cameraPostion );
	res.m_lightPosition += 180;
	res.m_cameraRotation.Yaw += 180;

	return res;
}

void CEdCharacterEntityEditor::UpdateAppearanceList()
{
	TSet< String > sortedNames;
	for ( auto apI = m_appearances.Begin(); apI != m_appearances.End(); ++apI )
	{
		sortedNames.Insert( apI->m_first );
	}

	m_appearanceChoice->wxItemContainer::Clear();

	for ( auto nameIt = sortedNames.Begin(); nameIt != sortedNames.End(); ++nameIt )
	{
		m_appearanceChoice->AppendString( nameIt->AsChar() );
	}
}

void CEdCharacterEntityEditor::OnCloseColorShiftPairEditor( wxCloseEvent& event )
{
	Part* part = ( Part* )( ( ( wxDialog* )event.GetEventObject() )->GetClientData() );
	if( part != nullptr )
	{
		m_partColorEditorMap[part] = nullptr;
	}

	event.Skip();
}

wxString CEdCharacterEntityEditor::GetShortTitle() 
{ 
	return m_template->GetFile()->GetFileName().AsChar() + wxString(TXT(" - Character Entity Editor")); 
}

Float CEdCharacterEntityEditor::CollectTextureUsage( CEntity* entity )
{
	Uint32 tdata = 0;
	TDynArray< CBitmapTexture* > usedTextures;
	
	SEntityStreamingState streamingState;
	entity->PrepareStreamingComponentsEnumeration( streamingState, false );
	entity->ForceFinishAsyncResourceLoads();

	TDynArray< CComponent* > comps = entity->GetComponents();
	for ( Uint32 i = 0; i < comps.Size(); ++i )
	{
		if ( comps[i]->IsA<CMeshTypeComponent>() )
		{
			CMeshTypeComponent* compMesh = Cast< CMeshTypeComponent >( comps[i] );
			CMeshTypeResource* meshResource = compMesh->GetMeshTypeResource();

			const CMeshTypeResource::TMaterials& materials = meshResource->GetMaterials();

			for ( Uint32 m=0; m<materials.Size(); m++ )
			{
				IMaterial* material = materials[m].Get();

				TDynArray< MeshTextureInfo* > usedTexturesInfo;
				MeshStatsNamespace::GatherTexturesUsedByMaterial( material, m, usedTexturesInfo );
				for ( Uint32 tex = 0; tex < usedTexturesInfo.Size(); ++tex )
				{
					usedTextures.PushBackUnique( usedTexturesInfo[tex]->m_texture );
				}
			}
		}
	}
	entity->FinishStreamingComponentsEnumeration( streamingState );

	for ( Uint32 it = 0; it < usedTextures.Size(); ++it )
	{
		tdata += MeshStatsNamespace::CalcTextureDataSize( usedTextures[it] );
	}
	Float texData = tdata / (1024.0f*1024.0f);
	m_alreadyCalculatedTexStats = true;
	return texData;
}
