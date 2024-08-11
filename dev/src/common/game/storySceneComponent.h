#pragma once

#include "storyScene.h"
#include "asyncCheckResult.h"

/// Component of the entity that is responsible for handling scenes for entity
class CStorySceneComponent : public CSpriteComponent
{
	DECLARE_ENGINE_CLASS( CStorySceneComponent, CSpriteComponent, 0 );

protected:
	TSoftHandle< CStoryScene >	m_storyScene;			//!< Definition of the scene
	TDynArray< String >			m_cachedInputs;
	Bool						m_sceneInputsCached;
public:
	CStorySceneComponent();

	virtual Bool OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue );

	//! Get scene definition used by this scene component
	RED_INLINE TSoftHandle< CStoryScene > GetStoryScene() const { return m_storyScene; }

	EAsyncCheckResult HasVoiceset( const String& voiceset );

public:
	// Called when component is attached to world ( layer gets visible, etc )
	virtual void OnAttached( CWorld* world );

	// Called when component is detached from world ( layer gets hidden, etc )
	virtual void OnDetached( CWorld* world );

public:
	// Should we update transform this node automatically when parent is update transformed /
	virtual bool UsesAutoUpdateTransform();

protected:
	// Get sprite icon
	virtual CBitmapTexture* GetSpriteIcon() const;
};

BEGIN_CLASS_RTTI( CStorySceneComponent );
	PARENT_CLASS( CSpriteComponent );
	PROPERTY_EDIT( m_storyScene, TXT("A scene file") );
END_CLASS_RTTI();