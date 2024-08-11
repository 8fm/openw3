/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/core/factory.h"
#include "../../common/core/2darray.h"


/// Factory for 2d array
class C2dArrayFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( C2dArrayFactory, IFactory, 0 );

public:
	C2dArrayFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );	
};

DEFINE_SIMPLE_RTTI_CLASS(C2dArrayFactory,IFactory);
IMPLEMENT_ENGINE_CLASS(C2dArrayFactory);

C2dArrayFactory::C2dArrayFactory()
{
	m_resourceClass = ClassID< C2dArray >();
}

CResource* C2dArrayFactory::DoCreate( const FactoryOptions& options )
{
	C2dArray* arr = ::CreateObject< C2dArray >( options.m_parentObject );

	arr->AddColumn(TXT("Index"), String::EMPTY);
	arr->AddRow();

	return arr;
}