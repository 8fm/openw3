/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/game/guiResource.h"

class CHudModuleResourceBlock;
class CHud;

//////////////////////////////////////////////////////////////////////////
// CHudModuleResourceBlock
//////////////////////////////////////////////////////////////////////////
class CHudModuleResourceBlock : public IGuiResourceBlock
{
	DECLARE_ENGINE_CLASS( CHudModuleResourceBlock, IGuiResourceBlock, 0 );

private:
	String								m_moduleName;
	CName								m_moduleClass;
	Bool								m_gameInputListener;

public:
	RED_INLINE const String&			GetModuleName() const { return m_moduleName; }
	RED_INLINE const CName&			GetModuleClass() const { return m_moduleClass; }
	RED_INLINE Bool					IsGameInputListener() const { return m_gameInputListener; }

public:
	//! CGraphBlock functions
	virtual String GetCaption() const override;

public:
	CHudModuleResourceBlock();
};

class CHudResource : public IGuiResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CHudResource, IGuiResource, "hud", "HUD Resource" );

private:
	CName							m_hudClass;
	TSoftHandle< CSwfResource >		m_hudFlashSwf;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CClass*					GetResourceBlockClass() const override { return ClassID< CHudModuleResourceBlock >(); }
#endif

public:
	RED_INLINE const TSoftHandle< CSwfResource >& GetHudFlashSwf() const { return m_hudFlashSwf; }
	RED_INLINE const CName& GetHudClass() const { return m_hudClass; }

public:
	CHudResource();

#ifndef NO_RESOURCE_IMPORT
	static void ResaveHudFlashSwf( CHudResource* hudResource, const String& path );
#endif
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CHudModuleResourceBlock );
PARENT_CLASS( IGuiResourceBlock );
	PROPERTY_EDIT( m_moduleName, TXT("Module name") );
	PROPERTY_CUSTOM_EDIT( m_moduleClass, TXT("Module class object template"), TXT("HudModuleClassList") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CHudResource );
PARENT_CLASS( IGuiResource );
	PROPERTY_CUSTOM_EDIT( m_hudClass, TXT("HUD class"), TXT("HudClassList") );
	PROPERTY_EDIT( m_hudFlashSwf, TXT("Root Flash SWF for the HUD that HUD modules are loaded into") );
END_CLASS_RTTI();
