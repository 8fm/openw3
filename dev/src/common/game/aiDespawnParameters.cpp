/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "aiDespawnParameters.h"

IMPLEMENT_ENGINE_CLASS( CAIDespawnParameters );
IMPLEMENT_ENGINE_CLASS( CAIDespawnTree );


Bool CAIDespawnTree::InitializeData()
{
	TBaseClass::InitializeData();

	m_params = CAIDespawnParameters::GetStaticClass()->CreateObject< CAIDespawnParameters >();
#ifndef NO_EDITOR
	m_params->OnCreatedInEditor();
#endif
	return true;
}
