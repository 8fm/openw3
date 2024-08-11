#pragma once
#include "../../common/core/tokenizer.h"

class IEdMassActionType;
class CEdMassActionContext;
class CEdMassActionDialog;

class IEdMassActionConditionType;

class IEdMassActionObject
{
public:
	virtual ~IEdMassActionObject(){}
	virtual int						GetPropertyCount() const = 0;
	virtual int						FindPropertyByName( const CName& name ) const = 0;
	virtual CName					GetPropertyName( int n ) const = 0;
	virtual wxString				GetPropertyString( int n ) const = 0;
	virtual int						GetPropertyInteger( int n ) const = 0;
	virtual double					GetPropertyDouble( int n ) const = 0;
	virtual bool					SetPropertyString( int n, const wxString& value ) = 0;
	virtual bool					SetPropertyInteger( int n, int value ) = 0;
	virtual bool					SetPropertyDouble( int n, double value ) = 0;
	virtual CClass*					GetClass() const = 0;
};

class CEdMassActionRTTIProxy : public IEdMassActionObject
{
private:
	void* m_object;
	CClass* m_class;
	TDynArray< CProperty* > m_properties;

public:
	CEdMassActionRTTIProxy( CObject* object )
		: m_object( object )
	{
		m_class = object->GetClass();
		m_class->GetProperties( m_properties );
	}

	CEdMassActionRTTIProxy( void* object, CClass* cls )
		: m_object( object )
	{
		cls->GetProperties( m_properties );
		m_class = cls;
	}

	inline void* GetObject() const { return m_object; }

	virtual int GetPropertyCount() const
	{
		return m_properties.SizeInt();
	}

	virtual int FindPropertyByName( const CName& name ) const
	{
		for ( int i=0; i<m_properties.SizeInt(); ++i )
		{
			if ( GetPropertyName( i ) == name )
			{
				return i;
			}
		}
		return -1;
	}

	virtual CName GetPropertyName( int n ) const
	{
		return m_properties[n]->GetName();
	}

	virtual wxString GetPropertyString( int n ) const
	{
		void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, m_properties[n]->GetType()->GetSize() );
		String result;
		if ( !buffer )
		{
			return wxEmptyString;
		}
		m_properties[n]->GetType()->Construct( buffer );
		m_properties[n]->Get( m_object, buffer );
		if ( dynamic_cast<CRTTIArrayType*>( m_properties[n]->GetType() ) )
		{
			CRTTIArrayType* arrType = static_cast<CRTTIArrayType*>( m_properties[n]->GetType() );
			CBaseArray* arr = static_cast<CBaseArray*>( buffer );
			IRTTIType* innerType = arrType->GetInnerType();
			Uint32 innerSize = innerType->GetSize();
			Bool failure = false;
			//String result;
			for ( Uint32 i=0; i<arr->Size(); ++i )
			{
				void* srcBuffer = arrType->GetArrayElement( arr, i );
				String part;
				if ( !innerType->ToString( srcBuffer, part ) )
				{
					failure = true;
					break;
				}

				if ( i > 0 )
				{
					result += TXT(",");
				}
				result += part;
			}

			if ( failure )
			{
				m_properties[n]->GetType()->Destruct( buffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
				return wxEmptyString;
			}

			//return wxString( result.AsChar() );
		}
		else if ( dynamic_cast< CRTTIHandleType* >( m_properties[n]->GetType() ) && ( static_cast< CRTTIHandleType* >( m_properties[n]->GetType() ) )->GetPointedType()->IsBasedOn( ClassID< CResource >() ) )
		{
			//String result = String::EMPTY;
			BaseSafeHandle* resHandle = static_cast< BaseSafeHandle* >( buffer );
			CResource* res = Cast< CResource >( resHandle->Get() );
			if ( res && res->GetFile() )
			{
				result = res->GetFile()->GetDepotPath();
			}
			//return wxString( result.AsChar() );
		}
		else
		{
			if ( !m_properties[n]->GetType()->ToString( buffer, result ) )
			{
				m_properties[n]->GetType()->Destruct( buffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
				return wxEmptyString;
			}
			else if ( m_properties[n]->GetType()->IsPointerType() ) // special case for null pointer checking
			{
				const IRTTIPointerTypeBase* pointerType = static_cast< IRTTIPointerTypeBase* >( m_properties[n]->GetType() );
				const CPointer pointer = pointerType->GetPointer( buffer );
				result = pointer.IsNull() ? TXT("null") : String::EMPTY;
			}
		}
		m_properties[n]->GetType()->Destruct( buffer );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
		return wxString( result.AsChar() );
	}

	virtual int GetPropertyInteger( int n ) const
	{
		long v = 0;
		if ( !GetPropertyString( n ).ToLong( &v ) ) return 0;
		return v;
	}

	virtual double GetPropertyDouble( int n ) const
	{
		double v = 0;
		if ( !GetPropertyString( n ).ToDouble( &v ) ) return 0;
		return v;
	}

	virtual bool SetPropertyString( int n, const wxString& value )
	{
		if ( m_class->IsA< CObject >() && !static_cast< CObject* >( m_object )->MarkModified() )
		{
			return false;
		}

		void* buffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, m_properties[n]->GetType()->GetSize() );
		if ( !buffer )
		{
			return false;
		}
		
		m_properties[n]->GetType()->Construct( buffer );
		if ( dynamic_cast<CRTTIArrayType*>( m_properties[n]->GetType() ) )
		{
			CRTTIArrayType* arrType = static_cast<CRTTIArrayType*>( m_properties[n]->GetType() );
			CBaseArray* arr = static_cast<CBaseArray*>( buffer );
			CTokenizer tokenizer( String( value.wc_str() ), TXT(",") );
			IRTTIType* innerType = arrType->GetInnerType();
			Uint32 innerSize = innerType->GetSize();
			Bool failure = false;
			for ( Uint32 i=0; i<tokenizer.GetNumTokens(); ++i )
			{
				String token = tokenizer.GetToken( i );
				token.Trim();
				if ( token.Empty() ) // avoid empty tokens to allow more intuitive use of replace/append actions
				{
					continue;
				}

				void* subBuffer = RED_MEMORY_ALLOCATE( MemoryPool_Default, MC_Editor, innerSize );
				if ( !subBuffer )
				{
					failure = true;
					break;
				}

				innerType->Construct( subBuffer );
				if ( !innerType->FromString( subBuffer, token ) )
				{
					innerType->Destruct( subBuffer );
					RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, subBuffer );
					failure = true;
				}

				Int32 index = arrType->AddArrayElement( arr );
				void* tgtBuffer = arrType->GetArrayElement( arr, index );
				innerType->Construct( tgtBuffer );

				innerType->Copy( tgtBuffer, subBuffer );

				innerType->Destruct( subBuffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, subBuffer );
			}

			if ( failure )
			{
				m_properties[n]->GetType()->Destruct( buffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
				return false;
			}
		}
		else
		{
			if ( !m_properties[n]->GetType()->FromString( buffer, String( value.wc_str() ) ) )
			{
				m_properties[n]->GetType()->Destruct( buffer );
				RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
				return false;
			}
		}
		m_properties[n]->Set( m_object, buffer );
		m_properties[n]->GetType()->Destruct( buffer );
		RED_MEMORY_FREE( MemoryPool_Default, MC_Editor, buffer );
		return true;
	}

	virtual bool SetPropertyInteger( int n, int value )
	{
		return SetPropertyString( n, wxString::Format( "%d", value ) );
	}

	virtual bool SetPropertyDouble( int n, double value )
	{
		return SetPropertyString( n, wxString::FromDouble( value ) );
	}

	virtual CClass* GetClass() const
	{
		return m_class;
	}
};

class IEdMassAction
{
	friend class CEdMassActionContext;
	friend class CEdMassActionList;

protected:
	IEdMassActionType*				m_type;
	CEdMassActionContext*			m_context;

	virtual void					OnActionSelect( IEdMassAction* previousSelection ){}
	virtual void					OnActionDeselect( IEdMassAction* newSelection ){}

	virtual void					ExportToXML( wxXmlNode* node ) = 0;
	virtual void					ImportFromXML( wxXmlNode* node ) = 0;

public:
	IEdMassAction( CEdMassActionContext* context )
		: m_context( context )
	{}

	virtual ~IEdMassAction(){};

	inline IEdMassActionType*		GetType() const { return m_type; }
	inline CEdMassActionContext*	GetContext() const { return m_context; }

	virtual wxWindow*				GetWindow( wxWindow* parent ) = 0;
	virtual wxString				GetDescription() const = 0;
	virtual bool					Validate() { return true; }
	virtual bool					Perform( IEdMassActionObject* object ) = 0;
};

class CEdMassActionPropertyModifierAction : public IEdMassAction
{
protected:
	wxPanel* m_panel;
	wxChoice* m_propsChoice;
	int m_previousProperty;

	virtual void					CreatePanelWidgets() = 0;

	inline wxString GetSelectedProperty() const
	{
		return m_propsChoice->GetStringSelection();
	}

	inline wxString GetPropertyString( IEdMassActionObject* object )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return wxEmptyString;
		return object->GetPropertyString( index );
	}

	inline int GetPropertyInteger( IEdMassActionObject* object )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return 0;
		return object->GetPropertyInteger( index );
	}

	inline double GetPropertyDouble( IEdMassActionObject* object )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return 0;
		return object->GetPropertyDouble( index );
	}

	inline bool SetPropertyString( IEdMassActionObject* object, const wxString& value )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return false;
		return object->SetPropertyString( index, value );
	}

	inline bool SetPropertyInteger( IEdMassActionObject* object, const int value )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return false;
		return object->SetPropertyInteger( index, value );
	}

	inline bool SetPropertyDouble( IEdMassActionObject* object, const double value )
	{
		int index = object->FindPropertyByName( CName( m_propsChoice->GetStringSelection() ) );
		if ( index == -1 ) return false;
		return object->SetPropertyDouble( index, value );
	}

	void							FillProperties();
	void							CreatePanel( wxWindow* parent );
	void							AddPanelWidget( wxWindow* widget, int proportion = 0 );

	virtual void					OnActionSelect( IEdMassAction* previousSelection );

	virtual void					ExportToXML( wxXmlNode* node );
	virtual void					ImportFromXML( wxXmlNode* node );

public:
	CEdMassActionPropertyModifierAction( CEdMassActionContext* context );

	virtual wxWindow*				GetWindow( wxWindow* parent );
};

class IEdMassActionType
{
	friend class CEdMassActionContext;
protected:
	CEdMassActionContext*			m_context;

public:
	IEdMassActionType(){};
	virtual ~IEdMassActionType(){};

	virtual wxString				GetName() const = 0;
	virtual IEdMassAction*			Create() = 0;
};

class IEdMassActionCondition
{
	friend class CEdMassActionContext;
	friend class CEdMassActionConditionList;

protected:
	IEdMassActionConditionType*		m_type;
	CEdMassActionContext*			m_context;

	virtual void					OnConditionSelect( IEdMassActionCondition* previousSelection ){}
	virtual void					OnConditionDeselect( IEdMassActionCondition* newSelection ){}

	virtual void					ExportToXML( wxXmlNode* node ) = 0;
	virtual void					ImportFromXML( wxXmlNode* node ) = 0;

public:
	IEdMassActionCondition( CEdMassActionContext* context )
		: m_context( context )
	{}

	virtual ~IEdMassActionCondition(){};

	inline IEdMassActionConditionType* GetType() const { return m_type; }
	inline CEdMassActionContext*	GetContext() const { return m_context; }

	virtual wxWindow*				GetWindow( wxWindow* parent ) = 0;
	virtual wxString				GetDescription() const = 0;
	virtual bool					Validate() { return true; }
	virtual bool					Check( IEdMassActionObject* object ) = 0;
};

class IEdMassActionConditionType
{
	friend class CEdMassActionContext;
protected:
	CEdMassActionContext*			m_context;

public:
	IEdMassActionConditionType(){};
	virtual ~IEdMassActionConditionType(){};

	virtual wxString				GetName() const = 0;
	virtual IEdMassActionCondition*	Create() = 0;
};

class IEdMassActionConditionComparator
{
public:
	virtual wxString				GetName() const = 0;
	virtual bool					Validate( const wxString& v ) { return true; }
	virtual bool					Compare( const wxString& a, const wxString& b ) = 0;
};

class CEdMassActionNumberComparator : public IEdMassActionConditionComparator
{
public:
	virtual bool Validate( const wxString& v );
};

class IEdMassActionIterator
{
public:
	virtual void					Rewind() = 0;
	virtual bool					HasMore() = 0;
	virtual IEdMassActionObject*	Next() = 0;
};

class CEdMassActionArrayIterator : public IEdMassActionIterator
{
protected:
	IEdMassActionObject** m_objects;
	int								m_head;
	int								m_count;

public:
	CEdMassActionArrayIterator( IEdMassActionObject** objects, int count );

	virtual void Rewind();
	virtual bool HasMore();
	virtual IEdMassActionObject* Next();
};

enum EEdMassActionConditionUsage
{
	CEDMACU_ALWAYS_PASS,
	CEDMACU_NEED_ALL_CONDITIONS,
	CEDMACU_NEED_ANY_CONDITION,
	CEDMACU_NEED_NO_CONDITIONS
};

class CEdMassActionContext
{
	friend class CEdMassActionDialog;

protected:
	CEdMassActionDialog*						m_dialog;
	bool										m_searchMode;
	bool										m_importing;
	wxVector<IEdMassActionType*>				m_actionTypes;
	wxVector<IEdMassAction*>					m_actions;
	wxVector<IEdMassActionConditionType*>		m_conditionTypes;
	wxVector<IEdMassActionCondition*>			m_conditions;
	EEdMassActionConditionUsage					m_conditionUsage;
	wxVector<IEdMassActionConditionComparator*>	m_comparators;
	TDynArray<CName>							m_properties;
	IEdMassActionIterator*						m_iterator;
	wxVector<IEdMassActionObject*>*				m_processedObjects;
	String										m_defaultPath;

	virtual void UpdateObjectInformation();

public:
	CEdMassActionContext( IEdMassActionIterator* iterator = NULL, bool searchMode = false );
	virtual ~CEdMassActionContext();

	void							SetIterator( IEdMassActionIterator* iterator );
	inline IEdMassActionIterator*	GetIterator() const { return m_iterator; }

	void							SetSearchMode( bool searchMode );
	inline bool						GetSearchMode() const { return m_searchMode; }

	virtual void					RewindIterator();
	virtual bool					HasMoreObjects() const;
	virtual IEdMassActionObject*	GetNextObject();

	void							SetProcessedObjects( wxVector<IEdMassActionObject*>* processedObjects );

	int								GetPropertyCount() const;
	CName							GetPropertyName( int n ) const;

	void							AddActionType( IEdMassActionType* actionType );
	void							AddConditionType( IEdMassActionConditionType* conditionType );
	void							AddConditionComparator( IEdMassActionConditionComparator* comparator );

	void							AddCommonActionAndConditionTypes();
	void							AddPropertyModifierActionTypes();
	void							AddPropertyConditionTypes();

	void							AddAction( IEdMassAction* action );
	void							RemoveAction( IEdMassAction* action );
	void							ReplaceAction( IEdMassAction* oldAction, IEdMassAction* newAction );

	int								GetActionTypeCount() const;
	IEdMassActionType*				GetActionType( int n );
	int								GetActionCount() const;
	IEdMassAction*					GetAction( int n );

	void							AddCondition( IEdMassActionCondition* condition );
	void							RemoveCondition( IEdMassActionCondition* condition );
	void							ReplaceCondition( IEdMassActionCondition* oldCondition, IEdMassActionCondition* newCondition );

	int								GetConditionTypeCount() const;
	IEdMassActionConditionType*		GetConditionType( int n );
	int								GetConditionCount() const;
	IEdMassActionCondition*			GetCondition( int n );
	int								GetConditionComparatorCount() const;
	IEdMassActionConditionComparator* GetConditionComparator( int n );

	EEdMassActionConditionUsage		GetConditionUsage() const;
	void							SetConditionUsage( EEdMassActionConditionUsage usage );

	bool							ValidateActions();
	bool							ValidateConditions();

	bool							PerformActions();

	inline CEdMassActionDialog*		GetDialog() const { return m_dialog; }

	bool							Export( const wxString& filename );
	bool							Import( const wxString& filename );
};

//////////////////////////////////////////////////////////////////////////

class CEdMassActionDialog :
	public wxDialog
{
	friend class CEdMassActionContext;

private:
	void OnRadioButtonSelected( wxCommandEvent& event );
	void OnOkClicked( wxCommandEvent& event );
	void OnImportClicked( wxCommandEvent& event );
	void OnExportClicked( wxCommandEvent& event );

protected:
	CEdMassActionContext*			m_context;
	CEdWidgetItemList*				m_actionsWindow;
	CEdWidgetItemList*				m_conditionsWindow;
	wxRadioButton*					m_alwaysRadioButton;
	wxRadioButton*					m_onlyWhenRadioButton;
	wxChoice*						m_conditionsMatchingChoice;
	wxButton*						m_importButton;
	wxButton*						m_exportButton;
	wxButton*						m_okButton;
	wxButton*						m_cancelButton;

	virtual bool ShouldPerformActions();
	virtual bool PerformActions();
	virtual void UpdateEnabledStatus();
	virtual EEdMassActionConditionUsage GetConditionUsage();
	virtual void SetConditionUsage( EEdMassActionConditionUsage usage );

public:
	CEdMassActionDialog( wxWindow* parent, const wxString& title, CEdMassActionContext* context );
	virtual ~CEdMassActionDialog();

	virtual void					SetDefaults( const String& defaultPath, const String& defaultFilename = TXT("default.madesc"));

	inline CEdMassActionContext*	GetContext() const { return m_context; }
};