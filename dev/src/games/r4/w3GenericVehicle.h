#pragma once


#include "../../common/game/vehicle.h"


/// Type of movement for horse
enum EHorseMoveType
{
	HMT_SlowWalk,
	HMT_Walk,
	HMT_Trot,
	HMT_Gallop,
	HMT_Canter
};

BEGIN_ENUM_RTTI( EHorseMoveType );
	ENUM_OPTION( HMT_SlowWalk );
	ENUM_OPTION( HMT_Walk );
	ENUM_OPTION( HMT_Trot );
	ENUM_OPTION( HMT_Gallop );
	ENUM_OPTION( HMT_Canter );
END_ENUM_RTTI();

//---------------------------------------------------------------------------------------------------------------------------------
// W3HorseComponent
class CHorseRiderSharedParams;
class W3HorseComponent : public CVehicleComponent
{
	DECLARE_ENGINE_CLASS( W3HorseComponent, CVehicleComponent, 0 );
public:
	W3HorseComponent();
public:
	THandle<CHorseRiderSharedParams> m_riderSharedParams;

	Bool PairWithRider( CActor* horseActor, CHorseRiderSharedParams *const riderSharedParams );
	Bool IsTamed();
	void Unpair();
	Bool IsDismounted() const;
	Bool IsFullyMounted() const;
	static Bool IsTamed( CActor* horseActor );

private:
	void funcPairWithRider( CScriptStackFrame& stack, void* result );
	void funcIsTamed( CScriptStackFrame& stack, void* result );
	void funcUnpair( CScriptStackFrame& stack, void* result );
	void funcIsDismounted( CScriptStackFrame& stack, void* result );
	void funcIsFullyMounted( CScriptStackFrame& stack, void* result );
};

BEGIN_CLASS_RTTI( W3HorseComponent );
	PARENT_CLASS( CVehicleComponent );
	PROPERTY( m_riderSharedParams );
	NATIVE_FUNCTION( "PairWithRider", funcPairWithRider );
	NATIVE_FUNCTION( "Unpair", funcUnpair );
	NATIVE_FUNCTION( "IsTamed", funcIsTamed );
	NATIVE_FUNCTION( "IsDismounted", funcIsDismounted );
	NATIVE_FUNCTION( "IsFullyMounted", funcIsFullyMounted );
END_CLASS_RTTI();
