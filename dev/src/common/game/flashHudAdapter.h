/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "flashSpriteAdapter.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashFunction;
class CFlashBindingAdapter;
class CFlashMovie;
class CHud;
class CHudModule;

//////////////////////////////////////////////////////////////////////////
// CFlashHudAdapter
//////////////////////////////////////////////////////////////////////////
class CFlashHudAdapter : public CFlashSpriteAdapter
{
private:
	typedef CFlashValue CFlashHudAdapter::*					TFlashFunctionImport;
	
private:
	struct SFlashFuncImportDesc
	{
		const Char*					m_memberName;
		TFlashFunctionImport		m_flashFuncImport;	
	};

private:
	static SFlashFuncImportDesc		sm_flashFunctionImportTable[];

private:
	CFlashValue						m_loadHudModuleHook;
	CFlashValue						m_unloadHudModuleHook;

public:
									CFlashHudAdapter( CFlashMovie* flashMovie, const CFlashValue& flashHud, CHud* hud );
	virtual							~CFlashHudAdapter();
	virtual Bool					Init();
	virtual void					OnDestroy() override;

public:
	Bool							LoadModuleAsync( const String& moduleName, Int32 userData = -1 );
	Bool							UnloadModuleAsync( const String& moduleName, Int32 userData = -1 );

private:
	Bool							HookFlashFunctions( CFlashValue& flashObject );
	void							UnhookFlashFunctions();
};
