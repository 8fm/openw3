/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "storySceneDialogset.h"
#include "storySceneEventCamera.h"

class CStorySceneEventCustomCamera : public CStorySceneEventCamera
{
	DECLARE_SCENE_EVENT_CLASS( CStorySceneEventCustomCamera, CStorySceneEventCamera )

protected:
	Vector						m_cameraTranslation;
	EulerAngles					m_cameraRotation;
	Float						m_cameraZoom;
	Float						m_cameraFov;
	Float						m_dofFocusDistFar;
	Float						m_dofBlurDistFar;
	Float						m_dofIntensity;
	Float						m_dofFocusDistNear;
	Float						m_dofBlurDistNear;
	Bool						m_enableCameraNoise;

	StorySceneCameraDefinition	m_cameraDefinition;	// TODO: this is used by Set*() functions only, i.e. Get*() functions don't use it - fix it.

public:
	CStorySceneEventCustomCamera();
	CStorySceneEventCustomCamera( const String& eventName, CStorySceneElement* sceneElement, Float startTime, const StorySceneCameraDefinition& camDef, const String& trackName );

	// SCENE_TOMSIN_TODO - co to jest?
	CStorySceneEventCustomCamera( const String& eventName, CStorySceneElement* sceneElement,
		Float startTime, const Vector& cameraTranslation, const EulerAngles& cameraRotation,
		Float cameraZoom, Float cameraFov, Float dofFocusDistFar, Float dofBlurDistFar,
		Float dofIntensity, Float dofFocusDistNear, Float dofBlurDistNear, const ApertureDofParams& apertureDofParams,
		const String& trackName );

	// compiler generated cctor is ok

	virtual CStorySceneEventCustomCamera* Clone() const override;

public:
	virtual void OnStart( CStorySceneInstanceBuffer& data, CStoryScenePlayer* scenePlayer, CStorySceneEventsCollector& collector, const SStorySceneEventTimeInfo& timeInfo ) const override;
	virtual void Serialize( IFile& file ) override;
	virtual const StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL ) const	override { return &m_cameraDefinition; }
	virtual StorySceneCameraDefinition* GetCameraDefinition( CSceneEventFunctionSimpleArgs* args = NULL )	override { return &m_cameraDefinition; }
	RED_INLINE void SetCameraDefinition( StorySceneCameraDefinition& def ) { m_cameraDefinition = def; }

public:
	RED_INLINE const Vector& GetCameraTranslation() const					{ return m_cameraTranslation; }
	RED_INLINE void SetCameraTranslation( const Vector& translation )		{ m_cameraDefinition.m_cameraTransform.SetPosition( translation ); }

	RED_INLINE const EulerAngles& GetCameraRotation() const				{ return m_cameraRotation; }
	RED_INLINE void SetCameraRotation( const EulerAngles& rotation )		{ m_cameraDefinition.m_cameraTransform.SetRotation( rotation ); }
	
	RED_INLINE Float GetCameraZoom() const								{ return m_cameraZoom; }
	RED_INLINE void SetCameraZoom( Float zoom )							{ m_cameraDefinition.m_cameraZoom = zoom; }
	
	RED_INLINE Float GetCameraFov() const									{ return m_cameraFov; }
	RED_INLINE void SetCameraFov( Float fov )								{ m_cameraDefinition.m_cameraFov = fov; }

	RED_INLINE Float GetCameraDofFocusDistFar() const						{ return m_dofFocusDistFar; }
	RED_INLINE Float GetCameraDofBlurDistFar() const						{ return m_dofBlurDistFar; }
	RED_INLINE Float GetCameraDofIntensity() const						{ return m_dofIntensity; }
	RED_INLINE Float GetCameraDofFocusDistNear() const					{ return m_dofFocusDistNear; }
	RED_INLINE Float GetCameraDofBlurDistNear() const						{ return m_dofBlurDistNear; }
	RED_INLINE Bool  GetEnableCameraNoise() const							{ return m_enableCameraNoise; }

	RED_INLINE void SetCameraDofFocusDistFar( Float dofFocusDistFar )		{ m_cameraDefinition.m_dofFocusDistFar = dofFocusDistFar; }
	RED_INLINE void SetCameraDofBlurDistFar( Float dofBlurDistFar )		{ m_cameraDefinition.m_dofBlurDistFar = dofBlurDistFar; }
	RED_INLINE void SetCameraDofIntensity( Float dofIntensity )			{ m_cameraDefinition.m_dofIntensity = dofIntensity; }
	RED_INLINE void SetCameraDofFocusDistNear( Float dofFocusDistNear )	{ m_cameraDefinition.m_dofFocusDistNear = dofFocusDistNear; }
	RED_INLINE void SetCameraDofBlurDistNear( Float dofBlurDistNear )		{ m_cameraDefinition.m_dofBlurDistNear = dofBlurDistNear; }
	RED_INLINE void SetEnableCameraNoise( Bool disableCameraNoise )		{ m_cameraDefinition.m_enableCameraNoise = disableCameraNoise; }
};

BEGIN_CLASS_RTTI( CStorySceneEventCustomCamera )
	PARENT_CLASS( CStorySceneEventCamera )
	PROPERTY( m_cameraTranslation )
	PROPERTY( m_cameraRotation )
	PROPERTY( m_cameraZoom )
	PROPERTY( m_cameraFov )
	PROPERTY( m_dofFocusDistFar )
	PROPERTY( m_dofBlurDistFar )
	PROPERTY( m_dofIntensity )
	PROPERTY( m_dofFocusDistNear )
	PROPERTY( m_dofBlurDistNear )
	PROPERTY_INLINED( m_cameraDefinition, TXT( "Camera Parameters" ) );
END_CLASS_RTTI()
