/*
 * Copyright © 2011 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CSoundEntityParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CSoundEntityParam, CEntityTemplateParam, 0 );

public:
	CSoundEntityParam();

	RED_INLINE const String&  GetFallTauntEvent() const { return m_fallTauntEvent; }

private:
	String		m_fallTauntEvent;
};

BEGIN_CLASS_RTTI( CSoundEntityParam )
	PARENT_CLASS( CEntityTemplateParam )
	PROPERTY_CUSTOM_EDIT( m_fallTauntEvent, TXT("Sound event name to be fired when npc is falling down the cliff"), TXT( "AudioEventBrowser") ) 
END_CLASS_RTTI()
