/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "../../common/engine/drawableComponent.h"

class CEdBehaviorEditor;

class CBehaviorDebugVisualizer : public CDrawableComponent
{
public:
	enum EBehaviorVisualizerBoneStyle
	{
		BVBS_Line,
		BVBS_3D
	};

private:
	DECLARE_ENGINE_CLASS( CBehaviorDebugVisualizer, CDrawableComponent, 0 );

	CAnimatedComponent*		m_animatedComponent;
	SBehaviorGraphOutput*	m_pose;
	Color					m_color;
	TDynArray<Int32>			m_boneIndex;
	TDynArray<Int32>			m_boneHelpers;
	Bool					m_showAxis;
	Bool					m_showNames;
	Bool					m_showBox;

	CEdBehaviorEditor*		m_behaviorEditor;

	CBehaviorGraphNode*		m_nodeToSample;
	CName					m_animation;
	Float					m_time;

	Float					m_dispTime;
	Float					m_dispProgress;

	Bool					m_collectData;
	
	EBehaviorVisualizerBoneStyle m_boneStyle;

public:
	CBehaviorDebugVisualizer();
	virtual ~CBehaviorDebugVisualizer();

    virtual Bool IsManualCreationAllowed() const { return false; }

	virtual void OnAttached( CWorld *world );

	virtual void OnDetached( CWorld *world );

	virtual void OnUpdateBounds();

	void OnTick( float timeDelta );

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

public:
	void SetNodeOfInterest( CBehaviorGraphNode *node );

	void SetAnimationAndTime( const CName& animation, Float time );

public:
	void SetAnimatedComponent( CAnimatedComponent *component );

	void SetColor( const Color &color );

	Color GetColor() const { return m_color; }

	void SetEditor( CEdBehaviorEditor *editor );

	void ChooseSelectBones();

	void SelectDefaultBones();

	void ChooseBonesHelpers();

	void ToggleBonesAxis() { m_showAxis = !m_showAxis; }

	void ToggleBonesName() { m_showNames = !m_showNames; }

	void ToggleBox() { m_showBox = !m_showBox; }

	void SetBoneStyle(EBehaviorVisualizerBoneStyle style) { m_boneStyle = style; }

	Bool IsAxisVisible() const { return m_showAxis; }

	Bool IsNameVisible() const { return m_showNames; }

	Bool IsBoxVisible() const { return m_showBox; }

	EBehaviorVisualizerBoneStyle GetBoneStyle() const { return m_boneStyle; }

	void GetPoseBones( TDynArray< Matrix >& bonesLS, TDynArray< Matrix >& bonesMS, TDynArray< Matrix >& bonesWS ) const;
	void GetPoseTracks( TDynArray< Float >& tracks ) const;
	void GetPoseCustomTracks( TDynArray< Float >& tracks ) const;
	void GetPoseEvents( TDynArray< String >& events ) const;
	void GetMotionEx( String& motionEx ) const;

	Bool HasNodeOfInterest() const;
	Bool HasAnimation() const;

protected:
	void DisplayPose( const SBehaviorGraphOutput& pose, CRenderFrame* frame ) const;
	void ResetPose();
	Bool CanWork() const;
	Uint32 GetVisualizerNum() const;
};

BEGIN_CLASS_RTTI( CBehaviorDebugVisualizer )
	PARENT_CLASS( CDrawableComponent  )
END_CLASS_RTTI();