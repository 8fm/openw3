/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "storySceneComponent.h"
#include "storySceneInput.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CStorySceneComponent );

CGatheredResource resStoryIcon( TXT("engine\\textures\\icons\\sceneicon.xbm"), RGF_NotCooked );

CStorySceneComponent::CStorySceneComponent()
	: m_sceneInputsCached( false )
{
}

CBitmapTexture* CStorySceneComponent::GetSpriteIcon() const
{
	return resStoryIcon.LoadAndGet< CBitmapTexture >();
}

void CStorySceneComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );
	m_sceneInputsCached = false;
}

void CStorySceneComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );
	m_sceneInputsCached = false;
}

bool CStorySceneComponent::UsesAutoUpdateTransform()
{
	// Story scene components are not transform updated :P
	return false;
}

Bool CStorySceneComponent::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( propertyName == TXT("storyScene") )
	{
		CStoryScene *scene = *( CStoryScene **) readValue.GetData();

		m_storyScene = scene;

		return true;
	}

	return TBaseClass::OnPropertyTypeMismatch( propertyName, existingProperty, readValue );
}

EAsyncCheckResult CStorySceneComponent::HasVoiceset( const String& voiceset )
{
	if( !m_sceneInputsCached )
	{		
		BaseSoftHandle::EAsyncLoadingResult asyncResult =  m_storyScene.GetAsync( true );

		if( asyncResult == BaseSoftHandle::ALR_InProgress )
			return ASR_InProgress;

		if( asyncResult == BaseSoftHandle::ALR_Failed )
			return ASR_Failed;

		CStoryScene* scene = m_storyScene.Get();
		if ( !scene )
		{
			return ASR_Failed;
		}			

		scene->CollectControlPartsNames< CStorySceneInput >( m_cachedInputs );		
		m_sceneInputsCached = true;
	}
	Int32 index = (Int32) m_cachedInputs.GetIndex( voiceset );
	return ( index >= 0 ? ASR_ReadyTrue : ASR_ReadyFalse );
}
