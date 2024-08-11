#include "build.h"
#include "storySceneOutput.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneOutput );

CStorySceneOutput::CStorySceneOutput()
	: m_name( TXT( "Output" ) )
	, m_questOutput( true )
	, m_gameplayCameraBlendTime( 0.0f )
	, m_environmentLightsBlendTime( 5.f )
	, m_gameplayCameraUseFocusTarget( true )
	, m_blackscreenColor( Color::BLACK )
#ifndef DISABLE_ABANDONED_CAMERA
	, m_enabledAbandonedCamera( false )
#endif
{
}

void CStorySceneOutput::OnPropertyPostChange( IProperty* prop )
{
	TBaseClass::OnPropertyPostChange( prop );

#ifndef DISABLE_ABANDONED_CAMERA
	if ( prop->GetName() == TXT("enabledAbandonedCamera") )
	{
		if ( m_gameplayCameraBlendTime == 0.0f )
		{
			m_gameplayCameraBlendTime = 1.5f;
		}
	}
#endif
}