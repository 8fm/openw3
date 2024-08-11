/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiActionParameters.h"

class CAIDespawnParameters : public IAIActionParameters
{
	typedef IAIActionParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIDespawnParameters );
public:
	THandle< IAIActionTree >				m_despawnAction;
};

BEGIN_CLASS_RTTI( CAIDespawnParameters )
	PARENT_CLASS( IAIActionParameters )
	PROPERTY_INLINED( m_despawnAction, TXT("Action to be performed on despawn") )
END_CLASS_RTTI()

class CAIDespawnTree : public IAIActionTree
{
	typedef IAIActionTree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIDespawnTree );
protected:
	THandle< CAIDespawnParameters >			m_params;
public:
	Bool InitializeData() override;
};

BEGIN_CLASS_RTTI( CAIDespawnTree )
	PARENT_CLASS( IAIActionTree )
	PROPERTY_INLINED( m_params, TXT("Action to be performed on despawn") )
END_CLASS_RTTI()