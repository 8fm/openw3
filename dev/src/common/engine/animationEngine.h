#pragma once
#ifndef ANIMATION_ENGING_H_
#define ANIMATION_ENGING_H_
enum EMotionType
{
	MT_Dynamic,
	MT_KeyFramed
};

BEGIN_ENUM_RTTI( EMotionType );
	ENUM_OPTION( MT_Dynamic );
	ENUM_OPTION( MT_KeyFramed );
END_ENUM_RTTI();
#endif