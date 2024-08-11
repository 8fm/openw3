#include "build.h"
#include "inputListener.h"

Bool CScriptInputListener::OnGameInputEvent( const SInputAction & action )
{
	IScriptable * listener = m_listener.Get();
	if( listener )
	{
		listener->CallEvent( m_eventName, action );
	}
	return false;
}
