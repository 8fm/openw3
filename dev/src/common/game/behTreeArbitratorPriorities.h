#pragma once

enum ETopLevelAIPriorities
{
	AIP_Unavailable			= -1,
	AIP_BelowIdle			= 15,
	AIP_AboveIdle			= 25,
	AIP_AboveIdle2			= 30,
	AIP_AboveEmergency		= 60,
	AIP_AboveEmergency2		= 65,
	AIP_AboveCombat			= 85,
	AIP_AboveCombat2		= 90
};


BEGIN_ENUM_RTTI( ETopLevelAIPriorities );
	ENUM_OPTION( AIP_Unavailable );
	ENUM_OPTION( AIP_BelowIdle );
	ENUM_OPTION( AIP_AboveIdle );
	ENUM_OPTION( AIP_AboveIdle2 );
	ENUM_OPTION( AIP_AboveEmergency );
	ENUM_OPTION( AIP_AboveEmergency2 );
	ENUM_OPTION( AIP_AboveCombat );
	ENUM_OPTION( AIP_AboveCombat2 );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////
enum EArbitratorPriorities
{
	BTAP_Unavailable		= -1,
	BTAP_Idle				= 20,
	BTAP_Emergency			= 50,
	BTAP_Combat				= 75,
	BTAP_FullCutscene		= 95,

	BTAP_BelowIdle			= AIP_BelowIdle+1,
	BTAP_AboveIdle			= AIP_AboveIdle+1,
	BTAP_AboveIdle2			= AIP_AboveIdle2+1,
	BTAP_AboveEmergency		= AIP_AboveEmergency+1,
	BTAP_AboveEmergency2	= AIP_AboveEmergency2+1,
	BTAP_AboveCombat		= AIP_AboveCombat+1,
	BTAP_AboveCombat2		= AIP_AboveCombat2+1
};

BEGIN_ENUM_RTTI( EArbitratorPriorities );
	ENUM_OPTION( BTAP_Unavailable );
	ENUM_OPTION( BTAP_Idle );
	ENUM_OPTION( BTAP_Emergency );
	ENUM_OPTION( BTAP_Combat );
	ENUM_OPTION( BTAP_FullCutscene );
	ENUM_OPTION( BTAP_BelowIdle );
	ENUM_OPTION( BTAP_AboveIdle );
	ENUM_OPTION( BTAP_AboveIdle2 );
	ENUM_OPTION( BTAP_AboveEmergency );
	ENUM_OPTION( BTAP_AboveEmergency2 );
	ENUM_OPTION( BTAP_AboveCombat );
	ENUM_OPTION( BTAP_AboveCombat2 );
END_ENUM_RTTI();


