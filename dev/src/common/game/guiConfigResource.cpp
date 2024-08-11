/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "guiConfigResource.h"

#include "../core/factory.h"

//////////////////////////////////////////////////////////////////////////
// CGuiConfigResourceFactory
//////////////////////////////////////////////////////////////////////////
class CGuiConfigResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CGuiConfigResourceFactory, IFactory, 0 );

public:
	CGuiConfigResourceFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );	
};

CGuiConfigResourceFactory::CGuiConfigResourceFactory()
{
	m_resourceClass = ClassID< CGuiConfigResource >();
}

CResource* CGuiConfigResourceFactory::DoCreate( const FactoryOptions& options )
{
	CGuiConfigResource* hudResource = ::CreateObject< CGuiConfigResource >( options.m_parentObject );
	return hudResource;
}

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( SHudDescription );
IMPLEMENT_ENGINE_CLASS( SMenuDescription );
IMPLEMENT_ENGINE_CLASS( SPopupDescription );
IMPLEMENT_ENGINE_CLASS( SGuiSceneDescription );
IMPLEMENT_ENGINE_CLASS( CGuiConfigResource );
DEFINE_SIMPLE_RTTI_CLASS( CGuiConfigResourceFactory, IFactory );
IMPLEMENT_ENGINE_CLASS( CGuiConfigResourceFactory );