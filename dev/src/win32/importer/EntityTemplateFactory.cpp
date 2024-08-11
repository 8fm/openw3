/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/entityTemplateParams.h"
#include "../../common/engine/entity.h"

/// Factory for entity templates instances
class CEntityTemplateFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CEntityTemplateFactory, IFactory, 0 );

public:
	CEntityTemplateFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CEntityTemplateFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CEntityTemplateFactory);

CEntityTemplateFactory::CEntityTemplateFactory()
{
	m_resourceClass = ClassID< CEntityTemplate >();
}

CResource* CEntityTemplateFactory::DoCreate( const FactoryOptions& options )
{
	CEntityTemplate* mat = ::CreateObject< CEntityTemplate >( options.m_parentObject );
	mat->SetEntityClass( ClassID< CEntity >() );
	return mat;
}
