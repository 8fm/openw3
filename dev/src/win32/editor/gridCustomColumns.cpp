/**
* Copyright c 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "gridCustomColumns.h"
#include "gridCellEditors.h"
#include "chooseItemDialog.h"
#include "editorExternalResources.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/game/storySceneVoiceTagsManager.h"

namespace
{

	// the editor for string data allowing to choose from the list of strings
	class CGridCellDescribedChoiceEditor : public wxGridCellEditor
	{
	protected:
		wxArrayString	m_values;
		wxString        m_value;
		wxArrayString   m_choices;
		bool            m_allowOthers;

	public:
		CGridCellDescribedChoiceEditor(
			const wxArrayString& values,
			const wxArrayString& choices,
			bool allowOthers = false)
			: wxGridCellEditor()
			, m_choices( choices )
			, m_allowOthers( m_allowOthers )
			, m_values( values )
		{}

		virtual wxGridCellEditor *Clone() const { return new CGridCellDescribedChoiceEditor( m_values, m_choices, m_allowOthers ); }

		CEdChoice* Combo() const { return (CEdChoice*)m_control; }

		// this overrides the wxGridCellChoiceEditor's Create method to
		// implement a *much* faster method for filling in the values
		// than the default implementation
		virtual void Create(wxWindow* parent,
                                    wxWindowID id,
                                    wxEvtHandler* evtHandler)
		{
			m_control = new CEdChoice( parent, wxDefaultPosition, wxDefaultSize, m_allowOthers );
			m_control->SetId( id );

			// fill the combobox with the choices using a single Append call inside
			// a Freeze/Thaw pair
			m_control->Freeze();
			((CEdChoice*)m_control)->Append( m_choices );
			m_control->Thaw();

			// skip wxGridCellChoiceEditor's Create method and call wxGridCellEditor's one
			wxGridCellEditor::Create(parent, id, evtHandler);
		}

		virtual void SetSize( const wxRect& rect )
		{
			wxRect rectTallEnough = rect;
			wxSize bestSize = m_control->GetBestSize();
			wxCoord diffY = bestSize.GetHeight() - rectTallEnough.GetHeight();

			if ( diffY > 0 )
			{
				rectTallEnough.height += diffY;
		        rectTallEnough.y -= diffY/2;
			}

			wxGridCellEditor::SetSize( rect );
		}

		virtual void BeginEdit(int row, int col, wxGrid* grid)
		{
			wxGridCellEditorEvtHandler* handler = NULL;

			if ( m_control )
			{
				handler = wxDynamicCast( m_control->GetEventHandler(), wxGridCellEditorEvtHandler );
			}

			if ( handler )
			{
				handler->SetInSetFocus( true );
			}

			m_value = grid->GetTable()->GetValue( row, col );
			Reset();

			if ( !m_allowOthers )
			{
				// find the right position, or default to the first if not found
				for ( Uint32 i = 0; i < m_values.size(); ++i )
				{
					if ( m_values[ i ] == m_value )
					{
						Combo()->SetSelection( i );
					}
				}
			}

			// open the drop down menu if we can only make specific choices
			if ( !m_allowOthers )
				Combo()->ShowPopup();
		}

		virtual bool EndEdit(int row, int col, const wxGrid* grid, const wxString& WXUNUSED(oldval), wxString *newval)
		{
			wxString value = GetValue();
			
			if ( value == m_value )
				return false;

			m_value = value;

			grid->GetTable()->SetValue(row, col, value);

			if ( newval )
			{
				*newval = value;
			}

			return true;
		}

		virtual void ApplyEdit(int row, int col, wxGrid* grid)
		{
			grid->GetTable()->SetValue( row, col, m_value );
		}

		virtual void Reset()
		{
			int pos = Combo()->FindString( m_value );
			if (pos == wxNOT_FOUND)
			{
				Combo()->SetValue( m_value );
			}
			else
			{
				Combo()->SetSelection( pos );
			}
			m_control->SetFocus();
		}

		// added GetValue so we can get the value which is in the control
		virtual wxString GetValue() const
		{
			int selection = Combo()->GetSelection();
			if ( selection >= 0 && selection < (int) m_values.size() )
			{
				return m_values[ selection ];
			}

			return Combo()->GetValue();
		}
	};

	class CGridCellDescribedChoiceRenderer : public CGridCellDefaultRenderer
	{
	protected:
		const CGridChoiceColumnDesc *	m_owner;
		int								m_selectionIdx;

	public:
		CGridCellDescribedChoiceRenderer( const CGridChoiceColumnDesc * owner )
			: m_owner( owner ) , m_selectionIdx( -1 )
		{}

		virtual wxGridCellRenderer *Clone() const { return new CGridCellDescribedChoiceRenderer( m_owner ); }

		virtual wxString GetText( wxGrid& grid, int row, int col )
		{
			// Cache text to display
			wxString value = grid.GetCellValue(row, col);
			
			const wxArrayString & values  = m_owner->GetValues();
			const wxArrayString & choices = m_owner->GetChoices();
			
			if ( m_selectionIdx >= 0 && value == values[ m_selectionIdx ] )
			{
				return choices[ m_selectionIdx ];
			}

			size_t count = Min( choices.size(), values.size() );
			for ( size_t i = 0; i < count; ++i )
			{
				if ( values[ i ] == value )
				{
					m_selectionIdx = i;
					return choices[ m_selectionIdx ];
				}
			}

			m_selectionIdx = -1;
			return value;
		}
	};

	class CGridCellReadOnlyEditor : public wxGridCellTextEditor
	{
	private:
		wxString m_oldCellValue;
	public:
		virtual wxGridCellEditor *Clone() const { return new CGridCellReadOnlyEditor(); }

		virtual bool IsAcceptedKey(wxKeyEvent& event) { return false; }

		virtual void BeginEdit( int row, int col, wxGrid* grid )
		{
			wxGridCellTextEditor::BeginEdit( row, col, grid );
			m_oldCellValue = grid->GetTable()->GetValue( row, col );
		}

		virtual bool EndEdit( int row, int col, wxGrid* grid )
		{
			grid->GetTable()->SetValue( row, col, m_oldCellValue );
			return true;
		}
	};

	class CGridCellVoicetagChoiceEditor : public CGridCellDescribedChoiceEditor
	{
	protected:
		const CRTTISoftHandleType *m_rttiSoftHandle;
		void                      *m_rttiSoftHandleData;

	public:
		CGridCellVoicetagChoiceEditor( bool allowOthers = false )
			: CGridCellDescribedChoiceEditor( wxArrayString(), wxArrayString(), allowOthers )
			, m_rttiSoftHandle( NULL )
			, m_rttiSoftHandleData( NULL )
		{}

		void SetCellInfo( const CRTTISoftHandleType *softHandle, void *data )
		{
			m_rttiSoftHandle = softHandle;
			m_rttiSoftHandleData = data;
		}

		void LoadData()
		{
			m_choices.Clear();
			m_values.Clear();
			TDynArray<String> choices;
			static_cast<wxItemContainer*>( Combo() )->Clear();

			if ( m_rttiSoftHandle == NULL ) return;

			CEntityTemplate *entTemplate = (CEntityTemplate *)m_rttiSoftHandle->GetPointed( m_rttiSoftHandleData );
			if ( entTemplate )
			{
				const TDynArray< VoicetagAppearancePair >& voicetagAppearances 
					= entTemplate->GetVoicetagAppearances();

				for ( Uint32 i = 0; i < voicetagAppearances.Size(); ++i )
				{
					choices.PushBackUnique( voicetagAppearances[ i ].m_voicetag.AsString() );
				}
			}
			else
			{
				// TODO: Voicetag list should be redesigned. Maybe in game resource?				
				const C2dArray* voiceTagArray = SStorySceneVoiceTagsManager::GetInstance().GetVoiceTags();
				if ( voiceTagArray )
				{
					for ( Uint32 i = 0; i < voiceTagArray->GetNumberOfRows(); ++i )
					{
						choices.PushBackUnique( voiceTagArray->GetValue( 0, i ) );
					}
				}
			}

			for( Uint32 i=0; i<choices.Size(); ++i )
			{
				m_choices.Add( choices[ i ].AsChar() );
			}
			m_values = m_choices;

			Combo()->Append( m_choices );
		}

		virtual wxGridCellEditor *Clone() const { return new CGridCellVoicetagChoiceEditor( m_allowOthers ); }

		virtual void BeginEdit(int row, int col, wxGrid* grid)
		{
			LoadData();

			CGridCellDescribedChoiceEditor::BeginEdit( row, col, grid );
		}
	};


	class CGridCellAppearaceChoiceEditor : public CGridCellDescribedChoiceEditor
	{
	protected:
		const CRTTISoftHandleType *m_rttiSoftHandle;
		void                      *m_rttiSoftHandleData;

	public:
		CGridCellAppearaceChoiceEditor( bool allowOthers = false )
			: CGridCellDescribedChoiceEditor( wxArrayString(), wxArrayString(), allowOthers )
			, m_rttiSoftHandle( NULL )
			, m_rttiSoftHandleData( NULL )
		{}
		
		void SetCellInfo( const CRTTISoftHandleType *softHandle, void *data )
		{
			m_rttiSoftHandle = softHandle;
			m_rttiSoftHandleData = data;
		}

		void LoadData()
		{
			m_choices.Clear();
			m_values.Clear();
			static_cast<wxItemContainer*>( Combo() )->Clear();

			if ( m_rttiSoftHandle == NULL ) return;

			CEntityTemplate *entTemplate = (CEntityTemplate *)m_rttiSoftHandle->GetPointed( m_rttiSoftHandleData );
			if ( entTemplate )
			{
				TDynArray< CEntityAppearance* > appearances;
				entTemplate->GetAllAppearances( appearances );
				for ( Uint32 i = 0; i < appearances.Size(); ++i )
				{
					m_choices.Add( appearances[ i ]->GetName().AsString().AsChar() );
				}
				m_values = m_choices;
			}

			Combo()->Append( m_choices );
		}

		virtual wxGridCellEditor *Clone() const { return new CGridCellAppearaceChoiceEditor( m_allowOthers ); }

		virtual void BeginEdit(int row, int col, wxGrid* grid)
		{
			LoadData();

			CGridCellDescribedChoiceEditor::BeginEdit( row, col, grid );
		}
	};
}

CGridChoiceColumnDesc::CGridChoiceColumnDesc( const wxArrayString & values, const wxArrayString * choices /*=NULL*/ )
	: m_values( values )
{
	if ( choices )
	{
		m_choices = * choices;
	}

	// Cannot have more choices than values
	if ( m_choices.size() > m_values.size() )
	{
		//m_choices.resize( m_values.size() ); // resize doesn't link :/
		m_choices.RemoveAt( m_values.size(), m_choices.size() - m_values.size() );
	}
	else // Cannot have less choices than values
	{
		for ( Uint32 i = m_choices.size(); i < m_values.size(); ++i )
		{
			m_choices.push_back( m_values[ i ] );
		}
	}
}

wxGridCellEditor* CGridChoiceColumnDesc::GetCellEditor() const
{
	return new ::CGridCellDescribedChoiceEditor( m_values, m_choices );
}

wxGridCellRenderer* CGridChoiceColumnDesc::GetCellRenderer() const
{
	return new ::CGridCellDescribedChoiceRenderer( this );
}

IGridColumnDesc* CGridChoiceColumnDesc::CreateFromStrings( const wxArrayString & values, const wxArrayString * choices /*= NULL*/ )
{
	return new CGridChoiceColumnDesc( values, choices );
}

IGridColumnDesc* CGridChoiceColumnDesc::CreateFrom2da( const C2dArray* valueTable, Int32 valueColumn, Int32 textColumn /*= -1*/ )
{
	wxArrayString choices, values;
	if ( valueTable != NULL )
	{
		if ( valueColumn >= 0 )
		{
			for ( Uint32 i = 0; i < valueTable->GetNumberOfRows(); ++i )
			{
				values.push_back( valueTable->GetValue( valueColumn, i ).AsChar() );
			}
		}

		if ( textColumn >= 0 )
		{
			for ( Uint32 i = 0; i < valueTable->GetNumberOfRows(); ++i )
			{
				choices.push_back( valueTable->GetValue( textColumn, i ).AsChar() );
			}
		}
	}
	return new CGridChoiceColumnDesc( values, &choices );
}

//////////////////////////////////////////////////////////////////////////

wxGridCellEditor* CGridReadOnlyColumnDesc::GetCellEditor() const
{
	return new CGridCellReadOnlyEditor;
}

wxGridCellRenderer* CGridReadOnlyColumnDesc::GetCellRenderer() const
{
	return new CGridCellDefaultRenderer;
}

//////////////////////////////////////////////////////////////////////////

wxGridCellEditor* CGridVoicetagColumnDesc::GetCellEditor() const
{
	CGridCellVoicetagChoiceEditor *ed = new ::CGridCellVoicetagChoiceEditor( true );
	ed->SetCellInfo( m_extraInfoType, m_data );
	return ed;
}

wxGridCellRenderer* CGridVoicetagColumnDesc::GetCellRenderer() const
{
	return NULL;
}

void CGridVoicetagColumnDesc::SetExtraInfo( const IRTTIType *type, void *data )
{
	m_extraInfoType = NULL;
	m_data = NULL;

	const CRTTISoftHandleType *extraInfoType = dynamic_cast< const CRTTISoftHandleType* >( type );
	if ( extraInfoType )
	{
		m_extraInfoType = extraInfoType;
		m_data = data;
	}
}

//////////////////////////////////////////////////////////////////////////

wxGridCellEditor* CGridAppearanceColumnDesc::GetCellEditor() const
{
	CGridCellAppearaceChoiceEditor *ed = new ::CGridCellAppearaceChoiceEditor( true );
	ed->SetCellInfo( m_extraInfoType, m_data );
	return ed;
}

wxGridCellRenderer* CGridAppearanceColumnDesc::GetCellRenderer() const
{
	return NULL;
}

void CGridAppearanceColumnDesc::SetExtraInfo( const IRTTIType *type, void *data )
{
	m_extraInfoType = NULL;
	m_data = NULL;

	const CRTTISoftHandleType *extraInfoType = dynamic_cast< const CRTTISoftHandleType* >( type );
	if ( extraInfoType )
	{
		m_extraInfoType = extraInfoType;
		m_data = data;
	}
}

//////////////////////////////////////////////////////////////////////////
// Inventory item column
//////////////////////////////////////////////////////////////////////////

class CGridCellInventoryItemEditor : public wxGridCellTextEditor, public wxEvtHandler
{
	friend class CGridCellChooseItemEditorEventHandler;

public:
	CName					m_item;
	CEdChooseItemDialog*	m_chooseItemDialog;
	CGridCellChooseItemEditorEventHandler m_handler;

public:
	CGridCellInventoryItemEditor()
		: m_handler( this )
	{}

	~CGridCellInventoryItemEditor()
	{
		Destroy();
	}

	virtual wxGridCellEditor *Clone() const { return new CGridCellInventoryItemEditor; }

	virtual bool IsAcceptedKey(wxKeyEvent& event) { return false; }

	virtual void Create( wxWindow* parent, wxWindowID id, wxEvtHandler* evtHandler ) override
	{
 		m_control = new wxStaticText( parent, wxID_ANY, TXT("") );
 		m_control->Hide();
 		wxGridCellTextEditor::Create(parent, id, evtHandler);
 		m_control->PushEventHandler( &m_handler );

		m_chooseItemDialog = new CEdChooseItemDialog( parent, m_item );
		m_chooseItemDialog->Connect( wxEVT_CHOOSE_ITEM_OK, wxCommandEventHandler( CGridCellInventoryItemEditor::OnEditorOk ), NULL, this );
		m_chooseItemDialog->Connect( wxEVT_CHOOSE_ITEM_CANCEL, wxCommandEventHandler( CGridCellInventoryItemEditor::OnEditorCancel ), NULL, this );
	}

	virtual void Destroy() override
	{
		if ( m_control )
		{
 			m_control->PopEventHandler( false );
 		}

		wxGridCellTextEditor::Destroy();
	}

	virtual void BeginEdit(int row, int col, wxGrid* grid) override
	{
		wxGridCellTextEditor::BeginEdit( row, col, grid );

		m_item = CName( GetValue().c_str() );
		m_chooseItemDialog->ShowModal();
	}

	virtual bool EndEdit(int row, int col, const wxGrid* grid, const wxString& oldval, wxString *newval) override
	{
		wxString val = GetValue();

		if ( val == oldval )
		{
			return false;
		}
		else
		{
			*newval = wxString( m_item.AsString().AsChar() );
			return true;
		}
	}

	virtual void ApplyEdit(int row, int col, wxGrid* grid) override
	{
		grid->GetTable()->SetValue( row, col, m_item.AsString().AsChar() );
	}

 	virtual wxString GetValue() const override
 	{
		return wxString( m_item.AsString().AsChar() );
 	}

	void OnEditorOk( wxCommandEvent &event )
	{
		ASSERT( m_chooseItemDialog );

		// Update tags
		m_item = m_chooseItemDialog->GetItem();

		// Force the selected value to be stored in the grid. It's crucial for the undo to work correctly when ctrl+z is pressed during editing.
		LeaveEditMode();
	}

	void OnEditorCancel( wxCommandEvent &event )
	{
		LeaveEditMode();
	}

	void LeaveEditMode()
	{
		ASSERT ( m_chooseItemDialog != NULL );
  		wxGrid* grid = dynamic_cast< wxGrid* >( m_chooseItemDialog->GetParent()->GetParent() );
  		ASSERT ( grid != NULL );
 		grid->DisableCellEditControl();
	}
};

//////////////////////////////////////////////////////////////////////////

wxGridCellEditor* CGridInventoryItemColumnDesc::GetCellEditor() const
{
	CGridCellInventoryItemEditor *ed = new ::CGridCellInventoryItemEditor;
	return ed;
}

wxGridCellRenderer*	CGridInventoryItemColumnDesc::GetCellRenderer() const
{
	return NULL;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ABSTRACT_CLASS(CGridCellChooseItemEditorEventHandler, wxEvtHandler)

BEGIN_EVENT_TABLE( CGridCellChooseItemEditorEventHandler, wxEvtHandler )
EVT_KILL_FOCUS( CGridCellChooseItemEditorEventHandler::OnKillFocus )
END_EVENT_TABLE()

CGridCellChooseItemEditorEventHandler::CGridCellChooseItemEditorEventHandler( CGridCellInventoryItemEditor * owner )
	: m_owner( owner )
{}

void CGridCellChooseItemEditorEventHandler::OnKillFocus( wxFocusEvent & event )
{
	wxWindow * parent = wxGetActiveWindow();

	while ( parent )
	{
		if ( parent == m_owner->m_chooseItemDialog )
			return;
		parent = parent->GetParent();
	}
	event.Skip();
}

