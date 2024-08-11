#include "build.h"
#include "soundAdditionalListener.h"
#include "soundSystem.h"

#ifndef NO_EDITOR
CSoundAdditionalListener::~CSoundAdditionalListener()
{
	if( m_listenerIndex == -1 ) return;

	CSoundSystem::SoundListenerDynArray& listeners = GSoundSystem->m_listeners;

	for( char i = 0; i != ( char ) listeners.Size(); ++i )
	{
		if( listeners[ i ] == this )
		{
			listeners[ m_listenerIndex - 1 ] = 0;
			break;
		}
	}

}

void CSoundAdditionalListener::UpdateSoundListener( const Vector& cameraPosition, const Vector& up, const Vector& forward, CSoundEmitterComponent* emitter )
{
	if( !emitter ) return;
	if( m_listenerIndex == -1 )
	{
		CSoundSystem::SoundListenerDynArray& listeners = GSoundSystem->m_listeners;

		for( char i = 0; i != ( char ) listeners.Size(); ++i )
		{
			if( !listeners[ i ] )
			{
				m_listenerIndex = i + 1;
				listeners[ m_listenerIndex - 1 ] = this;
				break;
			}
		}

		if( ( m_listenerIndex == -1 ) && listeners.Size() < 8 )
		{
			m_listenerIndex = ( char ) listeners.Size() + 1;
			listeners.PushBack( this );
		}

	}

	if( m_listenerIndex == -1 ) return;
	emitter->ListenerSwitch( m_listenerIndex );

	m_position = cameraPosition;
	m_up = up;
	m_forward = forward;
}
#endif
