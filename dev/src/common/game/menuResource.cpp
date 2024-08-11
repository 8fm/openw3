/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#include "build.h"

#include "menuResource.h"
#include "menu.h"

#include "../core/factory.h"

//////////////////////////////////////////////////////////////////////////
// CMenuDef
//////////////////////////////////////////////////////////////////////////
CMenuDef::CMenuDef()
{
	m_timeParam = ::CreateObject< CMenuPauseParam >( this );
	m_backgroundVideoParam = ::CreateObject< CMenuInheritBackgroundVideoParam >( this );
	m_renderParam = ::CreateObject< CMenuRenderBackgroundParam >( this );
	m_flashParam = ::CreateObject< CMenuFlashSwfParam >( this );
}

//////////////////////////////////////////////////////////////////////////
// CMenuResource
//////////////////////////////////////////////////////////////////////////
CMenuResource::CMenuResource()
	: m_layer( 0 )
{
	m_menuDef = ::CreateObject< CMenuDef >( this );
}

#ifndef NO_RESOURCE_IMPORT
void CMenuResource::ResaveMenuFlashSwf( CMenuResource* menuResource, const String& path )
{
	if ( menuResource )
	{
		menuResource->m_menuFlashSwf = TSoftHandle< CSwfResource >( path );
		menuResource->MarkModified();
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// CMenuResourceFactory
//////////////////////////////////////////////////////////////////////////
class CMenuResourceFactory : public IFactory
{
	DECLARE_ENGINE_CLASS( CMenuResourceFactory, IFactory, 0 );

public:
	CMenuResourceFactory();

	virtual CResource* DoCreate( const FactoryOptions& options );	
};

CMenuResourceFactory::CMenuResourceFactory()
{
	m_resourceClass = ClassID< CMenuResource >();
}

CResource* CMenuResourceFactory::DoCreate( const FactoryOptions& options )
{
	CMenuResource* menuResource = ::CreateObject< CMenuResource >( options.m_parentObject );
	return menuResource;
}

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
IMPLEMENT_RTTI_ENUM( EMenuPauseType );
IMPLEMENT_ENGINE_CLASS( IMenuTimeParam );
IMPLEMENT_ENGINE_CLASS( IMenuDisplayParam );
IMPLEMENT_ENGINE_CLASS( IMenuFlashParam );
IMPLEMENT_ENGINE_CLASS( IMenuBackgroundVideoParam );
IMPLEMENT_ENGINE_CLASS( CMenuInheritBackgroundVideoParam );
IMPLEMENT_ENGINE_CLASS( CMenuBackgroundVideoFileParam );
IMPLEMENT_ENGINE_CLASS( CMenuBackgroundVideoAliasParam );
IMPLEMENT_ENGINE_CLASS( CMenuClearBackgroundVideoParam );
IMPLEMENT_ENGINE_CLASS( CMenuTimeScaleParam );
IMPLEMENT_ENGINE_CLASS( CMenuPauseParam );
IMPLEMENT_ENGINE_CLASS( CMenuRenderBackgroundParam );
IMPLEMENT_ENGINE_CLASS( CMenuBackgroundEffectParam );
IMPLEMENT_ENGINE_CLASS( CMenuHudParam );
IMPLEMENT_ENGINE_CLASS( CMenuFlashSymbolParam );
IMPLEMENT_ENGINE_CLASS( CMenuFlashSwfParam );
IMPLEMENT_ENGINE_CLASS( CMenuDef );
IMPLEMENT_ENGINE_CLASS( CMenuResource );
DEFINE_SIMPLE_RTTI_CLASS( CMenuResourceFactory, IFactory );
IMPLEMENT_ENGINE_CLASS( CMenuResourceFactory );
