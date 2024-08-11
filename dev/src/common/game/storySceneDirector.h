/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "storyScene.h"
#include "storySceneDirectorPlacementHelper.h"
#include "storySceneIncludes.h"
#include "..\engine\cutsceneInstance.h"
#include "..\engine\pathlibWorld.h"
#include "..\engine\pathlibWalkableSpotQueryRequest.h"
#include "..\core\uniquePtr.h"


class CStoryScenePlayer;
class CStoryScene;
class CAnimatedComponent;
struct SceneActorPose;
class CTeleportNPCsJob;

enum ENearPlaneDistance : CEnum::TValueType;
enum EFarPlaneDistance : CEnum::TValueType;

// DIALOG_TOMSIN_TODO - remove this class
class CStorySceneDirector
{

private:
	Bool									m_isEnabled;
	Bool									m_allowCameraReactivaton;
	Bool									m_areActorPositionsValid;
	Bool									m_sceneCamerasActivated;

	CStoryScenePlayer*						m_parentPlayer;
	CStorySceneDirectorPlacementHelper		m_placementHelper;

	TDynArray< CName >						m_hiddenActors;

	TDynArray< THandle< CNewNPC > >			m_nearbyInterestedNPCs;
	
	// Bool delayed camera updates variables
	CName									m_activeCameraName;
	CName									m_activeCameraShot;
	Bool									m_isCustomCamera;
	Bool									m_isCameraAdjusted;

	ENearPlaneDistance						m_sceneNearPlane;
	EFarPlaneDistance						m_sceneFarPlane;

	ENearPlaneDistance						m_prevNearPlane;
	EFarPlaneDistance						m_prevFarPlane;

	Float									m_desiredSettingTimeElapsed;
	const CStorySceneDialogsetInstance*		m_desiredDialogsetInstance;

	const CStorySceneDialogsetInstance*		m_currentDialogsetInstance;

	Red::TUniquePtr< CTeleportNPCsJob >		m_teleportNPCsJob;

public:
	CStorySceneDirector();
	~CStorySceneDirector();

	Bool IsNPCTeleportJobInProgress() const;
	void FinalizeTeleportNPCsJob();
	void CancelTeleportNPCsJob();
	
	void Initialize( CStoryScenePlayer* parentPlayer, const CStorySceneInput* sceneInput );
	RED_INLINE CStoryScenePlayer* GetDirectorParent() const { return m_parentPlayer; }
	const CStorySceneDialogsetInstance* GetCurrentSceneDialogInstance() const { return m_currentDialogsetInstance; }
	void ClearAreaForSection( const CStorySceneSection* section );

	void ResetPlacements();
	
	CActor* SearchForActorInCommunity( const CName& voicetag );

	CActor* SpawnActorOnDemand( const CName& voicetag );
	CEntity* SpawnPropOnDemand( const CName& id );
	CEntity* SpawnLightOnDemand( const CName& id );
	CEntity* SpawnEffectOnDemand( const CName& id );

	void ActivateCamera( const StorySceneCameraDefinition& cameraDefinition );
	void ActivateCustomCamera();
	void SetCameraNoiseMovement( Bool enable );

	void SynchronizeCameraWithCS( const CCutsceneInstance::CameraState& state );
	
	void SetDialogBehaviorVariables( Bool enable );

	CCamera* GetSceneCamera( const CName& cameraName );
	CCamera* GetSceneCamera( Uint32 cameraNumber );

	void EnableDirector();
	void DisableDirector();

	void SetCameraData( const CCamera* camera );
	void ActivateCameras();
	void DeactivateCameras();

	void ApplyNearAndFarPlanes( CCameraComponent* cameraComponent );
	void RestoreNearAndFarPlanes( CCameraComponent* cameraComponent );

	Matrix GetActorTrajectory( const CName& actor ) const;

	void InformNPCsThatSceneIsFinished();

public:
	void SnapPlayerEntityToTeleportedTransform();

	void FinishChangingSettings();
	void ChangeDesiredSetting( const CStorySceneDialogsetInstance* desiredSetting );
	void KeepCurrentSetting();
	void Reinitialize( const CStorySceneInput* sceneInput );
	const CStorySceneDialogsetInstance* GetCurrentSettings() const;
	const CStorySceneDialogsetInstance* GetCurrentSettingsIfValid() const;
	const CStorySceneDialogsetInstance* GetDesiredSettings() const;

//private: TODO - This function should be private and called only from FinishChangingSettings - see storySceneSectionPreload
	void EnsureExistenceOfSettingActors( const CStorySceneDialogsetInstance* dialogsetInstance, Bool makeVisible );
	void EnsureExistenceOfProps();

public:
	EngineTransform	GetCurrentScenePlacement() const;
	EngineTransform	Debug_GetCurrentScenePlacement() const;

	const CStorySceneDialogsetInstance* GetPendingOrCurrentDialogsetInstance() const;
	EngineTransform GetScenePlacement( const CStorySceneDialogsetInstance* dialogsetInstance ) const;


	const CGUID&	GetActorsSlotID( const CName& actor ) const;

	void	GetSlotPlacement( const CStorySceneDialogsetSlot* slot, const CStorySceneDialogsetInstance* dialogsetInstance, EngineTransform& placement );

	RED_INLINE Bool IsEnabled() { return  m_isEnabled; }
	RED_INLINE void SetAllowCameraReactivation( Bool allow ) { m_allowCameraReactivaton = allow; }

	RED_INLINE Bool AreActorPositionsValid() const { return m_areActorPositionsValid; }

	CCamera* GetActiveCamera();

	Bool	IsContinousTransitionPossible( const CName& newSettingName ) const;

public:
	void	SetActorVisibleInScene( const CName& voicetag, Bool show );
	Bool 	AdjustCameraForActorHeight( const StorySceneCameraDefinition& cameraDefinition, EngineTransform* outAdjusted, Matrix* outCameraWS ) const;

	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );
	void SetCameraAdjustedDebugFrame( Bool adjusted );
private:
	Bool IsContinousTransitionPossible( const CStorySceneDialogsetInstance* newDialogset, const CStorySceneDialogsetInstance* oldDialogset ) const;
	Bool IsCameraMovementAllowed() const;

protected:
	const CWorld* GetSceneWorld() const;

	Matrix	BuildSpecialMatrix( const Vector& srcVect, const Vector& dstVect ) const;
	Bool	GetActorHeightData( CName slotName, Uint32 slotNr, Vector & currentPointLS, Matrix & actorL2W ) const;

	void DestroyDeadNPCs();
};

