
#pragma once

enum ETCrBoneId
{
	TCrB_Root			= 0,
	TCrB_Pelvis			= 1,
	TCrB_Torso1			= 2,
	TCrB_Torso2			= 3,
	TCrB_Torso3			= 4,
	TCrB_Neck			= 5,
	TCrB_Head			= 6,
	TCrB_ShoulderL		= 7,
	TCrB_BicepL			= 8,
	TCrB_ForearmL		= 9,
	TCrB_HandL			= 10,
	TCrB_WeaponL		= 11,
	TCrB_ShoulderR		= 12,
	TCrB_BicepR			= 13,
	TCrB_ForearmR		= 14,
	TCrB_HandR			= 15,
	TCrB_WeaponR		= 16,
	TCrB_ThighL			= 17,
	TCrB_ShinL			= 18,
	TCrB_FootL			= 19,
	TCrB_ToeL			= 20,
	TCrB_ThighR			= 21,
	TCrB_ShinR			= 22,
	TCrB_FootR			= 23,
	TCrB_ToeR			= 24,
	TCrB_Last			= 25,
	TCrB_ArmLROffset	= TCrB_ShoulderR - TCrB_ShoulderL,
	TCrB_LegLROffset	= TCrB_ThighR - TCrB_ThighL,
};

BEGIN_ENUM_RTTI( ETCrBoneId )
	ENUM_OPTION( TCrB_Root			)
	ENUM_OPTION( TCrB_Pelvis		)
	ENUM_OPTION( TCrB_Torso1		)
	ENUM_OPTION( TCrB_Torso2		)
	ENUM_OPTION( TCrB_Torso3		)
	ENUM_OPTION( TCrB_Neck			)
	ENUM_OPTION( TCrB_Head			)
	ENUM_OPTION( TCrB_ShoulderL		)
	ENUM_OPTION( TCrB_BicepL		)
	ENUM_OPTION( TCrB_ForearmL		)
	ENUM_OPTION( TCrB_HandL			)
	ENUM_OPTION( TCrB_WeaponL		)
	ENUM_OPTION( TCrB_ShoulderR		)
	ENUM_OPTION( TCrB_BicepR		)
	ENUM_OPTION( TCrB_ForearmR		)
	ENUM_OPTION( TCrB_HandR			)
	ENUM_OPTION( TCrB_WeaponR		)
	ENUM_OPTION( TCrB_ThighL		)
	ENUM_OPTION( TCrB_ShinL			)
	ENUM_OPTION( TCrB_FootL			)
	ENUM_OPTION( TCrB_ToeL			)
	ENUM_OPTION( TCrB_ThighR		)
	ENUM_OPTION( TCrB_ShinR			)
	ENUM_OPTION( TCrB_FootR			)
	ENUM_OPTION( TCrB_ToeR			)
	ENUM_OPTION( TCrB_Last			)
END_ENUM_RTTI();


enum ETCrEffectorId
{
	TCrEffector_First	= 0,
	TCrEffector_HandL	= 0,
	TCrEffector_HandR	= 1,
	TCrEffector_Last	= 2,
};

BEGIN_ENUM_RTTI( ETCrEffectorId );
	ENUM_OPTION( TCrEffector_HandL );
	ENUM_OPTION( TCrEffector_HandR );
END_ENUM_RTTI();
