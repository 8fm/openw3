/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Properties browser that automatically shows properties of the active world environment
class CEdEnvironmentProperties : 
	public CEdPropertiesBrowserWithStatusbar, 
	public IEdEventListener
{
public:
	CEdEnvironmentProperties( CEntity* entity, wxWindow* parent, CEdUndoManager* undoManager );
	~CEdEnvironmentProperties();

protected:
	void DispatchEditorEvent( const CName& name, IEdEventData* data );

protected:
	CEntity *m_pEntity;
};