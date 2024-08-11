
#pragma once

namespace EntityEditorHelper
{
	template< typename T >
	RED_INLINE Bool IsTemplateParamWithNameUnique( CEntityTemplate* templ, String* out )
	{
		TDynArray< T* > params;
		templ->GetAllParameters( params );

		const Uint32 size = params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			T* paramA = params[ i ];

			for ( Uint32 j=0; j<size; ++j )
			{
				T* paramB = params[ j ];

				if ( i != j && paramA->GetName() == paramB->GetName() )
				{
					if ( out )
					{
						*out = paramA->GetName();
					}

					return false;
				}
			}
		}

		return true;
	}

	template< typename T >
	RED_INLINE Bool HasTemplateParamWithName( CEntityTemplate* templ, const String& name )
	{
		TDynArray< T* > params;
		templ->GetAllParameters( params );

		const Uint32 size = params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			T* param = params[ i ];

			if ( param->GetName() == name )
			{
				return true;
			}
		}

		return false;
	}

	template< typename T >
	RED_INLINE T* AddParamWithNameToTemplate( wxWindow* win, CEntityTemplate* templ )
	{
		T* param = templ->FindParameter< T >( false );
		if ( param )
		{
			wxMessageBox( wxT("This entity template has already parameter from selected class"), wxT("Warning"), wxOK | wxICON_WARNING );
			return NULL;
		}

		String nameStr;
		if ( !InputBox( win, TXT("New parameter"), TXT("Enter name of the new parameter"), nameStr, false ) )
		{
			return NULL;
		}

		if ( HasTemplateParamWithName< T >( templ, nameStr ) )
		{
			wxMessageBox( wxString::Format( wxT("Param with name '%s' already exists in template."), nameStr.AsChar() ), wxT("Add parameter"), wxOK | wxICON_WARNING );
			return NULL;
		}

		param = CreateObject< T >( templ );
		if ( param )
		{
			param->SetName( nameStr );

			templ->AddParameterUnique( param );

			return param;
		}

		return NULL;
	}

	template< typename T >
	RED_INLINE void UpdateAnimTabList( CEntityTemplate* templ, wxListBox* list )
	{
		list->Freeze();
		list->Clear();

		TDynArray< T* > params;
		templ->GetAllParameters( params );

		const Uint32 size = params.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			T* param = params[ i ];

			wxString str = param->GetName().AsChar();

			if ( param->WasIncluded() )
			{
				str += wxT("- (Included)");	
			}

			list->Append( str, new TClientDataWrapper< T* >( param ) );
		}

		if ( size == 0 )
		{
			list->AppendString( wxT("(Empty)") );
		}

		list->Thaw();
	}
};

//////////////////////////////////////////////////////////////////////////

#define RefreshAnimPanelAddRemoveStyle( T, list )\
{\
	wxListBox* l = XRCCTRL( *this, list, wxListBox );\
	EntityEditorHelper::UpdateAnimTabList< T >( m_template, l );\
}

#define SetPropAnimPanelAddRemoveStyle( object, propPanel )\
{\
	wxPanel* rp = XRCCTRL( *this, propPanel, wxPanel );\
	CEdPropertiesPage* page = (CEdPropertiesPage*)rp->FindWindow( wxT("CEdPropertiesPage") );\
	if ( object )\
	{\
		page->SetObject( object );\
	}\
}

#define SetNoPropAnimPanelAddRemoveStyle( propPanel )\
{\
	wxPanel* rp = XRCCTRL( *this, propPanel, wxPanel );\
	CEdPropertiesPage* page = (CEdPropertiesPage*)rp->FindWindow( wxT("CEdPropertiesPage") );\
	page->SetNoObject();\
}

#define ConnectAnimPanelAddRemoveStyle( propPanel, funcPropModified, btnAdd, funcAdd, btnRemove, funcRemove, list, funcListSelectionChanged )\
{\
	wxButton* btnAddHead = XRCCTRL( *this, btnAdd, wxButton );\
	btnAddHead->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::funcAdd ), NULL, this );\
	wxButton* btnRemoveHead = XRCCTRL( *this, btnRemove, wxButton );\
	btnRemoveHead->Connect( wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler( CEdEntityEditor::funcRemove ), NULL, this );\
	wxListBox* l = XRCCTRL( *this, list, wxListBox );\
	l->Connect( wxEVT_COMMAND_LISTBOX_SELECTED, wxCommandEventHandler( CEdEntityEditor::funcListSelectionChanged ), NULL, this );\
	wxPanel* rp = XRCCTRL( *this, propPanel, wxPanel );\
	wxBoxSizer* sizer1 = new wxBoxSizer( wxVERTICAL );\
	PropertiesPageSettings settings;\
	CEdPropertiesPage* panelProp = new CEdPropertiesPage( rp, settings, m_undoManager );\
	panelProp->SetName( wxT("CEdPropertiesPage") );\
	panelProp->Connect( wxEVT_COMMAND_PROPERTY_CHANGED, wxCommandEventHandler( CEdEntityEditor::funcPropModified ), NULL, this );\
	sizer1->Add( panelProp, 1, wxEXPAND | wxALL, 0 );\
	rp->SetSizer( sizer1 );\
	rp->Layout();\
}
