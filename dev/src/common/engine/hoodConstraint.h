
#pragma once

#include "animSkeletalDangleConstraint.h"

namespace AnimDangleConstraints
{

class hoodBone
{
public:
	Matrix worldSpace;
	Matrix localSpace;
	Matrix calculate( const Matrix & parent, const Vector & cen, Float r )
	{
		worldSpace = Matrix::Mul( parent, localSpace );
		Vector p = worldSpace.GetTranslation();
		Vector tov = p - cen;

		Float vdir = tov.X*worldSpace.GetAxisZ().X + tov.Y*worldSpace.GetAxisZ().Y + tov.Z*worldSpace.GetAxisZ().Z;
		Float pierw = (vdir*vdir)-((tov.X*tov.X+tov.Y*tov.Y+tov.Z*tov.Z)-(r*r));
		if( pierw>=0 )
		{
			Float t = -vdir+sqrt(pierw);
			if( t>0 )
			{
				worldSpace.SetTranslation( p+worldSpace.GetAxisZ()*t );
			}
		}
		return worldSpace;
	}
};

}

class CAnimDangleConstraint_Hood : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimDangleConstraint_Hood, CAnimSkeletalDangleConstraint, 0 );

private:
	Vector					m_offset;
	Float					m_radius;

private:
	Int32					m_headIndex;
	Bool					m_cachedAllBones;
	Vector					m_spherePos;

	AnimDangleConstraints::hoodBone	m_bones[2];

public:
	CAnimDangleConstraint_Hood();

	virtual void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags );

	virtual void OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones );

protected:
	// Returns true if you have cached bones already
	virtual Bool HasCachedBones() const override;

	// Cache your bone index inside. Parent and/or skeleton can be null
	virtual void CacheBones( const CComponent* parent, const CSkeleton* skeleton ) override;
};

BEGIN_CLASS_RTTI( CAnimDangleConstraint_Hood );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
	PROPERTY_EDIT( m_offset,   TXT("Offset") );
	PROPERTY_EDIT( m_radius,   TXT("Radius") );
END_CLASS_RTTI();
