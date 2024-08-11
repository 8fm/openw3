#pragma once

enum EBreastPreset
{
	Default_Naked,
	Natural_Normal,
	Unnatural,
	Clothed,
	CUSTOM_PRESET
};

BEGIN_ENUM_RTTI( EBreastPreset );
	ENUM_OPTION( Default_Naked );
	ENUM_OPTION( Natural_Normal );
	ENUM_OPTION( Unnatural );
	ENUM_OPTION( Clothed );
	ENUM_OPTION( CUSTOM_PRESET );
END_ENUM_RTTI();

struct BreastPreset	
{
	String	name;
	Float	elAX;
	Float	elAY;
	Float	elAZ;
	Float	elAW;
	Float	velDamp;
	Float	bounceDamp;
	Float	inAcc;
	Float	inertiaScaler;
	Float	blackHole;
	Float	velClamp;
	Float	grav;
	Float	moveWeight;
	Float	rotWeight;
	Float	simTime;
	Float	offset;
};

static BreastPreset breastPreset[] =
{
	//				name		elax	elay	elaz	elaw	vdamp	bdamp	inacc	inscal	blackhole	velcla		grav	moveW		rotW	simtim	offset
	{ TXT("Default_Naked"), 	0.0f,	0.05f,	0.4f,	0.15f,	0.97f,	0.98f,	1.f,	1.f,	0.002f,		200.f,	-0.001f,	0.05f,		1.f,	0.01f,	0.0f },
	{ TXT("Natural_Normal"), 	0.0f,	0.03f,	0.21f,	0.14f,	0.88f,	0.96f,	1.f,	1.5f,	0.002f,		300.f,	-0.001f,	0.1f,		1.f,	0.01f,	0.0f },
	{ TXT("Unnatural"),			0.0f,	-0.02f,	0.3f,	0.2f,	0.95f,	0.99f,	1.f,	2.f,	0.0008f,	150.f,	-0.001f,	0.13f,		1.f,	0.01f,	0.0f },
	{ TXT("Clothed"),			0.0f,	0.03f,	0.21f,	0.14f,	0.8f,	0.9f,	1.f,	2.f,	0.05f,		300.f,	-0.001f,	0.05f,		0.05f,	0.01f,	0.0f }
};