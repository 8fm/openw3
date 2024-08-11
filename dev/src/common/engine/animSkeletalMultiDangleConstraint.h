
#pragma once

#include "animSkeletalDangleConstraint.h"

class CAnimSkeletalMultiDangleConstraint : public CAnimSkeletalDangleConstraint
{
	DECLARE_ENGINE_CLASS( CAnimSkeletalMultiDangleConstraint, IAnimDangleConstraint, 0 );

public:
	CAnimSkeletalMultiDangleConstraint();

#ifndef NO_EDITOR
	virtual void OnPropertyPostChange( IProperty* property );
#endif

private:
	void FindDangleBonesAndCreateConstraints();

	const CSkeleton* FindMainSkeleton() const;

//++ FOR USERS
// Please read description for CAnimSkeletalDangleConstraint
// Create all your constraints inside CreateConstraints function
protected:
	virtual void CreateConstraints( const TDynArray< Int32 >& dangleParentBones_skeletonMain, const TDynArray< Int32 >& dangleRootBones_skeletonDangle, const TDynArray< TPair< Int32, TDynArray< Int32 > > >& dangleChildrenBones_skeletonDangle );
//-- FOR USERS
};

BEGIN_CLASS_RTTI( CAnimSkeletalMultiDangleConstraint );
	PARENT_CLASS( CAnimSkeletalDangleConstraint );
END_CLASS_RTTI();
