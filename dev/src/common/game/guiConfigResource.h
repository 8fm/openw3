/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "hudResource.h"
#include "menuResource.h"
#include "guiConfigResource.h"

//////////////////////////////////////////////////////////////////////////
// SHudDescription
//////////////////////////////////////////////////////////////////////////
struct SHudDescription
{
	DECLARE_RTTI_STRUCT( SHudDescription );

	CName							m_hudName;
	TSoftHandle< CHudResource >		m_hudResource;

	RED_INLINE Bool operator==( const SHudDescription& rhs ) const { return m_hudName == rhs.m_hudName; }
	RED_INLINE Bool operator==( const CName& rhsHudName ) const { return m_hudName == rhsHudName; }
};

//////////////////////////////////////////////////////////////////////////
// SMenuDescription
//////////////////////////////////////////////////////////////////////////
struct SMenuDescription
{
	DECLARE_RTTI_STRUCT( SMenuDescription );

	CName							m_menuName;
	TSoftHandle< CMenuResource >	m_menuResource;

	RED_INLINE Bool operator==( const SMenuDescription& rhs ) const { return m_menuName == rhs.m_menuName; }
	RED_INLINE Bool operator==( const CName& rhsMenuName ) const { return m_menuName == rhsMenuName; }
};

//////////////////////////////////////////////////////////////////////////
// SPopupDescription
//////////////////////////////////////////////////////////////////////////
struct SPopupDescription
{
	DECLARE_RTTI_STRUCT( SPopupDescription );

	CName							m_popupName;
	TSoftHandle< CPopupResource >	m_popupResource;

	RED_INLINE Bool operator==( const SPopupDescription& rhs ) const { return m_popupName == rhs.m_popupName; }
	RED_INLINE Bool operator==( const CName& rhsPopupName ) const { return m_popupName == rhsPopupName; }
};

//////////////////////////////////////////////////////////////////////////
// SGuiSceneDescription
//////////////////////////////////////////////////////////////////////////
struct SGuiSceneDescription
{
	DECLARE_RTTI_STRUCT( SGuiSceneDescription );

	Bool									m_enabled;
	CName									m_worldClass;
	TSoftHandle< CEnvironmentDefinition >	m_defaultEnvDef;
	EulerAngles								m_defaultSunRotation;
	Bool									m_enablePhysics;
	
	SGuiSceneDescription()
		: m_enabled( false )
		, m_worldClass( CWorld::GetStaticClass()->GetName() )
		, m_defaultSunRotation( 0.f, 0.f, 0.f )
		, m_enablePhysics( false )
	{}
};

//////////////////////////////////////////////////////////////////////////
// CGuiConfigResource
//////////////////////////////////////////////////////////////////////////
class CGuiConfigResource : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CGuiConfigResource, CResource, "guiconfig", "GUI Game Configuration" );
	
	TDynArray< SHudDescription >				m_huds;
	TDynArray< SMenuDescription >				m_menus;
	TDynArray< SPopupDescription >				m_popups;
	SGuiSceneDescription						m_scene;

public:
	RED_INLINE const TDynArray< SHudDescription >&				GetHudDescriptions() const	{ return m_huds; }
	RED_INLINE const TDynArray< SMenuDescription >&				GetMenuDescriptions() const { return m_menus; }
	RED_INLINE const TDynArray< SPopupDescription >&				GetPopupDescriptions() const { return m_popups; }
	RED_INLINE const SGuiSceneDescription&						GetGuiSceneDescription() const { return m_scene; }
};

//////////////////////////////////////////////////////////////////////////
// RTTI Boilerplate
//////////////////////////////////////////////////////////////////////////
BEGIN_CLASS_RTTI( SHudDescription );
	PROPERTY_EDIT( m_hudName, TXT("HUD name") );
	PROPERTY_EDIT( m_hudResource, TXT("HUD resource") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( SMenuDescription );
	PROPERTY_EDIT( m_menuName, TXT("Menu name") );
	PROPERTY_EDIT( m_menuResource, TXT("Menu resource") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( SPopupDescription );
	PROPERTY_EDIT( m_popupName, TXT("Popup name") );
	PROPERTY_EDIT( m_popupResource, TXT("Popup resource") );
END_CLASS_RTTI();

BEGIN_CLASS_RTTI( SGuiSceneDescription );
	PROPERTY_EDIT( m_enabled, TXT("Enabled scene" ) );
	PROPERTY_CUSTOM_EDIT( m_worldClass, TXT("World class"), TXT("WorldClassList") );
	PROPERTY_EDIT( m_defaultEnvDef, TXT("Default environment definition") );
	PROPERTY_EDIT( m_defaultSunRotation, TXT("Default sun rotation") );
	PROPERTY_EDIT( m_enablePhysics, TXT("Enable physics") );
END_CLASS_RTTI();


BEGIN_CLASS_RTTI( CGuiConfigResource );
PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_huds, TXT("Game HUDs") );
	PROPERTY_EDIT( m_menus, TXT("Game menus") );
	PROPERTY_EDIT( m_popups, TXT("Game popups") );
	PROPERTY_EDIT( m_scene, TXT("Game GUI scenes"))
END_CLASS_RTTI();

