#pragma once

#include "storyScene.h"

class CStoryScene;

struct SVoicesetSlot
{
	DECLARE_RTTI_SIMPLE_CLASS( SVoicesetSlot )

	TSoftHandle< CStoryScene >	m_scene;
	String						m_name;
	CName						m_voiceTag;
};

BEGIN_CLASS_RTTI( SVoicesetSlot );
	PROPERTY_EDIT( m_scene, TXT( "Scene" ) );
	PROPERTY_EDIT( m_name, TXT( "Scene name" ) );
	PROPERTY_EDIT( m_voiceTag, TXT( "Voicetag" ) );
END_CLASS_RTTI();

class CVoicesetParam : public CEntityTemplateParam
{
	DECLARE_ENGINE_CLASS( CVoicesetParam, CEntityTemplateParam, 0 );

protected:
	TDynArray< SVoicesetSlot > m_slots;

public:
	CVoicesetParam();

	Bool AddVoiceset( const String& sceneName, CStoryScene* scene, const CName& voiceTag );
	Bool FindVoiceset( const String& sceneName, TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const;

	Bool GetRandomVoiceset( TSoftHandle< CStoryScene >& scene, CName& voiceTag ) const;

private:
	Bool HasVoicesetWithName( const String& sceneName ) const;
};

BEGIN_CLASS_RTTI( CVoicesetParam );
	PARENT_CLASS( CEntityTemplateParam );
	PROPERTY_EDIT( m_slots, TXT("") );
END_CLASS_RTTI();
