/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashPlayer.h"
#include "../../common/engine/flashValue.h"
#include "../../common/engine/flashValueStorage.h"

#include "../../common/game/guiObject.h"

#include "popupResource.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CPopupResource;
class CFlashMovie;
class CFlashMovieAdapter;
class CFlashSpriteAdapter;
class CGuiManager;
class CGuiScenePlayer;
class CFlashRenderTarget;
class CScriptedFlashObjectPool;

//////////////////////////////////////////////////////////////////////////
// CPopup
//////////////////////////////////////////////////////////////////////////
class CPopup
	: public CGuiObject
	, public CSoundEmitterOneShot
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CPopup, CGuiObject );

private:
	THandle< CGuiObject >			m_parentGuiObject;

private:
	TDynArray< CFlashRenderTarget* > m_flashRenderTargets;

private:
	CPopupResource*					m_popupResource;
	CFlashMovie*					m_flashMovie;
	CFlashMovieAdapter*				m_flashMovieAdapter;
	CFlashSpriteAdapter*			m_flashPopupAdapter;

private:
	CScriptedFlashObjectPool*				m_scriptedFlashObjectPool;
	THandle< CScriptedFlashSprite >			m_scriptedFlashSprite;
	THandle< CScriptedFlashValueStorage >	m_scriptedFlashValueStorage;

protected:
	THandle< CGuiManager >			m_guiManagerHandle;
	THandle< IScriptable >			m_popupScriptData;

private:
	CName							m_popupName;

public:			
	static CPopup*					CreateObjectFromResource( CPopupResource* popupResource, CObject* parent );

protected:
#ifndef RED_FINAL_BUILD
	//TBD: Have other panels override and return a more specific name
	virtual StringAnsi				GetSoundObjectName() const override { return "CPopup"; }
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
									CPopup();
	virtual Bool					Init( const CName& popupName, CGuiManager* guiManager );
	virtual Bool					InitWithFlashSprite( const CFlashValue& flashSprite ) override;
	virtual Bool					RequestSubPopup( const CName& popupName, const THandle< IScriptable >& scriptInitData ) { return false; }
	virtual	Bool					CanKeepHud() const { return false; }

public:
	RED_INLINE const CPopupResource* GetPopupResource() const { return m_popupResource; }
	RED_INLINE const CName&		GetPopupName() { return m_popupName; }
	RED_INLINE CFlashMovie*		GetFlashMovie() const { return m_flashMovie; }
	RED_INLINE CGuiObject*		GetParentGuiObject() const { return m_parentGuiObject.Get(); }
	RED_INLINE void				SetParentGuiObject( CGuiObject* object ) { m_parentGuiObject = object; }
	RED_INLINE void				SetPopupScriptData( const THandle< IScriptable >& data ) { m_popupScriptData = data; }

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
	void							funcGetPopupFlash( CScriptStackFrame& stack, void* result );
	void							funcGetPopupFlashValueStorage( CScriptStackFrame& stack, void* result );
	void							funcGetPopupInitData( CScriptStackFrame& stack, void* result );
	void							funcGetPopupName( CScriptStackFrame& stack, void* result );
	void							funcClosePopup( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CPopup );
PARENT_CLASS( CGuiObject );
	NATIVE_FUNCTION( "GetPopupFlash", funcGetPopupFlash );
	NATIVE_FUNCTION( "GetPopupFlashValueStorage", funcGetPopupFlashValueStorage );
	NATIVE_FUNCTION( "GetPopupInitData", funcGetPopupInitData );
	NATIVE_FUNCTION( "GetPopupName", funcGetPopupName );
	NATIVE_FUNCTION( "ClosePopup", funcClosePopup );
END_CLASS_RTTI();