#pragma once

#include "traitValidator.h"

class CGridEditor;
class CTraitData;

class CEdTraitEditor : public wxSmartLayoutPanel, public IEdEventListener
{
	DECLARE_EVENT_TABLE()

protected:
	wxNotebook* m_notebook;

	const CName m_traitTableProperty;
	const CName m_skillTableProperty;
		
	CGridEditor* m_traitTableGrid;
	CGridEditor* m_skillTableGrid;

	Bool m_invalidData;

	CTraitData *m_traitData;

	CTraitValidator m_dataValidator;

public:

	CEdTraitEditor( wxWindow* parent, CTraitData* traitData );
	~CEdTraitEditor();

	// ISavableToConfig interface
	virtual void SaveOptionsToConfig();
	virtual void LoadOptionsFromConfig();

	// IEdEventListener
	virtual void DispatchEditorEvent( const CName& name, IEdEventData* data );

protected:

	virtual void OnInternalIdle() override;

private:


	// Event handlers
	void OnSave( wxCommandEvent& event );
	void OnExit( wxCommandEvent& event );
	void OnValidate( wxCommandEvent& event );
	void OnGridValueChanged( wxCommandEvent& event );
	void OnPageChanged( wxCommandEvent& event );
	void OnMouseCaptureLost( wxMouseCaptureLostEvent& event ) {}

	// update new page with data from last page
	void UpdatePageData( Int32 index );

	// Other methods
	CGridEditor *CreateGridFromProperty( wxWindow *parent, CClass *classPtr, String propertyName );
	void SetGridObject( CGridEditor *grid, CProperty *prop );

};
