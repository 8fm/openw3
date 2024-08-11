
#include "build.h"
#include "storySceneParam.h"

IMPLEMENT_ENGINE_CLASS( SVoicesetSlot );
IMPLEMENT_ENGINE_CLASS( CVoicesetParam );

CVoicesetParam::CVoicesetParam()
{

}

Bool CVoicesetParam::AddVoiceset( const String& sceneName, CStoryScene* scene, const CName& voiceTag )
{
	if ( HasVoicesetWithName( sceneName ) )
	{
		return false;
	}

	Uint32 index = static_cast< Uint32 >( m_slots.Grow( 1 ) );
	SVoicesetSlot& slot = m_slots[ index ];

	slot.m_scene = scene;
	slot.m_name = sceneName;
	slot.m_voiceTag = voiceTag;

	return true;
}

Bool CVoicesetParam::FindVoiceset( const String& sceneName, TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const
{
	const Uint32 size = m_slots.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_slots[ i ].m_name == sceneName )
		{
			scene = m_slots[ i ].m_scene;
			voiceTag = m_slots[ i ].m_voiceTag;

			return true;
		}
	}
	return false;
}

Bool CVoicesetParam::GetRandomVoiceset( TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const
{
	if ( m_slots.Size() > 0 )
	{
		Uint32 sel = GEngine->GetRandomNumberGenerator().Get< Uint32 >( m_slots.Size() );

		ASSERT( sel < m_slots.Size() );

		scene = m_slots[ sel ].m_scene;
		voiceTag = m_slots[ sel ].m_voiceTag;

		return true;
	}
	return false;
}

Bool CVoicesetParam::HasVoicesetWithName( const String& sceneName ) const
{
	const Uint32 size = m_slots.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( m_slots[ i ].m_name == sceneName )
		{
			return true;
		}
	}
	return false;
}
