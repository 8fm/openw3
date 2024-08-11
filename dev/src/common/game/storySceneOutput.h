#pragma once

#include "storySceneControlPart.h"

// Requested by Konrad due to it being overused by designers
#define DISABLE_ABANDONED_CAMERA

class CStorySceneOutput : public CStorySceneControlPart
{
	DECLARE_ENGINE_CLASS( CStorySceneOutput, CStorySceneControlPart, 0 )

private:
	CName	m_name;
	Bool	m_questOutput;
	Bool	m_endsWithBlackscreen;
	Float	m_gameplayCameraBlendTime;
	Float	m_environmentLightsBlendTime;
	Bool	m_gameplayCameraUseFocusTarget;
	Color	m_blackscreenColor;
#ifndef DISABLE_ABANDONED_CAMERA
	Bool	m_enabledAbandonedCamera;
#endif

public:
	CStorySceneOutput();

	virtual String GetName() const { return m_name.AsString(); }
	void OnPropertyPostChange( IProperty* prop ) override;

	const CName& GetOutputName() const { return m_name; }

	RED_INLINE Bool IsQuestOutput() const { return m_questOutput; }

	RED_INLINE Bool ShouldEndWithBlackscreen() const { return m_endsWithBlackscreen; }
	RED_INLINE Color BlackscreenColor() const { return m_blackscreenColor; }

	RED_INLINE Float GetGameplayCameraBlendTime() const { return m_gameplayCameraBlendTime; }

	RED_INLINE Float GetLightsBlendTime() const { return m_environmentLightsBlendTime; }

#ifdef DISABLE_ABANDONED_CAMERA
	RED_INLINE Bool IsGameplayCameraAbandoned() const { return false; }
#else
	RED_INLINE Bool IsGameplayCameraAbandoned() const { return m_enabledAbandonedCamera; }
#endif
	RED_INLINE Bool GetGameplayCameraUseFocusTarget() const { return m_gameplayCameraUseFocusTarget; }
};

BEGIN_CLASS_RTTI( CStorySceneOutput )
	PARENT_CLASS( CStorySceneControlPart )
	PROPERTY_EDIT( m_name, TXT( "Output name" ) )
	PROPERTY_EDIT( m_questOutput, TXT( "Should this exit rout control back to the quest" ) )
	PROPERTY_EDIT( m_endsWithBlackscreen, TXT( "Should blackscreen stay after this output" ) )
	PROPERTY_EDIT( m_blackscreenColor, TXT("") );
	PROPERTY_EDIT( m_gameplayCameraBlendTime, TXT( "Gameplay camera blend time after scene is finished" ) )
	PROPERTY_EDIT( m_environmentLightsBlendTime, TXT( "Lights blend time after scene is finished" ) )
	PROPERTY_EDIT( m_gameplayCameraUseFocusTarget, TXT( "Use gameplay camera's focus target (player) to set up rotation during blend" ) )
#ifndef DISABLE_ABANDONED_CAMERA
	PROPERTY_EDIT( m_enabledAbandonedCamera, TXT( "Enable 'abandoned' camera mode; time of the blend to gameplay camera is specified via gameplayCameraBlendTime" ) )
#endif
END_CLASS_RTTI()