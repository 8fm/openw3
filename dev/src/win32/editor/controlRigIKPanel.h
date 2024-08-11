#pragma once

#include "..\..\common\engine\controlRigIncludes.h"
//#include "..\..\common\engine\behaviorGraphNodeControlRig.h"
#include "..\..\common\engine\controlRig.h"

class CEdControlRigPanel;

class CEdControlRigIKPanel : public wxPanel
{
	DECLARE_EVENT_TABLE();

private:
	CEdControlRigPanel*				m_parentWin;

	//CBehaviorGraphControlRigInterface m_ikInterface;
	TCrInstance*					m_rig;

	Bool							m_running;
	ETCrEffectorId					m_effectorId;
	Float							m_weight;
	Vector							m_position;

	Bool							m_isPoseCached;
	SBehaviorGraphOutput			m_animPose;
	SBehaviorGraphOutput			m_ikPose;

	TEngineQsTransformArray			m_boneTrans;
	TDynArray< Int32 >				m_boneIdx;

private:
	wxTextCtrl*						m_effectorText;
	wxTextCtrl*						m_weightText;
	wxSlider*						m_weightSlider;

public:
	CEdControlRigIKPanel( wxWindow* parent, CEdControlRigPanel* ed );
	virtual ~CEdControlRigIKPanel();

	void OnActorSelectionChange( const CActor* e );
	void OnUpdateTransforms( CStorySceneEventPoseKey* evt );

	void SelectEffector( const wxString& name );
	void RefreshEffector( Int32 effectorID, const EngineTransform& transWS );
	void ActivateEffector( Int32 effectorID );

	Matrix GetEffectorDefaultMatrixWorldSpace( Int32 effectorId ) const;

	void OnGenerateFragments( CRenderFrame *frame );
	void LoadEffectorFromEvent( Int32 id, const Vector& val, Float w );

private:
	void RefreshEffector();
	void SelectEffector( ETCrEffectorId id );
	void DeselectEffector();
	void SelectEntityHeleperFor( ETCrEffectorId id );

	void SetWeightEdit( Float w );
	void SetWeightSlider( Float w );
	void SetWeight( Float w, Bool updateUI );
	Bool GetDataFromEvent( Vector& vec, Float& w ) const;

	void SetEffectorName( const wxString& name );

	void SetPosition( const Vector& vec );

	void EnableUI( Bool flag );
	void PrintDebugInfo();

	Bool SamplePose( const CAnimatedComponent* ac, SBehaviorGraphOutput& pose, CName animName, Float animTime ) const;

protected:
	void OnWeightSlider( wxCommandEvent& event );
	void OnWeightEdit( wxCommandEvent& event );
	void OnHandR( wxCommandEvent& event );
	void OnHandL( wxCommandEvent& event );
};

