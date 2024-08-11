
#pragma once

#include "controlRigIncludes.h"
#include "behaviorIncludes.h"

class TCrInstance;
class TCrPropertySet;

class TCrDefinition : public CObject
{
	DECLARE_ENGINE_CLASS( TCrDefinition, CObject, 0 );

public:
	CName m_root;
	CName m_pelvis;
	CName m_torso1;
	CName m_torso2;
	CName m_torso3;
	CName m_neck;
	CName m_head;
	CName m_shoulderL;
	CName m_bicepL;
	CName m_forearmL;
	CName m_handL;
	CName m_weaponL;
	CName m_shoulderR;
	CName m_bicepR;
	CName m_forearmR;
	CName m_handR;
	CName m_weaponR;
	CName m_thighL;
	CName m_shinL;
	CName m_footL;
	CName m_toeL;
	CName m_thighR;
	CName m_shinR;
	CName m_footR;
	CName m_toeR;

	Int32 m_indexRoot;
	Int32 m_indexPelvis;
	Int32 m_indexTorso1;
	Int32 m_indexTorso2;
	Int32 m_indexTorso3;
	Int32 m_indexNeck;
	Int32 m_indexHead;
	Int32 m_indexShoulderL;
	Int32 m_indexBicepL;
	Int32 m_indexForearmL;
	Int32 m_indexHandL;
	Int32 m_indexWeaponL;
	Int32 m_indexShoulderR;
	Int32 m_indexBicepR;
	Int32 m_indexForearmR;
	Int32 m_indexHandR;
	Int32 m_indexWeaponR;
	Int32 m_indexThighL;
	Int32 m_indexShinL;
	Int32 m_indexFootL;
	Int32 m_indexToeL;
	Int32 m_indexThighR;
	Int32 m_indexShinR;
	Int32 m_indexFootR;
	Int32 m_indexToeR;

	EAxis m_pelvisUpDir;
	EAxis m_torso1UpDir;
	EAxis m_torso2UpDir;
	EAxis m_torso3UpDir;

public:
	TCrDefinition();

	TCrInstance* CreateRig( const TCrPropertySet* propertySet ) const;

	Bool IsValid() const;

public:
	virtual void OnPropertyPostChange( IProperty* property );

public:
	void SetRigFromPoseLS( const SBehaviorGraphOutput& poseIn, TCrInstance* rigOut ) const;
	void SetPoseLSFromRig( const TCrInstance* rigIn, SBehaviorGraphOutput& poseOut ) const;

	void SetRigFromPoseLS( const RedQsTransform* poseInBuffer, Uint32 poseInNum, TCrInstance* rigOut ) const;
	void SetPoseLSFromRig( const TCrInstance* rigIn, RedQsTransform* poseOutBuffer, Uint32 poseOutNum ) const;

	Bool IsRigBone( Int32 index ) const;
};

BEGIN_CLASS_RTTI( TCrDefinition )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_root, String::EMPTY );
	PROPERTY_RO( m_indexRoot, String::EMPTY );
	PROPERTY_EDIT( m_pelvis, String::EMPTY );
	PROPERTY_RO( m_indexPelvis, String::EMPTY );
	PROPERTY_EDIT( m_torso1, String::EMPTY );
	PROPERTY_RO( m_indexTorso1, String::EMPTY );
	PROPERTY_EDIT( m_torso2, String::EMPTY );
	PROPERTY_RO( m_indexTorso2, String::EMPTY );
	PROPERTY_EDIT( m_torso3, String::EMPTY );
	PROPERTY_RO( m_indexTorso3, String::EMPTY );
	PROPERTY_EDIT( m_neck, String::EMPTY );
	PROPERTY_RO( m_indexNeck, String::EMPTY );
	PROPERTY_EDIT( m_head, String::EMPTY );
	PROPERTY_RO( m_indexHead, String::EMPTY );
	PROPERTY_EDIT( m_shoulderL, String::EMPTY );
	PROPERTY_RO( m_indexShoulderL, String::EMPTY );
	PROPERTY_EDIT( m_bicepL, String::EMPTY );
	PROPERTY_RO( m_indexBicepL, String::EMPTY );
	PROPERTY_EDIT( m_forearmL, String::EMPTY );
	PROPERTY_RO( m_indexForearmL, String::EMPTY );
	PROPERTY_EDIT( m_handL, String::EMPTY );
	PROPERTY_RO( m_indexHandL, String::EMPTY );
	PROPERTY_EDIT( m_weaponL, String::EMPTY );
	PROPERTY_RO( m_indexWeaponL, String::EMPTY );
	PROPERTY_EDIT( m_shoulderR, String::EMPTY );
	PROPERTY_RO( m_indexShoulderR, String::EMPTY );
	PROPERTY_EDIT( m_bicepR, String::EMPTY );
	PROPERTY_RO( m_indexBicepR, String::EMPTY );
	PROPERTY_EDIT( m_forearmR, String::EMPTY );
	PROPERTY_RO( m_indexForearmR, String::EMPTY );
	PROPERTY_EDIT( m_handR, String::EMPTY );
	PROPERTY_RO( m_indexHandR, String::EMPTY );
	PROPERTY_EDIT( m_weaponR, String::EMPTY );
	PROPERTY_RO( m_indexWeaponR, String::EMPTY );
	PROPERTY_EDIT( m_thighL, String::EMPTY );
	PROPERTY_RO( m_indexThighL, String::EMPTY );
	PROPERTY_EDIT( m_shinL, String::EMPTY );
	PROPERTY_RO( m_indexShinL, String::EMPTY );
	PROPERTY_EDIT( m_footL, String::EMPTY );
	PROPERTY_RO( m_indexFootL, String::EMPTY );
	PROPERTY_EDIT( m_toeL, String::EMPTY );
	PROPERTY_RO( m_indexToeL, String::EMPTY );
	PROPERTY_EDIT( m_thighR, String::EMPTY );
	PROPERTY_RO( m_indexThighR, String::EMPTY );
	PROPERTY_EDIT( m_shinR, String::EMPTY );
	PROPERTY_RO( m_indexShinR, String::EMPTY );
	PROPERTY_EDIT( m_footR, String::EMPTY );
	PROPERTY_RO( m_indexFootR, String::EMPTY );
	PROPERTY_EDIT( m_toeR, String::EMPTY );
	PROPERTY_RO( m_indexToeR, String::EMPTY );
	PROPERTY_EDIT( m_pelvisUpDir, String::EMPTY );
	PROPERTY_EDIT( m_torso1UpDir, String::EMPTY );
	PROPERTY_EDIT( m_torso2UpDir, String::EMPTY );
	PROPERTY_EDIT( m_torso3UpDir, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CControlRigSettings : public CObject
{
	DECLARE_ENGINE_CLASS( CControlRigSettings, CObject, 0 );

private:
	TDynArray< CName >	m_fkBonesNames_l1;
	TDynArray< Int32 >	m_fkBones_l1;

	TDynArray< CName >	m_ikBonesNames;
	TDynArray< Int32 >	m_ikBones;

public:
	const TDynArray< CName >& GetFkBoneNamesL1() const { return m_fkBonesNames_l1; }
	const TDynArray< Int32 >& GetFkBonesL1() const { return m_fkBones_l1; }

	const TDynArray< CName >& GetIkBoneNames() const { return m_ikBonesNames; }
	const TDynArray< Int32 >& GetIkBones() const { return m_ikBones; }

public:
	virtual void OnPropertyPostChange( IProperty* property );

private:
	void CacheFKBones();
	void CacheIKBones();
};

BEGIN_CLASS_RTTI( CControlRigSettings )
	PARENT_CLASS( CObject );
	PROPERTY_EDIT( m_fkBonesNames_l1, String::EMPTY );
	PROPERTY_RO( m_fkBones_l1, String::EMPTY );
	PROPERTY_EDIT( m_ikBonesNames, String::EMPTY );
	PROPERTY_RO( m_ikBones, String::EMPTY );
END_CLASS_RTTI();
