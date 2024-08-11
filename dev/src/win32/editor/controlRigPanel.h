/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../../common/engine/controlRigIncludes.h"

class CEdDialogPreview;
class CEdSceneHelperEntitiesCtrl;
class TCrInstance;
class CEdSceneEditor;
class CEdControlRigBoneTransformPanel;
class CEdControlRigHandsTransformPanel;
class CEdControlRigPresetPanel;
class CEdControlRigIKPanel;

//////////////////////////////////////////////////////////////////////////

enum ESSEdControlRigBones
{
	SSB_Hand_L = 0,
	SSB_Hand_R,
	SSB_FingerA1_L,
	SSB_FingerB1_L,
	SSB_FingerC1_L,
	SSB_FingerD1_L,
	SSB_FingerE1_L,
	SSB_FingerA2_L,
	SSB_FingerB2_L,
	SSB_FingerC2_L,
	SSB_FingerD2_L,
	SSB_FingerE2_L,
	SSB_FingerA3_L,
	SSB_FingerB3_L,
	SSB_FingerC3_L,
	SSB_FingerD3_L,
	SSB_FingerE3_L,
	SSB_FingerA1_R,
	SSB_FingerB1_R,
	SSB_FingerC1_R,
	SSB_FingerD1_R,
	SSB_FingerE1_R,
	SSB_FingerA2_R,
	SSB_FingerB2_R,
	SSB_FingerC2_R,
	SSB_FingerD2_R,
	SSB_FingerE2_R,
	SSB_FingerA3_R,
	SSB_FingerB3_R,
	SSB_FingerC3_R,
	SSB_FingerD3_R,
	SSB_FingerE3_R,
	SSB_Last
};

//////////////////////////////////////////////////////////////////////////

class CEdControlRigPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

protected:
	CEdSceneEditor*				m_mediator;
	wxToggleButton*				m_editing;
	Bool						m_showSkeletonMode_FK;
	Bool						m_showSkeletonModePrev_FK;
	Bool						m_showSkeletonMode_IK;
	Bool						m_showSkeletonModePrev_IK;
	Bool						m_selectedEvent;

	CStorySceneEventPoseKey*	m_event;

	CEdControlRigBoneTransformPanel*	m_boneFKTranfromPanel;
	CEdControlRigHandsTransformPanel*	m_handsTransformPanel;
	CEdControlRigIKPanel*				m_boneIkTransformPanel;
	CEdControlRigPresetPanel*			m_presetsPanel;

	THandle< CActor >					m_selectedActorH;
	Float						m_currentSectionTime;

	CName						m_handsMappingNames[ SSB_Last ];
	Int32						m_handsMappingIdx[ SSB_Last ];

	struct SHelper
	{
		CGUID					m_guid;
		String					m_name;
		Int32					m_bone;
		Int32					m_parentBone;
		CName					m_nameAsName;
		EngineTransform			m_initialTransformWS;
		EngineTransform			m_currentTransformWS;
		Bool					m_isEditable;
		Bool					m_isSpawned;
		Bool					m_isDirty;
		Bool					m_justMoved;
		Bool					m_helperDirtyFlag;
		Bool					m_selectedByUser;

		SHelper();

		void					InitFK( const String& name, Int32 boneIdx, Int32 parentBoneIdx );
		void					InitIK( const Char* name, Int32 idx );
		void					Deinit();
		void					Spawn( Bool enable, CEdSceneEditor* mediator );
	};

	TStaticArray< SHelper, 256 > m_fkHelpers;
	TStaticArray< SHelper, 64 > m_ikHelpers;
	Bool						m_anyHelperSpawned;
	Bool						m_anyHelperDirty;
	Bool						m_anyHelperJustMoved;
	Bool						m_forceCache;
	Bool						m_handlingUserSelection;

public:
	CEdControlRigPanel( wxPanel* parent, CEdSceneEditor* editor );

	void ShowSkeletonFK();
	void HideSkeletonFK();
	void ShowSkeletonIK();
	void HideSkeletonIK();

	RED_INLINE Bool IsEditing() const { return m_editing->GetValue(); }

	void OnTick( Float sectionTimeBeforePreviewTick, Float sectionTimeAfterPreviewTick, Float timeDelta );
	void OnGenerateFragments( CRenderFrame *frame );

	void OnActorSelectionChange( CActor* newSelection );
	void OnPreviewSelectionChanged( const TDynArray< CEdSceneHelperEntity* >& entities );
	void OnEditButtonClicked( wxCommandEvent& event );

	void OnNewKeyframe();
	void SetData( CStorySceneEventPoseKey* e, CActor* actor );
	void Reload();

public: // Subeditors callbacks
	Bool OnFkTransformPanel_IsBoneModified( const String& name ) const;
	void OnFkTransformPanel_BoneSelected( const String& boneName );
	void OnFkTransformPanel_Rotate( Float valX, Float valY, Float valZ, const String& boneName );
	void OnFkTransformPanel_UpdateSliders();
	void OnFkTransformPanel_ResetBone( const String& boneName );
	Bool OnFkTransformPanel_HelperExists( const String& boneName ) const;

	void OnHandsTransformPanel_Rotate( Float valX, Float valY, Float valZ, CName boneName );
	void OnHandsTransformPanel_ResetBone( CName name );

	void OnIkTransformPanel_EffectorSelected( Int32 id );
	void OnIkTransformPanel_RefreshPlayer();
private:
	void ToggleEditModeIfNeeded();
	void RefreshEditButton();

	void InitHelpers();
	void UpdateSpawn( Int32 spawnFK = -1 , Int32 spawnIK = -1 );
	void ProcessHelpers( Int32 spawnHelperInd );
	void UpdateDirty();
	void UpdateTransforms();
	void LoadDataFromEvent();
	void UpdateSlidersFK();

	void CommitChanges( const Float* time = nullptr );
	void DiscardChanges();

	void SetEventBoneTransformFromFkHelper( CStorySceneEventPoseKey* evt, const SHelper& h );
	void TransformFKHelperLS( SHelper& h, const Matrix& transformLS );

	const SHelper* FindFkHelper( Int32 boneIndex ) const;
	const SHelper* FindFkHelper( const String& boneName ) const;
	SHelper* FindFkHelper( const String& boneName );
	const SHelper* FindFkParentHelper( Int32 boneIndex ) const;

	Matrix GetEffectorDefaultMatrixWorldSpace( Int32 effectorId ) const;
	SHelper* FindIkHelper( Int32 effectorId );

	const CSkeleton* GetActorsSkeleton() const;
	void CreateHandsMapping();
	void CacheHandsMapping();
	
public:
	CStorySceneEvent* FindSelectedEvent( const CClass* c, const CName& actorId );
	CStorySceneEventPoseKey* GetEvent();

	const CAnimatedComponent* GetActorsAnimatedComponent() const;
	Bool GetEvtParentAnimation( CStorySceneEventPoseKey* e, CName& animName, Float& animTime ) const;
};
