/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/swfResource.h"

#include "guiResource.h"

class CMenu;

//////////////////////////////////////////////////////////////////////////
// EMenuPauseType
//////////////////////////////////////////////////////////////////////////
enum EMenuPauseType
{
	MPT_NoPause,
	MPT_ActivePause,
	MPT_FullPause,
};

//////////////////////////////////////////////////////////////////////////
// IMenuTimeParam
//////////////////////////////////////////////////////////////////////////
class IMenuTimeParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMenuTimeParam, CObject );
};

//////////////////////////////////////////////////////////////////////////
// IMenuDisplayParam
//////////////////////////////////////////////////////////////////////////
class IMenuDisplayParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMenuDisplayParam, CObject );
};

//////////////////////////////////////////////////////////////////////////
// IMenuFlashParam
//////////////////////////////////////////////////////////////////////////
class IMenuFlashParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMenuFlashParam, CObject );
};

//////////////////////////////////////////////////////////////////////////
// IMenuBackgroundVideoParam
//////////////////////////////////////////////////////////////////////////
class IMenuBackgroundVideoParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMenuBackgroundVideoParam, CObject );
};

class CMenuInheritBackgroundVideoParam : public IMenuBackgroundVideoParam
{
	DECLARE_ENGINE_CLASS( CMenuInheritBackgroundVideoParam, IMenuBackgroundVideoParam, 0 );
};

class CMenuBackgroundVideoFileParam : public IMenuBackgroundVideoParam
{
	DECLARE_ENGINE_CLASS( CMenuBackgroundVideoFileParam, IMenuBackgroundVideoParam, 0 );

private:
	String	m_videoFile;

public:
	const String& GetVideoFile() const { return m_videoFile; }
};

class CMenuBackgroundVideoAliasParam : public IMenuBackgroundVideoParam
{
	DECLARE_ENGINE_CLASS( CMenuBackgroundVideoAliasParam, IMenuBackgroundVideoParam, 0 );

private:
	CName	m_videoAlias;

public:
	CName GetVideoAlias() const { return m_videoAlias; }
};

class CMenuClearBackgroundVideoParam : public IMenuBackgroundVideoParam
{
	DECLARE_ENGINE_CLASS( CMenuClearBackgroundVideoParam, IMenuBackgroundVideoParam, 0 );
};

//////////////////////////////////////////////////////////////////////////
// CMenuTimeScaleParam
//////////////////////////////////////////////////////////////////////////
class CMenuTimeScaleParam : public IMenuTimeParam
{
	DECLARE_ENGINE_CLASS( CMenuTimeScaleParam, IMenuTimeParam, 0 );

private:
	Float				m_timeScale;
	Bool				m_multiplicative;


public:
	RED_INLINE Float GetTimeScale() const { return m_timeScale; }

public:
	CMenuTimeScaleParam() : m_timeScale( 1.0 ), m_multiplicative( false ) {}
};

//////////////////////////////////////////////////////////////////////////
// CMenuPauseParam
//////////////////////////////////////////////////////////////////////////
class CMenuPauseParam : public IMenuTimeParam
{
	DECLARE_ENGINE_CLASS( CMenuPauseParam, IMenuTimeParam, 0 );

private:
	EMenuPauseType		m_pauseType;

public:
	RED_INLINE EMenuPauseType GetPauseType() const { return m_pauseType; }

public:
	CMenuPauseParam() : m_pauseType( MPT_NoPause ) {}
};

//////////////////////////////////////////////////////////////////////////
// CMenuRenderBackgroundParam
//////////////////////////////////////////////////////////////////////////
class CMenuRenderBackgroundParam : public IMenuDisplayParam
{
	DECLARE_ENGINE_CLASS( CMenuRenderBackgroundParam, IMenuDisplayParam, 0 );

private:
	Bool		m_renderGameWorld;

public:
	RED_INLINE Bool IsRenderGameWorld() const { return m_renderGameWorld; }

public:
	CMenuRenderBackgroundParam() : m_renderGameWorld( true ) {}
};

//////////////////////////////////////////////////////////////////////////
// CMenuBackgroundEffectParam
//////////////////////////////////////////////////////////////////////////
class CMenuBackgroundEffectParam : public IMenuDisplayParam
{
	DECLARE_ENGINE_CLASS( CMenuBackgroundEffectParam, IMenuDisplayParam, 0 );

private:
	//TBD: e.g., something like a blur the world or some color effect
};

//////////////////////////////////////////////////////////////////////////
// CMenuHudParam
//////////////////////////////////////////////////////////////////////////
class CMenuHudParam : public IMenuDisplayParam
{
	DECLARE_ENGINE_CLASS( CMenuHudParam, IMenuDisplayParam, 0 );

private:
	Bool m_keepHud;

public:
	RED_INLINE Bool GetKeepHud() const { return m_keepHud; }
};

//////////////////////////////////////////////////////////////////////////
// CMenuFlashSymbolParam
//////////////////////////////////////////////////////////////////////////
class CMenuFlashSymbolParam : public IMenuFlashParam
{
	DECLARE_ENGINE_CLASS( CMenuFlashSymbolParam, IMenuFlashParam, 0 );

private:
	String	m_flashSymbolName;

public:
	RED_INLINE String GetFlashSymbolName() const { return m_flashSymbolName; }
};

//////////////////////////////////////////////////////////////////////////
// CMenuFlashSwfParam
//////////////////////////////////////////////////////////////////////////
class CMenuFlashSwfParam : public IMenuFlashParam
{
	DECLARE_ENGINE_CLASS( CMenuFlashSwfParam, IMenuFlashParam, 0 );

private:
	TSoftHandle< CSwfResource >	m_flashSwfHandle;

public:
	RED_INLINE TSoftHandle< CSwfResource > GetFlashSwfHandle() const { return m_flashSwfHandle; }
};

//////////////////////////////////////////////////////////////////////////
// CMenuDef
//////////////////////////////////////////////////////////////////////////
class CMenuDef : public CObject
{
	DECLARE_ENGINE_CLASS( CMenuDef, CObject, 0 );

private:
	//TBD: Control showing the mouse or not in Flash itself...

	IMenuTimeParam*				m_timeParam;
	IMenuBackgroundVideoParam*	m_backgroundVideoParam;
	IMenuDisplayParam*			m_renderParam;
	IMenuFlashParam*			m_flashParam;

public:
	RED_INLINE IMenuTimeParam* GetTimeParam() const { return m_timeParam; }
	RED_INLINE IMenuBackgroundVideoParam* GetBackgroundVideoParam() const { return m_backgroundVideoParam; }
	RED_INLINE IMenuDisplayParam* GetRenderParam() const { return m_renderParam; }
	RED_INLINE IMenuFlashParam* GetFlashParam() const { return m_flashParam; }

public:
	CMenuDef();
};

class CMenuResource : public IGuiResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMenuResource, IGuiResource, "menu", "Menu Resource" );

private:
	CName							m_menuClass;
	TSoftHandle< CSwfResource >		m_menuFlashSwf;
	Uint32							m_layer;
	CMenuDef*						m_menuDef;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CClass*					GetResourceBlockClass() const override { return nullptr; }
#endif

public:
	RED_INLINE const TSoftHandle< CSwfResource >& GetMenuFlashSwf() const { return m_menuFlashSwf; }
	RED_INLINE const CName& GetMenuClass() const { return m_menuClass; }
	RED_INLINE Uint32 GetLayer() const { return m_layer; }
	RED_INLINE CMenuDef* GetMenuDef() const { return m_menuDef; }

public:
	CMenuResource();

public:
#ifndef NO_RESOURCE_IMPORT
	static void ResaveMenuFlashSwf( CMenuResource* menuResource, const String& path );
#endif
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CMenuInheritBackgroundVideoParam );
	PARENT_CLASS( IMenuBackgroundVideoParam );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuBackgroundVideoFileParam );
	PARENT_CLASS( IMenuBackgroundVideoParam );
	PROPERTY_EDIT( m_videoFile, TXT("Video file name without directory path") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuBackgroundVideoAliasParam );
	PARENT_CLASS( IMenuBackgroundVideoParam );
	PROPERTY_EDIT( m_videoAlias, TXT("Video alias. Actual video resolved at runtime") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuClearBackgroundVideoParam );
	PARENT_CLASS( IMenuBackgroundVideoParam );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuResource );
PARENT_CLASS( IGuiResource );
	PROPERTY_CUSTOM_EDIT( m_menuClass, TXT("Menu object template"), TXT("MenuClassList") );
	PROPERTY_EDIT( m_menuFlashSwf, TXT("Root Flash SWF for the menu") );
	PROPERTY_EDIT( m_layer, TXT("Rendering layer of SWF") );
	PROPERTY_INLINED( m_menuDef, TXT("Menu def") );
END_CLASS_RTTI();

BEGIN_ENUM_RTTI( EMenuPauseType );
	ENUM_OPTION( MPT_NoPause );
	ENUM_OPTION( MPT_ActivePause );
	ENUM_OPTION( MPT_FullPause );
END_ENUM_RTTI();

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IMenuTimeParam, CObject );
DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IMenuDisplayParam, CObject );
DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IMenuFlashParam, CObject );
DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IMenuBackgroundVideoParam, CObject );

BEGIN_CLASS_RTTI( CMenuTimeScaleParam );
PARENT_CLASS( IMenuTimeParam );
	PROPERTY_EDIT_RANGE( m_timeScale, TXT("Time scale [0.01 2.0]"), 0.01f, 2.0f );
	PROPERTY_EDIT( m_multiplicative, TXT("Whether timescale is multiplicative or overrides current timescale") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuPauseParam );
PARENT_CLASS( IMenuTimeParam );
	PROPERTY_EDIT( m_pauseType, TXT("Pause type") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuRenderBackgroundParam );
PARENT_CLASS( IMenuDisplayParam );
	PROPERTY_EDIT( m_renderGameWorld, TXT("Render the game world or not") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuBackgroundEffectParam );
PARENT_CLASS( IMenuDisplayParam );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuHudParam );
PARENT_CLASS( IMenuDisplayParam );
	PROPERTY_EDIT( m_keepHud, TXT("Keep the HUD active when this menu is displayed") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuFlashSymbolParam );
PARENT_CLASS( IMenuFlashParam );
	PROPERTY_EDIT( m_flashSymbolName, TXT("Flash symbol to instantiate in the menu's parent's Flash movie") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuFlashSwfParam );
PARENT_CLASS( IMenuFlashParam );
 	PROPERTY_EDIT( m_flashSwfHandle, TXT("Flash SWF to load") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CMenuDef );
PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_timeParam, TXT("Time parameter") );
	PROPERTY_INLINED( m_backgroundVideoParam, TXT("Background video parameter") );
	PROPERTY_INLINED( m_renderParam, TXT("Render parameter") );
	//PROPERTY_INLINED( m_flashParam, TXT("Flash parameter") );
END_CLASS_RTTI();