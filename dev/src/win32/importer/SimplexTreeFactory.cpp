/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/simplexTreeResource.h"
#include "../../common/core/factory.h"


class CSimplexTreeFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CSimplexTreeFactory, IFactory, 0 );

public:
	CSimplexTreeFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CSimplexTreeFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CSimplexTreeFactory);

CSimplexTreeFactory::CSimplexTreeFactory()
{
	m_resourceClass = ClassID< CResourceSimplexTree >();
}

CResource* CSimplexTreeFactory::DoCreate( const FactoryOptions& options )
{
	CResourceSimplexTree* mat = ::CreateObject< CResourceSimplexTree >( options.m_parentObject );

	SSimplexTreeStruct nod;
	nod.Set( 400.0f, 400.0f, 1.0f, 0.0f );
	mat->GetNodes().PushBack( nod );
	return mat;
}