/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/flashFunction.h"
#include "../../common/engine/flashValue.h"
#include "../../common/engine/flashMovie.h"
#include "../../common/game/guiObject.h"

#include "hudResource.h"

//////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////
class CFlashMovie;
class CFlashMovieAdapter;
class CFlashHudAdapter;
class CGuiManager;
class CHudResource;
class CScriptedFlashObjectPool;

//////////////////////////////////////////////////////////////////////////
// CHud
//////////////////////////////////////////////////////////////////////////
class CHud
	: public CGuiObject
	, public CSoundEmitterOneShot
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CHud, CGuiObject );

	friend class CHudModule;

	struct SHudModuleDesc
	{
		typedef THandle< CHudModuleResourceBlock > TModuleResourceBlock;

		String								 m_moduleName;
		CName								 m_moduleClassName;
		TModuleResourceBlock				 m_moduleResourceBlock;

		Bool operator==( const SHudModuleDesc& rhs ) const 
			{ return m_moduleName == rhs.m_moduleName; }

		SHudModuleDesc( String moduleName, CName moduleClassName, CHudModuleResourceBlock* moduleResourceBlock )
			: m_moduleName( moduleName )
			, m_moduleClassName( moduleClassName )
			, m_moduleResourceBlock( moduleResourceBlock )
		{
		}

		SHudModuleDesc()
			: m_moduleName( String::EMPTY )
			, m_moduleClassName( CName::NONE )
			, m_moduleResourceBlock()
		{}
	};

private:
	typedef THashMap< CName, const CClass* >				THudModuleClassMap;
	typedef THashMap< String, SHudModuleDesc >				THudModuleDescMap;
	typedef THashMap< String, CHudModule* >					THudModuleMap;
	typedef TDynArray< CHudModule* >						THudModuleList;

private:
	THudModuleClassMap				m_hudModuleClassMap;
	THudModuleDescMap				m_hudModuleDescMap;
	THudModuleMap					m_hudModuleMap;

	THudModuleList					m_hudModuleTickList;

private:
	THandle< CHudResource >			m_hudResource;
	CFlashMovie*					m_flashMovie;
	CFlashMovieAdapter*				m_flashMovieAdapter;
	CFlashHudAdapter*				m_flashHudAdapter;

private:
	CScriptedFlashObjectPool*				m_scriptedFlashObjectPool;
	THandle< CScriptedFlashSprite >			m_scriptedFlashSprite;
	THandle< CScriptedFlashValueStorage >	m_scriptedFlashValueStorage;

private:
	Bool							m_isInTick;
	Bool							m_isAttached;

private:
	THandle< CGuiManager >			m_guiManagerHandle;

public:
	static CHud*					CreateObjectFromResource( CHudResource* hudResource, CObject* parent );

public:
	virtual	void					Tick( float timeDelta ) override;

public:
	Bool							CreateHudModule( const String& moduleName, Int32 userData =-1 );
	Bool							DiscardHudModule( const String& moduleName, Int32 userData = -1 );

public:
	void							Attach();
	void							Detach();
	RED_INLINE Bool				IsAttached() const { return m_isAttached; }

public:
	RED_INLINE CFlashMovie*		GetFlashMovie() const { return m_flashMovie; }

public:
	//! CObject functions
	virtual void					OnSerialize( IFile& file ) override;
	virtual void					OnFinalize() override;
// 	virtual void					OnScriptReloading() override;
// 	virtual void					OnScriptReloaded() override;
		
#ifndef NO_EDITOR
//	virtual void					OnCreatedInEditor() override;
#endif

public:
	virtual Bool					RegisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;
	virtual Bool					UnregisterFlashValueStorage( CFlashValueStorage* flashValueStorage ) override;

public:
	Bool							CollectForTick( CHudModule* hudModule );

public:
	CHud();
	Bool							Init( CGuiManager* guiManager );

public:
	void							SetIgnoreKeys( const TDynArray< EInputKey >& keysToSet );
	void							ClearIgnoreKeys( const TDynArray< EInputKey >& keysToClear );

	void							SetIgnoreKey( EInputKey keyToSet );
	void							ClearIgnoreKey( EInputKey keyToClear );
	void							ClearAllIgnoreKeys();

public:
	virtual Bool					InitWithFlashSprite( const CFlashValue& flashSprite ) override;
	virtual Bool					IsInitWithFlashSprite() const override;

	virtual CGuiObject*				GetChild( const String& childName ) override;

protected:
#ifndef RED_FINAL_BUILD
	virtual StringAnsi				GetSoundObjectName() const override { return "CHud"; }
#endif

private:
	void							Cleanup();

private:
	Bool							FindHudModuleDescByName( const String& moduleName, SHudModuleDesc& outHudModuleDesc );

private:
	void							funcCreateHudModule( CScriptStackFrame& stack, void* result );
	void							funcDiscardHudModule( CScriptStackFrame& stack, void* result );
	void							funcGetHudFlash( CScriptStackFrame& stack, void* result );
	void							funcGetHudFlashValueStorage( CScriptStackFrame& stack, void* result );
	void							funcGetHudModule( CScriptStackFrame& stack, void* result );
};

//////////////////////////////////////////////////////////////////////////
// RTTI
//////////////////////////////////////////////////////////////////////////
BEGIN_ABSTRACT_CLASS_RTTI( CHud );
	PARENT_CLASS( CGuiObject );
	NATIVE_FUNCTION( "CreateHudModule", funcCreateHudModule );
	NATIVE_FUNCTION( "DiscardHudModule", funcDiscardHudModule );
	NATIVE_FUNCTION( "GetHudFlash", funcGetHudFlash );
	NATIVE_FUNCTION( "GetHudFlashValueStorage", funcGetHudFlashValueStorage );
	NATIVE_FUNCTION( "GetHudModule", funcGetHudModule );
END_CLASS_RTTI();
