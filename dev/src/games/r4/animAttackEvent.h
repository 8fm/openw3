
#pragma once

class CExtAnimAttackEvent : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimAttackEvent )

public:
	CExtAnimAttackEvent();

	CExtAnimAttackEvent( const CName& eventName, const CName& animationName, Float startTime, const String& trackName );
	~CExtAnimAttackEvent();

protected:
	CName m_soundAttackType;
};

BEGIN_CLASS_RTTI( CExtAnimAttackEvent );
	PARENT_CLASS( CExtAnimEvent );
	PROPERTY_EDIT( m_soundAttackType, TXT( "Animation identification for sound for combat sounds" ) );
END_CLASS_RTTI();
