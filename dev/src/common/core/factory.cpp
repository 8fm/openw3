/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"

#include "factory.h"
#include "resource.h"

IMPLEMENT_ENGINE_CLASS( IFactory );

Bool IFactory::SupportsResource( CClass* resourceClass ) const
{
	return m_resourceClass == resourceClass;
}

String IFactory::GetFriendlyName() const
{
#ifndef NO_EDITOR
	return m_resourceClass->GetDefaultObject< CResource >()->GetFriendlyDescription();
#else
	return String::EMPTY;
#endif
}

IFactory* IFactory::FindFactory( CClass* resourceClass )
{
	// Request all factory classes
	TDynArray< CClass* > factoryClasses;
	SRTTI::GetInstance().EnumClasses( ClassID<IFactory>(), factoryClasses );

	// Linear search :P
	for ( Uint32 i=0; i<factoryClasses.Size(); i++ )
	{
		IFactory* factory = factoryClasses[i]->GetDefaultObject< IFactory >();
		if ( factory->SupportsResource( resourceClass ) )
		{
			return factory;
		}
	}

	// Format not supported
	return NULL;
}

void IFactory::EnumFactoryClasses( TDynArray< CClass* >& factoryClasses )
{
	// Request factory classes
	SRTTI::GetInstance().EnumClasses( ClassID<IFactory>(), factoryClasses );
}
