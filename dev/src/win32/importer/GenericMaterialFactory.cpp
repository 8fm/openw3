/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/engine/materialGraph.h"
#include "../../common/engine/material.h"

/// Factory for generic materials
class CGenericMaterialFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CGenericMaterialFactory, IFactory, 0 );

public:
	CGenericMaterialFactory();
	virtual CResource* DoCreate( const FactoryOptions& options );
};

DEFINE_SIMPLE_RTTI_CLASS(CGenericMaterialFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(CGenericMaterialFactory);

CGenericMaterialFactory::CGenericMaterialFactory()
{
	m_resourceClass = ClassID< CMaterialGraph >();
}

CResource* CGenericMaterialFactory::DoCreate( const FactoryOptions& options )
{
	CMaterialGraph* mat = ::CreateObject< CMaterialGraph >( options.m_parentObject );
	if ( mat )
	{
#ifndef NO_RUNTIME_MATERIAL_COMPILATION
		mat->UpdateParametersList();
#endif
		mat->ForceRecompilation();
	}
	return mat;
}
