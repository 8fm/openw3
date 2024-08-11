/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/localizedContent.h"
#include "../../common/game/dlcMounter.h"
#include "../../common/game/dlcDefinition.h"

class CDLCDefinitionFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CDLCDefinitionFactory, IFactory, 0 );

public:
	CDLCDefinitionFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CDLCDefinitionFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CDLCDefinitionFactory);

CDLCDefinitionFactory::CDLCDefinitionFactory()
{
	m_resourceClass = ClassID< CDLCDefinition >();
}

CResource* CDLCDefinitionFactory::DoCreate( const FactoryOptions& options )
{
	return ::CreateObject< CDLCDefinition >( options.m_parentObject );
}
