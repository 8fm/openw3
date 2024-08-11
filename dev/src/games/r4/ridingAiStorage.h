#pragma once

#include "../../common/game/aiScriptableStorage.h"

enum EVehicleSlot
{
	EVS_driver_slot,
	EVS_passenger_slot,
};
BEGIN_ENUM_RTTI( EVehicleSlot )
	ENUM_OPTION( EVS_driver_slot );
	ENUM_OPTION( EVS_passenger_slot );
END_ENUM_RTTI()

enum EVehicleMountStatus
{
	VMS_mountInProgress,
	VMS_mounted,
	VMS_dismountInProgress,
	VMS_dismounted
};

BEGIN_ENUM_RTTI( EVehicleMountStatus )
	ENUM_OPTION( VMS_mountInProgress );
	ENUM_OPTION( VMS_mounted );
	ENUM_OPTION( VMS_dismountInProgress );
	ENUM_OPTION( VMS_dismounted );
END_ENUM_RTTI()


enum EMountType
{
	MT_normal		= 1<<0,
	MT_instant		= 1<<1,
	MT_fromScript	= 1<<10		// used to know if the mount request was called from within the behtree or from without
};
BEGIN_ENUM_RTTI( EMountType )
	ENUM_OPTION( MT_normal );
	ENUM_OPTION( MT_instant );
	ENUM_OPTION( MT_fromScript );
END_ENUM_RTTI()

enum EDismountType
{
	DT_normal		= 1<<0,
	DT_shakeOff		= 1<<1,
	DT_ragdoll		= 1<<2,
	DT_instant		= 1<<3,
	DT_fromScript	= 1<<10		// used to know if the dismount request was called from within the behtree or from without
};
BEGIN_ENUM_RTTI( EDismountType )
	ENUM_OPTION( DT_normal );
	ENUM_OPTION( DT_shakeOff );
	ENUM_OPTION( DT_ragdoll );
	ENUM_OPTION( DT_instant );
	ENUM_OPTION( DT_fromScript );
END_ENUM_RTTI()

enum EVehicleMountType
{
	VMT_None,
	VMT_ApproachAndMount,
	VMT_MountIfPossible,
	VMT_TeleportAndMount,
	VMT_ImmediateUse
};
BEGIN_ENUM_RTTI( EVehicleMountType )
	ENUM_OPTION( VMT_None );
	ENUM_OPTION( VMT_ApproachAndMount );
	ENUM_OPTION( VMT_MountIfPossible );
	ENUM_OPTION( VMT_TeleportAndMount );
	ENUM_OPTION( VMT_ImmediateUse );
END_ENUM_RTTI()
class CHorseRiderSharedParams : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CHorseRiderSharedParams );
public:
	CHorseRiderSharedParams();
	
	THandle< CActor >			m_rider;
	// If this si not null rider is paired with a horse
	THandle< CActor >			m_horse;
	EVehicleMountStatus			m_mountStatus;

	// Handle on the boat the npc needs to use
	EntityHandle				m_boat;

	EVehicleSlot				m_vehicleSlot;
};
BEGIN_CLASS_RTTI( CHorseRiderSharedParams );
	PARENT_CLASS( IScriptable );
	PROPERTY( m_rider );
	PROPERTY( m_horse );
	PROPERTY( m_mountStatus );
	PROPERTY( m_boat );
	PROPERTY( m_vehicleSlot );
END_CLASS_RTTI();

enum ERidingManagerTask
{
	RMT_None			= 0,
	RMT_MountHorse		= 1,
	RMT_DismountHorse	= 2,
	RMT_MountBoat		= 3,
	RMT_DismountBoat	= 4,
};
BEGIN_ENUM_RTTI( ERidingManagerTask )
	ENUM_OPTION( RMT_None );
	ENUM_OPTION( RMT_MountHorse );
	ENUM_OPTION( RMT_DismountHorse );
	ENUM_OPTION( RMT_MountBoat );
	ENUM_OPTION( RMT_DismountBoat );
END_ENUM_RTTI()


class CAIStorageRiderData : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CAIStorageRiderData );
public:
	CAIStorageRiderData();
	~CAIStorageRiderData();
	typedef TAIScriptableStorageItemPtr< CAIStorageRiderData > CStoragePtr;
	
	THandle< CHorseRiderSharedParams >	m_sharedParams;
	
	THandle< IAIActionTree >			m_horseScriptedActionTree;

	// parameters for riding manager
	Bool								m_ridingManagerMountError;
	ERidingManagerTask					m_ridingManagerCurrentTask;
	Bool								m_ridingManagerCurrentTaskIsFromScript;
	EDismountType						m_ridingManagerDismountType;
	Bool								m_ridingManagerInstantMount;

	TDynArray< CActor * >		m_unreachableHorseList;

	/// Check if 
	/// the horse is not player's main horse
	/// if horse is not wild
	/// if horse is unreachable through navigation
	Bool CheckHorse( CActor *const horseActor, CActor *const riderActor );
	Bool PairWithTaggedHorse( CActor *const actor, CName horseTag, Float range );
	Bool IsHorseReachable( CActor *const horseActor, CActor *const riderActor );
	void RegisterUnreachableHorse( CActor *const horseActor ){ m_unreachableHorseList.PushBackUnique( horseActor ); }
	Bool IsMounted()const;

	void funcPairWithTaggedHorse( CScriptStackFrame& stack, void* result );
	void funcOnInstantDismount( CScriptStackFrame& stack, void* result );

	static void QueryHorses( TDynArray< CActor* >& output, const Vector& origin, Float range, CName tag );
	static CAIStorageRiderData* Get( CActor* actor );
};
BEGIN_CLASS_RTTI( CAIStorageRiderData );
	PARENT_CLASS( IScriptable );
	PROPERTY( m_sharedParams );
	PROPERTY( m_ridingManagerMountError );
	PROPERTY( m_ridingManagerCurrentTask );
	PROPERTY( m_horseScriptedActionTree );
	PROPERTY( m_ridingManagerDismountType );
	PROPERTY( m_ridingManagerInstantMount );
	NATIVE_FUNCTION( "PairWithTaggedHorse", funcPairWithTaggedHorse );
	NATIVE_FUNCTION( "OnInstantDismount", funcOnInstantDismount );
END_CLASS_RTTI();

class CAIStorageAnimalData : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CAIStorageAnimalData );
public:
	CAIStorageAnimalData(){ EnableReferenceCounting( true ); }
	typedef TAIScriptableStorageItemPtr< CAIStorageAnimalData > CStoragePtr;
};
BEGIN_CLASS_RTTI( CAIStorageAnimalData );
	PARENT_CLASS( IScriptable );
END_CLASS_RTTI();

class CAIStorageHorseData : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CAIStorageHorseData );
public:
	CAIStorageHorseData();
	typedef TAIScriptableStorageItemPtr< CAIStorageHorseData > CStoragePtr;
};
BEGIN_CLASS_RTTI( CAIStorageHorseData );
	PARENT_CLASS( IScriptable );
END_CLASS_RTTI();

