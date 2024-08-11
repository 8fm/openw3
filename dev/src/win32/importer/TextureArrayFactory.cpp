/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/engine/textureArray.h"


/// Factory for texture array instances
class CTextureArrayFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CTextureArrayFactory, IFactory, 0 );

public:
	CTextureArrayFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CTextureArrayFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CTextureArrayFactory);

CTextureArrayFactory::CTextureArrayFactory()
{
	m_resourceClass = ClassID< CTextureArray >();
}

CResource* CTextureArrayFactory::DoCreate( const FactoryOptions& options )
{
	CTextureArray::FactoryInfo factoryInfo;
	CTextureArray* texture = CTextureArray::Create( factoryInfo );
	return texture;
}
