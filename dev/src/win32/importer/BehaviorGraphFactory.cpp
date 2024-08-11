/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/engine/behaviorGraph.h"

class CResource;

/// Factory for animation set
class CBehaviorGraphFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphFactory, IFactory, 0 );

public:
	CBehaviorGraphFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CBehaviorGraphFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CBehaviorGraphFactory);

CBehaviorGraphFactory::CBehaviorGraphFactory()
{
	m_resourceClass = ClassID< CBehaviorGraph >();
}

CResource* CBehaviorGraphFactory::DoCreate( const FactoryOptions& options )
{
	CBehaviorGraph* graph = ::CreateObject< CBehaviorGraph >( options.m_parentObject );

	return graph;
}
