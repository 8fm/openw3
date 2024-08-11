#include "build.h"
#include "colourSelectionDialog.h"

#include "Scintilla.h"
#include "app.h"

struct SStyleEventData : public wxClientData
{
	EStyle m_style;
	ESubStyle m_subStyle;

	SStyleEventData( EStyle style, ESubStyle subStyle )
	:	m_style( style ),
		m_subStyle( subStyle )
	{

	}
};

CSSColourSelectionDialog::CSSColourSelectionDialog( wxWindow* parent )
:	m_resetColoursBitmap( NULL )
{
	// Load layout from XRC
	bool frameLoaded = wxXmlResource::Get()->LoadFrame( this, parent, wxT( "ColourSelectionDialog" ) );
	RED_ASSERT( frameLoaded );

	SetSize( 700, 600 );
	Centre( wxBOTH );

	wxListbook* mainContainer = XRCCTRL( *this, "Main", wxListbook );
	RED_ASSERT( mainContainer );

	m_resetColoursBitmap = wxTheSSApp->LoadBitmap( wxT( "IMG_RESET" ) );

	CreateEditingPage( mainContainer );
	CreateHoverInfoPage( mainContainer );
	CreateCaretPage( mainContainer );

	wxPanel* previewContainer = XRCCTRL( *this, "PreviewPanel", wxPanel );

	m_preview = new CSSStyledDocument( previewContainer );
	m_preview->Bind( wxEVT_STC_PAINTED, &CSSColourSelectionDialog::OnPreviewPainted, this );

	m_preview->SetText
	(
		wxT( "/* This is an example */\n\n" )
		wxT( "import class CExampleClass extends CObject\n" )
		wxT( "{\n" )
		wxT( "\t// Another Comment\n" )
		wxT( "\timport final function ExampleFunction( param1 : name, param2 : int, param3 : CObject, optional param4 : string ) : bool;\n\n" )
		wxT( "\tfunction Memberunction() : bool\n" )
		wxT( "\t{\n" )
		wxT( "\t\tvar foo : int;\n" )
		wxT( "\t\tvar bar : string;\n\n" )
		wxT( "\t\tfoo = 5;\n" )
		wxT( "\t\tbar = \"test string\";\n" )
		wxT( "\t}\n" )
		wxT( "}\n" )
	);

	previewContainer->GetSizer()->Add( m_preview, 1, wxEXPAND, 5 );

	Bind( wxEVT_CLOSE_WINDOW, &CSSColourSelectionDialog::OnClose, this );
}

CSSColourSelectionDialog::~CSSColourSelectionDialog()
{
}

void CSSColourSelectionDialog::OnClose( wxCloseEvent& event )
{
	CSSStyleManager::GetInstance().WriteStyles();

	event.Skip();
}

void CSSColourSelectionDialog::CreateEditingPage( wxListbook* book )
{
	wxPanel* page = new wxPanel( book );

	wxFlexGridSizer* grid;
	grid = new wxFlexGridSizer( 1, 0, 0, 0 );
	grid->AddGrowableCol( 0 );
	grid->SetFlexibleDirection( wxBOTH );
	grid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	page->SetSizer( grid );

	CreateColumn( grid, wxT( "Style" ) );
	CreateColumn( grid, wxT( "Foreground" ) );
	CreateColumn( grid, wxT( "Background" ) );
	CreateColumn( grid, wxT( "Font" ) );
	CreateColumn( grid, wxT( "Reset" ) );

	CreateRow( grid, wxT( "Default" ), Style_Default );
	CreateRow( grid, wxT( "Comment" ), Style_Comment );
	CreateRow( grid, wxT( "Number" ), Style_Number );
	CreateRow( grid, wxT( "String" ), Style_String );
	CreateRow( grid, wxT( "Character" ), Style_Character );
	CreateRow( grid, wxT( "Identifier" ), Style_Identifier );
	CreateRow( grid, wxT( "Operator" ), Style_Operator );
	CreateRow( grid, wxT( "Word" ), Style_Word );
	CreateRow( grid, wxT( "Word2" ), Style_Word2 );
	CreateRow( grid, wxT( "GlobalClass" ), Style_GlobalClass );
	CreateRow( grid, wxT( "Bracket Highlighting" ), Style_BracketHighlight );
	CreateRow( grid, wxT( "Line Numbers" ), Style_LineNumbers );
	CreateRow( grid, wxT( "Opcodes" ), Style_Opcodes );

	Bind( wxEVT_COMMAND_COLOURPICKER_CHANGED, &CSSColourSelectionDialog::OnColourChanged, this );
	Bind( wxEVT_COMMAND_FONTPICKER_CHANGED, &CSSColourSelectionDialog::OnFontChanged, this );

	page->Layout();
	grid->Fit( page );

	book->AddPage( page, wxT( "Editing" ) );
}

void CSSColourSelectionDialog::CreateHoverInfoPage( wxListbook* book )
{
	wxPanel* page = new wxPanel( book );

	wxFlexGridSizer* grid;
	grid = new wxFlexGridSizer( 1, 0, 0, 0 );
	grid->AddGrowableCol( 0 );
	grid->SetFlexibleDirection( wxBOTH );
	grid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	page->SetSizer( grid );

	CreateColumn( grid, wxT( "Style" ) );
	CreateColumn( grid, wxT( "Foreground" ) );
	CreateColumn( grid, wxT( "Background" ) );
	CreateColumn( grid, wxT( "Font" ) );
	CreateColumn( grid, wxT( "Reset" ) );

	CreateRow( grid, wxT( "Default" ), Style_HoverInfoDefault );
	CreateRow( grid, wxT( "Comment" ), Style_HoverInfoComment );
	CreateRow( grid, wxT( "Number" ), Style_HoverInfoNumber );
	CreateRow( grid, wxT( "String" ), Style_HoverInfoString );
	CreateRow( grid, wxT( "Character" ), Style_HoverInfoCharacter );
	CreateRow( grid, wxT( "Identifier" ), Style_HoverInfoIdentifier );
	CreateRow( grid, wxT( "Operator" ), Style_HoverInfoOperator );
	CreateRow( grid, wxT( "Word" ), Style_HoverInfoWord );
	CreateRow( grid, wxT( "Word2" ), Style_HoverInfoWord2 );
	CreateRow( grid, wxT( "GlobalClass" ), Style_HoverInfoGlobalClass );

	Bind( wxEVT_COMMAND_COLOURPICKER_CHANGED, &CSSColourSelectionDialog::OnColourChanged, this );
	Bind( wxEVT_COMMAND_FONTPICKER_CHANGED, &CSSColourSelectionDialog::OnFontChanged, this );

	page->Layout();
	grid->Fit( page );

	book->AddPage( page, wxT( "Hover Info" ) );
}

void CSSColourSelectionDialog::CreateColumn( wxFlexGridSizer* grid, const wxString& label )
{
	grid->SetCols( grid->GetCols() + 1 );

	CreateLabelCell( grid, label );
}

void CSSColourSelectionDialog::CreateRow( wxFlexGridSizer* grid, const wxString& label, EStyle style )
{
	grid->SetRows( grid->GetRows() + 1 );

	CreateLabelCell( grid, label );
	CreateColourCell( grid, style, SubStyle_Foreground );
	CreateColourCell( grid, style, SubStyle_Background );
	CreateFontCell( grid, style );
	CreateResetCell( grid, style );
}

inline wxWindow* CSSColourSelectionDialog::CreateCell( wxFlexGridSizer* grid )
{
	return new wxPanel( grid->GetContainingWindow(), wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSIMPLE_BORDER );
}

inline void CSSColourSelectionDialog::FormatCell( wxFlexGridSizer* grid, wxWindow* cell, wxSizer* cellSizer )
{
	cell->SetSizer( cellSizer );
	cell->Layout();
	cellSizer->Fit( cell );
	cell->Centre( wxBOTH );

	grid->Add( cell, 1, wxEXPAND, 5 );
}

void CSSColourSelectionDialog::CreateLabelCell( wxFlexGridSizer* grid, const wxString& label )
{
	wxWindow* cell = CreateCell( grid );

	wxStaticText* columnLabel = new wxStaticText( cell, wxID_ANY, label, wxDefaultPosition, wxDefaultSize, 0 );
	columnLabel->Wrap( -1 );

	wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
	cellSizer->Add( columnLabel, 0, wxALL, 5 );

	FormatCell( grid, cell, cellSizer );
}

void CSSColourSelectionDialog::CreateColourCell( wxFlexGridSizer* grid, EStyle style, ESubStyle subStyle )
{
	wxWindow* cell = CreateCell( grid );

	const wxColour& colour = CSSStyleManager::GetInstance().GetColour( style, subStyle );

	wxColourPickerCtrl* colourPicker = new wxColourPickerCtrl( cell, wxID_ANY, colour, wxDefaultPosition, wxDefaultSize );

	colourPicker->SetClientObject( new SStyleEventData( style, subStyle ) );

	m_colourPickers.push_back( colourPicker );

	wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
	cellSizer->Add( colourPicker, 1, wxEXPAND, 5 );

	FormatCell( grid, cell, cellSizer );
}

void CSSColourSelectionDialog::CreateFontCell( wxFlexGridSizer* grid, EStyle style )
{
	wxWindow* cell = CreateCell( grid );

	const wxFont& font = CSSStyleManager::GetInstance().GetFont( style );

	wxFontPickerCtrl* fontPicker = new wxFontPickerCtrl
	(
		cell,
		wxID_ANY,
		font,
		wxDefaultPosition,
		wxDefaultSize,
		wxFNTP_USE_TEXTCTRL
	);
	fontPicker->SetMaxPointSize( 100 );
	fontPicker->SetClientObject( new SStyleEventData( style, SubStyle_Font ) );

	m_fontPickers.push_back( fontPicker );

	wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
	cellSizer->Add( fontPicker, 0, 0, 5 );

	FormatCell( grid, cell, cellSizer );
}

void CSSColourSelectionDialog::CreateResetCell( wxFlexGridSizer* grid, EStyle style )
{
	wxWindow* cell = CreateCell( grid );

	wxBitmapButton* button = new wxBitmapButton
	(
		cell,
		wxID_ANY,
		m_resetColoursBitmap
	);

	button->SetClientObject( new SStyleEventData( style, SubStyle_All ) );
	button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetStyle, this );

	wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
	cellSizer->Add( button, 0, wxLEFT | wxRIGHT, 8 );

	FormatCell( grid, cell, cellSizer );
}

void CSSColourSelectionDialog::OnColourChanged( wxColourPickerEvent& event )
{
	wxObject* object = event.GetEventObject();

	wxWindow* window = wxDynamicCast( object, wxWindow );

	if( window )
	{
		wxClientData* baseData = window->GetClientObject();
		RED_ASSERT( baseData != NULL );

		SStyleEventData* data = static_cast< SStyleEventData* >( baseData );

		CSSStyleManager::GetInstance().SetColour( data->m_style, data->m_subStyle, event.GetColour() );

		m_preview->ReloadStyling();

		RefreshWidgets();
	}
}

void CSSColourSelectionDialog::OnFontChanged( wxFontPickerEvent& event )
{
	wxObject* object = event.GetEventObject();

	wxWindow* window = wxDynamicCast( object, wxWindow );

	if( window )
	{
		wxClientData* baseData = window->GetClientObject();
		RED_ASSERT( baseData != NULL );

		SStyleEventData* data = static_cast< SStyleEventData* >( baseData );

		CSSStyleManager::GetInstance().SetFont( data->m_style, event.GetFont() );

		m_preview->ReloadStyling();

		RefreshWidgets();
	}
}

void CSSColourSelectionDialog::OnResetStyle( wxCommandEvent& event )
{
	wxObject* object = event.GetEventObject();

	wxWindow* window = wxDynamicCast( object, wxWindow );

	if( window )
	{
		wxClientData* baseData = window->GetClientObject();
		RED_ASSERT( baseData != NULL );

		SStyleEventData* data = static_cast< SStyleEventData* >( baseData );

		CSSStyleManager::GetInstance().ResetStyle( data->m_style );

		m_preview->ReloadStyling();

		RefreshWidgets();
	}
}

void CSSColourSelectionDialog::RefreshWidgets()
{
	for( unsigned int i = 0; i < m_colourPickers.size(); ++i )
	{
		SStyleEventData* data = static_cast< SStyleEventData* >( m_colourPickers[ i ]->GetClientObject() );
		RED_ASSERT( data != NULL );

		const wxColour& colour = CSSStyleManager::GetInstance().GetColour( data->m_style, data->m_subStyle );
		
		m_colourPickers[ i ]->SetColour( colour );
	}

	for( unsigned int i = 0; i < m_fontPickers.size(); ++i )
	{
		SStyleEventData* data = static_cast< SStyleEventData* >( m_fontPickers[ i ]->GetClientObject() );
		RED_ASSERT( data != NULL );

		const wxFont& font = CSSStyleManager::GetInstance().GetFont( data->m_style );

		m_colourPickers[ i ]->SetFont( font );
	}

	m_caret.m_colour->SetColour( CSSStyleManager::GetInstance().GetCaretColour() );
	m_caret.m_blinkRate->SetValue( CSSStyleManager::GetInstance().GetCaretBlinkRate() );
	m_caret.m_thickness->SetValue( CSSStyleManager::GetInstance().GetCaretThickness() );
	m_caret.m_highlight->SetValue( CSSStyleManager::GetInstance().GetCaretHighlight() );
	m_caret.m_highlightColour->SetColour( CSSStyleManager::GetInstance().GetCaretHighlightColour() );
	m_caret.m_wordHighlightColour->SetColour( CSSStyleManager::GetInstance().GetWordHighlightColour() );
}

void CSSColourSelectionDialog::OnPreviewPainted( wxStyledTextEvent& )
{

	wxString hoverText;
	wxString hoverStyles;

	const int line = 4;
	int position = m_preview->PositionFromLine( line );
	wxChar c;

	do 
	{
		//Text
		c = m_preview->GetCharAt( position );

		hoverText += c;

		//Style
		wxChar style = m_preview->GetStyleAt( position );

		if( style == 0 )
		{
			style = STYLE_DEFAULT;
		}

		hoverStyles += style;

		++position;

	} while( c != L';' );

	m_preview->AnnotationSetText( line + 1, hoverText );
	m_preview->AnnotationSetStyles( line + 1, hoverStyles );
	m_preview->AnnotationSetVisible( ANNOTATION_BOXED );

	// HACK: Force redraw by calling colourise (fixed in later version of scintilla)
	int redrawFromPosition = m_preview->PositionFromLine( line - 2 );
	int redrawToPosition = m_preview->PositionFromLine( line + 3 );
	m_preview->HACKForceRedraw( redrawFromPosition, redrawToPosition );

	m_preview->Unbind( wxEVT_STC_PAINTED, &CSSColourSelectionDialog::OnPreviewPainted, this );
}

void CSSColourSelectionDialog::CreateCaretPage( wxListbook* book )
{
	wxPanel* page = new wxPanel( book );

	wxFlexGridSizer* grid;
	grid = new wxFlexGridSizer( 1, 0, 0, 0 );
	grid->AddGrowableCol( 0 );
	grid->SetFlexibleDirection( wxBOTH );
	grid->SetNonFlexibleGrowMode( wxFLEX_GROWMODE_SPECIFIED );
	grid->SetCols( 3 );
	page->SetSizer( grid );

	// Row 1
	CreateLabelCell( grid, wxT( "Colour" ) );

	{
		wxWindow* cell = CreateCell( grid );
		m_caret.m_colour = new wxColourPickerCtrl( cell, wxID_ANY, CSSStyleManager::GetInstance().GetCaretColour(), wxDefaultPosition, wxDefaultSize );
		m_caret.m_colour->Bind( wxEVT_COMMAND_COLOURPICKER_CHANGED, &CSSColourSelectionDialog::OnCaretColourChanged, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( m_caret.m_colour, 1, wxEXPAND, 5 );
		FormatCell( grid, cell, cellSizer );
	}

	{
		wxWindow* cell = CreateCell( grid );

		wxBitmapButton* button = new wxBitmapButton
		(
			cell,
			wxID_ANY,
			m_resetColoursBitmap
		);

		button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetCaretColour, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( button, 0, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	// Row 2
	CreateLabelCell( grid, wxT( "Thickness" ) );

	{
		wxWindow* cell = CreateCell( grid );
		m_caret.m_thickness = new wxSlider( cell, wxID_ANY, CSSStyleManager::GetInstance().GetCaretThickness(), 1, 3 );
		m_caret.m_thickness->Bind( wxEVT_SCROLL_THUMBRELEASE, &CSSColourSelectionDialog::OnCaretThicknessChanged, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( m_caret.m_thickness, 0, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	{
		wxWindow* cell = CreateCell( grid );

		wxBitmapButton* button = new wxBitmapButton
		(
			cell,
			wxID_ANY,
			m_resetColoursBitmap
		);

		button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetCaretThickness, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( button, 1, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	// Row 3
	CreateLabelCell( grid, wxT( "Blink Rate" ) );

	{
		wxWindow* cell = CreateCell( grid );
		m_caret.m_blinkRate = new wxSlider( cell, wxID_ANY, CSSStyleManager::GetInstance().GetCaretBlinkRate(), 0, 1000 );
		m_caret.m_blinkRate->Bind( wxEVT_SCROLL_THUMBRELEASE, &CSSColourSelectionDialog::OnCaretBlinkRateChanged, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( m_caret.m_blinkRate, 1, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	{
		wxWindow* cell = CreateCell( grid );

		wxBitmapButton* button = new wxBitmapButton
		(
			cell,
			wxID_ANY,
			m_resetColoursBitmap
		);

		button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetCaretBlinkRate, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( button, 0, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	// Row 4
	CreateLabelCell( grid, wxT( "Highlight current line" ) );

	{
		wxWindow* cell = CreateCell( grid );

		m_caret.m_highlight = new wxCheckBox( cell, wxID_ANY, wxT( "Enabled" ) );
		m_caret.m_highlight->SetValue( CSSStyleManager::GetInstance().GetCaretHighlight() );
		m_caret.m_highlight->Bind( wxEVT_COMMAND_CHECKBOX_CLICKED, &CSSColourSelectionDialog::OnCaretHighlightChanged, this );

		m_caret.m_highlightColour = new wxColourPickerCtrl( cell, wxID_ANY, CSSStyleManager::GetInstance().GetCaretHighlightColour(), wxDefaultPosition, wxDefaultSize );
		m_caret.m_highlightColour->Bind( wxEVT_COMMAND_COLOURPICKER_CHANGED, &CSSColourSelectionDialog::OnCaretHighlightColourChanged, this );
		
		wxBoxSizer* cellSizer = new wxBoxSizer( wxHORIZONTAL );
		cellSizer->Add( m_caret.m_highlight, 1, wxLEFT | wxRIGHT | wxALIGN_CENTRE, 8 );
		cellSizer->Add( m_caret.m_highlightColour, 1, wxLEFT | wxRIGHT | wxALIGN_CENTRE, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	{
		wxWindow* cell = CreateCell( grid );

		wxBitmapButton* button = new wxBitmapButton
		(
			cell,
			wxID_ANY,
			m_resetColoursBitmap
		);

		button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetCaretHighlight, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( button, 0, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}


	// Row 5
	CreateLabelCell( grid, wxT( "Word highlight colour" ) );
	{
		wxWindow* cell = CreateCell( grid );

		m_caret.m_wordHighlightColour = new wxColourPickerCtrl( cell, wxID_ANY, CSSStyleManager::GetInstance().GetWordHighlightColour(), wxDefaultPosition, wxDefaultSize );
		m_caret.m_wordHighlightColour->Bind( wxEVT_COMMAND_COLOURPICKER_CHANGED, &CSSColourSelectionDialog::OnWordHighlightColourChanged, this );
		
		wxBoxSizer* cellSizer = new wxBoxSizer( wxHORIZONTAL );
		cellSizer->Add( m_caret.m_wordHighlightColour, 1, wxLEFT | wxRIGHT | wxALIGN_CENTRE, 8 );

		FormatCell( grid, cell, cellSizer );
	}

	{
		wxWindow* cell = CreateCell( grid );

		wxBitmapButton* button = new wxBitmapButton
		(
			cell,
			wxID_ANY,
			m_resetColoursBitmap
		);

		button->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CSSColourSelectionDialog::OnResetWordHighlightColour, this );

		wxBoxSizer* cellSizer = new wxBoxSizer( wxVERTICAL );
		cellSizer->Add( button, 0, wxLEFT | wxRIGHT, 8 );

		FormatCell( grid, cell, cellSizer );
	}


	page->Layout();
	grid->Fit( page );

	book->AddPage( page, wxT( "Caret" ) );
}

void CSSColourSelectionDialog::OnCaretColourChanged( wxColourPickerEvent& event )
{
	CSSStyleManager::GetInstance().SetCaretColour( event.GetColour() );
	m_preview->ReloadStyling();
}

void CSSColourSelectionDialog::OnCaretThicknessChanged( wxScrollEvent& event )
{
	CSSStyleManager::GetInstance().SetCaretThickness( event.GetPosition() );
	m_preview->ReloadStyling();
}

void CSSColourSelectionDialog::OnCaretBlinkRateChanged( wxScrollEvent& event )
{
	CSSStyleManager::GetInstance().SetCaretBlinkRate( event.GetPosition() );
	m_preview->ReloadStyling();
}

void CSSColourSelectionDialog::OnCaretHighlightChanged( wxCommandEvent& event )
{
	CSSStyleManager::GetInstance().SetCaretHighlight( event.IsChecked() );
	m_preview->ReloadStyling();
}

void CSSColourSelectionDialog::OnCaretHighlightColourChanged( wxColourPickerEvent& event )
{
	CSSStyleManager::GetInstance().SetCaretHighlightColour( event.GetColour() );
	m_preview->ReloadStyling();
}

void CSSColourSelectionDialog::OnWordHighlightColourChanged( wxColourPickerEvent& event )
{
	CSSStyleManager::GetInstance().SetWordHighlightColour( event.GetColour() );
	m_preview->ReloadStyling();
}


void CSSColourSelectionDialog::OnResetCaretColour( wxCommandEvent& )
{
	CSSStyleManager::GetInstance().ResetCaretColour();
	m_preview->ReloadStyling();
	RefreshWidgets();
}

void CSSColourSelectionDialog::OnResetCaretBlinkRate( wxCommandEvent& )
{
	CSSStyleManager::GetInstance().ResetCaretBlinkRate();
	m_preview->ReloadStyling();
	RefreshWidgets();
}

void CSSColourSelectionDialog::OnResetCaretThickness( wxCommandEvent& )
{
	CSSStyleManager::GetInstance().ResetCaretThickness();
	m_preview->ReloadStyling();
	RefreshWidgets();
}

void CSSColourSelectionDialog::OnResetCaretHighlight( wxCommandEvent& )
{
	CSSStyleManager::GetInstance().ResetCaretHighlight();
	CSSStyleManager::GetInstance().ResetCaretHighlightColour();
	m_preview->ReloadStyling();
	RefreshWidgets();
}

void CSSColourSelectionDialog::OnResetWordHighlightColour( wxCommandEvent& )
{
	CSSStyleManager::GetInstance().ResetWordHighlightColour();
	m_preview->ReloadStyling();
	RefreshWidgets();
}
