/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/core/scopedPtr.h"

class CPropertyItem;

/// Custom property editor for property item
class ICustomPropertyEditor
{
protected:
	CPropertyItem*		m_propertyItem;		//!< Connected property item

public:
	ICustomPropertyEditor( CPropertyItem* propertyItem ) : m_propertyItem( propertyItem ) {};
	virtual ~ICustomPropertyEditor() {};

	//! Property item got selected and should spawn its controls.
	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) = 0;

	//! Close property item editors
	virtual void CloseControls() {};

	//! Draw property, return true if you handle this message
	virtual Bool DrawValue( wxDC& dc, const wxRect &valueRect, const wxColour& textColour ) { return false; }

	//! Save property value, return true if you process this message
	virtual Bool SaveValue() { return false; }

	//! Grab property value, return true if you process this message
	virtual Bool GrabValue( String& displayValue ) { return false; }

	CPropertyItem* GetPropertyItem() const { return m_propertyItem; }
};

class CPropertyTransaction;

class CPropertyItem : public CBasePropItem, public IEdTextEditorHook
{
public:
	CPropertyItem( CEdPropertiesPage* page, CBasePropItem* parent );
	~CPropertyItem();

	virtual void Init( IProperty* prop, Int32 arrayIndex = -1 );
	virtual void Init( IRTTIType *type, Int32 arrayIndex = -1 );

	virtual Int32 GetIndent() const override;
	virtual Int32 GetHeight() const override;
	virtual Int32 GetLocalIndent() const override;
	virtual String GetCaption() const override;
	virtual Bool IsReadOnly() const override;
	virtual Bool IsInlined() const override;
	virtual String GetName() const override;
	virtual String GetCustomEditorType() const override;
	virtual const SEdPropertiesPagePropertyStyle* GetPropertyStyle() const override;
	virtual STypedObject GetParentObject( Int32 objectIndex ) const override;
    virtual Bool CanUseSelectedResource() const override;
	virtual void Expand() override;

	CProperty* GetProperty() const { return m_property; }

	IRTTIType* GetPropertyType() const { return m_propertyType; }

	Int32 GetArrayIndex() const { return m_arrayIndex; }

	void SetCaption( const String &caption ) { m_caption = caption; } 

	// copy value from property to control
	void GrabPropertyValue();

	// copy value from control/m_displayValue to property
	void SavePropertyValue( Bool readFromCtrls = true );

	// read property operated by this property item (simple as that :)
	Bool Read( void *buffer, Int32 objectIndex = 0 );
	Bool Write( void *buffer, Int32 objectIndex = 0 );

	void OnEditKeyDown( wxKeyEvent& event );		// TODO: make this private!
	void OnChoiceSelected( wxCommandEvent& event ); // TODO: make this private!

protected:
	IRTTIType*			m_propertyType;
	String				m_displayValue;
	Bool				m_isDetermined;
	wxTextCtrl*			m_ctrlText;
	CEdTextEditor*		m_textEditor;
	CEdChoice*			m_ctrlChoice;		// TODO: Move this to CPropertyItemEnum
	CEdColorPicker*		m_ctrlColorPicker;	// TODO: Move this to color picker editor

protected:
	// overrides from CBasePropItem
	virtual void DrawLayout( wxDC& dc ) override;
	virtual void CreateControls() override;
	virtual void CloseControls() override;
	virtual Bool ReadImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
	virtual Bool WriteImp( CPropertyItem* childItem, void *buffer, Int32 objectIndex = 0 ) override;
    virtual Bool SerializeXML( IXMLFile& file ) override;
	virtual void OnBrowserMouseEvent( wxMouseEvent& event ) override;
	virtual void OnBrowserKeyDown( wxKeyEvent& event ) override;

	// newly introduced virtuals
	virtual void CreateMainControl();
	virtual void DrawValue( wxDC &dc, const wxRect& valueRect, const wxColour& textColor );

	wxRect CalcValueRect() const;
	wxRect CalcCheckBoxRect() const;

	void FinishTransaction();

private:
	CProperty*			m_property;
	Int32				m_arrayIndex;
    String              m_initialValue;
    Bool				m_isDefaultValue;
	Bool				m_isFocused;
	String				m_caption;

    Red::TScopedPtr< CPropertyTransaction > m_transaction;
	ICustomPropertyEditor*	m_customEditor;

	Bool				m_mouseMove;
	wxPoint				m_mouseStart;
	String				m_mouseValue;

	THandle< CObject >	m_grabFrom;
	CProperty*			m_grabProperty;

private:
	void Init();
	Bool CreateCustomEditor();	
	void CloseEditors();
	void CyclePropertyValue();

	// IEdTextEditorHook implementation
	virtual void OnTextEditorModified( CEdTextEditor* editor ) override;
	virtual void OnTextEditorClosed( CEdTextEditor* editor ) override;

	void OnEditTextEnter( wxCommandEvent& event );
	void OnArrayDeleteItem( wxCommandEvent& event );
	void OnArrayInsertItem( wxCommandEvent& event );
	void OnGrabProperty( wxCommandEvent& event );
	void OnEditTextMotion( wxCommandEvent& event );
	void OnMouseMoveValueChange();
	void OnShowTextEditor( wxCommandEvent& event );
	void OnColorOpenBrowser( wxCommandEvent& event );
	void OnColorPicked( wxCommandEvent& event );
	void OnEditorKillFocus( wxFocusEvent& event );
};
