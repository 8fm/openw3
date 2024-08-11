/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/engine/cubeTexture.h"

/// Factory for cubemaps instances
class CCubeTextureFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CCubeTextureFactory, IFactory, 0 );

public:
	CCubeTextureFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CCubeTextureFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CCubeTextureFactory);

CCubeTextureFactory::CCubeTextureFactory()
{
	m_resourceClass = ClassID< CCubeTexture >();
}

CResource* CCubeTextureFactory::DoCreate( const FactoryOptions& options )
{
	CCubeTexture* mat = ::CreateObject< CCubeTexture >( options.m_parentObject );
	return mat;
}
