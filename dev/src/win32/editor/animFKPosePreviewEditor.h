
#pragma once

#include "../../common/engine/virtualAnimation.h"


class AnimFKPosePreviewEditor;

class AnimFKPosePreviewItem : public IPreviewItem
{
	Int32							m_id;
	AnimFKPosePreviewEditor*	m_editor;

	Vector						m_referencePoseTranslationLS;
	EulerAngles					m_referencePoseRotationLS;

	Vector						m_referencePoseTranslationMS;
	EulerAngles					m_referencePoseRotationMS;

public:
	AnimFKPosePreviewItem( AnimFKPosePreviewEditor* editor, Int32 id );

	virtual IPreviewItemContainer* GetItemContainer() const;

	virtual Bool IsValid() const { return true; }
	virtual void Refresh();

	virtual void SetPositionFromPreview( const Vector& prevPos, Vector& newPos );
	virtual void SetRotationFromPreview( const EulerAngles& prevRot, EulerAngles& newRot );

public:
	void SetReferencePose( const Matrix& transformLS, const Matrix& transformMS );
	void SetPose( const Vector& positionLS );
	void SetPose( const EulerAngles& rotationLS );
};

class AnimFKPosePreviewEditor : public IPreviewItemContainer
{
	CWorld*						m_world;
	VirtualAnimationPoseFK*		m_data;
	const CSkeleton*			m_skeleton;
	Bool						m_show;

public:
	AnimFKPosePreviewEditor( CWorld* world );

	virtual CWorld* GetPreviewItemWorld() const;

public:
	void Set( VirtualAnimationPoseFK& data );
	void Reset();
	
	void FillPose( const CSkeleton* skeleton );
	void UpdatePosePre( Uint32 boneNumIn, AnimQsTransform* bonesOut );
	void UpdatePosePost( Uint32 boneNumIn, AnimQsTransform* bonesOut );

public: // Callback function
	void SetRotationFromPreview( Int32 id, const EulerAngles& refRot, const EulerAngles& prevRot, const EulerAngles& newRot );

private:
	void ShowItems( Bool flag );
};
