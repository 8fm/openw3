/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Properties browser that automatically shows properties of the loaded world
class CEdWorldProperties : public CEdPropertiesBrowserWithStatusbar, public IEdEventListener
{
public:
    CEdWorldProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager );
	~CEdWorldProperties();

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );
};