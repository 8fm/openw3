#include "build.h"
#include <wx/artprov.h>
#include <wx/listimpl.cpp>
#include <wx/spinctrl.h>
#include <wx/xml/xml.h>
#include <set>

#include "../../common/core/depot.h"
#include "../../common/core/feedback.h"


//////////////////////////////////////////////////////////////////////////

class CEdMassActionList : public CEdWidgetItemList
{
	CEdMassActionContext* m_context;

	class ItemPanel : public wxPanel
	{
		CEdMassActionContext*		m_context;
		wxChoice*					m_actions;
		IEdMassAction*				m_action;
		wxWindow*					m_actionWindow;

		void SetSelectedAction( int selection )
		{
			IEdMassAction* newAction = NULL;

			if ( selection > -1 && selection < m_context->GetActionTypeCount() )
			{
				IEdMassActionType* actionType = m_context->GetActionType( selection );

				if ( actionType )
				{
					newAction = actionType->Create();
					if ( newAction )
					{
						newAction->OnActionSelect( m_action );
					}
				}
			}

			if ( m_actionWindow )
			{
				m_actionWindow->Destroy();
				m_actionWindow = NULL;
			}
			if ( m_action )
			{
				m_action->OnActionDeselect( newAction );
				if ( newAction )
				{
					m_context->ReplaceAction( m_action, newAction );
				}
				else
				{
					m_context->RemoveAction( m_action );
				}
				m_action = NULL;
			}
			else if ( newAction ) {
				m_context->AddAction( newAction );
			}

			if ( newAction )
			{
				m_action = newAction;
				m_actionWindow = m_action->GetWindow( this );
				if ( m_actionWindow )
				{
					m_actionWindow->Reparent( this );
					GetSizer()->Add( m_actionWindow, 1, wxALIGN_CENTRE_VERTICAL, 0);
				}
			}

			Layout();
		}

		void OnActionsChange( wxCommandEvent& event )
		{
			SetSelectedAction( event.GetSelection() );
		}

	public:
		ItemPanel( CEdMassActionContext* context, wxWindow* parent )
			: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize )
			, m_context( context )
			, m_actions( NULL )
			, m_action( NULL )
			, m_actionWindow( NULL )
		{
			SetBackgroundColour( parent->GetBackgroundColour() );
			SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			m_actions = new wxChoice( this, wxID_ANY );
			m_actions->Freeze();
			for ( int i=0; i<m_context->GetActionTypeCount(); ++i )
			{
				IEdMassActionType* actionType = m_context->GetActionType( i );
				m_actions->Append( actionType->GetName(), actionType );
			}
			m_actions->Thaw();
			m_actions->SetSelection( 0 );
			m_actions->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &ItemPanel::OnActionsChange, this );
			GetSizer()->Add( m_actions, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 2 );
			SetSelectedAction( 0 );
		}

		void OnRemoved()
		{
			if ( m_action )
			{
				m_context->RemoveAction( m_action );
			}
		}

		void SetActionType( IEdMassActionType* type )
		{
			int index = m_actions->FindString( type->GetName() );
			m_actions->SetSelection( index );
			SetSelectedAction( index );
		}

		IEdMassAction* GetAction() const
		{
			return m_action;
		}
	};

protected:
	virtual void OnAddItem()
	{
		AddItem( new ItemPanel( m_context, this ) );
	}

	virtual void RemoveItem( CEdWidgetItemListItem* item )
	{
		((ItemPanel*)item->GetWidget())->OnRemoved();
		CEdWidgetItemList::RemoveItem( item );
	}

public:
	CEdMassActionList( CEdMassActionContext* context, wxWindow* parent, wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize )
		: CEdWidgetItemList( parent, pos, size )
	{
		m_context = context;
		SetAddButtonTitle( wxT("Add Action") );
		SetRemoveButtonTip( wxT("Remove Action") );
	}

	IEdMassAction* AddAction( IEdMassActionType* type )
	{
		ItemPanel* panel = new ItemPanel( m_context, this );
		AddItem( panel );
		panel->SetActionType( type );
		return panel->GetAction();
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionConditionList : public CEdWidgetItemList
{
	CEdMassActionContext* m_context;

	class ItemPanel : public wxPanel
	{
		CEdMassActionContext*		m_context;
		wxChoice*					m_conditions;
		IEdMassActionCondition*		m_condition;
		wxWindow*					m_conditionWindow;

		void SetSelectedCondition( int selection )
		{
			IEdMassActionCondition* newCondition = NULL;

			if ( selection > -1 && selection < m_context->GetConditionTypeCount() )
			{													 
				IEdMassActionConditionType* conditionType = m_context->GetConditionType( selection );

				if ( conditionType )
				{
					newCondition = conditionType->Create();
					if ( newCondition )
					{
						newCondition->OnConditionSelect( m_condition );
					}
				}
			}

			if ( m_conditionWindow )
			{
				m_conditionWindow->Destroy();
				m_conditionWindow = NULL;
			}
			if ( m_condition )
			{
				m_condition->OnConditionDeselect( newCondition );
				if ( newCondition )
				{
					m_context->ReplaceCondition( m_condition, newCondition );
				}
				else
				{
					m_context->RemoveCondition( m_condition );
				}
				m_condition = NULL;
			}
			else if ( newCondition ) {
				m_context->AddCondition( newCondition );
			}

			if ( newCondition )
			{
				m_condition = newCondition;
				m_conditionWindow = m_condition->GetWindow( this );
				if ( m_conditionWindow )
				{
					m_conditionWindow->Reparent( this );
					GetSizer()->Add( m_conditionWindow, 1, wxALIGN_CENTRE_VERTICAL, 0);
				}
			}

			Layout();
		}

		void OnConditionsChange( wxCommandEvent& event )
		{
			SetSelectedCondition( event.GetSelection() );
		}

	public:
		ItemPanel( CEdMassActionContext* context, wxWindow* parent )
			: wxPanel( parent, wxID_ANY, wxDefaultPosition, wxDefaultSize )
			, m_context( context )
			, m_conditions( NULL )
			, m_condition( NULL )
			, m_conditionWindow( NULL )
		{
			SetBackgroundColour( parent->GetBackgroundColour() );
			SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			m_conditions = new wxChoice( this, wxID_ANY );
			m_conditions->Freeze();
			for ( int i=0; i<m_context->GetConditionTypeCount(); ++i )
			{
				IEdMassActionConditionType* conditionType = m_context->GetConditionType( i );
				m_conditions->Append( conditionType->GetName(), conditionType );
			}
			m_conditions->Thaw();
			m_conditions->SetSelection( 0 );
			m_conditions->Bind( wxEVT_COMMAND_CHOICE_SELECTED, &ItemPanel::OnConditionsChange, this );
			GetSizer()->Add( m_conditions, 0, wxALIGN_CENTRE_VERTICAL | wxLEFT, 2 );
			SetSelectedCondition( 0 );
		}

		void OnRemoved()
		{
			if ( m_condition )
			{
				m_context->RemoveCondition( m_condition );
			}
		}

		void SetConditionType( IEdMassActionConditionType* type )
		{
			int index = m_conditions->FindString( type->GetName() );
			m_conditions->SetSelection( index );
			SetSelectedCondition( index );
		}

		IEdMassActionCondition* GetCondition() const
		{
			return m_condition;
		}
	};

protected:
	virtual void OnAddItem()
	{
		AddItem( new ItemPanel( m_context, this ) );
	}

	virtual void RemoveItem( CEdWidgetItemListItem* item )
	{
		((ItemPanel*)item->GetWidget())->OnRemoved();
		CEdWidgetItemList::RemoveItem( item );
	}

public:
	CEdMassActionConditionList( CEdMassActionContext* context, wxWindow* parent, wxPoint pos = wxDefaultPosition, wxSize size = wxDefaultSize )
		: CEdWidgetItemList( parent, pos, size )
	{
		m_context = context;
		SetAddButtonTitle( wxT("Add Condition") );
		SetRemoveButtonTip( wxT("Remove Condition") );
	}

	IEdMassActionCondition* AddCondition( IEdMassActionConditionType* type )
	{
		ItemPanel* panel = new ItemPanel( m_context, this );
		AddItem( panel );
		panel->SetConditionType( type );
		return panel->GetCondition();
	}
};

//////////////////////////////////////////////////////////////////////////

CEdMassActionContext::CEdMassActionContext( IEdMassActionIterator* iterator, bool searchMode )
	: m_dialog( NULL )
	, m_iterator( iterator )
	, m_processedObjects( NULL )
	, m_searchMode( searchMode )
{
	UpdateObjectInformation();
}

CEdMassActionContext::~CEdMassActionContext()
{
	for ( wxVector<IEdMassActionType*>::iterator it =  m_actionTypes.begin();
												 it != m_actionTypes.end();
												 ++it )
	{
		delete *it;
	}
	for ( wxVector<IEdMassAction*>::iterator it =  m_actions.begin();
											 it != m_actions.end();
											 ++it )
	{
		delete *it;
	}
	for ( wxVector<IEdMassActionConditionType*>::iterator it =  m_conditionTypes.begin();
														  it != m_conditionTypes.end();
														  ++it )
	{
		delete *it;
	}
	for ( wxVector<IEdMassActionCondition*>::iterator it =  m_conditions.begin();
													  it != m_conditions.end();
													  ++it )
	{
		delete *it;
	}
	for ( wxVector<IEdMassActionConditionComparator*>::iterator it =  m_comparators.begin();
																it != m_comparators.end();
																++it )
	{
		delete *it;
	}
}

void CEdMassActionContext::UpdateObjectInformation()
{
	THashSet<CName> props;
	IEdMassActionObject* obj;

	RewindIterator();

	while ( ( obj = GetNextObject() ) ) {
		for ( int i=0; i<obj->GetPropertyCount(); ++i )
		{
			props.Insert( obj->GetPropertyName( i ) );
		}
	}

	m_properties.Clear();
	for ( auto it=props.Begin(); it != props.End(); ++it )
	{
		m_properties.PushBack( *it );
	}
	SortAsCharArray( m_properties );
}

void CEdMassActionContext::SetIterator( IEdMassActionIterator* iterator )
{
	m_iterator = iterator;
	UpdateObjectInformation();
}

void CEdMassActionContext::SetSearchMode( bool searchMode )
{
	m_searchMode = searchMode;
}

void CEdMassActionContext::RewindIterator()
{
	if ( m_iterator )
	{
		m_iterator->Rewind();
	}
}

bool CEdMassActionContext::HasMoreObjects() const
{
	return m_iterator ? m_iterator->HasMore() : false;
}

IEdMassActionObject* CEdMassActionContext::GetNextObject()
{
	return m_iterator ? m_iterator->Next() : NULL;
}

void CEdMassActionContext::SetProcessedObjects( wxVector<IEdMassActionObject*>* processedObjects )
{
	m_processedObjects = processedObjects;
}

int	CEdMassActionContext::GetPropertyCount() const
{
	return m_properties.Size();
}

CName CEdMassActionContext::GetPropertyName( int n ) const
{
	return m_properties[ n ];
}

void CEdMassActionContext::AddActionType( IEdMassActionType* actionType )
{
	actionType->m_context = this;
	m_actionTypes.push_back( actionType );
}

void CEdMassActionContext::AddConditionType( IEdMassActionConditionType* conditionType )
{
	conditionType->m_context = this;
	m_conditionTypes.push_back( conditionType );
}

void CEdMassActionContext::AddConditionComparator( IEdMassActionConditionComparator* comparator )
{
	m_comparators.push_back( comparator );
}

int CEdMassActionContext::GetActionTypeCount() const
{
	return (int)m_actionTypes.size();
}

IEdMassActionType* CEdMassActionContext::GetActionType( int n )
{
	return n < (int)m_actionTypes.size() ? m_actionTypes.at( (int)n ) : NULL;
}

int CEdMassActionContext::GetActionCount() const
{
	return (int)m_actions.size();
}

IEdMassAction* CEdMassActionContext::GetAction( int n )
{
	return n < (int)m_actions.size() ? m_actions.at( (int)n ) : NULL;
}

void CEdMassActionContext::AddAction( IEdMassAction* action )
{
	m_actions.push_back( action );
}

void CEdMassActionContext::RemoveAction( IEdMassAction* action )
{
	for ( wxVector<IEdMassAction*>::iterator it =  m_actions.begin();
											 it != m_actions.end();
											 ++it )
	{
		if ( *it == action )
		{
			m_actions.erase( it );
			delete action;
			break;
		}
	}
}

void CEdMassActionContext::ReplaceAction( IEdMassAction* oldAction, IEdMassAction* newAction )
{
	for ( wxVector<IEdMassAction*>::iterator it =  m_actions.begin();
											 it != m_actions.end();
											 ++it )
	{
		if ( *it == oldAction )
		{
			m_actions.insert( it, newAction );
			break;
		}
	}
	RemoveAction( oldAction );
}


void CEdMassActionContext::AddCondition( IEdMassActionCondition* condition )
{
	m_conditions.push_back( condition );
}

void CEdMassActionContext::RemoveCondition( IEdMassActionCondition* condition )
{
	for ( wxVector<IEdMassActionCondition*>::iterator it =  m_conditions.begin();
													  it != m_conditions.end();
													  ++it )
	{
		if ( *it == condition )
		{
			m_conditions.erase( it );
			delete condition;
			break;
		}
	}
}

void CEdMassActionContext::ReplaceCondition( IEdMassActionCondition* oldCondition, IEdMassActionCondition* newCondition )
{
	for ( wxVector<IEdMassActionCondition*>::iterator it =  m_conditions.begin();
													  it != m_conditions.end();
													  ++it )
	{
		if ( *it == oldCondition )
		{
			m_conditions.insert( it, newCondition );
			break;
		}
	}
	RemoveCondition( oldCondition );
}

int	CEdMassActionContext::GetConditionTypeCount() const
{
	return (int)m_conditionTypes.size();
}

IEdMassActionConditionType* CEdMassActionContext::GetConditionType( int n )
{
	return n < (int)m_conditionTypes.size() ? m_conditionTypes.at( (int)n ) : NULL;
}

int CEdMassActionContext::GetConditionCount() const
{
	return (int)m_conditions.size();
}

IEdMassActionCondition* CEdMassActionContext::GetCondition( int n )
{
	return n < (int)m_conditions.size() ? m_conditions.at( (int)n ) : NULL;
}

int CEdMassActionContext::GetConditionComparatorCount() const
{
	return m_comparators.size();
}

IEdMassActionConditionComparator* CEdMassActionContext::GetConditionComparator( int n )
{
	return n < (int)m_comparators.size() ? m_comparators.at( (int)n ) : NULL;
}

EEdMassActionConditionUsage CEdMassActionContext::GetConditionUsage() const
{
	return m_conditionUsage;
}

void CEdMassActionContext::SetConditionUsage( EEdMassActionConditionUsage usage )
{
	m_conditionUsage = usage;
}

bool CEdMassActionContext::ValidateActions()
{
	for ( wxVector<IEdMassAction*>::iterator it =  m_actions.begin();
											 it != m_actions.end();
											 ++it )
	{
		if ( !(*it)->Validate() ) return false;
	}

	return true;
}

bool CEdMassActionContext::ValidateConditions()
{
	for ( wxVector<IEdMassActionCondition*>::iterator it =  m_conditions.begin();
													  it != m_conditions.end();
													  ++it )
	{
		if ( !(*it)->Validate() ) return false;
	}

	return true;
}

bool CEdMassActionContext::PerformActions()
{
	int processedObjects = 0, totalObjects = 0;
	bool dontBotherMe = false;

	RewindIterator();
	
	if ( m_processedObjects )
	{
		m_processedObjects->clear();
	}

	while ( HasMoreObjects() )
	{
		size_t failed = 0, passedConditions = 0;
		wxString failedActions = wxEmptyString;
		IEdMassActionObject* object = GetNextObject();
		bool process = false;
		totalObjects++;

		for ( wxVector<IEdMassActionCondition*>::iterator it =  m_conditions.begin();
														  it != m_conditions.end();
														  ++it )
		{
			IEdMassActionCondition* condition = *it;
			if ( condition->Check( object ) ) passedConditions++;
		}

		switch ( GetConditionUsage() )
		{
		case CEDMACU_ALWAYS_PASS:
			process = true;
			break;
		case CEDMACU_NEED_ALL_CONDITIONS:
			process = passedConditions == m_conditions.size();
			break;
		case CEDMACU_NEED_ANY_CONDITION:
			process = passedConditions > 0;
			break;
		case CEDMACU_NEED_NO_CONDITIONS:
			process = passedConditions == 0;
			break;
		}

		if ( process )
		{
			for ( wxVector<IEdMassAction*>::iterator it =  m_actions.begin();
													 it != m_actions.end();
													 ++it )
			{
				if ( !(*it)->Perform( object ) )
				{
					failedActions = failedActions + wxT("     ") + (*it)->GetDescription() + wxT("\r\n");
					failed++;
				}
			}

			if ( failed == 0 )
			{
				processedObjects++;
				if ( m_processedObjects )
				{
					m_processedObjects->push_back( object );
				}
			}
			else if ( !dontBotherMe )
			{
				wxString msg = wxString::Format( wxT("%u of %u actions failed:\n\n%s\n\nDo you want to continue?"), failed, m_actions.size(), failedActions );
				if ( wxMessageBox( msg, wxT("Some actions failed!"), wxYES_NO|wxYES_DEFAULT|wxCENTER|wxICON_ERROR, m_dialog ) == wxNO )
				{
					GFeedback->EndTask();
					return false;
				}
				if ( wxMessageBox( wxT("Do you want to see more failure messages?"), wxT("Question"), wxYES_NO|wxNO_DEFAULT|wxCENTER|wxICON_QUESTION, m_dialog ) == wxNO )
				{
					dontBotherMe = true;
				}
			}
		}
	}

	wxString message = m_searchMode ? wxT("%u of %u known objects match") : wxT("%u of %u known objects processed");
	message = wxString::Format( message, processedObjects, totalObjects );
	wxMessageBox( message, wxT("Done"), wxCENTRE|wxOK|wxICON_INFORMATION, m_dialog );

	return true;
}

bool CEdMassActionContext::Export( const wxString& filename )
{
	wxXmlDocument doc;
	wxXmlNode* root = new wxXmlNode( wxXML_ELEMENT_NODE, wxT("madesc") );
	wxXmlNode* conditionsNode = new wxXmlNode( root, wxXML_ELEMENT_NODE, wxT("conditions") );
	wxXmlNode* actionsNode = new wxXmlNode( root, wxXML_ELEMENT_NODE, wxT("actions") );

	switch ( GetConditionUsage() )
	{
	case CEDMACU_NEED_NO_CONDITIONS:
		conditionsNode->AddAttribute( wxT("usage"), wxT("none") );
		break;
	case CEDMACU_NEED_ALL_CONDITIONS:
		conditionsNode->AddAttribute( wxT("usage"), wxT("all") );
		break;
	case CEDMACU_NEED_ANY_CONDITION:
		conditionsNode->AddAttribute( wxT("usage"), wxT("any") );
		break;
	case CEDMACU_ALWAYS_PASS:
		conditionsNode->AddAttribute( wxT("usage"), wxT("always") );
		break;
	}

	for ( int i=0; i<GetConditionCount(); ++i )
	{
		IEdMassActionCondition* cond = GetCondition( i );
		if ( !cond->GetType() ) continue;
		wxXmlNode* condNode = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT("condition") );
		condNode->AddAttribute( wxT("type"), cond->GetType()->GetName() );
		cond->ExportToXML( condNode );
		conditionsNode->AddChild( condNode );
	}

	for ( int i=0; i<GetActionCount(); ++i )
	{
		IEdMassAction* action = GetAction( i );
		if ( !action->GetType() ) continue;
		wxXmlNode* actNode = new wxXmlNode( NULL, wxXML_ELEMENT_NODE, wxT("action") );
		actNode->AddAttribute( wxT("type"), action->GetType()->GetName() );
		action->ExportToXML( actNode );
		actionsNode->AddChild( actNode );
	}

	doc.SetRoot( root );

	return doc.Save( filename );
}

bool CEdMassActionContext::Import( const wxString& filename )
{
	wxXmlDocument doc;

	if ( !doc.Load( filename ) )
	{
		wxMessageBox( wxString::Format( wxT("Failed to load '%s'"), filename ), wxT("Import Error"), wxOK|wxICON_ERROR|wxCENTRE, m_dialog );
		return false;
	}

	if ( doc.GetRoot()->GetName() != "madesc" )
	{
		wxMessageBox( wxString::Format( wxT("Invalid mass action description file '%s'"), filename ), wxT("Import Error"), wxOK|wxICON_ERROR|wxCENTRE, m_dialog );
		return false;
	}

	GetDialog()->Freeze();

	m_dialog->m_conditionsWindow->RemoveAllItems();
	while ( m_conditions.size() > 0 ) RemoveCondition( m_conditions.at( 0 ) );

	if ( !m_searchMode )
	{
		m_dialog->m_actionsWindow->RemoveAllItems();
		while ( m_actions.size() > 0 ) RemoveAction( m_actions.at( 0 ) );
	}

	for ( wxXmlNode* child = doc.GetRoot()->GetChildren(); child; child=child->GetNext() )
	{
		if ( child->GetName() == "conditions" )
		{
			const wxString& condUsage = child->GetAttribute( "usage", "all" );

			if ( condUsage == "none" )
				SetConditionUsage( CEDMACU_NEED_NO_CONDITIONS );
			else if ( condUsage == "all" )
				SetConditionUsage( CEDMACU_NEED_ALL_CONDITIONS );
			else if ( condUsage == "any" )
				SetConditionUsage( CEDMACU_NEED_ANY_CONDITION );
			else if ( condUsage == "always" )
				SetConditionUsage( CEDMACU_ALWAYS_PASS );

			for ( wxXmlNode* condNode = child->GetChildren(); condNode; condNode=condNode->GetNext() )
			{
				if ( condNode->GetName() != "condition" ) continue;

				IEdMassActionConditionType* condType = NULL;
				wxString typeName = condNode->GetAttribute( "type" );

				for ( int i=0; i<GetConditionTypeCount(); ++i )
				{
					if ( GetConditionType( i )->GetName() == typeName )
					{
						condType = GetConditionType( i );
						break;
					}
				}

				if ( !condType ) {
					wxMessageBox( wxString::Format( wxT("Unknown condition: %s"), filename ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_dialog );
					continue;
				}

				IEdMassActionCondition* cond = ((CEdMassActionConditionList*)GetDialog()->m_conditionsWindow)->AddCondition( condType );
				cond->ImportFromXML( condNode );
			}
		}
		else if ( child->GetName() == "actions"  && !m_searchMode )
		{
			for ( wxXmlNode* actNode = child->GetChildren(); actNode; actNode=actNode->GetNext() )
			{
				if ( actNode->GetName() != "action" ) continue;

				IEdMassActionType* actionType = NULL;
				wxString typeName = actNode->GetAttribute( "type" );

				for ( int i=0; i<GetActionTypeCount(); ++i )
				{
					if ( GetActionType( i )->GetName() == typeName )
					{
						actionType = GetActionType( i );
						break;
					}
				}

				if ( !actionType ) {
					wxMessageBox( wxString::Format( wxT("Unknown action: %s"), filename ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_dialog );
					continue;
				}

				IEdMassAction* action = ((CEdMassActionList*)GetDialog()->m_actionsWindow)->AddAction( actionType );
				action->ImportFromXML( actNode );
			}
		}
	}

	GetDialog()->SetConditionUsage( m_conditionUsage );

	if ( !m_searchMode )
	{
		GetDialog()->m_actionsWindow->Scroll( 0, 0 );
	}
	GetDialog()->m_conditionsWindow->Scroll( 0, 0 );

	GetDialog()->Thaw();
	GetDialog()->Refresh( true );

	return true;
}

//////////////////////////////////////////////////////////////////////////

void CEdMassActionPropertyModifierAction::FillProperties()
{
	m_propsChoice->Freeze();
	for ( int i=0; i<m_context->GetPropertyCount(); ++i )
	{
		m_propsChoice->Append( m_context->GetPropertyName( i ).AsChar() );
	}
	m_propsChoice->Thaw();
}

void CEdMassActionPropertyModifierAction::CreatePanel( wxWindow* parent )
{
	m_panel = new wxPanel( parent );
	m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
	m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

	AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("property") ) );

	m_propsChoice = new wxChoice( m_panel, wxID_ANY );
	FillProperties();
	m_propsChoice->SetSelection( m_previousProperty );
	AddPanelWidget( m_propsChoice );

	CreatePanelWidgets();
}

void CEdMassActionPropertyModifierAction::AddPanelWidget( wxWindow* widget, int proportion /* = 0 */ )
{
	m_panel->GetSizer()->Add( widget, proportion, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
	m_panel->Layout();
}

void CEdMassActionPropertyModifierAction::OnActionSelect( IEdMassAction* previousSelection )
{
	CEdMassActionPropertyModifierAction* prevSel;
	if ( ( prevSel = dynamic_cast<CEdMassActionPropertyModifierAction*>(previousSelection) ) )
	{
		m_previousProperty = prevSel->m_propsChoice->GetSelection();
	}
}

void CEdMassActionPropertyModifierAction::ExportToXML( wxXmlNode* node )
{
	node->AddAttribute( wxT("property"), m_propsChoice->GetStringSelection() );
}

void CEdMassActionPropertyModifierAction::ImportFromXML( wxXmlNode* node )
{
	int index = m_propsChoice->FindString( node->GetAttribute( wxT("property") ) );
	if ( index == -1 )
	{
		wxMessageBox( wxString::Format( wxT("Unknown property: %s"), node->GetAttribute( wxT("property") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
		index = 0;
	}
	m_propsChoice->SetSelection( index );
}

CEdMassActionPropertyModifierAction::CEdMassActionPropertyModifierAction( CEdMassActionContext* context )
	: IEdMassAction( context )
	, m_panel( NULL )
	, m_previousProperty( 0 )
{
}

wxWindow* CEdMassActionPropertyModifierAction::GetWindow( wxWindow* parent )
{
	if ( !m_panel )
	{
		CreatePanel( parent );
	}
	return m_panel;
}

//////////////////////////////////////////////////////////////////////////

class CEdMassActionSetProperty : public IEdMassActionType
{
	class Action : public CEdMassActionPropertyModifierAction
	{
		wxTextCtrl* m_text;
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("to") ) );
			AddPanelWidget( m_text = (new wxTextCtrl( m_panel, wxID_ANY )), 1 );
		}

		virtual wxString GetDescription() const
		{
			return wxT("Set property ") + GetSelectedProperty() + wxT(" to \"") + m_text->GetValue() + wxT("\"");
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			return SetPropertyString( object, m_text->GetValue() );
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("value"), m_text->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			m_text->SetValue( node->GetAttribute( wxT("value") ) );
		}
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Set");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionIncreaseDecreaseProperty : public IEdMassActionType
{
	bool m_increase;

	class Action : public CEdMassActionPropertyModifierAction
	{
		bool m_increase;
		wxSpinCtrl* m_spin;

	public:
		Action( IEdMassActionType* type, bool increase, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
			, m_increase( increase )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("by") ) );
			AddPanelWidget( m_spin = new wxSpinCtrl( m_panel, wxID_ANY ) );
			m_spin->SetRange( INT_MIN, INT_MAX );
		}

		virtual wxString GetDescription() const
		{
			return ( m_increase ? wxT("Increase property ") : wxT("Decrease property ") ) +
				     GetSelectedProperty() + wxT(" by ") + wxString::Format( "%d", m_spin->GetValue() );
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			if ( m_increase )
				return SetPropertyDouble( object, GetPropertyDouble( object ) + m_spin->GetValue() );
			else
				return SetPropertyDouble( object, GetPropertyDouble( object ) - m_spin->GetValue() );
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("value"), wxString::Format("%d", m_spin->GetValue() ) );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			long l = 0;
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			node->GetAttribute( wxT("value") ).ToLong( &l );
			m_spin->SetValue( l );
		}
	};

public:
	CEdMassActionIncreaseDecreaseProperty( bool increase )
		: IEdMassActionType()
		, m_increase( increase )
	{}

	virtual wxString GetName() const
	{
		return m_increase ? wxT("Increase") : wxT("Decrease");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_increase, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionLimitProperty : public IEdMassActionType
{
	class Action : public CEdMassActionPropertyModifierAction
	{
		wxSpinCtrl* m_lower_spin;
		wxSpinCtrl* m_higher_spin;
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("between") ) );
			AddPanelWidget( m_lower_spin = new wxSpinCtrl( m_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(32, wxDefaultSize.y) ), 1 );
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("and") ) );
			AddPanelWidget( m_higher_spin = new wxSpinCtrl( m_panel, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(32, wxDefaultSize.y) ), 1 );
			m_lower_spin->SetRange( INT_MIN, INT_MAX );
			m_higher_spin->SetRange( INT_MIN, INT_MAX );
		}
		
		virtual wxString GetDescription() const
		{
			return wxT("Limit property ") + GetSelectedProperty() +
				wxString::Format( wxT(" between %d and %d"),
				m_lower_spin->GetValue(), m_higher_spin->GetValue() );
		}
		
		virtual bool Validate()
		{
			if ( m_lower_spin->GetValue() > m_higher_spin->GetValue() )
			{
				wxMessageBox( wxString::Format( wxT("The lower value in limit property %s (%d) is greater than the higher value (%d)"),
					GetSelectedProperty(), m_lower_spin->GetValue(), m_higher_spin->GetValue() ),
					wxT("Invalid values in limit property"), wxOK | wxCENTER | wxICON_ERROR, m_context->GetDialog() );
				return false;
			}
			return true;
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			double min = m_lower_spin->GetValue();
			double max = m_higher_spin->GetValue();
			double val = GetPropertyDouble( object );
			if ( val < min ) val = min;
			if ( val > max ) val = max;
			return SetPropertyDouble( object, val );
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("min"), wxString::Format("%d", m_lower_spin->GetValue() ) );
			node->AddAttribute( wxT("max"), wxString::Format("%d", m_higher_spin->GetValue() ) );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			long min = 0, max = 0;
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			node->GetAttribute( wxT("min") ).ToLong( &min );
			node->GetAttribute( wxT("max") ).ToLong( &max );
			m_lower_spin->SetValue( min );
			m_higher_spin->SetValue( max );
		}
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Limit");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionReplacePartOfProperty : public IEdMassActionType
{
	class Action : public CEdMassActionPropertyModifierAction
	{
		wxChoice* m_times;
		wxTextCtrl* m_before;
		wxTextCtrl* m_after;
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("from") ) );
			AddPanelWidget( m_before = (new wxTextCtrl( m_panel, wxID_ANY )), 1 );
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("to") ) );
			AddPanelWidget( m_after = (new wxTextCtrl( m_panel, wxID_ANY )), 1 );
			m_times = new wxChoice( m_panel, wxID_ANY );
			m_times->Append("once at start");
			m_times->Append("once at end");
			m_times->Append("repeatedly");
			m_times->SetSelection( 2 );
			AddPanelWidget( m_times);
		}

		virtual wxString GetDescription() const
		{
			return wxT("Replace part of ") + GetSelectedProperty() + wxT(" from \"") + m_before->GetValue() + wxT("\" to \"") + m_after->GetValue() + wxT("\"");
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			wxString value = GetPropertyString( object );
			int index;
			switch ( m_times->GetSelection() )
			{
			case 0: /* once at start */
				value.Replace( m_before->GetValue(), m_after->GetValue(), false );
				break;
			case 1: /* once at end */
				index = value.rfind( m_before->GetValue() );
				if ( index != wxString::npos )
				{
					value.replace( index, m_before->GetValue().Length(), m_after->GetValue() );
				}
				break;
			case 2: /* repeatedly */
				value.Replace( m_before->GetValue(), m_after->GetValue(), true );
				break;
			}
			
			return SetPropertyString( object, value );
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("before"), m_before->GetValue() );
			node->AddAttribute( wxT("after"), m_after->GetValue() );
			node->AddAttribute( wxT("times"), m_times->GetStringSelection() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			m_before->SetValue( node->GetAttribute( wxT("before") ) );
			m_after->SetValue( node->GetAttribute( wxT("after") ) );
			m_times->SetSelection( m_times->FindString( node->GetAttribute( wxT("times") ) ) );
		}
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Replace part of");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionAppendToProperty : public IEdMassActionType
{
	class Action : public CEdMassActionPropertyModifierAction
	{
		wxTextCtrl* m_value;
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("the value") ) );
			AddPanelWidget( m_value = (new wxTextCtrl( m_panel, wxID_ANY )), 1 );
		}

		virtual wxString GetDescription() const
		{
			return wxT("Append to property ") + GetSelectedProperty() + wxT(" the value \"") + m_value->GetValue() + wxT("\"");
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			return SetPropertyString( object, GetPropertyString( object ) + m_value->GetValue() );
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("value"), m_value->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			m_value->SetValue( node->GetAttribute( wxT("value") ) );
		}
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Append to");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionAddToArray : public IEdMassActionType
{
	class Action : public CEdMassActionPropertyModifierAction
	{
		wxTextCtrl* m_value;
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: CEdMassActionPropertyModifierAction( context )
		{
			m_type = type;
		}

		virtual void CreatePanelWidgets()
		{
			AddPanelWidget( new wxStaticText( m_panel, wxID_ANY, wxT("the value") ) );
			AddPanelWidget( m_value = (new wxTextCtrl( m_panel, wxID_ANY )), 1 );
		}

		virtual wxString GetDescription() const
		{
			return wxT("Add to array ") + GetSelectedProperty() + wxT(" the value \"") + m_value->GetValue() + wxT("\"");
		}

		virtual bool Perform( IEdMassActionObject* object )
		{
			// We can only work with arrays
			CEdMassActionRTTIProxy* rttiObject = dynamic_cast< CEdMassActionRTTIProxy* >( object );
			if ( rttiObject == nullptr ) return false;

			// Find the class
			CClass* cls = rttiObject->GetClass();
			if ( cls == nullptr ) return false;

			// Find the property
			CProperty* property = cls->FindProperty( CName( GetSelectedProperty().wc_str() ) );
			if ( property == nullptr ) return false;

			// Make sure the property is an array
			if ( !property->GetType()->IsArrayType() ) return false;
			CRTTIArrayType* arrayType = static_cast< CRTTIArrayType* >( property->GetType() );
			void* arr = property->GetOffsetPtr( rttiObject->GetObject() );
			
			// Create new object of the array's inner type
			IRTTIType* innerType = arrayType->GetInnerType();
			void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, innerType->GetSize() );
			if ( buffer == nullptr ) return false;
			innerType->Construct( buffer );

			// Try to set the new object's value from the string
			if ( !innerType->FromString( buffer, m_value->GetValue().wc_str() ) )
			{
				innerType->Destruct( buffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
				return false;
			}

			// Add new array element
			Int32 index = arrayType->AddArrayElement( arr );
			void* tgtBuffer = arrayType->GetArrayElement( arr, index );
			innerType->Construct( tgtBuffer );
			innerType->Copy( tgtBuffer, buffer );
			innerType->Destruct( buffer );
			RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );

			return true;
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ExportToXML( node );
			node->AddAttribute( wxT("value"), m_value->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			CEdMassActionPropertyModifierAction::ImportFromXML( node );
			m_value->SetValue( node->GetAttribute( wxT("value") ) );
		}
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Add to array");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

bool CEdMassActionNumberComparator::Validate( const wxString& v )
{
	double dummy;
	if ( !v.ToDouble( &dummy ) )
	{
		wxMessageBox( wxT("The reference value is not a valid number!"), wxT("Comparison error"), wxCENTRE|wxOK|wxICON_ERROR );
		return false;
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////

class CEdMassActionConditionEqualComparator : public IEdMassActionConditionComparator
{
public:
	virtual wxString GetName() const
	{
		return wxT("equal to");
	}

	virtual bool Compare( const wxString& a, const wxString& b )
	{
		return a.IsSameAs( b );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionConditionGreaterComparator : public CEdMassActionNumberComparator
{
public:
	virtual wxString GetName() const
	{
		return wxT("greater than");
	}

	virtual bool Compare( const wxString& a, const wxString& b )
	{
		double da, db;
		if ( a.IsEmpty() || !a.ToDouble( &da ) ) return false;
		if ( b.IsEmpty() || !b.ToDouble( &db ) ) return false;
		return da > db;
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionConditionLesserComparator : public CEdMassActionNumberComparator
{
public:
	virtual wxString GetName() const
	{
		return wxT("lesser than");
	}

	virtual bool Compare( const wxString& a, const wxString& b )
	{
		double da, db;
		if ( a.IsEmpty() || !a.ToDouble( &da ) ) return false;
		if ( a.IsEmpty() || !b.ToDouble( &db ) ) return false;
		return da < db;
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionConditionPartiallyComparator : public IEdMassActionConditionComparator
{
public:
	virtual wxString GetName() const
	{
		return wxT("partially");
	}

	virtual bool Validate(  const wxString& v )
	{
		if ( v.IsEmpty() )
		{
			wxMessageBox( wxT("The partial value cannot be empty!"), wxT("Comparison error"), wxCENTRE|wxOK|wxICON_ERROR );
			return false;
		}
		return true;
	}

	virtual bool Compare( const wxString& a, const wxString& b )
	{
		return a.Find( b ) > -1;
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionPropertyCondition : public IEdMassActionConditionType
{
	bool m_anyProperty;

	class Condition : public IEdMassActionCondition
	{
		bool m_anyProperty;
		wxChoice* m_propsChoice;
		wxChoice* m_equality;
		wxChoice* m_comparatorChoice;
		wxTextCtrl* m_value;
		int m_previousProperty;
		int m_previousEquality;
		int m_previousComparator;
		wxString m_previousValue;
		wxPanel* m_panel;

	public:
		wxString GetSelectedProperty() const
		{
			return m_anyProperty ? wxEmptyString : m_propsChoice->GetStringSelection();
		}

		void CreatePanel( wxWindow* parent )
		{
			m_panel = new wxPanel( parent );
			m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
			m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			if ( !m_anyProperty )
			{
				m_propsChoice = new wxChoice( m_panel, wxID_ANY );
				m_propsChoice->Freeze();
				for ( int i=0; i<m_context->GetPropertyCount(); ++i )
				{
					m_propsChoice->Append( m_context->GetPropertyName( i ).AsChar() );
				}
				m_propsChoice->Thaw();
				m_propsChoice->SetSelection( m_previousProperty );
				AddPanelWidget( m_propsChoice );
			}

			m_equality = new wxChoice( m_panel, wxID_ANY );
			m_equality->Append( "is" );
			m_equality->Append( "is not" );
			m_equality->SetSelection( m_previousEquality );
			AddPanelWidget( m_equality );

			m_comparatorChoice = new wxChoice( m_panel, wxID_ANY );
			m_comparatorChoice->Freeze();
			for ( int i=0; i<m_context->GetConditionComparatorCount(); ++i )
			{
				m_comparatorChoice->Append( m_context->GetConditionComparator( i )->GetName(), m_context->GetConditionComparator( i ) );
			}
			m_comparatorChoice->Thaw();
			m_comparatorChoice->SetSelection( m_previousComparator );
			AddPanelWidget( m_comparatorChoice );

			m_value = new wxTextCtrl( m_panel, wxID_ANY, m_previousValue );
			AddPanelWidget( m_value, 1 );
		}

		void AddPanelWidget( wxWindow* widget, int proportion = 0 )
		{
			m_panel->GetSizer()->Add( widget, proportion, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
			m_panel->Layout();
		}

		virtual void OnConditionSelect( IEdMassActionCondition* previousSelection )
		{
			Condition* prevSel;
			if ( ( prevSel = dynamic_cast<Condition*>(previousSelection) ) )
			{
				m_previousProperty = prevSel->m_propsChoice ? prevSel->m_propsChoice->GetSelection() : 0;
				m_previousEquality = prevSel->m_equality->GetSelection();
				m_previousComparator = prevSel->m_comparatorChoice->GetSelection();
				m_previousValue = prevSel->m_value->GetValue();
			}
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			if ( !m_anyProperty )
			{
				node->AddAttribute( wxT("property"), m_propsChoice->GetStringSelection() );
			}
			node->AddAttribute( wxT("equality"), m_equality->GetStringSelection() );
			node->AddAttribute( wxT("comparator"), m_comparatorChoice->GetStringSelection() );
			node->AddAttribute( wxT("value"), m_value->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			int index;
			if ( !m_anyProperty )
			{
				index = m_propsChoice->FindString( node->GetAttribute( wxT("property") ) );
				if ( index == -1 )
				{
					wxMessageBox( wxString::Format( wxT("Unknown property: %s"), node->GetAttribute( wxT("property") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
					index = 0;
				}
				m_propsChoice->SetSelection( index );
			}

			index = m_equality->FindString( node->GetAttribute( wxT("equality") ) );
			if ( index == -1 )
			{
				wxMessageBox( wxString::Format( wxT("Unknown equality: %s"), node->GetAttribute( wxT("equality") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
				index = 0;
			}
			m_equality->SetSelection( index );

			index = m_comparatorChoice->FindString( node->GetAttribute( wxT("comparator") ) );
			if ( index == -1 )
			{
				wxMessageBox( wxString::Format( wxT("Unknown comparator: %s"), node->GetAttribute( wxT("comparator") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
				index = 0;
			}
			m_comparatorChoice->SetSelection( index );

			m_value->SetValue( node->GetAttribute( wxT("value") ) );
		}

	public:
		Condition( IEdMassActionConditionType* type, CEdMassActionContext* context, bool anyProperty )
			: IEdMassActionCondition( context )
			, m_propsChoice( NULL )
			, m_panel( NULL )
			, m_previousProperty( 0 )
			, m_previousEquality( 0 )
			, m_previousComparator( 0 )
			, m_anyProperty( anyProperty )
		{
			m_type = type;
		}

		virtual wxWindow* GetWindow( wxWindow* parent )
		{
			if ( !m_panel )
			{
				CreatePanel( parent );
			}
			return m_panel;
		}

		virtual wxString GetDescription() const
		{
			if ( m_anyProperty )
			{
				return wxString::Format( wxT("The value of any property %s %s \"%s\""),
					m_equality->GetStringSelection(),
					m_comparatorChoice->GetStringSelection(),
					m_value->GetValue() );
			}
			else
			{
				return wxString::Format( wxT("The value of the property %s %s %s \"%s\""),
					m_propsChoice->GetStringSelection(),
					m_equality->GetStringSelection(),
					m_comparatorChoice->GetStringSelection(),
					m_value->GetValue() );
			}
		}

		virtual bool Validate()
		{
			IEdMassActionConditionComparator* comparator = (IEdMassActionConditionComparator*)m_comparatorChoice->GetClientData( m_comparatorChoice->GetSelection() );
			return comparator->Validate( m_value->GetValue() );
		}

		virtual bool Check( IEdMassActionObject* object )
		{
			bool ok = false;
			IEdMassActionConditionComparator* comparator = (IEdMassActionConditionComparator*)m_comparatorChoice->GetClientData( m_comparatorChoice->GetSelection() );
			
			if ( m_anyProperty )
			{
				for ( int i=0; i<object->GetPropertyCount(); ++i )
				{
					if ( comparator->Compare( object->GetPropertyString( i ), m_value->GetValue() ) )
					{
						ok = true;
						break;
					}
				}
			}
			else
			{
				int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
				if ( index == -1 ) return false;
				ok = comparator->Compare( object->GetPropertyString( index ), m_value->GetValue() );
			}

			if ( m_equality->GetSelection() == 1 /* `Is not' */ ) ok = !ok;

			return ok;
		}
	};

public:
	CEdMassActionPropertyCondition( bool anyProperty )
		: IEdMassActionConditionType()
		, m_anyProperty( anyProperty )
	{}

	virtual wxString GetName() const
	{
		return m_anyProperty ? wxT("The value of any property") : wxT("The value of the property");
	}

	virtual IEdMassActionCondition*	Create()
	{
		return new Condition( this, m_context, m_anyProperty );
	}
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionObjectTypeCondition : public IEdMassActionConditionType
{
	class Condition : public IEdMassActionCondition
	{
		wxChoice* m_equality;
		wxChoice* m_typesChoice;
		int m_previousEquality;
		int m_previousType;
		wxPanel* m_panel;

	protected:
		void CreatePanel( wxWindow* parent )
		{
			m_panel = new wxPanel( parent );
			m_panel->SetBackgroundColour( parent->GetBackgroundColour() );
			m_panel->SetSizer( new wxBoxSizer( wxHORIZONTAL ) );

			m_equality = new wxChoice( m_panel, wxID_ANY );
			m_equality->Append( "is" );
			m_equality->Append( "is not" );
			m_equality->SetSelection( m_previousEquality );
			AddPanelWidget( m_equality );

			m_typesChoice = new wxChoice( m_panel, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, NULL, wxCB_SORT );
			THashSet< CClass* > classes;
			m_context->RewindIterator();
			while ( m_context->HasMoreObjects() )
			{
				CClass* cls = m_context->GetNextObject()->GetClass();
				if ( !cls ) continue;
				classes.Insert( cls );
			}
			m_typesChoice->Freeze();
			for ( auto it=classes.Begin(); it != classes.End(); ++it )
			{
				m_typesChoice->Append( (*it)->GetName().AsString().AsChar() );
			}
			m_typesChoice->Thaw();
			m_typesChoice->SetSelection( m_previousType );
			AddPanelWidget( m_typesChoice );
		}

		void AddPanelWidget( wxWindow* widget, int proportion = 0 )
		{
			m_panel->GetSizer()->Add( widget, proportion, wxALIGN_CENTRE_VERTICAL | wxLEFT, 5 );
			m_panel->Layout();
		}

		virtual void OnConditionSelect( IEdMassActionCondition* previousSelection )
		{
			Condition* prevSel;
			if ( ( prevSel = dynamic_cast<Condition*>(previousSelection) ) )
			{
				m_previousEquality = prevSel->m_equality->GetSelection();
				m_previousType = prevSel->m_typesChoice->GetSelection();
			}
		}

		virtual void ExportToXML( wxXmlNode* node )
		{
			node->AddAttribute( wxT("equality"), m_equality->GetStringSelection() );
			node->AddAttribute( wxT("objecttype"), m_typesChoice->GetStringSelection() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			int index;

			index = m_equality->FindString( node->GetAttribute( wxT("equality") ) );
			if ( index == -1 )
			{
				wxMessageBox( wxString::Format( wxT("Unknown equality: %s"), node->GetAttribute( wxT("equality") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
				index = 0;
			}
			m_equality->SetSelection( index );

			index = m_typesChoice->FindString( node->GetAttribute( wxT("objecttype") ) );
			if ( index == -1 )
			{
				wxMessageBox( wxString::Format( wxT("Unknown type: %s"), node->GetAttribute( wxT("objecttype") ) ), wxT("Import Warning"), wxOK|wxICON_ERROR|wxCENTRE, m_context->GetDialog() );
				index = 0;
			}
			m_typesChoice->SetSelection( index );
		}

	public:
		Condition( IEdMassActionConditionType* type, CEdMassActionContext* context )
			: IEdMassActionCondition( context )
			, m_typesChoice( NULL )
			, m_panel( NULL )
			, m_previousEquality( 0 )
		{
			m_type = type;
		}

		virtual wxWindow* GetWindow( wxWindow* parent )
		{
			if ( !m_panel )
			{
				CreatePanel( parent );
			}
			return m_panel;
		}

		virtual wxString GetDescription() const
		{
			return wxString::Format( wxT("The type of the item %s %s"),
				m_equality->GetStringSelection(),
				m_typesChoice->GetStringSelection()
			);
		}

		virtual bool Validate()
		{
			return true;
		}

		virtual bool Check( IEdMassActionObject* object )
		{
			CClass* cls = object->GetClass();
			/* in case if no class, return true if the "is not" equality is
			 * selected and false if the "is" equality is selected */
			if ( !cls ) return m_equality->GetSelection() == 1;
				
			wxString className( object->GetClass()->GetName().AsString().AsChar() );
			bool ok = className == m_typesChoice->GetStringSelection();
			if ( m_equality->GetSelection() == 1 /* is not */ )
			{
				ok = !ok;
			}

			return ok;
		}
	};

public:
	CEdMassActionObjectTypeCondition()
		: IEdMassActionConditionType()
	{}

	virtual wxString GetName() const
	{
		return wxT("The type of the item");
	}

	virtual IEdMassActionCondition*	Create()
	{
		return new Condition( this, m_context );
	}
};

//////////////////////////////////////////////////////////////////////////

CEdMassActionArrayIterator::CEdMassActionArrayIterator( IEdMassActionObject** objects, int count )
	: m_objects( objects )
	, m_count( count )
	, m_head( 0 )
{
}

void CEdMassActionArrayIterator::Rewind()
{
	m_head = 0;
}

bool CEdMassActionArrayIterator::HasMore()
{
	return m_head < m_count;
}

IEdMassActionObject* CEdMassActionArrayIterator::Next()
{
	return HasMore() ? m_objects[m_head++] : NULL;
}

//////////////////////////////////////////////////////////////////////////

CEdMassActionDialog::CEdMassActionDialog( wxWindow* parent, const wxString& title, CEdMassActionContext* context )
	: wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX )
{
	m_context = context;
	context->m_dialog = this;

	wxStaticText* staticText;

	SetSizeHints( wxSize( 800, context->GetSearchMode() ? 300 : 450 ), wxDefaultSize );

	wxBoxSizer* paddingSizer = new wxBoxSizer( wxVERTICAL );

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer( wxVERTICAL );
	paddingSizer->Add( bSizer1, 1, wxALL | wxEXPAND, 10 );

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxHORIZONTAL );

	if ( !context->GetSearchMode() )
	{
		staticText = new wxStaticText( this, wxID_ANY, wxT("Perform the following actions:"), wxDefaultPosition, wxDefaultSize, 0 );
		staticText->Wrap( -1 );
		bSizer1->Add( staticText, 0, wxLEFT | wxRIGHT | wxTOP, 5 );

		m_actionsWindow = new CEdMassActionList( m_context, this );
		bSizer1->Add( m_actionsWindow, 1, wxALL | wxEXPAND, 5 );

		m_alwaysRadioButton = new wxRadioButton( this, wxID_ANY, wxT("&Always"), wxDefaultPosition, wxDefaultSize, 0 );
		m_alwaysRadioButton->Bind( wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CEdMassActionDialog::OnRadioButtonSelected, this);
		bSizer1->Add( m_alwaysRadioButton, 0, wxLEFT | wxRIGHT | wxTOP, 5 );

		m_onlyWhenRadioButton = new wxRadioButton( this, wxID_ANY, wxT("Only &when"), wxDefaultPosition, wxDefaultSize, 0 );
		m_onlyWhenRadioButton->SetValue( true ); 
		m_onlyWhenRadioButton->Bind( wxEVT_COMMAND_RADIOBUTTON_SELECTED, &CEdMassActionDialog::OnRadioButtonSelected, this);
		bSizer2->Add( m_onlyWhenRadioButton, 0, wxLEFT | wxTOP | wxBOTTOM | wxEXPAND, 5 );
	}
	else
	{
		staticText = new wxStaticText( this, wxID_ANY,  "Matches occur only when" );
		bSizer2->Add( staticText, 0, wxALIGN_CENTRE_VERTICAL | wxALL, 5 );
	}

	wxString choices[] = { wxT("All"), wxT("Any"), wxT("None") };
	m_conditionsMatchingChoice = new wxChoice( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 3, choices, 0 );
	m_conditionsMatchingChoice->SetSelection( 0 );
	bSizer2->Add( m_conditionsMatchingChoice, 0, wxALIGN_CENTER | wxBOTTOM | wxTOP, 5 );

	staticText = new wxStaticText( this, wxID_ANY, wxT("of the following conditions are met:"), wxDefaultPosition, wxDefaultSize, 0 );
	staticText->Wrap( -1 );
	bSizer2->Add( staticText, 0, wxALIGN_CENTER | wxALL, 5 );

	bSizer1->Add( bSizer2, 0, wxALIGN_LEFT|wxALIGN_TOP|wxEXPAND, 5 );

	m_conditionsWindow = new CEdMassActionConditionList( m_context, this );
	bSizer1->Add( m_conditionsWindow, 1, wxEXPAND |wxLEFT | wxRIGHT, 5 );

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxHORIZONTAL );

	if ( !m_context->GetSearchMode() )
	{
		m_importButton = new wxButton( this, wxID_ANY, wxT("&Import..."), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer4->Add( m_importButton, 0, wxALL, 5 );

		m_exportButton = new wxButton( this, wxID_ANY, wxT("&Export..."), wxDefaultPosition, wxDefaultSize, 0 );
		m_exportButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdMassActionDialog::OnExportClicked, this );
		bSizer4->Add( m_exportButton, 0, wxBOTTOM | wxRIGHT | wxTOP, 5 );
	}
	else
	{
		m_importButton = new wxButton( this, wxID_ANY, wxT("&Import Conditions..."), wxDefaultPosition, wxDefaultSize, 0 );
		bSizer4->Add( m_importButton, 0, wxALL, 5 );
	}
	m_importButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdMassActionDialog::OnImportClicked, this );

	bSizer4->Add( 0, 0, 1, wxEXPAND, 5 );

	m_okButton = new wxButton( this, wxID_OK, wxT("&OK"), wxDefaultPosition, wxDefaultSize, 0 );
	m_okButton->SetDefault(); 
	m_okButton->Bind( wxEVT_COMMAND_BUTTON_CLICKED, &CEdMassActionDialog::OnOkClicked, this );
	bSizer4->Add( m_okButton, 0, wxALL, 5 );

	m_cancelButton = new wxButton( this, wxID_CANCEL, wxT("&Cancel"), wxDefaultPosition, wxDefaultSize, 0 );
	bSizer4->Add( m_cancelButton, 0, wxBOTTOM | wxRIGHT | wxTOP, 5 );

	bSizer1->Add( bSizer4, 0, wxEXPAND, 5 );

	SetSizer( paddingSizer );
	Layout();
	Fit();
	CentreOnParent();
	UpdateEnabledStatus();
}

void CEdMassActionDialog::UpdateEnabledStatus()
{
	bool enabled = m_context->GetSearchMode() || m_onlyWhenRadioButton->GetValue();

	m_conditionsWindow->Enable( enabled );
	m_conditionsMatchingChoice->Enable( enabled );
}

EEdMassActionConditionUsage CEdMassActionDialog::GetConditionUsage()
{
	if ( !m_context->GetSearchMode() && m_alwaysRadioButton->GetValue() )
	{
		return CEDMACU_ALWAYS_PASS;
	}

	switch ( m_conditionsMatchingChoice->GetSelection() )
	{
	case 0: // All conditions must pass
		return CEDMACU_NEED_ALL_CONDITIONS;
	case 1: // Any condition must pass
		return CEDMACU_NEED_ANY_CONDITION;
	case 2: // None of the conditions must pass
		return CEDMACU_NEED_NO_CONDITIONS;
	}

	// this should never happen, but just in case it does...
	return CEDMACU_NEED_ALL_CONDITIONS;
}

void CEdMassActionDialog::SetConditionUsage( EEdMassActionConditionUsage usage )
{
	switch ( usage )
	{
	case CEDMACU_ALWAYS_PASS:
		if ( !m_context->GetSearchMode() )
		{
			m_alwaysRadioButton->SetValue( true );
		}
		break;
	case CEDMACU_NEED_ALL_CONDITIONS:
		m_conditionsMatchingChoice->SetSelection( 0 );
		break;
	case CEDMACU_NEED_ANY_CONDITION:
		m_conditionsMatchingChoice->SetSelection( 1 );
		break;
	case CEDMACU_NEED_NO_CONDITIONS:
		m_conditionsMatchingChoice->SetSelection( 2 );
		break;
	}
	if ( !m_context->GetSearchMode() )
	{
		m_onlyWhenRadioButton->SetValue( usage != CEDMACU_ALWAYS_PASS );
	}
	UpdateEnabledStatus();
}

void CEdMassActionDialog::OnRadioButtonSelected( wxCommandEvent& event )
{
	m_context->SetConditionUsage( GetConditionUsage() );
	UpdateEnabledStatus();
}

void CEdMassActionDialog::OnOkClicked( wxCommandEvent& event )
{
	if ( ShouldPerformActions() )
	{
		if ( PerformActions() )
		{
			Close();
		}
	}
}

void CEdMassActionDialog::OnImportClicked( wxCommandEvent& event )
{
	wxFileDialog* fd = new wxFileDialog( this, m_context->GetSearchMode() ? wxT("Import Conditions from Mass Actions") : wxT("Import Mass Actions"), "", "", "Mass Action Descriptions (*.madesc)|*.madesc", wxFD_OPEN|wxFD_FILE_MUST_EXIST );
	String finalPath = GDepot->GetRootDataPath() + m_context->m_defaultPath.LeftString( m_context->m_defaultPath.GetLength() - 1 );
	fd->SetDirectory( finalPath.AsChar() );
	if ( fd->ShowModal() == wxID_OK )
	{
		m_context->Import( fd->GetPath() );
	}
	fd->Destroy();
}

void CEdMassActionDialog::OnExportClicked( wxCommandEvent& event )
{
	wxFileDialog* fd = new wxFileDialog( this, wxT("Export Mass Actions"), "", "", "Mass Action Descriptions (*.madesc)|*.madesc", wxFD_SAVE|wxFD_OVERWRITE_PROMPT );
	String finalPath = GDepot->GetRootDataPath() + m_context->m_defaultPath.LeftString( m_context->m_defaultPath.GetLength() - 1 );
	fd->SetDirectory( finalPath.AsChar() );
	if ( fd->ShowModal() == wxID_OK )
	{
		m_context->SetConditionUsage( GetConditionUsage() );
		m_context->Export( fd->GetPath() );
		
		CDirectory* dir = GDepot->FindPath( m_context->m_defaultPath.AsChar() );
		if ( dir ) dir->Repopulate();
	}
	fd->Destroy();
}

bool CEdMassActionDialog::ShouldPerformActions()
{
	m_context->SetConditionUsage( GetConditionUsage() );

	if ( !m_context->GetSearchMode() && m_context->GetActionCount() == 0 )
	{
		wxMessageBox( wxT("No actions to perform"), wxT("Error"), wxOK | wxCENTER | wxICON_ERROR, this );
		return false;
	}

	if ( ( m_context->GetSearchMode() || m_onlyWhenRadioButton->GetValue() ) && m_context->GetConditionCount() == 0 )
	{
		if ( m_context->GetSearchMode() )
		{
			wxMessageBox( wxT("No conditions set for searching"), wxT("Error"), wxOK | wxCENTER | wxICON_ERROR, this );
		}
		else
		{
			wxMessageBox( wxT("No conditions set (maybe you want to use the `Always' setting?)"), wxT("Error"), wxOK | wxCENTER | wxICON_ERROR, this );
		}
		return false;
	}

	if ( m_context->GetSearchMode() )
	{
		return true;
	}

	if ( m_alwaysRadioButton->GetValue() )
		return m_context->ValidateActions();
	else
		return m_context->ValidateActions() && m_context->ValidateConditions();
}

bool CEdMassActionDialog::PerformActions()
{
	m_context->SetConditionUsage( GetConditionUsage() );
	return m_context->PerformActions();
}

CEdMassActionDialog::~CEdMassActionDialog()
{
}

void CEdMassActionDialog::SetDefaults( const String& defaultPath, const String& defaultFilename )
{
	m_context->m_defaultPath = TXT("engine\\massactions\\") + defaultPath + TXT("\\");
	CDirectory* dir = GDepot->FindPath( m_context->m_defaultPath.AsChar() );
	if ( !dir )
	{
		GDepot->CreatePath( m_context->m_defaultPath );
	}
	CFilePath path( m_context->m_defaultPath );
	String basePath;
	GDepot->GetAbsolutePath( basePath );
	for ( Uint32 i = 0; i < path.GetDirectories().Size(); ++i )
	{
		basePath = basePath + path.GetDirectories()[i];
		::CreateDirectory( basePath.AsChar(), NULL );
		basePath = basePath + TXT("\\");
	}

	CDiskFile* file = GDepot->FindFile( m_context->m_defaultPath + defaultFilename );
	if ( file )
	{
		m_context->Import( file->GetAbsolutePath().AsChar() );
	}
}

///////////////////////////////////////////////////////////////////////////

void CEdMassActionContext::AddCommonActionAndConditionTypes()
{
	AddPropertyModifierActionTypes();
	AddPropertyConditionTypes();
}

void CEdMassActionContext::AddPropertyModifierActionTypes()
{
	AddActionType( new CEdMassActionSetProperty() );
	AddActionType( new CEdMassActionIncreaseDecreaseProperty( true ) );
	AddActionType( new CEdMassActionIncreaseDecreaseProperty( false ) );
	AddActionType( new CEdMassActionLimitProperty() );
	AddActionType( new CEdMassActionReplacePartOfProperty() );
	AddActionType( new CEdMassActionAppendToProperty() );
	AddActionType( new CEdMassActionAddToArray() );
}

void CEdMassActionContext::AddPropertyConditionTypes()
{
	AddConditionType( new CEdMassActionPropertyCondition( false ) );
	AddConditionType( new CEdMassActionPropertyCondition( true ) );
	AddConditionType( new CEdMassActionObjectTypeCondition() );
	AddConditionComparator( new CEdMassActionConditionEqualComparator() );
	AddConditionComparator( new CEdMassActionConditionGreaterComparator() );
	AddConditionComparator( new CEdMassActionConditionLesserComparator() );
	AddConditionComparator( new CEdMassActionConditionPartiallyComparator() );
}