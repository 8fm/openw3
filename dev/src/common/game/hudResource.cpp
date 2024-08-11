/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "hudResource.h"

#include "../core/factory.h"

//////////////////////////////////////////////////////////////////////////
// CHudModuleResourceBlock
//////////////////////////////////////////////////////////////////////////
CHudModuleResourceBlock::CHudModuleResourceBlock()
	: m_gameInputListener( false )
{
}

String CHudModuleResourceBlock::GetCaption() const 
{
	return m_moduleName;
}

//////////////////////////////////////////////////////////////////////////
// CHudResource
//////////////////////////////////////////////////////////////////////////
CHudResource::CHudResource()
{
}

#ifndef NO_RESOURCE_IMPORT
void CHudResource::ResaveHudFlashSwf( CHudResource* hudResource, const String& path )
{
	if ( hudResource )
	{
		hudResource->m_hudFlashSwf = TSoftHandle< CSwfResource >( path );
		hudResource->MarkModified();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// CHudResourceFactory
//////////////////////////////////////////////////////////////////////////
class CHudResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CHudResourceFactory, IFactory, 0 );

public:
	CHudResourceFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );	
};

CHudResourceFactory::CHudResourceFactory()
{
	m_resourceClass = ClassID< CHudResource >();
}

CResource* CHudResourceFactory::DoCreate( const FactoryOptions& options )
{
	CHudResource* hudResource = ::CreateObject< CHudResource >( options.m_parentObject );
	return hudResource;
}

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CHudModuleResourceBlock );
IMPLEMENT_ENGINE_CLASS( CHudResource );
DEFINE_SIMPLE_RTTI_CLASS( CHudResourceFactory, IFactory );
IMPLEMENT_ENGINE_CLASS( CHudResourceFactory );