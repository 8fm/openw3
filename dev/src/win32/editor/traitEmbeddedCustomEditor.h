#pragma once

class CEdTraitEmbeddedCustomEditor : public wxEvtHandler, public ICustomPropertyEditor
{
protected:
	wxBitmap			m_iconImport;
	wxBitmap			m_iconEdit;
	wxBitmap			m_iconClear;

	wxWindow*			m_embeddedResourceEditor;

public:
	CEdTraitEmbeddedCustomEditor( CPropertyItem* propertyItem );
	virtual ~CEdTraitEmbeddedCustomEditor();

	virtual Bool GrabValue( String& displayValue ) override;

	virtual Bool SaveValue() override;

	virtual void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;

public:
	void	OnResourceImport( wxCommandEvent& event );
	void	OnResourceEdit( wxCommandEvent& event );
	void	OnResourceClear( wxCommandEvent& event );

};