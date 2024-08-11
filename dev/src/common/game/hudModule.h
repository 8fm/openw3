/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashPlayer.h"
#include "../../common/game/guiObject.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CHud;
class CFlashHudModuleAdapter;
class CFlashSpriteAdapter;
class CScriptedFlashObjectPool;

//////////////////////////////////////////////////////////////////////////
// SHudModuleInitParams
//////////////////////////////////////////////////////////////////////////
struct SHudModuleInitParams
{
	String	m_moduleName;
	Bool	m_gameInputListener;

	SHudModuleInitParams( const String& moduleName, Bool gameInputListener )
		: m_moduleName( moduleName )
		, m_gameInputListener( gameInputListener )
	{
	}
};

//////////////////////////////////////////////////////////////////////////
// CHudModule
//////////////////////////////////////////////////////////////////////////
class CHudModule : public CGuiObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CHudModule, CGuiObject );

private:
	String								m_moduleName;

private:
	THandle< CHud >						m_hudHandle;

private:
	CFlashSpriteAdapter*				m_flashHudModuleAdapter;

private:
	CScriptedFlashObjectPool*				m_scriptedFlashObjectPool;
	THandle< CScriptedFlashSprite >			m_scriptedFlashSprite;
	THandle< CScriptedFlashValueStorage >	m_scriptedFlashValueStorage;

public:
	Bool								Init( CHud* hud, const SHudModuleInitParams& initParams );

public:
	virtual Bool						InitWithFlashSprite( const CFlashValue& flashSprite ) override;
	virtual Bool						IsInitWithFlashSprite() const override;

public:
	virtual Bool						RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;
	virtual Bool						UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;

public:
	virtual void						Tick( Float timeDelta ) override;

public:
	//! CObject functions
	virtual void						OnFinalize() override;
	virtual void						OnSerialize( IFile& file ) override;

public:
	RED_INLINE const String&			GetModuleName() const { return m_moduleName; }
	RED_INLINE THandle< CScriptedFlashSprite > GetFlashSprite() { return m_scriptedFlashSprite; }

public:
										CHudModule();

private:
	void								Cleanup();

private:
	void								funcGetModuleFlash( CScriptStackFrame& stack, void* result );
	void								funcGetModuleFlashValueStorage( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CHudModule );
PARENT_CLASS( CGuiObject );
	NATIVE_FUNCTION( "GetModuleFlash", funcGetModuleFlash );
	NATIVE_FUNCTION( "GetModuleFlashValueStorage", funcGetModuleFlashValueStorage );
END_CLASS_RTTI();
