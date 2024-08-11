/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashPlayer.h"
#include "../../common/engine/flashValue.h"
#include "../../common/engine/flashValueStorage.h"

#include "../../common/game/guiObject.h"

#include "menuResource.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CMenuResource;
class CFlashMovie;
class CFlashMovieAdapter;
class CFlashSpriteAdapter;
class CGuiManager;
class CGuiScenePlayer;
class CFlashRenderTarget;
class CScriptedFlashObjectPool;

//////////////////////////////////////////////////////////////////////////
// CMenu
//////////////////////////////////////////////////////////////////////////
class CMenu
	: public CGuiObject
	, public CSoundEmitterOneShot
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMenu, CGuiObject );

private:
	THandle< CGuiObject >			m_parentGuiObject;

private:
	TDynArray< CFlashRenderTarget* > m_flashRenderTargets;

private:
	CMenuResource*					m_menuResource;
	CFlashMovie*					m_flashMovie;
	CFlashMovieAdapter*				m_flashMovieAdapter;
	CFlashSpriteAdapter*			m_flashMenuAdapter;

private:
	CScriptedFlashObjectPool*				m_scriptedFlashObjectPool;
	THandle< CScriptedFlashSprite >			m_scriptedFlashSprite;
	THandle< CScriptedFlashValueStorage >	m_scriptedFlashValueStorage;

protected:
	THandle< CGuiManager >			m_guiManagerHandle;
	THandle< IScriptable >			m_menuScriptData;

private:
	CName							m_menuName;

public:			
	static CMenu*					CreateObjectFromResource( CMenuResource* menuResource, CObject* parent );

protected:
#ifndef RED_FINAL_BUILD
	//TBD: Have other panels override and return a more specific name
	virtual StringAnsi				GetSoundObjectName() const override { return "CMenu"; }
#endif

public:
	//! CObject functions
	virtual void					OnFinalize() override;
	virtual void					OnSerialize( IFile& file ) override;

public:
	//! CGuiObject functions
	virtual Bool					RegisterRenderTarget( const String& targetName, Uint32 width, Uint32 height ) override;
	virtual Bool					UnregisterRenderTarget( const String& targetName ) override;

public:
									CMenu();
	virtual Bool					Init( const CName& menuName, CGuiManager* guiManager );
	virtual Bool					InitWithFlashSprite( const CFlashValue& flashSprite ) override;
	virtual Bool					RequestSubMenu( const CName& menuName, const THandle< IScriptable >& scriptInitData ) { return false; }
	virtual	Bool					CanKeepHud() const { return false; }

public:
	RED_INLINE const CMenuResource* GetMenuResource() const { return m_menuResource; }
	RED_INLINE const CName&		GetMenuName() const { return m_menuName; }
	RED_INLINE CFlashMovie*		GetFlashMovie() const { return m_flashMovie; }
	RED_INLINE CGuiObject*		GetParentGuiObject() const { return m_parentGuiObject.Get(); }
	RED_INLINE void				SetParentGuiObject( CGuiObject* object ) { m_parentGuiObject = object; }
	RED_INLINE void				SetMenuScriptData( const THandle< IScriptable >& data ) { m_menuScriptData = data; }

public:
	void							SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet );
	void							ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear );

	void							SetIgnoreKey( EInputKey keyToSet );
	void							ClearIgnoreKey( EInputKey keyToClear );
	void							ClearAllIgnoreKeys();

public:
	virtual Bool					RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;
	virtual Bool					UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;

public:
	virtual void					Tick( Float timeDelta ) override;

private:
	void							Cleanup();

private:
	void							funcGetMenuFlash( CScriptStackFrame& stack, void* result );
	void							funcGetMenuFlashValueStorage( CScriptStackFrame& stack, void* result );
	void							funcGetMenuInitData( CScriptStackFrame& stack, void* result );
	void							funcGetMenuName( CScriptStackFrame& stack, void* result );
	void							funcRequestSubMenu( CScriptStackFrame& stack, void* result );
	void							funcCloseMenu( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CMenu );
PARENT_CLASS( CGuiObject );
	NATIVE_FUNCTION( "GetMenuFlash", funcGetMenuFlash );
	NATIVE_FUNCTION( "GetMenuFlashValueStorage", funcGetMenuFlashValueStorage );
	NATIVE_FUNCTION( "GetMenuInitData", funcGetMenuInitData );
	NATIVE_FUNCTION( "GetMenuName", funcGetMenuName );
	NATIVE_FUNCTION( "RequestSubMenu", funcRequestSubMenu );
	NATIVE_FUNCTION( "CloseMenu", funcCloseMenu );
END_CLASS_RTTI();