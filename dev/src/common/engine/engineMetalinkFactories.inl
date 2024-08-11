/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibMetalinkComponent.h"
#include "stripeComponent.h"

// pathlib part (in case of project split)
PathLib::CMetalinkSetupFactory g_metalinkSetupFactory;

// engine part
// engine part
class CGenericMetalinkSetupFactory : public PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< PathLib::CGenericMetalinkSetup >
{
	typedef PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< PathLib::CGenericMetalinkSetup > Super;
public:
	CGenericMetalinkSetupFactory()
		: Super( ClassFactoryId( EEngineMetalinkType::T_GENERIC ), ClassFactoryId( EEngineMetalinkType::T_REAL_COUNT ) )		{}
} g_genericMetalinkComponentFactory;


class CStripeComponentSetupFactory : public PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< CStripeComponentSetup >
{
	typedef PathLib::CMetalinkSetupFactory::TClassFactory_GlobalObject< CStripeComponentSetup > Super;
public:
	CStripeComponentSetupFactory()
		: Super( ClassFactoryId( EEngineMetalinkType::T_STRIPE ), ClassFactoryId( EEngineMetalinkType::T_REAL_COUNT ) )		{}
} g_stripeComponentFactory;

