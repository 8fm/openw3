#pragma once

#include "behaviorGraphOutput.h"
#include "animSkeletalDangleConstraint.h"

class CAnimatedComponent;

class CAnimDangleConstraint_Dress : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Dress, CAnimSkeletalDangleConstraint, 0 );

protected:
	Bool				m_cachedAllBones;

	Float				m_thighBoneWeight;
	Float				m_shinBoneWeight;
	Float				m_kneeRollBoneWeight;

	Float				m_ofweight;
	Vector				m_p1;
	Vector				m_p2;
	Vector				m_p3;

	Vector				m_r1;
	Vector				m_r2;
	Vector				m_r3;

	Int32	m_indicesCache[7];

	enum EBoneIndex		
	{
		BI_ThighLeft, 
		BI_ShinLeft, 
		BI_KneeRollLeft, 
		BI_ThighRight, 
		BI_ShinRight, 
		BI_KneeRollRight,  
		BI_Pelvis,
		BI_Size 
	};

	virtual Bool HasCachedBones() const override;
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;
	Matrix RollMatrix( const Matrix & mat, Float val );
public:
	CAnimDangleConstraint_Dress();
	virtual ~CAnimDangleConstraint_Dress();

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );
	void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Dress )
	PARENT_CLASS( CAnimSkeletalDangleConstraint )
	PROPERTY_EDIT( m_thighBoneWeight, TXT("") )
	PROPERTY_EDIT( m_shinBoneWeight, TXT("") )
	PROPERTY_EDIT( m_kneeRollBoneWeight, TXT("") )
	PROPERTY_EDIT( m_ofweight, TXT("") )
	PROPERTY_EDIT( m_p1, TXT("") )
	PROPERTY_EDIT( m_p2, TXT("") )
	PROPERTY_EDIT( m_p3, TXT("") )
	PROPERTY_EDIT( m_r1, TXT("") )
	PROPERTY_EDIT( m_r2, TXT("") )
	PROPERTY_EDIT( m_r3, TXT("") )
END_CLASS_RTTI()