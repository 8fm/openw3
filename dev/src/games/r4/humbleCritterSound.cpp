#include "build.h"
#include "humbleCritterSound.h"
#include "humbleCrittersLairEntity.h"



/*
///////////////////////////////////////////////////////////////
// CHumbleCrittersSound
///////////////////////////////////////////////////////////////
void CHumbleCrittersSound::Update( const Vector& cameraPos, IBoidLairEntity& lair )
{
	CHumbleCrittersLairEntity& crittersLair = static_cast< CHumbleCrittersLairEntity& >( lair );
	CHumbleCrittersAlgorithmData* crittersData = const_cast< CHumbleCrittersAlgorithmData* >( crittersLair.GetAlgorithmData() );

	const CHumbleCrittersAlgorithmData::SoundData& soundData = crittersData->GetSoundData( m_soundId );
	m_boidsCount = soundData.m_soundIntesity;
	m_position = soundData.m_soundCenter;
	m_radius = soundData.m_radius;
}
*/