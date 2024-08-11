
#pragma once

enum ESliderResult
{
	SR_NotSliding,
	SR_Sliding,
	SR_FinishedSliding
};

class IExpSlider
{
public:
	
};

class ExpSimpleSlider : public IExpSlider
{
	Float			m_startTrans;
	Float			m_endTrans;

	Float			m_startRot;
	Float			m_endRot;

public:
	void Setup( Float transStart, Float transEnd, Float rotStart, Float rotEnd );
	ESliderResult Update( AnimQsTransform& delta, const AnimQuaternion& exRotation, const AnimVector4& exTranslation, Float prevTime, Float currTime, const CNode* node, const Vector& point, Float yaw, Float ifYawDiffExceeds, Bool allowGoingBeyondEnd );
	void GenerateDebugFragments( CRenderFrame* frame ) {};
};

// class ExpAnimExpSlider : public IExpSlider
// {
// public:
// 	AnimExpSlider( const CSkeletalAnimationSetEntry* animation, const hkQsTransform& dist );
// };
