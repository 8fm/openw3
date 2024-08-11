
#pragma once

#include "../../common/engine/controlRigIncludes.h"
#include "../../common/engine/virtualAnimation.h"


class CWorld;
class AnimIKPosePreviewEditor;

class AnimIKPosePreviewItem : public IPreviewItem
{
	Int32							m_boneIndex;
	ETCrEffectorId				m_effectorId;
	AnimIKPosePreviewEditor*	m_editor;
	Bool						m_isSet;

public:
	AnimIKPosePreviewItem( AnimIKPosePreviewEditor* editor, ETCrEffectorId effector, Int32 boneIndex );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );

public:
	RED_INLINE Int32 GetBoneIndex() const				{ return m_boneIndex; }
	RED_INLINE ETCrEffectorId GetEffectorId() const	{ return m_effectorId; }

	void SetReferencePose( const Matrix& transformLS, const Matrix& transformMS );

	void SetInitialTransformation( const Vector& posMS, const EulerAngles& rotMS );
};

class AnimIKPosePreviewEditor : public IPreviewItemContainer
{
	CWorld*						m_world;
	VirtualAnimationPoseIK*		m_data;
	const CSkeleton*			m_skeleton;
	Bool						m_show;

public:
	AnimIKPosePreviewEditor( CWorld* world );

	virtual CWorld* GetPreviewItemWorld() const;

public:
	void Set( VirtualAnimationPoseIK& data );
	void Reset();

	void FillPose( const CSkeleton* skeleton );
	void UpdatePosePre( Uint32 boneNumIn, AnimQsTransform* bonesOut );
	void UpdatePosePost( Uint32 boneNumIn, AnimQsTransform* bonesOut );

public: // Callback function
	void SetPositionFromPreview( ETCrEffectorId effector, const Vector& positionMS );

private:
	void ShowItems( Bool flag );
};
