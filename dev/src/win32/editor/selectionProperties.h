/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Properties browser that automatically shows properties of the selected objects
class CEdSelectionProperties : public CEdPropertiesBrowserWithStatusbar, public IEdEventListener
{
    CWorld* m_world;

public:
    CEdSelectionProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager, CWorld *world = NULL );
	~CEdSelectionProperties();

    void SetWorld( CWorld *world );
	void SetEntityEditorAsOwner(); // properties page is created by entity editor

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
};