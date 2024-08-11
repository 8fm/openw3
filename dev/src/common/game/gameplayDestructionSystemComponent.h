/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/destructionSystemComponent.h"


class CGameplayDestructionSystemComponent : public CDestructionSystemComponent
{
	DECLARE_ENGINE_CLASS( CGameplayDestructionSystemComponent, CDestructionSystemComponent, 0 )
};

BEGIN_CLASS_RTTI( CGameplayDestructionSystemComponent )
	PARENT_CLASS( CDestructionSystemComponent )

END_CLASS_RTTI()