/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/engineMetalinkFactories.inl"

#include "metalinkComponent.h"


class CMetalinkComponentSetupFactory : public PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< CMetalinkComponentNavigationSetup >
{
	typedef PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< CMetalinkComponentNavigationSetup > Super;
public:
	CMetalinkComponentSetupFactory()
		: Super( ClassFactoryId( EGameMetalinkType::T_METALINK ), ClassFactoryId( EGameMetalinkType::T_COUNT ) )			{}
} g_MetalinkComponentsSetupFactory;