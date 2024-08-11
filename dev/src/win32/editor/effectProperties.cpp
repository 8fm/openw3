/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityPreviewPanel.h"
#include "effectProperties.h"

CEdEffectEditorProperties::CEdEffectEditorProperties( wxWindow* parent, const PropertiesPageSettings& settings, CEdUndoManager* undoManager, CEntity* entity )
	: CEdPropertiesPage( parent, settings, undoManager )
	, m_entity( entity )
{
}

