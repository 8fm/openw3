
#pragma once

#include "../../common/game/storySceneIncludes.h"
#include "../../common/game/storySceneCameraDefinition.h"

enum EScenePreviewCameraMode
{
	SPCM_FREE_CAM,
	SPCM_PREVIEW,
	SPCM_EDIT,
	SPCM_GAMEPLAY
};

class CEdRenderingPanel;

class CEdSceneCameraCtrl
{
	EScenePreviewCameraMode				m_mode;

	CEdRenderingPanel*					m_previewViewportCamera;
	CEdRenderingPanel*					m_gameViewportCamera;
	CEdRenderingPanel*					m_activeViewportCamera;
	StorySceneCameraState				m_sceneCamera;

	StorySceneCameraDefinition*			m_selectedCameraDefinition;
	StorySceneCameraDefinition			m_freeCameraDefinition;

	CStorySceneEvent*					m_cameraDefOwner; //!< The owner of the m_selectedCameraDefinition

public:
	CEdSceneCameraCtrl();

	void Init( CEdRenderingPanel* vc );

	void Update( const StorySceneCameraState& state );

	void SetFreeMode();
	void SetEditMode();
	void SetPreviewMode();
	void SetGameplayMode();
	void ResetGameplayMode();

	Bool IsPreviewMode() const;
	Bool IsEditMode() const;
	Bool IsFreeMode() const;
	Bool IsGameplayMode() const;

	void SetCameraPreviewMode();
	void SetCameraGameMode();

	void ReadFrom( const StorySceneCameraDefinition* cameraDefinition, const Matrix& camWS );
	void WriteTo( StorySceneCameraDefinition* cameraDefinition, const Matrix& sceneL2W );

	void SelectDefinition( StorySceneCameraDefinition* cameraDefinition, CStorySceneEvent* cameraDefOwner );
	const StorySceneCameraDefinition* GetSelectedDefinition() const;
	StorySceneCameraDefinition* GetSelectedDefinition();
	CStorySceneEvent* GetSelectedDefinitionOwner() const { return m_cameraDefOwner; }

	EScenePreviewCameraMode	GetCurrentMode() const { return m_mode; }

	Bool CalculateCamera() const;
	Bool CanMoveViewportCamera() const;
	void OnWorldViewportInput( enum EInputKey key, Float data );

	Bool IsPointOnScreen( const Vector& pointWS, Vector& screenBestPosition ) const;
	Bool CalcCameraRay( Vector& rayStart, Vector& rayDirection ) const;

	void SyncFromGameplayCamera();
	void GetCameraLocalToWorld( Matrix& out );

public:
	void SetCameraFromNet( const Vector& pos, const EulerAngles& rot, Float fov );

private:
	void SyncFromPreviewCamera();
	Bool GetCameraData( Matrix& camera, Float& fov, Uint32& w, Uint32& h ) const;
};
