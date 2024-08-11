
#include "build.h"
#include "sceneAnimEvents.h"

IMPLEMENT_ENGINE_CLASS( CExtAnimDialogKeyPoseMarker );

CExtAnimDialogKeyPoseMarker::CExtAnimDialogKeyPoseMarker()
{
	m_reportToScript = false;
}

CExtAnimDialogKeyPoseMarker::CExtAnimDialogKeyPoseMarker( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimEvent( eventName, animationName, startTime, trackName )
{
	m_reportToScript = false;
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CExtAnimDialogKeyPoseDuration );

CExtAnimDialogKeyPoseDuration::CExtAnimDialogKeyPoseDuration()
	: m_transition( true )
	, m_keyPose( true )
{
	m_reportToScript = false;
}

CExtAnimDialogKeyPoseDuration::CExtAnimDialogKeyPoseDuration( const CName& eventName, const CName& animationName, Float startTime, const String& trackName )
	: CExtAnimDurationEvent( eventName, animationName, startTime, 1.f, trackName )
	, m_transition( true )
	, m_keyPose( true )
{
	m_reportToScript = false;
}
