#pragma once

#include "behTreeEditedItem.h"

class CEdBehTreeEditor;
class CBehTreeEditedProperty;
class COpenTreePropertyEditor;

////////////////////////////////////////////////////////////////////////////////
class COpenTreePropertyEditor : public ICustomPropertyEditor, public wxEvtHandler
{
protected:
	wxBitmap			m_iconEdit;

	void OnEditRequest( wxCommandEvent &event );
	Bool IsEditing();
public:
	COpenTreePropertyEditor( CPropertyItem* propertyItem );
	~COpenTreePropertyEditor();

	//! ICustomPropertyEditor interface
	void CreateControls( const wxRect &propertyRect, TDynArray< wxControl* >& outSpawnedControls ) override;
	Bool GrabValue( String& displayValue ) override;

	// CBehTreeEditedProperty interface
	//void EditorClosed();
	//void EditorOpen();
	//IBehTreeNodeDefinition* GetRootNode();
	//void SetRootNode( IBehTreeNodeDefinition* node );
};



