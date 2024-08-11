/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CEdEffectEditorProperties : public CEdPropertiesPage
{
protected:
	CEntity* m_entity;

public:
	CEdEffectEditorProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager, CEntity* entity );

	//! Query interface
	virtual CEdEffectEditorProperties* QueryEffectEditorProperties() { return this; }

	//! Get edited entity
	CEntity* GetEntity() { return m_entity; }
};