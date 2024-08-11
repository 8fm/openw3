
#pragma once

//////////////////////////////////////////////////////////////////////////

class CExtAnimDialogKeyPoseMarker : public CExtAnimEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimDialogKeyPoseMarker )

public:
	CExtAnimDialogKeyPoseMarker();
	CExtAnimDialogKeyPoseMarker( const CName& eventName, const CName& animationName, Float startTime,  const String& trackName );
};

BEGIN_CLASS_RTTI( CExtAnimDialogKeyPoseMarker );
	PARENT_CLASS( CExtAnimEvent );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CExtAnimDialogKeyPoseDuration : public CExtAnimDurationEvent
{
	DECLARE_RTTI_SIMPLE_CLASS( CExtAnimDialogKeyPoseDuration )

	Bool	m_transition;
	Bool	m_keyPose;

public:
	CExtAnimDialogKeyPoseDuration();
	CExtAnimDialogKeyPoseDuration( const CName& eventName, const CName& animationName, Float startTime,  const String& trackName );

	RED_INLINE Bool IsTransition() const { return m_transition; }
	RED_INLINE Bool IsKeyPose() const { return m_keyPose; }
};

BEGIN_CLASS_RTTI( CExtAnimDialogKeyPoseDuration );
	PARENT_CLASS( CExtAnimDurationEvent );
	PROPERTY_EDIT( m_transition, TXT("") );
	PROPERTY_EDIT( m_keyPose, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
