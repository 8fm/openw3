#pragma once
enum EInteractionPriority
{
	IP_Prio_0           = 0,
	IP_Prio_1 ,
	IP_Prio_2 ,
	IP_Prio_3 ,
	IP_Prio_4 ,
	IP_Prio_5 ,
    IP_Prio_6 ,
    IP_Prio_7 ,
    IP_Prio_8 ,
    IP_Prio_9 ,
    IP_Prio_10 ,
    IP_Prio_11 ,
    IP_Prio_12 ,
    IP_Prio_13 ,
    IP_Prio_14 ,
	IP_Max_Unpushable   = 0xFFFFFFFE,	// force Uint32
    IP_NotSet           = 0xFFFFFFFF
};

//////////////////////////////////////////////////////////////////////////

BEGIN_ENUM_RTTI(EInteractionPriority);
	ENUM_OPTION( IP_Prio_0 );
	ENUM_OPTION( IP_Prio_1 );
	ENUM_OPTION( IP_Prio_2 );
	ENUM_OPTION( IP_Prio_3 );
	ENUM_OPTION( IP_Prio_4 );
	ENUM_OPTION( IP_Prio_5 );
    ENUM_OPTION( IP_Prio_6 );
    ENUM_OPTION( IP_Prio_7 );
    ENUM_OPTION( IP_Prio_8 );
    ENUM_OPTION( IP_Prio_9 );
    ENUM_OPTION( IP_Prio_10 );
    ENUM_OPTION( IP_Prio_11 );
    ENUM_OPTION( IP_Prio_12 );
    ENUM_OPTION( IP_Prio_13 );
    ENUM_OPTION( IP_Prio_14 );
	ENUM_OPTION( IP_Max_Unpushable );
    ENUM_OPTION( IP_NotSet );
END_ENUM_RTTI();

//////////////////////////////////////////////////////////////////////////

#define USE_NEW_PRIORITY_SYSTEM

#ifdef USE_NEW_PRIORITY_SYSTEM 
typedef EInteractionPriority            InteractionPriorityType;
#define InteractionPriorityTypeMax      IP_Max_Unpushable
#define InteractionPriorityTypeMean		IP_Prio_7
#define InteractionPriorityTypeZero     IP_Prio_0
#define InteractionPriorityTypeNotSet   IP_NotSet
#define InteractionPriorityTypeCompare  Uint32
#else
typedef Float                           InteractionPriorityType;
#define InteractionPriorityTypeMax      NumericLimits<Float>::Max()
#define InteractionPriorityTypeZero     0.0f
#define InteractionPriorityTypeNotSet   -1.0f
#define InteractionPriorityTypeCompare  Float
#endif