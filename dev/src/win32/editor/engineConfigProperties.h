/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "propertiesPage.h"

#include "../../common/core/configVar.h"
#include "../../common/core/configVarRegistry.h"
#include "../../common/core/configVarHierarchy.h"

/// Property grid that is used to display configuration
class CEdConfigPropertiesPage : public CEdPropertiesPage
{
public:
	CEdConfigPropertiesPage( wxWindow* parent, CEdUndoManager* undoManager );
	~CEdConfigPropertiesPage();

	void Fill();

private:
	// storage for the config groups
	Config::CConfigVarHierarchy		m_configGroup;
};

