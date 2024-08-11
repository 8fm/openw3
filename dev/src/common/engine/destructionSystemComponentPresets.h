#pragma once

enum EDestructionPreset
{
	CUSTOM_PRESET_D	
};

BEGIN_ENUM_RTTI( EDestructionPreset );
	ENUM_OPTION( CUSTOM_PRESET_D );
END_ENUM_RTTI();

struct DestructionPreset	
{
	//	enum name, probability, CrubleChunks, AccumulateDam, damCap, damThreshold, damToRad, forceToDam, fracImpulseScale, impactDamDefaultDepth, impactVelThreshold, 
	//	materialStrangth, maxChunkSpeed, useWorldSupport, UseHardSleeping, useStressSolver, stressSolverDelay, stressSolverMassThreshold, sleepVelocityFrameDecay.
	String	name;
	Float	probability;
	Bool	crumbleChunks;
	Bool	accumulateDam;
	Float	damCap;
	Float	damThres;
	Float	damToRad;
	//Float	forToDam;
	Float	fracImpScale;
	Int32	impDefDepth;
	Float	impVelThres;
	Float	matStrength;
	Float	maxChunkSpeed;
	Bool	useWorldSupport;
	Bool	useHardSleep;
	Bool	useStresSolver;
	Float	stressSolverDelay;
	Float	stressSolverMassThres;
	Float	sleepVelFrameDecay;
};

static DestructionPreset destructionPreset[] =
{
	//	enum name,			probability, crumbChun, accDam, damCap, damThres, damToRad, forToDam, fracImpSc, impDefDepth, impVThrs, matStr, maxChSpd, useWorldSup, UseHardSleep, useStrsSol, strsSolDel, strsSolMasThres, slpVelDecay.
	{ TXT("Default_Destruction"), 	1.0f,	false,	true,	0.f,		1.f,	0.1f,				0.f,			-1,		0.f,	0.f,		0.f,	false,		false,		false,		1.0f,			0.f,			1.f  }
};