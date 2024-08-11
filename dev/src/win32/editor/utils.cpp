/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include <wx/webview.h>
#include "inputBoxDlg.h"
#include "comboBoxDialog.h"
#include "multiBoolDlg.h"
#include "objectInspector.h"
#include "entityList.h"
#include "sceneExplorer.h"
#include "frame.h"
#include "../../common/core/feedback.h"
#include "../../common/core/configFileManager.h"
#include "../../common/core/garbageCollector.h"
#include "../../common/core/stringLocale.h"

#if defined( W2_PLATFORM_WIN32 )
// Needed for:
// GetModuleFileNameEx()
// EnumProcessModules()
// EnumProcesses()
#include <Psapi.h>
#pragma comment( lib, "psapi.lib" )
#endif
#include "../../common/engine/layerInfo.h"


/*static*/ CEdSizeConstaints CEdSizeConstaints::EMPTY = CEdSizeConstaints( -1, -1, -1, -1 );


//////////////////////////////////////////////////////////////////////////////////////////


/*
 * Formatted dialog box code description: the code should be thought as a
 * "format pattern" in the sense of scanf, printf, etc but a bit more complex
 * due to the nature of the task.  It is made up of zero or more control
 * characters, each one defining a control in the generated dialog.  The
 * syntax of a single control is:
 *
 *   <character> [parameters] [^] [;] [*<proportion>] [=<size>]
 *
 * The ^ character makes the control focused while the ; character reverses
 * the "expanded" flag of the control (different controls have different states
 * for the flag using a "sane default" based on the control type).  The * and =
 * must be followed immediately with a positive integer number that specifies
 * the proportion and size (the same rules as with wxWidgets apply for 
 * proportions, whereas the size is the horizontal size for controls placed in
 * a horizontal box - see below - and vertical size for vertical box).
 *
 * The parameters are control-dependent (some may have no parameters).  Some
 * controls need a string which is enclosed in single quotes (for example 'foo')
 * while others need a list of strings which is formed as zero or more
 * strings enclosed in parentheses (for example ('one item' 'two items' 'etc')).
 * Whitespace is ignored unless it is inside strings (so 'foo' 'bar' is the
 * same as 'foo''bar').
 *
 * As a shortcut, single standalone string defines a label control.
 *
 * The control characters and the controls they define are as follows
 *
 *     'text'			(special case) Defines a label
 *     H [title] {...}  Defines a sub-box with horizontal alignment
 *                      for the subcontrols.  If a title is specified
 *                      the box is enclosed in a titled frame
 *     V [title] {...}  Same as H but for vertical alignment
 *     T [cols] {...}   Defines a sub-box with a tabular layout for
 *                      the subcontrols.  Cols is an integer that
 *						specifies the number of columns and if
 *						omitted, it is assumed to be 2
 *	   S				Defines a string field box
 *     F				Defines a float field box
 *     I				Defines an integer field box
 *     C <list>			Defines a choice box with the given items
 *     R [title] <list>	Defines a box with radio buttons.  If a title
 *						is given, the box is enclosed in a frame with
 *						that title
 *	   L <list>			Defines a list box
 *     M <list>         Defines a multi-check list box
 *     B [@] <caption>	Defines a push button.  If @ is used, it makes the
 *						button the default button of the dialog
 *	   X <caption>		Defines a check box
 *	   |				Defines a spacer of 5 pixels (use to put padding)
 *	   ~				Defines a stretchable spacer (use to align controls)
 *
 * The HVTCRLX controls are expanded by default while the SFI controls are
 * expanded only in vertical boxes.  You can invert the expand flag using the
 * ; character after the parameters, as mentioned above.
 * Note that the root box is vertical.
 *
 * The SFICRLX controls define data fields which are accessed via indices
 * in the CEdFormattedDialog object or via the parameters in the vararg
 * methods and functions, starting from zero (the index depends on the
 * position of the control in the code string).  The expected types for
 * these controls are:
 *
 *		S		String
 *		F		Float
 *		I		Int32
 *		C		Int32 (selection index)
 *		R		Int32 (selection index)
 *		L		Int32 (selection index)
 *      M       Bool* (checked status for each item in the list)
 *		X		Bool (checked status)
 *
 * Note that for M the function expects a pointer to the first Bool, not a
 * pointer to an array of Bools.  You can pass a TDynArray<Bool>::TypedData
 * directly there (assuming the array has enough Bools).
 *
 * Example for the FormattedDialogBox function:
 *
 *   Float x = 0.0f, y = 0.0f, z = 0.0f;
 *   FormattedDialogBox( this, wxT("Move To"), wxT("H{'Vector:'|F=32F=32F=32}H{~B@'OK'~}"), &x, &y, &z );
 *
 * This creates a box with two horizontal sub-boxes (the two Hs).  The first
 * sub-box contains a label with the "Vector:" string, a 5px spacer and tree
 * floating point number fields each 32px wide.  The second sub-box contains
 * an OK push button set as default (the @ character) which is set between
 * two stretchable spaces (this makes the button centered in the horizontal
 * sub-box).  The three pointers to floats at the end are used to fill the
 * default values for the three fields (in the order they appear) and when
 * the box is closed, the new values (if any) will be stored to them.
 *
 * The return value of FormattedDialogBox (and other vararg functions) is
 * the button number (starting from 0) that the user clicked, or -1 if the
 * user closed the window in some other way (like pressing the X button).
 * In the above example we only have one button, so we don't need this.
 *
 * You can use wxtest.exe from T:\badsector\fdb to try out codes.
 */

CEdFormattedDialog::CEdFormattedDialog()
	: m_head( 0 )
	, m_root( NULL )
	, m_nextIndex( 0 )
	, m_buttons( 0 )
	, m_hiddenFrame( NULL )
	, m_defaultWindow( NULL )
	, m_focusedWindow( NULL )
{
}

CEdFormattedDialog::~CEdFormattedDialog()
{
	ResetGUI();
	if ( m_hiddenFrame )
	{
		m_hiddenFrame->Destroy();
	}
}

void CEdFormattedDialog::SubPanelElement::Parse( CEdFormattedDialog* fmt )
{
	// Check if we are inside a block and if so, skip the block start character
	Bool insideBlock = fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '{';
	if ( insideBlock )
	{
		fmt->m_head++;
	}

	// Create the panel
	wxPanel* panel = new wxPanel( GetParentWindow() );
	if ( m_title.IsEmpty() )
	{
		switch ( m_subPanelType )
		{
		case SPT_Horizontal:
			panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
			break;
		case SPT_Vertical:
			panel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
			break;
		case SPT_Table:
			panel->SetSizer( new wxFlexGridSizer( m_columns, 5, 5 ) );
			break;
		}
	}
	else
	{
		switch ( m_subPanelType )
		{
		case SPT_Horizontal:
			panel->SetSizer( new wxStaticBoxSizer( wxHORIZONTAL, panel, m_title ) );
			break;
		case SPT_Vertical:
			panel->SetSizer( new wxStaticBoxSizer( wxVERTICAL, panel, m_title ) );
			break;
		}
	}
	m_window = panel;

	// Scan the block contents
	while ( fmt->m_head < fmt->m_code.Length() )
	{
		wxString boxTitle;				// title for boxes (only for H and V)
		Element* newElement = NULL;		// newly created element 
		SubPanelType type;				// type for subpanels
		int columns = 2;				// table columns
		int sizerFlags = 0;				// additional flags for the sizer
		Bool defaultFocus = false;		// if true, the control will get focus if no other control has focus
		fmt->SkipSpaces();

		// End of code, stop
		if ( fmt->m_head >= fmt->m_code.Length() )
		{
			break;
		}

		// Create subelement based on character code
		switch ( fmt->m_code[fmt->m_head].GetValue() )
		{
		case 'H':
		case 'V':
			type = fmt->m_code[fmt->m_head] == 'H' ? SPT_Horizontal : SPT_Vertical;
			fmt->m_head++;
			fmt->SkipSpaces();
			if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '\'' ) // optional title
			{
				boxTitle = fmt->ScanString();
			}
			if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '{' ) // block start
			{
				newElement = new SubPanelElement( this );
				static_cast< SubPanelElement* >( newElement )->m_title = boxTitle;
				static_cast< SubPanelElement* >( newElement )->m_subPanelType = type;
			}
			sizerFlags = wxEXPAND;
			// H/V without a block start is an error and this will stop the parsing
			break;
		case 'T':
			fmt->m_head++;
			fmt->SkipSpaces();
			if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] >= '1' && fmt->m_code[fmt->m_head] <= '9' ) // columns
			{
				columns = fmt->ScanInteger();
			}
			if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '{' ) // block start
			{
				newElement = new SubPanelElement( this );
				static_cast< SubPanelElement* >( newElement )->m_subPanelType = SPT_Table;
				static_cast< SubPanelElement* >( newElement )->m_columns = columns;
			}
			sizerFlags = wxEXPAND;
			// H/V without a block start is an error and this will stop the parsing
			break;
		case '}':	// end of block
			if ( insideBlock )
			{
				fmt->m_head++;
				return;
			}
			// end-of-block outside of a block is an error and this will stop the parsing
			break;
		case '\'':	// shortcut
			newElement = new LabelElement( this );
			break;
		case 'S':
		case 'F':
		case 'I':
			switch ( fmt->m_code[fmt->m_head].GetValue() )
			{
			case 'S':
				newElement = new FieldElement( this, FT_String );
				break;
			case 'F':
				newElement = new FieldElement( this, FT_Float );
				break;
			case 'I':
				newElement = new FieldElement( this, FT_Integer );
				break;
			}
			fmt->m_head++;
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = m_subPanelType == SPT_Vertical ? wxEXPAND : 0;
			defaultFocus = true;
			break;
		case 'C':
			fmt->m_head++;
			newElement = new ChoiceElement( this );
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = wxEXPAND;
			defaultFocus = true;
			break;
		case 'R':
			fmt->m_head++;
			newElement = new RadioElement( this );
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = wxEXPAND;
			defaultFocus = true;
			break;
		case 'L':
			fmt->m_head++;
			newElement = new ListElement( this );
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = wxEXPAND;
			defaultFocus = true;
			break;
		case 'M':
			fmt->m_head++;
			newElement = new MultiCheckListElement( this );
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = wxEXPAND;
			defaultFocus = true;
			break;
		case 'B':
			fmt->m_head++;
			newElement = new ButtonElement( this, fmt->m_buttons++ );
			break;
		case 'X':
			fmt->m_head++;
			newElement = new CheckBoxElement( this );
			newElement->m_index = fmt->m_nextIndex++;
			sizerFlags = wxEXPAND;
			defaultFocus = true;
			break;
		case '|':
			fmt->m_head++;
			panel->GetSizer()->AddSpacer( 5 );
			continue;
		case '~':
			fmt->m_head++;
			panel->GetSizer()->AddStretchSpacer();
			continue;
		}

		// If the element wasn't created stop parsing here
		if ( !newElement )
		{
			break;
		}

		// Parse the element's subcode
		newElement->Parse( fmt );

		// Check for focus
		fmt->SkipSpaces();
		if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '^' )
		{
			fmt->m_head++;
			fmt->m_focusedWindow = newElement->m_window;
		}

		// Check for un/expand
		fmt->SkipSpaces();
		if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == ';' )
		{
			fmt->m_head++;
			if ( sizerFlags & wxEXPAND )
			{
				sizerFlags &= ~wxEXPAND;
			}
			else
			{
				sizerFlags |= wxEXPAND;
			}
		}

		// Check for proportion
		Int32 proportion = 0;
		fmt->SkipSpaces();
		if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '*' )
		{
			fmt->m_head++;
			proportion = fmt->ScanInteger();
		}

		// Check for size
		Int32 size = 0;
		fmt->SkipSpaces();
		if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '=' )
		{
			fmt->m_head++;
			size = fmt->ScanInteger();
		}

		// Add the element's window to the sizer
		if ( newElement->m_window )
		{
			if ( size > 0 )
			{
				wxSize best = newElement->m_window->GetBestSize();
				if ( m_subPanelType == SPT_Horizontal || m_subPanelType == SPT_Table )
				{
					best.SetWidth( size );
				}
				else
				{
					best.SetHeight( size );
				}
				newElement->m_window->SetMinSize( best );
				newElement->m_window->SetSize( best );
			}
			panel->GetSizer()->Add( newElement->m_window, proportion, sizerFlags | ( m_subPanelType == wxVERTICAL ? wxALIGN_LEFT : wxALIGN_CENTER_VERTICAL ), 0 );
			if ( defaultFocus && !fmt->m_focusedWindow )
			{
				fmt->m_focusedWindow = newElement->m_window;
			}
		}
	}

	// Resize the panel to fit the contents
	FitRecursively( panel, false );
}

void CEdFormattedDialog::LabelElement::Parse( CEdFormattedDialog* fmt )
{
	wxString caption = fmt->ScanString();
	wxStaticText* lbl = new wxStaticText( GetParentWindow(), wxID_ANY, caption );
	m_window = lbl;
}

void CEdFormattedDialog::ButtonElement::Parse( CEdFormattedDialog* fmt )
{
	bool isDefault = fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '@';
	if ( isDefault )
	{
		fmt->m_head++;
	}
	wxString caption = fmt->ScanString();
	wxButton* btn = new wxButton( GetParentWindow(), wxID_ANY, caption );
	if ( isDefault )
	{
		fmt->m_defaultWindow = btn;
		btn->SetDefault();
	}
	btn->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdFormattedDialog::ButtonElement::OnClick ), NULL, this );
	m_window = btn;
}

void CEdFormattedDialog::ButtonElement::OnClick( wxCommandEvent& event )
{
	wxWindow* top = m_window;
	while ( !wxIS_KIND_OF( top, wxDialog ) )
	{
		top = top->GetParent();
	}

	if ( top )
	{
		wxDialog* dlg = static_cast< wxDialog* >( top );
		dlg->EndModal( ModalResultBase + m_buttonIndex );
	}
}

void CEdFormattedDialog::CheckBoxElement::StoreData( void* data )
{
	Bool* target = static_cast< Bool* >( data );
	*target = static_cast< wxCheckBox* >( m_window )->GetValue();
}

void CEdFormattedDialog::CheckBoxElement::LoadData( void* data )
{
	static_cast< wxCheckBox* >( m_window )->SetValue( *static_cast< Bool* >( data ) );
}

void CEdFormattedDialog::CheckBoxElement::Parse( CEdFormattedDialog* fmt )
{
	wxString caption = fmt->ScanString();
	wxCheckBox* box = new wxCheckBox( GetParentWindow(), wxID_ANY, caption );
	m_window = box;
}

void CEdFormattedDialog::FieldElement::StoreData( void* data )
{
	String* targets = static_cast< String* >( data );
	Int32* targeti = static_cast< Int32* >( data );
	Float* targetf = static_cast< Float* >( data );
	long l;
	double f;

	switch ( m_type )
	{
	case FT_String:
		*targets = static_cast< wxTextCtrl* >( m_window )->GetValue().wc_str();
		break;
	case FT_Integer:
		if ( static_cast< wxTextCtrl* >( m_window )->GetValue().ToLong( &l ) )
		{
			*targeti = static_cast<Int32>( l );
		}
		else
		{
			*targeti = 0;
		}
		break;
	case FT_Float:
		if ( static_cast< wxTextCtrl* >( m_window )->GetValue().ToDouble( &f ) )
		{
			*targetf = static_cast<Float>( f );
		}
		else
		{
			*targetf = 0;
		}
		break;
	}
}

void CEdFormattedDialog::FieldElement::LoadData( void* data )
{
	String* sources = static_cast< String* >( data );
	Int32* sourcei = static_cast< Int32* >( data );
	Float* sourcef = static_cast< Float* >( data );

	switch ( m_type )
	{
	case FT_String:
		static_cast< wxTextCtrl* >( m_window )->SetValue( (*sources).AsChar() );
		break;
	case FT_Integer:
		static_cast< wxTextCtrl* >( m_window )->SetValue( wxString::Format( wxT("%d"), *sourcei ) );
		break;
	case FT_Float:
		static_cast< wxTextCtrl* >( m_window )->SetValue( wxString::Format( wxT("%f"), *sourcef ) );
		break;
	}
}

void CEdFormattedDialog::FieldElement::Parse( CEdFormattedDialog* fmt )
{
	wxTextCtrl* fld = new wxTextCtrl( GetParentWindow(), wxID_ANY );
	m_window = fld;
}

void CEdFormattedDialog::ChoiceElement::StoreData( void* data )
{
	Int32* target = static_cast< Int32* >( data );
	*target = static_cast< wxChoice* >( m_window )->GetSelection();
}

void CEdFormattedDialog::ChoiceElement::LoadData( void* data )
{
	static_cast< wxChoice* >( m_window )->SetSelection( *static_cast< Int32* >( data ) );
}

void CEdFormattedDialog::ChoiceElement::Parse( CEdFormattedDialog* fmt )
{
	wxChoice* ch = new wxChoice( GetParentWindow(), wxID_ANY );
	wxVector<wxString> items = fmt->ScanList();
	ch->Freeze();
	for ( size_t i=0; i < items.size(); ++i )
	{
		ch->Append( items[i] );
	}
	if ( items.size() > 0 )
	{
		ch->SetSelection( 0 );
	}
	ch->Thaw();
	m_window = ch;
}

void CEdFormattedDialog::RadioElement::StoreData( void* data )
{
	Int32* target = static_cast< Int32* >( data );
	for ( size_t i=0; i < m_buttons.size(); ++i )
	{
		if ( m_buttons[i]->GetValue() )
		{
			*target = i;
			break;
		}
	}
}

void CEdFormattedDialog::RadioElement::LoadData( void* data )
{
	Int32* source = static_cast< Int32* >( data );
	for ( size_t i=0; i < m_buttons.size(); ++i )
	{
		m_buttons[i]->SetValue( *source == static_cast<Int32>( i ) );
	}
}

void CEdFormattedDialog::RadioElement::Parse( CEdFormattedDialog* fmt )
{
	wxPanel* panel = new wxPanel( GetParentWindow() );
	if ( fmt->m_head < fmt->m_code.Length() && fmt->m_code[fmt->m_head] == '\'' )
	{
		panel->SetSizer( new wxStaticBoxSizer( wxVERTICAL, panel, fmt->ScanString() ) );
	}
	else
	{
		panel->SetSizer( new wxBoxSizer( wxVERTICAL ) );
	}
	wxVector<wxString> items = fmt->ScanList();
	m_window = panel;
	for ( size_t i=0; i < items.size(); ++i )
	{
		wxRadioButton* button = new wxRadioButton( panel, wxID_ANY|( i == 0 ? wxRB_GROUP : 0 ), items[i] );
		panel->GetSizer()->Add( button, 0, wxEXPAND|( i == 0 ? 0 : wxTOP ), 2 );
		m_buttons.push_back( button );
	}
	if ( items.size() > 0 )
	{
		m_buttons[0]->SetValue( true );
	}
	FitRecursively( panel, false );
}

void CEdFormattedDialog::ListElement::StoreData( void* data )
{
	Int32* target = static_cast< Int32* >( data );
	*target = static_cast< wxListBox* >( m_window )->GetSelection();
}

void CEdFormattedDialog::ListElement::LoadData( void* data )
{
	static_cast< wxListBox* >( m_window )->SetSelection( *static_cast< Int32* >( data ) );
}

void CEdFormattedDialog::ListElement::Parse( CEdFormattedDialog* fmt )
{
	wxListBox* lb = new wxListBox( GetParentWindow(), wxID_ANY );
	wxVector<wxString> items = fmt->ScanList();
	lb->Freeze();
	for ( size_t i=0; i < items.size(); ++i )
	{
		lb->Append( items[i] );
	}
	if ( items.size() > 0 )
	{
		lb->SetSelection( 0 );
	}
	lb->Thaw();
	m_window = lb;
}

void CEdFormattedDialog::MultiCheckListElement::StoreData( void* data )
{
	Bool* target = static_cast< Bool* >( data );
	wxCheckListBox* list = static_cast< wxCheckListBox* >( m_window );
	for ( unsigned int i=0; i < list->GetCount(); ++i )
	{
		target[i] = list->IsChecked( i );
	}
}

void CEdFormattedDialog::MultiCheckListElement::LoadData( void* data )
{
	Bool* target = static_cast< Bool* >( data );
	wxCheckListBox* list = static_cast< wxCheckListBox* >( m_window );
	for ( unsigned int i=0; i < list->GetCount(); ++i )
	{
		list->Check( i, target[i] );
	}
}

void CEdFormattedDialog::MultiCheckListElement::Parse( CEdFormattedDialog* fmt )
{
	wxCheckListBox* lb = new wxCheckListBox( GetParentWindow(), wxID_ANY );
	wxVector<wxString> items = fmt->ScanList();
	lb->Freeze();
	for ( size_t i=0; i < items.size(); ++i )
	{
		lb->Append( items[i] );
	}
	lb->Thaw();
	m_window = lb;
}
void CEdFormattedDialog::SkipSpaces()
{
	while ( m_head < m_code.Length() && ( m_code[m_head] == '\n' || m_code[m_head] == '\r' || m_code[m_head] == '\t' || m_code[m_head] == ' ' ) )
	{
		m_head++;
	}
}

wxString CEdFormattedDialog::ScanString()
{
	SkipSpaces();

	if ( m_head < m_code.Length() && m_code[m_head] == '\'' )
	{
		m_head++;
	}
	else
	{
		return wxEmptyString;
	}

	wxString result;

	while ( m_head < m_code.Length() )
	{
		if ( m_code[m_head] == '\\' )
		{
			m_head++;
			result += m_code[m_head++];
		}
		else if ( m_code[m_head] == '\'' )
		{
			m_head++;
			break;
		}
		else
		{
			result += m_code[m_head++];
		}
	}

	return result;
}

wxVector<wxString> CEdFormattedDialog::ScanList()
{
	wxVector<wxString> result;
	SkipSpaces();

	if ( m_head < m_code.Length() && m_code[m_head] != '(' )
	{
		return result;
	}

	m_head++;

	while ( m_head < m_code.Length() )
	{
		SkipSpaces();

		// End of list
		if ( m_head < m_code.Length() && m_code[m_head] == ')' )
		{
			m_head++;
			break;
		}

		// Make sure we have a string following
		if ( m_head < m_code.Length() && m_code[m_head] != '\'' )
		{
			break;
		}

		// Add a scanned string
		result.push_back( ScanString() );
	}

	return result;
}

Int32 CEdFormattedDialog::ScanInteger()
{
	Int32 result = 0;

	SkipSpaces();
	Bool negate = m_code[m_head] == '-';
	if ( negate )
	{
		m_head++;
	}

	while ( m_head < m_code.Length() && m_code[m_head] >= '0' && m_code[m_head] <= '9' )
	{
		result = result*10 + ( m_code[m_head] - '0' );
		m_head++;
	}

	return negate ? -result : result;
}

CEdFormattedDialog::Element* CEdFormattedDialog::FindElementChildByIndex( Element* element, Int32 index ) const
{
	// Check the passed element
	if ( element->m_index == index )
	{
		return element;
	}

	// Check the element's children
	for ( Element* child=element->m_firstChild; child; child=child->m_next )
	{
		Element* childElement = FindElementChildByIndex( child, index );
		if ( childElement )
		{
			return childElement;
		}
	}

	// Element not found
	return NULL;
}

CEdFormattedDialog::Element* CEdFormattedDialog::FindElementByIndex( Int32 index ) const
{
	return FindElementChildByIndex( m_root, index );
}

void CEdFormattedDialog::ResetGUI()
{
	// Delete the root
	if ( m_root )
	{
		if ( m_root->m_window )
		{
			m_root->m_window->Destroy();
		}
		delete m_root;
	}
	m_root = NULL;
	m_nextIndex = 0;
	m_buttons = 0;
	m_defaultWindow = NULL;
	m_focusedWindow = NULL;
}

void CEdFormattedDialog::DoParse()
{
	// Reset GUI
	ResetGUI();

	// Create root
	if ( !m_hiddenFrame )
	{
		m_hiddenFrame = new wxFrame( NULL, wxID_ANY, wxT("Hidden") );
	}
	m_root = new RootElement( m_hiddenFrame );

	// Begin parsing
	SkipSpaces();
	m_root->Parse( this );
}

Bool CEdFormattedDialog::Parse( const wxString& code )
{
	m_code = code;
	DoParse();
	return m_root != NULL && m_root->m_window != NULL;
}

wxWindow* CEdFormattedDialog::GetRootWindow() const
{
	return m_root ? m_root->m_window : NULL;
}

wxDialog* CEdFormattedDialog::CreateDialog( wxWindow* parent, const wxString& caption )
{
	// Get root window
	wxWindow* rootWindow = GetRootWindow();
	if ( !rootWindow )
	{
		return 0;
	}

	// Create new dialog
	wxDialog* dlg = new wxDialog( parent, wxID_ANY, caption );
	dlg->SetSizer( new wxBoxSizer( wxVERTICAL ) );

	// Add root window to the dialog
	rootWindow->Reparent( dlg );
	dlg->GetSizer()->Add( rootWindow, 1, wxALL, 5 );
	FitRecursively( dlg, false );
	dlg->CenterOnParent();

	// Activate the default window, if any
	if ( GetDefaultWindow() )
	{
		// Set the button to look as default
		if ( wxIS_KIND_OF( GetDefaultWindow(), wxButton ) )
		{
			static_cast< wxButton* >( GetDefaultWindow() )->SetDefault();
		}

		dlg->SetDefaultItem( GetDefaultWindow() );
	}

	// Focus the window, if specified
	if ( GetFocusedWindow() )
	{
		GetFocusedWindow()->SetFocus();
	}

	return dlg;
}

void CEdFormattedDialog::StoreElementDataTo( Int32 index, void* data ) const
{
	Element* element = FindElementByIndex( index );
	if ( element )
	{
		element->StoreData( data );
	}
}

void CEdFormattedDialog::LoadElementDataFrom( Int32 index, void* data )
{
	Element* element = FindElementByIndex( index );
	if ( element )
	{
		element->LoadData( data );
	}
}

Int32 CEdFormattedDialog::ShowFormattedDialogVA( wxWindow* parent, const wxString& caption, const wxString& code, va_list va )
{
	CEdFormattedDialog fmt;

	// Try to parse the code
	if ( !fmt.Parse( code ) )
	{
		return 0;
	}

	// Grab the pointers to data
	void** pointers = new void*[fmt.GetIndexCount()];
	for ( Int32 i=0; i < fmt.GetIndexCount(); ++i )
	{
		pointers[i] = va_arg( va, void* );
	}

	// Load the values in the passed pointers
	for ( Int32 i=0; i < fmt.GetIndexCount(); ++i )
	{
		fmt.LoadElementDataFrom( i, pointers[i] );
	}

	// Create the dialog
	wxDialog* dlg = fmt.CreateDialog( parent, caption );
	if ( !dlg )
	{
		delete[] pointers;
		return 0;
	}

	// Show the dialog
	Int32 r = dlg->ShowModal();
	dlg->Destroy();

	// Store the values in the passed pointers
	for ( Int32 i=0; i < fmt.GetIndexCount(); ++i )
	{
		fmt.StoreElementDataTo( i, pointers[i] );
	}

	// Get rid of the pointers array
	delete[] pointers;

	// Return dialog result
	if ( r < ModalResultBase )
	{
		return -1;
	}
	return r - ModalResultBase;
}

Int32 CEdFormattedDialog::ShowFormattedDialog( wxWindow* parent, const wxString& caption, wxString code, ... )
{
	va_list va;
	va_start( va, code );
	Int32 r = ShowFormattedDialogVA( parent, caption, code, va );
	va_end( va );
	return r;
}

Int32 FormattedDialogBox( wxWindow* parent, const wxString& caption, wxString code, ... )
{
	va_list va;
	va_start( va, code );
	Int32 r = CEdFormattedDialog::ShowFormattedDialogVA( parent, caption, code, va );
	va_end( va );
	return r;
}

Int32 FormattedDialogBox( const wxString& caption, wxString code, ... )
{
	va_list va;
	va_start( va, code );
	Int32 r = CEdFormattedDialog::ShowFormattedDialogVA( NULL, caption, code, va );
	va_end( va );
	return r;
}

//////////////////////////////////////////////////////////////////////////

void HtmlBox( wxWindow* parent, const String& title, const String& html, Bool navbar /* = false */, SimpleWebViewHandler* handler /* = nullptr */ )
{
	struct TheDialog : public wxDialog
	{
		wxTextCtrl*				m_url;
		wxWebView*				m_webView;
		SimpleWebViewHandler*	m_handler;

		TheDialog( wxWindow* parent, const wxString& title, const wxString& html, Bool navbar, SimpleWebViewHandler* handler )
			: m_handler( nullptr )
		{
			Create( parent, wxID_ANY, title, wxDefaultPosition, wxSize( 800, 600 ), wxCAPTION|wxRESIZE_BORDER|wxSYSTEM_MENU|wxCLOSE_BOX|wxMAXIMIZE_BOX );
			SetSizer( new wxBoxSizer( wxVERTICAL ) );

			if ( navbar )
			{
				wxPanel* panel = new wxPanel( this, wxID_ANY );
				panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );
				panel->GetSizer()->Add( new wxBitmapButton( panel, 100, wxArtProvider::GetBitmap( wxART_GO_BACK ), wxDefaultPosition, wxDefaultSize, 0 ) );
				panel->GetSizer()->Add( new wxBitmapButton( panel, 101, wxArtProvider::GetBitmap( wxART_GO_FORWARD ), wxDefaultPosition, wxDefaultSize, 0 ) );
				m_url = new wxTextCtrl( panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER );
				panel->GetSizer()->Add( m_url, 1, wxALIGN_CENTER_VERTICAL|wxLEFT, 5 );
				GetSizer()->Add( panel, 0, wxEXPAND );
			}
			else
			{ 
				m_url = nullptr; 
			}

			m_webView = wxWebView::New();
			m_webView->Create( this, wxID_ANY );
			GetSizer()->Add( m_webView, 1, wxEXPAND );
			m_webView->SetPage( html, wxEmptyString );

			if( handler )
			{
				m_handler = handler;
			}
					

			Bind( wxEVT_CHAR_HOOK, &TheDialog::OnKey, this );

			if ( navbar )
			{
				Bind( wxEVT_COMMAND_BUTTON_CLICKED, &TheDialog::OnBack, this, 100 );
				Bind( wxEVT_COMMAND_BUTTON_CLICKED, &TheDialog::OnForward, this, 101 );
				Bind( wxEVT_COMMAND_TEXT_ENTER, &TheDialog::OnGo, this, m_url->GetId() );
			}
			Bind( wxEVT_COMMAND_WEB_VIEW_NAVIGATING, &TheDialog::OnNavigating, this, m_webView->GetId() );

			CenterOnScreen();
		}

		void OnKey( wxKeyEvent& evt )
		{
			if( evt.GetKeyCode() == WXK_ESCAPE )
			{
				Close();
			}
		}

		void OnBack( wxCommandEvent& event )
		{
			if ( m_webView->CanGoBack() )
			{
				m_webView->GoBack();
			}
		}

		void OnForward( wxCommandEvent& event )
		{
			if ( m_webView->CanGoForward() )
			{
				m_webView->GoForward();
			}
		}

		void OnGo( wxCommandEvent& event )
		{
			m_webView->LoadURL( m_url->GetValue() );
		}

		void OnNavigating( wxWebViewEvent& event )
		{
			if ( m_handler )
			{			
				event.Veto();
				m_handler->HandlePage( event.GetURL().wc_str() );				
			}			

			wxString url = m_webView->GetCurrentURL();
			if ( m_url != nullptr )
			{
				m_url->SetValue( url );
			}
		}
	} dialog( parent, title.AsChar(), html.AsChar(), navbar, handler );
	dialog.ShowModal();
	dialog.Destroy();
}

//////////////////////////////////////////////////////////////////////////////////////////

void ScreenToWorkspace( Int32& x, Int32& y )
{
 	POINT p = { x, y };
 	MONITORINFO mi;
	mi.cbSize = sizeof( MONITORINFO );
 	::GetMonitorInfo( ::MonitorFromPoint( p, MONITOR_DEFAULTTONEAREST ), &mi );
	x -= mi.rcWork.left - mi.rcMonitor.left;
	y -= mi.rcWork.top  - mi.rcMonitor.top;
}

void WorkspaceToScreen( Int32& x, Int32& y )
{
 	POINT p = { x, y };
 	MONITORINFO mi;
	mi.cbSize = sizeof( MONITORINFO );
 	::GetMonitorInfo( ::MonitorFromPoint( p, MONITOR_DEFAULTTONEAREST ), &mi );
	x += mi.rcWork.left - mi.rcMonitor.left;
	y += mi.rcWork.top  - mi.rcMonitor.top;
}

//////////////////////////////////////////////////////////////////////////////////////////

void RunLaterEx( CEdRunnable* runnable )
{
	if( wxEdApp != nullptr )
	{
		wxEdApp->RunLater( runnable );
	}
}

void RunLaterOnceEx( CEdRunnable* runnable )
{
	if( wxEdApp != nullptr )
	{
		wxEdApp->RunLaterOnce( runnable );
	}
}

void RunParallelEx( CEdRunnable* taskRunnable, class CEdRunnable* afterFinishRunnable )
{
	if( wxEdApp != nullptr )
	{
		wxEdApp->RunParallel( taskRunnable, afterFinishRunnable );
	}
}

void RunInMainThreadEx( CEdRunnable* taskRunnable )
{
	struct RunInMainThreadTask : public CEdRunnable
	{
		Red::Threads::CMutex m_mutex;
		CEdRunnable* m_taskRunnable;
		Bool m_done;

		RunInMainThreadTask( CEdRunnable* taskRunnable ) 
			: m_taskRunnable( taskRunnable ), m_done( false ) 
		{}

		void Run()
		{
			m_taskRunnable->Run();

			// Set flag
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
				m_done = true;
			}

			// Wait until the flag is reset
			while ( true )
			{
				Red::Threads::CScopedLock< Red::Threads::CMutex > lock( m_mutex );
				if ( !m_done )
				{
					break;
				}
				::Sleep( 1 );
			}
		}
	};

	// If we're on the main thread, just run the task immediatelly
	if ( SIsMainThread() )
	{
		taskRunnable->Run();
		return;
	}

	// Schedule task for the main thread
	RunInMainThreadTask* task = new RunInMainThreadTask( taskRunnable );
	RunLaterEx( task );

	// Wait until the flag is set from the task
	while ( true )
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( task->m_mutex );
		if ( task->m_done )
		{
			break;
		}
		::Sleep( 1 );
	}

	// Reset the flag so that the main thread task will finish
	{
		Red::Threads::CScopedLock< Red::Threads::CMutex > lock( task->m_mutex );
		task->m_done = false;
	}
}

void DestroyLater( wxWindow* window )
{
	struct DisposeLaterRunnable : public CEdRunnable {
		wxWindow* m_window;
		DisposeLaterRunnable( wxWindow* window ) : m_window( window ) {}

		virtual void Run()
		{
			m_window->Destroy();
		}

		virtual bool IsDuplicate( CEdRunnable* runnable )
		{
			return CEdRunnable::IsDuplicate( runnable ) && static_cast<DisposeLaterRunnable*>( runnable )->m_window == m_window;
		}
	};
	RunLaterOnceEx( new DisposeLaterRunnable( window ) );
}

void RefreshLater( wxWindow* window )
{
	// We store the HWND instead of the wxWindow directly
	// in case the window gets destroyed in the meanwhile
	struct RefreshLaterRunnable : public CEdRunnable {
		HWND m_hwnd;
		RefreshLaterRunnable( wxWindow* window ) : m_hwnd( window->GetHWND() ) {}

		virtual void Run()
		{
			wxWindow* window = wxGetWindowFromHWND( m_hwnd );
			if ( window != nullptr )
			{
				window->Refresh();
			}
		}

		virtual bool IsDuplicate( CEdRunnable* runnable )
		{
			return CEdRunnable::IsDuplicate( runnable ) && static_cast<RefreshLaterRunnable*>( runnable )->m_hwnd == m_hwnd;
		}
	};
	RunLaterOnceEx( new RefreshLaterRunnable( window ) );
}

Bool InputBox( wxWindow* parent, const String& title, const String& message, String &value, Bool multiline /* = false */ )
{
	CEdInputBox dlg( parent, title, message, value, multiline );
	if ( dlg.ShowModal() == 1 )
	{
		value = dlg.GetEditText();
		return true;
	}

	return false;
}


String InputBox( wxWindow* parent, const String& title, const String& message, const String &value, Bool multiline /* = false */ )
{
	String tempValue = value;
	InputBox( parent, title, message, tempValue, multiline );
	return tempValue;
}

Bool InputDoubleBox( wxWindow* parent, const String& title, const String& message, String &valueA, String &valueB, Bool multiline, Bool useConfig )
{
	CEdInputDoubleBox dlg( parent, title, message, valueA, valueB, multiline, useConfig );
	if ( dlg.ShowModal() == 1 )
	{
		return true;
	}

	return false;
}

Bool InputMultiBox( wxWindow* parent, const String& title, const String& message, TDynArray< String >&values )
{
	CEdInputMultiBox dlg( parent, title, message, values );
	if ( dlg.ShowModal() == 0 )
	{
		return true;
	}

	return false;
}

Bool InputBoxFileName( wxWindow* parent, const String& title, const String& message, String &value, const String& fileExtension )
{
	CEdInputBoxFileName dlg( parent, title, message, value, fileExtension );
	if ( dlg.ShowModal() == 1 )
	{
		value = dlg.GetEditText();
		return value.Empty() ? false : true;
	}

	return false;
};

Bool YesNo( const Char* message, ... )
{
	size_t fmtSize = 256 + Red::System::StringLength(message);
	Char* formattedMessage = NULL;

	while ( !formattedMessage )
	{
		formattedMessage = static_cast<Char*>( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary , fmtSize*sizeof(Char)) );
		va_list arglist;
		va_start( arglist, message );
		int r = Red::System::VSNPrintF( formattedMessage, fmtSize, message, arglist );
		va_end( arglist );
		if ( r < 0 )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, formattedMessage );
			formattedMessage = NULL;
			fmtSize *= 2;
		}
	}

	Bool r = IDYES == MessageBox( NULL, formattedMessage, TXT("Question"), MB_ICONQUESTION | MB_YESNO | MB_TASKMODAL | MB_TOPMOST | MB_DEFBUTTON2 );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, formattedMessage );

	return r;
}

extern Bool MultiBoolDialog(wxWindow* parent, const String& title, const TDynArray<String>& questions, TDynArray<Bool>& answers)
{
	if(questions.Size() == 0)
		return false;
	
	CEdMultiBoolDlg dlg(parent, title, questions, answers);
	if ( dlg.ShowModal() )
	{
		dlg.GetValues(answers);
		return answers.Empty() ? false : true;
	}
	return false;
}

Int32 YesNoCancel( const Char* message, ... )
{
	size_t fmtSize = 256 + Red::System::StringLength(message);
	Char* formattedMessage = NULL;

	while ( !formattedMessage )
	{
		formattedMessage = static_cast<Char*>( RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Temporary , fmtSize*sizeof(Char)) );
		va_list arglist;
		va_start( arglist, message );
		int r = Red::System::VSNPrintF( formattedMessage, fmtSize, message, arglist );
		va_end( arglist );
		if ( r < 0 )
		{
			RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, formattedMessage );
			formattedMessage = NULL;
			fmtSize *= 2;
		}
	}

	int r = MessageBox( NULL, formattedMessage, TXT("Question"), MB_ICONQUESTION | MB_YESNOCANCEL | MB_TASKMODAL | MB_TOPMOST | MB_DEFBUTTON2 );

	RED_MEMORY_FREE( MemoryPool_Default, MC_Temporary, formattedMessage );

	return r;
}

String InputComboBox( wxWindow* parent, const String& title, const String& message, const String &defaultChoice, const TDynArray<String>& choices)
{
	CEdComboBoxDlg dlg( parent, title, message, defaultChoice, choices);
	dlg.ShowModal();
	return dlg.GetSelectText();
}

static TDynArray<String> SActiveResources;

void SetActiveResources( const TDynArray<String>& resourcePaths )
{
	SActiveResources = resourcePaths;

#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().DispatchEvent( CNAME( OnActiveResChanged ), NULL );
#endif

}

Bool GetActiveResource( String& resourcePath )
{
	if ( !SActiveResources.Empty() )
	{
		resourcePath = SActiveResources[0];
		return true;
	}

	return false;
}

Bool GetActiveResource( String& resourcePath, CClass* filterClass )
{
	String path;
	if ( GetActiveResource( path ) )
	{
		// Assume at least a resource class
		if ( !filterClass || !filterClass->IsA< CResource >() )
		{
			filterClass = ClassID< CResource >();
		}

		// Get all matching resource classes
		TDynArray< CClass* > matchingResourceClasses;
		SRTTI::GetInstance().EnumClasses( filterClass, matchingResourceClasses );

		// Check extension
		Bool validExtensionFound = false;
		for ( Uint32 i=0; i<matchingResourceClasses.Size(); ++i )
		{
			CClass* matchingFilterClass = matchingResourceClasses[i];
			CResource* defaultResource = matchingFilterClass->GetDefaultObject< CResource >();
			if ( defaultResource )
			{
				const String resourceExtension = String( defaultResource->GetExtension() ).ToLower();
				if ( path.ToLower().EndsWith( resourceExtension ) )
				{
					validExtensionFound = true;
					break;
				}
			}

		}

		// It's a valid resource path for given resource class
		if ( validExtensionFound )
		{
			// Valid resource
			resourcePath = path;			
			return true;
		}
	}

	// Not so valid...
	return false;
}

const TDynArray<String>& GetActiveResources()
{
	return SActiveResources;
}

static String SActiveDirectory;

void SetActiveDirectory( const String& path )
{
	SActiveDirectory = path;
}

const String& GetActiveDirectory()
{
	return SActiveDirectory;
}

CEntity* GetSelectedEntity()
{
	if ( GGame->GetActiveWorld() )
	{
		// Get all selected entities
		TDynArray< CEntity * > entities;
		wxTheFrame->GetWorldEditPanel()->GetSelectionManager()->GetSelectedEntities( entities );

		// We may have only one entity selected
		if ( entities.Size() == 1 )
		{
			return entities[0];
		}
	}

	// Nothing
	return NULL;
}

CEntity* GetSelectedEntityInTheSameLayerAs( CObject* object )
{
	// Get selected entity in any layer 
	CEntity* entity = GetSelectedEntity();

	// Entity should be in the same layer as given object
	if ( entity && object && entity->GetRoot() == object->GetRoot() )
	{
		return entity;
	}

	// Nothing
	return NULL;
}

wxMenu* GetSubMenu( wxMenu* baseMenu, const String& menuName, Bool createIfNotFound )
{
	// No menu
	if ( !baseMenu || menuName.Empty() )
	{
		return NULL;
	}

	// Find item of given name
	size_t count = baseMenu->GetMenuItemCount();
	for ( size_t i=0; i<count; i++ )
	{
		// Get item
		wxMenuItem* item = baseMenu->FindItemByPosition( i );
		if ( item )
		{
			// Only sub menu items
			if ( item->IsSubMenu() )
			{
				// Check label
				if ( menuName.EqualsNC( item->GetItemLabel().wc_str() ) )
				{
					// Get sub menu
					wxMenu* subMenu = item->GetSubMenu();
					ASSERT( subMenu );
					return subMenu;
				}
			}
		}
	}

	// Create item
	if ( createIfNotFound )
	{
		// Create sub menu
		wxMenu* subMenu = new wxMenu;
		baseMenu->AppendSubMenu( subMenu, menuName.AsChar() );
		return subMenu;
	}

	// Not found and not created
	return NULL;
}

wxMenu* GetSubMenuPath( wxMenu* baseMenu, const String& menuPath, Bool createIfNotFound )
{
	// Split
	TDynArray< String > parts;
	menuPath.Slice( parts, TXT(".") );

	// Go down finding ( or creating ) each sub menu
	wxMenu* menu = baseMenu;
	for ( Uint32 i=0; i<parts.Size() && menu; i++ )
	{
		menu = GetSubMenu( menu, parts[i], createIfNotFound );
	}

	// Return final menu
	return menu;
}

namespace wxMenuUtils
{
    void CloneMenu(wxMenu *srcMenu, wxMenu *&dstMenu)
    {
        if (srcMenu == NULL)
        {
            dstMenu = NULL;
            return;
        }

        if (dstMenu == NULL)
            dstMenu = new wxMenu();
        else
            dstMenu->AppendSeparator();

        wxNode* node = srcMenu->GetMenuItems().First();
        while ( node )
        {
            wxMenuItem* item = wxStaticCast( node->GetData(), wxMenuItem );
            wxMenu* subMenuClone = NULL;

			if ( item )
			{
				CloneMenu( item->GetSubMenu(), subMenuClone );

				wxMenuItem* itemClone;

				if ( subMenuClone )
				{
					itemClone = dstMenu->Append( item->GetId(), item->GetItemLabel(), subMenuClone, item->GetHelp() );
				}
				else
				{
					itemClone = dstMenu->Append( item->GetId(), item->GetItemLabel(), item->GetHelp(), item->GetKind() );
				}
				if ( item->GetKind() != wxITEM_SEPARATOR )
				{
					itemClone->SetBitmaps( item->GetBitmap(true), item->GetBitmap(false) );
					itemClone->SetDisabledBitmap( item->GetDisabledBitmap() );
				}

				if ( item->GetKind() == wxITEM_CHECK )
				{
					itemClone->Check( item->IsChecked() );
				}
			}

			node = node->Next();
        }
    }

    void CloneMenuBar(wxMenuBar *srcMenu, wxMenuBar *&dstMenu,
                      Bool onlyWithDolars  /*= false*/)
    {
        if (!srcMenu)
        {
            dstMenu = NULL;
            return;
        }

        if (!dstMenu)
            dstMenu = new wxMenuBar();

        for ( Uint32 i = 0; i < srcMenu->GetMenuCount(); ++i )
        {
            wxMenu  *menu          = srcMenu->GetMenu( i );
            wxString labelOriginal = srcMenu->GetMenuLabelText( i );
            wxString labelClean    = labelOriginal;

			Bool clone = !onlyWithDolars;
			Int32 insertionPos = static_cast<Int32>( dstMenu->GetMenuCount() );

			int dollarPos = labelOriginal.Find( TXT('$'), true );
			if ( dollarPos >= 0 )
			{
				labelClean = labelOriginal.SubString( 0, static_cast<size_t>( dollarPos ) - 1 );

				if ( dollarPos < static_cast<int>( labelOriginal.Length() ) - 1 )
				{
					if ( !FromString( labelOriginal.SubString( dollarPos + 1, labelOriginal.Length() ).wc_str(), insertionPos ) )
					{
						insertionPos = static_cast<Int32>( dstMenu->GetMenuCount() );
					}
				}

				insertionPos = Min( static_cast<Int32>( dstMenu->GetMenuCount() ), insertionPos );

				clone = true;
			}

			if ( clone )
			{
				wxMenu* clonedMenu = NULL;

				int menuPos = dstMenu->FindMenu( labelOriginal );
				if ( menuPos < 0 && dollarPos >= 0 )
					menuPos = dstMenu->FindMenu( labelClean );
				if ( menuPos >= 0 )
					clonedMenu = dstMenu->GetMenu( menuPos );
				if ( !clonedMenu )
					clonedMenu = new wxMenu( menu->GetStyle() );

				CloneMenu( menu, clonedMenu );

				if ( menuPos < 0 )
				{
					dstMenu->Insert( insertionPos, clonedMenu, labelClean );
				}
			}
#if 0
            Bool clone = !onlyWithDolars;
            Int32  insertionPos = dstMenu->GetMenuCount();

            // Extract menu insertion position
            Int32 pos = labelOriginal.Find(TXT('$'), true);
            if (pos >= 0)
            {
                labelClean = labelOriginal.substr(0, pos);

                if (pos < static_cast<Int32>(labelOriginal.Length())-1)
                    if ( !FromString( labelOriginal.substr(pos+1).wc_str(), insertionPos ) )
                        insertionPos = dstMenu->GetMenuCount();

				insertionPos = Min( Int32(dstMenu->GetMenuCount()), insertionPos );

                clone = true;
            }

            if (clone)
            {
                wxMenu *menuClone = NULL;

                Int32 menuPos = dstMenu->FindMenu( labelOriginal );
                if (menuPos < 0 && pos >= 0)
                    menuPos = dstMenu->FindMenu( labelClean );
                if (menuPos >= 0)
                    menuClone = dstMenu->GetMenu( menuPos );

                CloneMenu( menu, menuClone );

                if (menuPos < 0)
                {
                    dstMenu->Insert( insertionPos, menuClone, labelClean );
                }
            }
#endif
        }
    }

	String ChangeItemLabelPreservingAccel( wxMenuItem* item, const String& newLabel )
	{
		wxString prevLabel = item->GetItemLabel();
        wxString accel = prevLabel.Contains(TXT("\t"))
            ? TXT('\t') + prevLabel.AfterLast(TXT('\t'))
            : wxEmptyString;
        item->SetItemLabel( wxString( newLabel.AsChar() ) + accel );
		return String( prevLabel.c_str() );
	}

	wxMenuItem* AppendMenuItemWithBitmap( wxMenu* parent, int id, const String& name, const String& bmpName )
	{
		wxMenuItem* item = new wxMenuItem( parent, id, name.AsChar() );
		item->SetBitmap( SEdResources::GetInstance().LoadBitmap( bmpName.AsChar() ) );
		parent->Append( item );
		return item;
	}

	void CleanUpSeparators( wxMenu* menu )
	{
		wxMenuItemList& items = menu->GetMenuItems(); // important: make a copy of the list

		for ( wxMenuItemList::Node* curNode = items.GetLast(); curNode != NULL; )
		{
			wxMenuItemList::Node* prevNode = curNode->GetPrevious();
			if ( curNode->GetData()->IsSubMenu() )
			{ 
				CleanUpSeparators( curNode->GetData()->GetSubMenu() ); // act recursively
			}
			else if ( curNode->GetData()->IsSeparator() )
			{
				if ( !curNode->GetNext() || 
					 !curNode->GetPrevious() ||
					 curNode->GetPrevious()->GetData()->IsSeparator()
					)
				{
					menu->Delete( curNode->GetData() );
				}
			}
			curNode = prevNode;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

static void LayoutRecursivelyForThisWindow( wxWindow* win )
{
	wxWindowList children = win->GetChildren();
	win->Layout();
	for ( Uint32 i=0; i<children.GetCount(); ++i )
	{
		LayoutRecursivelyForThisWindow( children[i] );
	}
}

void LayoutRecursively( wxWindow* win, bool fromTopWindow /* = true */ )
{
	// Find top window, if specified
	if ( fromTopWindow )
	{
		while ( win->GetParent() ) win = win->GetParent();
	}

	LayoutRecursivelyForThisWindow( win );
}

//////////////////////////////////////////////////////////////////////////////////////////

static void FitRecursivelyForThisWindow( wxWindow* win )
{
	wxWindowList children = win->GetChildren();
	for ( Uint32 i=0; i<children.GetCount(); ++i )
	{
		LayoutRecursivelyForThisWindow( children[i] );
	}
	win->Fit();
	win->Layout();
}

void FitRecursively( wxWindow* win, bool fromTopWindow /* = true */ )
{
	// Find top window, if specified
	if ( fromTopWindow )
	{
		while ( win->GetParent() ) win = win->GetParent();
	}

	FitRecursivelyForThisWindow( win );
}

//////////////////////////////////////////////////////////////////////////////////////////

class CEdBoneTree : public wxDialog 
{
protected:
	wxTreeCtrl*		m_boneList;
	wxButton*		m_btnOK;
	wxButton*		m_btnCancel;
	CName			m_selectedBone;

public:
	RED_INLINE CName GetSelectedBone() { return m_selectedBone; }

public:
	CEdBoneTree( wxWindow* parent, ISkeletonDataProvider* skeleton, const CName& boneName );
	~CEdBoneTree();

protected:
	void OnBoneActivated( wxTreeEvent& event );
	void OnOK( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );

private:
	CName GetSelectedTreeBone();
};

class wxBoneTreeNodeDataWrapper : public wxTreeItemData
{
public:
	CName	m_boneName;

public:
	wxBoneTreeNodeDataWrapper( const CName& boneName )
		: m_boneName( boneName )
	{};
};

CEdBoneTree::CEdBoneTree( wxWindow* parent, ISkeletonDataProvider* skeleton, const CName& currentBoneName )
	: wxDialog( parent, wxID_ANY, wxT("Class tree"), wxDefaultPosition, wxSize( 540,575 ), wxDEFAULT_DIALOG_STYLE )
{
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	this->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );

	wxBoxSizer* bSizer557;
	bSizer557 = new wxBoxSizer( wxVERTICAL );

	m_boneList = new wxTreeCtrl( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_DEFAULT_STYLE|wxTR_HIDE_ROOT );
	bSizer557->Add( m_boneList, 1, wxALL|wxEXPAND, 5 );

	wxImageList* images = new wxImageList( 16, 16, true, 2 );
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_SKELETON") ) ); // 0
	images->Add( SEdResources::GetInstance().LoadBitmap( TEXT("IMG_CONNECT") ) ); // 1	
	m_boneList->AssignImageList( images );

	m_boneList->Freeze();

	wxTreeItemId root = m_boneList->AddRoot( wxT("Root") );
	
	// Get bones from skeleton
	TDynArray< ISkeletonDataProvider::BoneInfo > allBones;
	skeleton->GetBones( allBones );

	// Create crap
	wxTreeItemId selectedNode;
	TDynArray< wxTreeItemId > boneItems;
	boneItems.Resize( allBones.Size() );	
	for ( Uint32 i=0; i<allBones.Size(); ++i )
	{
		const ISkeletonDataProvider::BoneInfo& bone = allBones[i];

		// Get parent node
		Int32 icon = (bone.m_parent == -1) ? 0 : 1;
		wxTreeItemId parentNode = (bone.m_parent == -1) ? root : boneItems[ bone.m_parent ];
		wxTreeItemId boneNode = m_boneList->AppendItem( parentNode, bone.m_name.AsString().AsChar(), icon, icon, new wxBoneTreeNodeDataWrapper( bone.m_name ) );

		// Remember
		boneItems[i] = boneNode;

		// Selection
		if ( bone.m_name == currentBoneName )
		{
			selectedNode = boneNode;
		}
	}

	m_boneList->ExpandAll();
	m_boneList->Thaw();
	m_boneList->Refresh();

	if ( selectedNode.IsOk() )
	{
		m_boneList->SelectItem( selectedNode );
		m_boneList->EnsureVisible( selectedNode);
	}

	wxBoxSizer* bSizer558;
	bSizer558 = new wxBoxSizer( wxHORIZONTAL );

	m_btnOK = new wxButton( this, wxID_ANY, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnOK->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdBoneTree::OnOK ), NULL, this );

	m_btnOK->SetDefault(); 
	bSizer558->Add( m_btnOK, 0, wxALL, 5 );

	m_btnCancel = new wxButton( this, wxID_ANY, wxT("Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	m_btnCancel->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdBoneTree::OnCancel ), NULL, this );
	bSizer558->Add( m_btnCancel, 0, wxALL, 5 );

	bSizer557->Add( bSizer558, 0, wxALIGN_CENTER_HORIZONTAL, 5 );

	this->SetSizer( bSizer557 );
	this->Layout();

	this->Centre( wxBOTH );

	m_boneList->Connect( wxEVT_COMMAND_TREE_ITEM_ACTIVATED, wxTreeEventHandler( CEdBoneTree::OnBoneActivated ), NULL, this );
}

CEdBoneTree::~CEdBoneTree()
{
}


void CEdBoneTree::OnBoneActivated( wxTreeEvent& event )
{
	CName boneName = GetSelectedTreeBone();
	if ( boneName )
	{
		m_selectedBone = boneName;
		EndDialog(0);
	}
}

void CEdBoneTree::OnOK( wxCommandEvent& event )
{
	CName boneName = GetSelectedTreeBone();
	if ( boneName )
	{
		m_selectedBone = boneName;
		EndDialog(0);
	}
}

void CEdBoneTree::OnCancel( wxCommandEvent& event )
{
	m_selectedBone = CName::NONE;
	EndDialog( -1 );
}

CName CEdBoneTree::GetSelectedTreeBone()
{
	wxTreeItemId selected = m_boneList->GetSelection();
	if ( selected.IsOk() )
	{
		wxBoneTreeNodeDataWrapper* data = static_cast< wxBoneTreeNodeDataWrapper * >( m_boneList->GetItemData( selected ) );
		if ( data )
		{
			return data->m_boneName;
		}
	}

	return CName::NONE;
}

Bool PickBone( wxWindow* parent, ISkeletonDataProvider* skeleton, CName& boneName )
{
	CEdBoneTree theBoneTree( parent, skeleton, boneName );
	if ( 0 == theBoneTree.ShowModal() )
	{
		boneName = theBoneTree.GetSelectedBone();
		return true;
	}
	else
	{
		return false;
	}

}

//////////////////////////////////////////////////////////////////////////////////////////

DWORD FindAssociatedProcessWithName( String name )
{
	// Start
	DWORD processIDs[ 1024 ];
	DWORD size = 0;
	EnumProcesses( processIDs, sizeof( processIDs ), &size );

	size /= sizeof(DWORD);

	DWORD foundId = -1;
	DWORD thisProcessId = GetCurrentProcessId();

	// Iterate through all currently open processes and see if there is a valid instance of script studio among them
	for( Uint32 i = 0; i < size; ++i )
	{
		if( processIDs[ i ] != thisProcessId )
		{
			TCHAR processName[ MAX_PATH ] = TXT( "<unknown>" );

			// Get a handle to the process.
			HANDLE processHandle = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIDs[ i ] );

			// Get the process name.
			if ( processHandle != NULL )
			{
				HMODULE moduleListing;
				DWORD bytesRequiredForAllModules;

				// Get the file path of the current process being checked
				if ( EnumProcessModules( processHandle, &moduleListing, sizeof(moduleListing), &bytesRequiredForAllModules) )
				{
					if( GetModuleFileNameEx( processHandle, moduleListing, processName, sizeof(processName)/sizeof(TCHAR) ) )
					{
						CFilePath processFilepath( processName );
						processFilepath.Normalize();

						if( processFilepath.GetFileName().ContainsSubstring( name ) )
						{
							CFilePath editorFilepath( wxStandardPaths::Get().GetExecutablePath().wc_str() );
							editorFilepath.Normalize();

							// Run from the same directory?
							if( processFilepath.GetPathString() == editorFilepath.GetPathString() )
							{
								foundId = processIDs[ i ];
								break;
							}
						}
					}
				}
			}

			// Release the handle to the process.
			CloseHandle( processHandle );
		}
	}

	return foundId;
}

//////////////////////////////////////////////////////////////////////////

Bool OpenExternalFile( String path, Bool silent /* = false */ )
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	Char sysdir[MAX_PATH];
	::GetSystemDirectoryW( sysdir, MAX_PATH );
	String command = String::Printf( TXT("%s\\cmd.exe /c %s"), sysdir, path.AsChar() );
	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW|STARTF_USEPOSITION;
	si.wShowWindow = SW_HIDE;
	if ( !CreateProcessW( NULL, (LPWSTR)command.AsChar(), NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) )
	{
		if ( !silent )
		{
			MessageBoxW(NULL, TXT("Failed to open the file"), TXT("Error"), MB_ICONERROR|MB_OK);
		}
		return false;
	}
	CloseHandle( pi.hThread );
	CloseHandle( pi.hProcess );
	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool IsObjectInspectorAvailable()
{
#ifdef RELEASE
	Bool isObjectInspectorAvailable = false;
	if ( !SUserConfigurationManager::GetInstance().ReadParam( TXT("User"), TXT("Editor"), TXT("EnableObjectInspector"), isObjectInspectorAvailable ) )
	{
		return false;
	}

	return isObjectInspectorAvailable;
#else
	return true;
#endif
}

wxFrame* InspectObject( ISerializable * object, const String& tag )
{
	// If you want to override that use GObjectInspector::CreateInspector directly
	if ( Cast< CObject >( object ) )
	{
		return IsObjectInspectorAvailable() ? CEdObjectInspector::CreateInspector( static_cast< CObject* >( object ), tag ) : nullptr;
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

void* GetPropertyDataPtr( CObject* obj, const CName& propertyName )
{
	CProperty* prop = obj ? obj->GetClass()->FindProperty( propertyName ) : nullptr;
	if ( prop )
	{
		return prop->GetOffsetPtr( obj );
	}
	return nullptr;
}

void* GetPropertyDataPtr( CObject* obj, const String& propertyName )
{
	return GetPropertyDataPtr( obj, CName( propertyName ) );
}

Bool GetPropertyValueIndirect( CObject* obj, const CName& propertyName, void* buffer )
{
	CProperty* prop = obj ? obj->GetClass()->FindProperty( propertyName ) : nullptr;
	if ( prop )
	{
		prop->Get( obj, buffer );
		return true;
	}
	return false;
}

Bool GetPropertyValueIndirect( CObject* obj, const String& propertyName, void* buffer )
{
	return GetPropertyValueIndirect( obj, CName( propertyName ), buffer );
}

Bool SetPropertyValueIndirect( CObject* obj, const CName& propertyName, const void* buffer, Bool emitEditorEvents )
{
	CProperty* prop = obj ? obj->GetClass()->FindProperty( propertyName ) : nullptr;
	if ( prop )
	{
		CEdPropertiesPage::SPropertyEventData eventData( nullptr, STypedObject( obj ), propertyName );
		if ( emitEditorEvents )
		{
			SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyPreChange ), CreateEventData( eventData ) );
		}
		prop->Set( obj, buffer );
		if ( emitEditorEvents )
		{
			SEvents::GetInstance().DispatchEvent( RED_NAME( EditorPropertyPostChange ), CreateEventData( eventData ) );
		}
		return true;
	}
	return false;
}

Bool SetPropertyValueIndirect( CObject* obj, const String& propertyName, const void* buffer, Bool emitEditorEvents )
{
	return SetPropertyValueIndirect( obj, CName( propertyName ), buffer, emitEditorEvents );
}

Color RandomPastelColor()
{
	Uint8 r = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 g = GEngine->GetRandomNumberGenerator().Get< Uint8 >();
	Uint8 b = GEngine->GetRandomNumberGenerator().Get< Uint8 >();

	r = ( 127 + r & 127 );
	g = ( 127 + g & 127 );
	b = ( 127 + b & 127 );

	return Color( r, g, b, 255 );
}

void LoadLayersWithEntitiesAroundPosition( CWorld* world, const Vector& position, Float radius, Bool showFeedback /* = true */ )
{
	TDynArray< CLayerGroup* > toLoad;
	ULONGLONG lastTime = ::GetTickCount64();

	// Grab all layers
	TDynArray< CLayerInfo* > layers;
	world->GetWorldLayers()->GetLayers( layers, false, true, true );

	// Scan them
	if ( showFeedback )
	{
		GFeedback->BeginTask( TXT("Scanning Layers"), false );
	}
	for ( auto it=layers.Begin(); it != layers.End(); ++it )
	{
		CLayerInfo* layerInfo = *it;

		// Skip the layer if the group is already in the toLoad array
		if ( toLoad.Exist( layerInfo->GetLayerGroup() ) )
		{
			continue;
		}

		// Load layer
		Bool unloadLayer = !layerInfo->IsLoaded();
		if ( unloadLayer )
		{
			layerInfo->SyncLoad( LayerLoadingContext() );
		}

		// Make sure the layer was really loaded
		if ( layerInfo->GetLayer() == nullptr )
		{
			continue;
		}

		// Scan the entities
		const LayerEntitiesArray& entities = layerInfo->GetLayer()->GetEntities();
		for ( auto it2=entities.Begin(); it2 != entities.End(); ++it2 )
		{
			CEntity* entity = *it2;

			// Check if the entity is near the position
			if ( position.DistanceTo( entity->GetWorldPositionRef() ) < radius )
			{
				toLoad.PushBackUnique( layerInfo->GetLayerGroup() );

				// change the flag so that the layer remains loaded
				unloadLayer = false; 
				break;
			}
		}

		// Unload the layer if necessary
		if ( unloadLayer )
		{
			layerInfo->SyncUnload();
		}

		// Collect some garbage
		if ( ::GetTickCount64() - lastTime > 1000 )
		{
			SEvents::GetInstance().ProcessPendingEvens();
			SGarbageCollector::GetInstance().CollectNow();
			lastTime = ::GetTickCount64();
		}

		// Update progress
		if ( showFeedback )
		{
			GFeedback->UpdateTaskProgress( (Int32)( it - layers.Begin() ), (Int32)( layers.End() - layers.Begin() ) );
		}
	}
	if ( showFeedback )
	{
		GFeedback->EndTask();
	}

	// Load layer groups
	if ( showFeedback )
	{
		GFeedback->BeginTask( TXT("Loading layer groups"), false );
	}
	for ( auto it=toLoad.Begin(); it != toLoad.End(); ++it )
	{
		CLayerGroup* group = *it;

		group->SyncLoad( LayerLoadingContext() );

		// Update progress
		if ( showFeedback )
		{
			GFeedback->UpdateTaskProgress( (Int32)( it - toLoad.Begin() ), (Int32)( toLoad.End() - toLoad.Begin() ) );
		}
	}
	if ( showFeedback )
	{
		GFeedback->EndTask();
	}
}

void ClearEntityList( const String& name )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name, false );
	if ( list != nullptr )
	{
		list->Clear();
	}
}

void AddEntityToEntityList( const String& name, CEntity* entity )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name );
	
	// Add the entity
	list->Add( entity );

	// Show the list if it isn't visible
	if ( !list->IsVisible() )
	{
		list->Show();
	}
}

void AddEntitiesToEntityList( const String& name, const TDynArray< CEntity* >& entities )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name );
	
	// Add the entity
	list->Add( entities );

	// Show the list if it isn't visible
	if ( !list->IsVisible() )
	{
		list->Show();
	}
}

void RemoveEntityFromEntityList( const String& name, CEntity* entity )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name );
	
	// Add the entity
	list->Remove( entity );
}

void ShowEntityList( const String& name )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name );
	
	// Show the list if it isn't visible
	if ( !list->IsVisible() )
	{
		list->Show();
	}
}

class CEntityList* LoadEntityListFromFile( const String& name, const String& absolutePath )
{
	// Find the scene entity list manager
	CEntityListManager* manager = wxTheFrame->GetSceneExplorer()->GetEntityListManager();
	if ( manager == nullptr )
	{
		return nullptr;
	}

	// Find the entity list
	CEntityList* list = manager->FindByName( name );
	
	// Load the file into it
	if ( !list->LoadFromFile( absolutePath ) )
	{
		wxMessageBox( wxString::Format( wxT("There was a problem loading the entity list '%s' from '%s', some entities might be missing (this is normal if the world was modified since the list was created)."), name.AsChar(), absolutePath.AsChar() ), wxT("Warning") );
	}

	return list;
}

class CEntityList* LoadEntityListFromFile( const String& absolutePath )
{
	CFilePath path( absolutePath );
	return LoadEntityListFromFile( path.GetFileName(), absolutePath );
}

int StringNaturalCompare( const Char* a, const Char* b )
{
	static struct {
		static int CompareRight( const Char* a, const Char* b )
		{
			int bias = 0;

			// Scan the digits to find the greatest run or number if both
			// digit substrings have the same length
			while ( true )
			{
				// Reached a point where both sides are not numbers, just return the bias
				if ( !IsNumber( *a ) && !IsNumber( *b ) )
				{
					return bias;
				}
				else if ( !IsNumber( *a ) ) // The left side isn't a number anymore, right side is greater
				{
					return -1;
				}
				else if ( !IsNumber( *b ) ) // the right side isn't a number anymore, left side is greater
				{
					return 1;
				}
				else if ( *a < *b ) // Right side is greater, update bias
				{
					if ( bias != 0 )
					{
						bias = -1;
					}
				}
				else if ( *a > *b ) // Left side is greater, update bias
				{
					if ( bias != 0 )
					{
						bias = 1;
					}
				}
				else if ( *a == '\0' && *b == '\0' ) // End of string, report bias
				{
					return bias;
				}

				++a;
				++b;
			}

			// Should never hit this
			return 0;
		}

		static int CompareLeft( const Char* a, const Char* b )
		{
			while ( true )
			{
				// Reached a point where both sides are not numbers and none was different than the other
				if ( !IsNumber( *a ) && !IsNumber( *b ) )
				{
					return 0;
				}
				else if ( !IsNumber( *a ) ) // The left side isn't a number anymore, right side is greater
				{
					return -1;
				}
				else if ( !IsNumber( *b ) ) // the right side isn't a number anymore, left side is greater
				{
					return 1;
				}
				else if ( *a < *b ) // Right side is greater
				{
					return -1;
				}
				else if ( *a > *b ) // Left side is greater
				{
					return 1;
				}

				++a;
				++b;
			}

			// Should never hit this
			return 0;
		}

		static int Compare( const Char* a, const Char* b )
		{
			while ( true )
			{
				// Skip whitespaces
				while ( IsWhiteSpace( *a ) ) ++a;
				while ( IsWhiteSpace( *b ) ) ++b;

				// When both sides reach a number, do a numeric comparison
				if ( IsNumber( *a ) && IsNumber( *b ) )
				{
					int result;

					// Left aligned case, just go for it until they become different (0010 vs 0002)
					if ( *a == '0' || *b == '0' )
					{
						result = CompareLeft( a, b );
					}
					else // Right aligned case (10 vs 2)
					{
						result = CompareRight( a, b );
					}

					// We have a difference
					if ( result != 0 )
					{
						return result;
					}
				}

				// Reached the end of both strings, the strings are equal
				if ( *a == '\0' && *b == '\0' )
				{
					return 0;
				}

				// Reached a non-digit, try to do lexic comparison
				if ( *a < *b )
				{
					return -1;
				}
				else if ( *a > *b )
				{
					return 1;
				}

				++a;
				++b;
			}
		}
	} local;

	return local.Compare( a, b );
}

String GetPathWithLastDirOnly( const String& path )
{
	TDynArray< String > parts = path.Split( TXT("\\") );
	if ( parts.Size() > 2 )
	{
		return parts[ parts.Size()-2 ] + TXT("\\") + parts[ parts.Size()-1 ];
	}
	else
	{
		return path;
	}
}

void CStringSanitizer::SanitizeString( String& stringVal )
{
	stringVal.Erase( RemoveIf( stringVal.Begin(), stringVal.End(), []( const Char& c )
		{
			return c > 0 && c < 32 && c != 10 && c != 13;
		}),
		stringVal.End() );
}

//////////////////////////////////////////////////////////////////////////////////////////

// Quickhull 2D implementation as described in Wikipedia (except using a loop instad of recursion)
// Points are random points on plane (Z is ignored) and hullPoints is indices to the points in the
// first array that form the convex hull
Bool ComputeQuickHull2D( const TDynArray< Vector >& points, TDynArray< Uint32 >& hullPoints )
{
	// Clear the points
	hullPoints.ClearFast();

	// Make sure we have enough points
	if ( points.Size() < 3 )
	{
		return false;
	}

	// Find min and max X points
	Uint32 minXIdx = 0, maxXIdx = 0;
	for ( Uint32 i=0; i < points.Size(); ++i )
	{
		if ( points[i].X < points[minXIdx].X ) minXIdx = i;
		if ( points[i].X > points[maxXIdx].X ) maxXIdx = i;
	}
	for ( Uint32 i=0; i < points.Size(); ++i )
	{
		if ( points[i].X < points[minXIdx].X ) minXIdx = i;
		if ( points[i].X > points[maxXIdx].X ) maxXIdx = i;
	}

	// Find point farthest from the edge formed by the min-max poins
	Uint32 farthestIdx = 0;
	Float farthestDistance = 0;
	Plane minMaxPlane(
		Vector( points[minXIdx].X, points[minXIdx].Y, 0 ),
		Vector( points[maxXIdx].X, points[maxXIdx].Y, 0 ),
		Vector( points[minXIdx].X, points[minXIdx].Y, 1 )
	);
	for ( Uint32 i=0; i < points.Size(); ++i )
	{
		Float distance = Vector::Dot2( minMaxPlane.NormalDistance, points[i] ) + minMaxPlane.NormalDistance.A[3];
		if ( distance > farthestDistance)
		{
			farthestIdx = i;
			farthestDistance = distance;
		}
	}

	// Reserve worst case memory
	hullPoints.Reserve( points.Size() );

	// Construct initial hull using the triangle from the three points
	hullPoints.PushBack( minXIdx );
	hullPoints.PushBack( farthestIdx );
	hullPoints.PushBack( maxXIdx );
	hullPoints.PushBack( minXIdx );

	// Construct mask for the points so that they wont be added twice
	TDynArray< Bool > used( points.Size() );
	for ( Int32 i=0; i < points.SizeInt(); ++i ) used[i] = false;

	// Mark initial hull points as used
	used[ minXIdx ] = true;
	used[ farthestIdx ] = true;
	used[ maxXIdx ] = true;

	// Loop through the points that make up the current hull, checking for points behind each edge
	for ( Int32 i = hullPoints.SizeInt() - 1; i > 0; )
	{
		Plane edgePlane(
			Vector( points[hullPoints[i - 1]].X, points[hullPoints[i - 1]].Y, 0 ),
			Vector( points[hullPoints[i]].X, points[hullPoints[i]].Y, 0 ),
			Vector( points[hullPoints[i - 1]].X, points[hullPoints[i - 1]].Y, 1 )
		);

		// Find the farthest point from the edge
		Bool found = false;
		farthestDistance = 0;
		farthestIdx = hullPoints[i];
		for ( Uint32 j=0; j < points.Size(); ++j )
		{
			if ( !used[j] )
			{
				Float distance = Vector::Dot2( edgePlane.NormalDistance, points[j] ) + edgePlane.NormalDistance.A[3];
				if ( distance > 0.0001f &&				// This is done instead of >0 to avoid tiny imprecisions
					 distance > farthestDistance )
				{
					farthestIdx = j;
					farthestDistance = distance;
					found = true;
				}
			}
		}

		// If a point was found, replace this edge with the edges formed from this
		// edge's first point to the far point and from the far point to the second
		// point of this edge
		if ( found )
		{
			used[farthestIdx] = true;				// Mark point as used so it wont be considered again
			hullPoints.Insert( i, hullPoints[i] );	// Second point of new edge is this edge's second point
			hullPoints[i] = farthestIdx;			// Replace ending point of this edge with the far point
			++i;									// Check the new edge in the next iteration
		}
		else // no point found, this edge is fine, proceed to the previous edge
		{
			--i;
		}
	}

	// Remove redundant last point
	hullPoints.PopBackFast();

	// Done
	return true;
}
