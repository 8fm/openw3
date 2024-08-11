#pragma  once

//////////////////////////////////////////////////////////////////////////
class CAnimatedPropertyEditor
	: public wxEvtHandler
	, public ICustomPropertyEditor
{
	wxTextCtrl*			m_ctrlText;
	String				m_selectedPropertyName;

public:
	CAnimatedPropertyEditor( CPropertyItem* propertyItem );
	~CAnimatedPropertyEditor();

	// edit text box methods
	void OnEditTextEnter( wxCommandEvent& event );
	void OnEditKeyDown( wxKeyEvent& event );

	// ICustomPropertyEditor
	virtual void CreateControls( const wxRect& propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	virtual Bool GrabValue( String& displayValue ) override;
	virtual Bool SaveValue() override;
	virtual void CloseControls() override;

protected:
	void OnSelectPropertyEditorDialog( wxCommandEvent& event );
	RED_INLINE Bool IsModifiableProperty( CProperty* property );
};

class CSelectPropertyEditorDialog : public wxDialog
{
	DECLARE_EVENT_TABLE();

public:
	CSelectPropertyEditorDialog( wxWindow* parent, TDynArray<String>& properties, const String& selectedProp );
	~CSelectPropertyEditorDialog();

	// common
	const String& GetSelectedPropertyName();

protected:
	void OnOk( wxCommandEvent& event );
	void OnCancel( wxCommandEvent& event );
	void OnTreeItemActivated( wxTreeEvent& event );

protected:
	// props
	wxTreeCtrl*		m_treelist;
	String			m_selectedPropertyName;
};