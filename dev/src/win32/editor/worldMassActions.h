#pragma once
#include "classHierarchyMapper.h"

// mass actions for changing mesh components class to another one in non-templated entities
class CEdMassActionChangeComponentClass : public IEdMassActionType
{
	class Action : public IEdMassAction
	{
		wxPanel*	m_panel;
		wxChoice*	m_typesChoice;
		wxChoice*	m_classesChoice;

		THashMap< String, CClass* > m_oldClasses;
		THashMap< String, CClass* > m_classes;

		CClassHierarchyMapper	m_componentClassHierarchy;
		Bool					m_classHierarchyInitialized;
		String					m_lastProcessed;

		void CreatePanel( wxWindow* parent );
		virtual void OnActionSelect(IEdMassAction* previousSelection) override;

		void OnComponentsChoiceChanged( wxCommandEvent& event );
		void FillComponentsChoice();
		void FillClassesChoice();

		virtual void ExportToXML( wxXmlNode* node );
		virtual void ImportFromXML( wxXmlNode* node );

	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context );
		
		virtual wxWindow* GetWindow( wxWindow* parent );
		virtual wxString GetDescription() const;
		bool Perform( IEdMassActionObject* object );

	};

public:
	virtual wxString GetName() const
	{
		return wxT("Change mesh components class");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};


// Mass action that selects a node, its entity (for components or
// the node itself for entities) or its layer
class CEdMassActionSelectNode : public IEdMassActionType
{
	THashSet< CNode* >&		m_nodes;	//<! Nodes to select

	class Action : public IEdMassAction
	{
		wxPanel*				m_panel;
		wxChoice*				m_selection;
		int						m_previousSelection;
		THashSet< CNode* >&		m_nodes;	//<! Nodes to select

		void CreatePanel( wxWindow* parent );
		virtual void OnActionSelect( IEdMassAction* previousSelection ) override;

	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context, THashSet< CNode* >& nodes );

		virtual void ExportToXML( wxXmlNode* node ) override;
		virtual void ImportFromXML( wxXmlNode* node ) override;

		wxWindow* GetWindow( wxWindow* parent ) override;
		wxString GetDescription() const;

		bool Perform( IEdMassActionObject* object );
	};

public:
	CEdMassActionSelectNode( THashSet< CNode* >& nodes )
		: IEdMassActionType()
		, m_nodes( nodes )
	{
	}

	virtual wxString GetName() const
	{
		return wxT("Select");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context, m_nodes );
	}
};

// Mass action that shows or hides an entity
class CEdMassActionShowHideEntity : public IEdMassActionType
{
	THashSet< CEntity* >&		m_entities;	//<! Entities to show or hide
	Bool						m_hide;

	class Action : public IEdMassAction
	{
		wxPanel*				m_panel;
		THashSet< CEntity* >&	m_entities;
		Bool					m_hide;

		void CreatePanel( wxWindow* parent );

	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context, Bool hide, THashSet< CEntity* >& entities );

		virtual void ExportToXML( wxXmlNode* node ) override {}
		virtual void ImportFromXML( wxXmlNode* node ) override {}

		wxWindow* GetWindow( wxWindow* parent ) override;
		wxString GetDescription() const;

		bool Perform( IEdMassActionObject* object );
	};

public:
	CEdMassActionShowHideEntity( Bool hide, THashSet< CEntity* >& entities )
		: IEdMassActionType()
		, m_hide( hide )
		, m_entities( entities )
	{
	}

	virtual wxString GetName() const
	{
		return m_hide ? wxT("Hide the entity") : wxT("Reveal the entity");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context, m_hide, m_entities );
	}
};

// Mass action that asks the node to perform some node-specific action
class CEdMassActionNodeSpecific : public IEdMassActionType
{
	class Action : public IEdMassAction
	{
		wxPanel*	m_panel;
		wxTextCtrl*	m_text;

		void CreatePanel( wxWindow* parent );

		virtual void ExportToXML( wxXmlNode* node )
		{
			node->AddAttribute( wxT("actiontext"), m_text->GetValue() );
		}

		virtual void ImportFromXML( wxXmlNode* node )
		{
			m_text->SetValue( node->GetAttribute( wxT("actiontext") ) );
		}

	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: IEdMassAction( context )
			, m_panel( nullptr )
			, m_text( nullptr )
		{
			m_type = type;
		}

		virtual wxWindow* GetWindow( wxWindow* parent );
		virtual wxString GetDescription() const;

		bool Perform( IEdMassActionObject* object );
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Perform action");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

// Mass action that adds the node to the global scene filter
class CEdMassActionAddToSceneFilter : public IEdMassActionType
{
	class Action : public IEdMassAction
	{
	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: IEdMassAction( context )
		{
			m_type = type;
		}

		virtual void ExportToXML( wxXmlNode* node ) override {}
		virtual void ImportFromXML( wxXmlNode* node ) override {}
		wxWindow* GetWindow( wxWindow* parent ) override { return nullptr; }
		wxString GetDescription() const;

		bool Perform( IEdMassActionObject* object );
	};

public:
	virtual wxString GetName() const
	{
		return wxT("Add to scene tree filter");
	}

	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

// Base class for mass actions that modify scene entity lists
class CEdMassActionEntityListModifierBase : public IEdMassActionType
{
	// Those need to be implemented by the subclasses
	virtual Bool PerformAction( const wxString& listName, CEntity* entity )=0;
	virtual wxString GetDescription( const wxString& listName )=0;

	class Action : public IEdMassAction
	{
		wxPanel*								m_panel;
		wxTextCtrl*								m_listName;

		void CreatePanel( wxWindow* parent );

	public:
		Action( IEdMassActionType* type, CEdMassActionContext* context )
			: IEdMassAction( context )
			, m_panel( nullptr )
		{
			m_type = type;
		}

		virtual void ExportToXML( wxXmlNode* node ) override;
		virtual void ImportFromXML( wxXmlNode* node ) override;

		wxWindow* GetWindow( wxWindow* parent ) override;
		wxString GetDescription() const;

		bool Perform( IEdMassActionObject* object );
	};

public:
	virtual IEdMassAction* Create()
	{
		return new Action( this, m_context );
	}
};

// Mass action for adding an entity to an entity list
class CEdMassActionAddEntityToList : public  CEdMassActionEntityListModifierBase
{
	virtual Bool PerformAction( const wxString& listName, CEntity* entity ) override;
	virtual wxString GetDescription( const wxString& listName ) override;

public:
	virtual wxString GetName() const
	{
		return wxT("Add entity to list");
	}
};

// Mass action for removing an entity from an entity list
class CEdMassActionRemoveEntityFromList : public  CEdMassActionEntityListModifierBase
{
	virtual Bool PerformAction( const wxString& listName, CEntity* entity ) override;
	virtual wxString GetDescription( const wxString& listName ) override;

public:
	virtual wxString GetName() const
	{
		return wxT("Remove entity to list");
	}
};

// Mass action for clearing an entity list
class CEdMassActionClearEntityList : public  CEdMassActionEntityListModifierBase
{
	virtual Bool PerformAction( const wxString& listName, CEntity* entity ) override;
	virtual wxString GetDescription( const wxString& listName ) override;

public:
	virtual wxString GetName() const
	{
		return wxT("Clear entity list");
	}
};