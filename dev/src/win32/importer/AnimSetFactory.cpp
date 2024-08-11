/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/engine/skeletalAnimationSet.h"


/// Factory for animation set
class CAnimSetFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CAnimSetFactory, IFactory, 0 );

public:
	CAnimSetFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CAnimSetFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CAnimSetFactory);

CAnimSetFactory::CAnimSetFactory()
{
	m_resourceClass = ClassID< CSkeletalAnimationSet >();
}

CResource* CAnimSetFactory::DoCreate( const FactoryOptions& options )
{
	CSkeletalAnimationSet* mat = ::CreateObject< CSkeletalAnimationSet >( options.m_parentObject );
	return mat;
}
