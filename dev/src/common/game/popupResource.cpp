/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "popupResource.h"
#include "popup.h"

#include "../core/factory.h"

//////////////////////////////////////////////////////////////////////////
// CPopupDef
//////////////////////////////////////////////////////////////////////////
CPopupDef::CPopupDef()
{
	m_timeParam = ::CreateObject< CPopupPauseParam >( this );
}

//////////////////////////////////////////////////////////////////////////
// CPopupResource
//////////////////////////////////////////////////////////////////////////
CPopupResource::CPopupResource()
	: m_layer( 0 )
{
	m_popupDef = ::CreateObject< CPopupDef >( this );
}

#ifndef NO_RESOURCE_IMPORT
void CPopupResource::ResavePopupFlashSwf( CPopupResource* popupResource, const String& path )
{
	if ( popupResource )
	{
		popupResource->m_popupFlashSwf = TSoftHandle< CSwfResource >( path );
		popupResource->MarkModified();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// CPopupResourceFactory
//////////////////////////////////////////////////////////////////////////
class CPopupResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CPopupResourceFactory, IFactory, 0 );

public:
	CPopupResourceFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );	
};

CPopupResourceFactory::CPopupResourceFactory()
{
	m_resourceClass = ClassID< CPopupResource >();
}

CResource* CPopupResourceFactory::DoCreate( const FactoryOptions& options )
{
	CPopupResource* popupResource = ::CreateObject< CPopupResource >( options.m_parentObject );
	return popupResource;
}

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_RTTI_ENUM( EPopupPauseType );
IMPLEMENT_ENGINE_CLASS( IPopupTimeParam );
IMPLEMENT_ENGINE_CLASS( CPopupTimeScaleParam );
IMPLEMENT_ENGINE_CLASS( CPopupPauseParam );
IMPLEMENT_ENGINE_CLASS( CPopupDef );
IMPLEMENT_ENGINE_CLASS( CPopupResource );
DEFINE_SIMPLE_RTTI_CLASS( CPopupResourceFactory, IFactory );
IMPLEMENT_ENGINE_CLASS( CPopupResourceFactory );
