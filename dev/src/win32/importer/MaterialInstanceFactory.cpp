/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/materialInstance.h"

/// Factory for material instances
class CMaterialInstanceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CMaterialInstanceFactory, IFactory, 0 );

public:
	CMaterialInstanceFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CMaterialInstanceFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CMaterialInstanceFactory);

CMaterialInstanceFactory::CMaterialInstanceFactory()
{
	m_resourceClass = ClassID< CMaterialInstance >();
}

CResource* CMaterialInstanceFactory::DoCreate( const FactoryOptions& options )
{
	CMaterialInstance* mat = ::CreateObject< CMaterialInstance >( options.m_parentObject );
	return mat;
}
