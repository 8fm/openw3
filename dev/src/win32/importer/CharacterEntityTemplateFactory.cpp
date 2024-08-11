/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/engine/characterEntityTemplate.h"
#include "../../common/engine/entityTemplateParams.h"
#include "../../common/engine/entity.h"

/// Factory for character entity template instances
class CCharacterEntityTemplateFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CCharacterEntityTemplateFactory, IFactory, 0 );

public:
	CCharacterEntityTemplateFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CCharacterEntityTemplateFactory, IFactory);
IMPLEMENT_ENGINE_CLASS(CCharacterEntityTemplateFactory);

CCharacterEntityTemplateFactory::CCharacterEntityTemplateFactory()
{
	m_resourceClass = ClassID< CCharacterEntityTemplate >();
}

CResource* CCharacterEntityTemplateFactory::DoCreate( const FactoryOptions& options )
{
	CEntityTemplate* mat = ::CreateObject< CCharacterEntityTemplate >( options.m_parentObject );
	mat->SetEntityClass( ClassID< CEntity >() );
	return mat;
}
