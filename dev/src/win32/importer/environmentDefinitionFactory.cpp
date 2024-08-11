/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../common/core/factory.h"
#include "../../common/engine/environmentDefinition.h"

/// Factory for environment definitions
class CEnvironmentDefinitionFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CEnvironmentDefinitionFactory, IFactory, 0 );

public:
	CEnvironmentDefinitionFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CEnvironmentDefinitionFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CEnvironmentDefinitionFactory);

CEnvironmentDefinitionFactory::CEnvironmentDefinitionFactory()
{
	m_resourceClass = ClassID< CEnvironmentDefinition >();
}

CResource* CEnvironmentDefinitionFactory::DoCreate( const FactoryOptions& options )
{
	CEnvironmentDefinition::FactoryInfo factoryInfo;
	CEnvironmentDefinition* envDef = CEnvironmentDefinition::Create( factoryInfo );
	return envDef;
}
