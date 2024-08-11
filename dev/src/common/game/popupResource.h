/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "../../common/engine/swfResource.h"

#include "guiResource.h"

class CPopup;

//////////////////////////////////////////////////////////////////////////
// EPopupPauseType
//////////////////////////////////////////////////////////////////////////
enum EPopupPauseType
{
	PPT_NoPause,
	PPT_ActivePause,
	PPT_FullPause,
};

//////////////////////////////////////////////////////////////////////////
// IPopupTimeParam
//////////////////////////////////////////////////////////////////////////
class IPopupTimeParam : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IPopupTimeParam, CObject );
};

//////////////////////////////////////////////////////////////////////////
// CPopupTimeScaleParam
//////////////////////////////////////////////////////////////////////////
class CPopupTimeScaleParam : public IPopupTimeParam
{
	DECLARE_ENGINE_CLASS( CPopupTimeScaleParam, IPopupTimeParam, 0 );

private:
	Float				m_timeScale;
	Bool				m_multiplicative;

public:
	RED_INLINE Float GetTimeScale() const { return m_timeScale; }

public:
	CPopupTimeScaleParam() : m_timeScale( 1.0 ), m_multiplicative( false ) {}
};

//////////////////////////////////////////////////////////////////////////
// CPopupPauseParam
//////////////////////////////////////////////////////////////////////////
class CPopupPauseParam : public IPopupTimeParam
{
	DECLARE_ENGINE_CLASS( CPopupPauseParam, IPopupTimeParam, 0 );

private:
	EPopupPauseType		m_pauseType;

public:
	RED_INLINE EPopupPauseType GetPauseType() const { return m_pauseType; }

public:
	CPopupPauseParam() : m_pauseType( PPT_NoPause ) {}
};

//////////////////////////////////////////////////////////////////////////
// CPopupDef
//////////////////////////////////////////////////////////////////////////
class CPopupDef : public CObject
{
	DECLARE_ENGINE_CLASS( CPopupDef, CObject, 0 );

private:
	IPopupTimeParam*				m_timeParam;

public:
	RED_INLINE IPopupTimeParam* GetTimeParam() const { return m_timeParam; }

public:
	CPopupDef();
};

class CPopupResource : public IGuiResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CPopupResource, IGuiResource, "popup", "Popup Resource" );

private:
	CName							m_popupClass;
	TSoftHandle< CSwfResource >		m_popupFlashSwf;
	Uint32							m_layer;
	CPopupDef*						m_popupDef;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual CClass*					GetResourceBlockClass() const override { return nullptr; }
#endif

public:
	RED_INLINE const TSoftHandle< CSwfResource >& GetPopupFlashSwf() const { return m_popupFlashSwf; }
	RED_INLINE const CName& GetPopupClass() const { return m_popupClass; }
	RED_INLINE Uint32 GetLayer() const { return m_layer; }
	RED_INLINE CPopupDef* GetPopupDef() const { return m_popupDef; }

public:
	CPopupResource();

public:
#ifndef NO_RESOURCE_IMPORT
	static void ResavePopupFlashSwf( CPopupResource* popupResource, const String& path );
#endif
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( CPopupResource );
PARENT_CLASS( IGuiResource );
	PROPERTY_CUSTOM_EDIT( m_popupClass, TXT("Popup object template"), TXT("PopupClassList") );
	PROPERTY_EDIT( m_popupFlashSwf, TXT("Root Flash SWF for the popup") );
	PROPERTY_EDIT( m_layer, TXT("Rendering layer of SWF") );
	PROPERTY_INLINED( m_popupDef, TXT("Popup def") );
END_CLASS_RTTI();

BEGIN_ENUM_RTTI( EPopupPauseType );
	ENUM_OPTION( PPT_NoPause );
	ENUM_OPTION( PPT_ActivePause );
	ENUM_OPTION( PPT_FullPause );
END_ENUM_RTTI();

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( IPopupTimeParam, CObject );

BEGIN_CLASS_RTTI( CPopupTimeScaleParam );
PARENT_CLASS( IPopupTimeParam );
	PROPERTY_EDIT_RANGE( m_timeScale, TXT("Time scale [0.01 2.0]"), 0.01f, 2.0f );
	PROPERTY_EDIT( m_multiplicative, TXT("Whether timescale is multiplicative or overrides current timescale") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CPopupPauseParam );
PARENT_CLASS( IPopupTimeParam );
	PROPERTY_EDIT( m_pauseType, TXT("Pause type") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( CPopupDef );
PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_timeParam, TXT("Time parameter") );
END_CLASS_RTTI();