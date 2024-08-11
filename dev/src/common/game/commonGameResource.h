/**
* Copyright c 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/gameResource.h"
#include "quest.h"
#include "guiConfigResource.h"

class CCommonGameResource : public CGameResource
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CCommonGameResource, CGameResource );

protected:
	TSoftHandle< CQuest >						m_mainQuest;
	TSoftHandle< CGuiConfigResource >			m_guiConfigOverride;

public:
	RED_INLINE const TSoftHandle< CQuest >&						GetMainQuest() const			{ return m_mainQuest; }
	RED_INLINE const TSoftHandle< CGuiConfigResource >&			GetGuiConfigOverride() const	{ return m_guiConfigOverride; }
};

BEGIN_ABSTRACT_CLASS_RTTI( CCommonGameResource );
	PARENT_CLASS( CGameResource );
	PROPERTY_EDIT( m_mainQuest, TXT( "Game main quest" ) );
	PROPERTY_EDIT( m_guiConfigOverride, TXT("GUI configuration") );
END_CLASS_RTTI();
