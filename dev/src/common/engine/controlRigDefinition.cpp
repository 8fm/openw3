
#include "build.h"
#include "controlRigDefinition.h"
#include "controlRig.h"
#include "controlRigUtils.h"
#include "controlRigPropertySet.h"
#include "behaviorGraphOutput.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( TCrDefinition );
IMPLEMENT_RTTI_ENUM( ETCrBoneId );
IMPLEMENT_RTTI_ENUM( ETCrEffectorId );

TCrDefinition::TCrDefinition()
	: m_indexRoot( -1 )
	, m_indexPelvis( -1 )
	, m_indexTorso1( -1 )
	, m_indexTorso2( -1 )
	, m_indexTorso3( -1 )
	, m_indexNeck( -1 )
	, m_indexHead( -1 )
	, m_indexShoulderL( -1 )
	, m_indexBicepL( -1 )
	, m_indexForearmL( -1 )
	, m_indexHandL( -1 )
	, m_indexWeaponL( -1 )
	, m_indexShoulderR( -1 )
	, m_indexBicepR( -1 )
	, m_indexForearmR( -1 )
	, m_indexHandR( -1 )
	, m_indexWeaponR( -1 )
	, m_indexThighL( -1 )
	, m_indexShinL( -1 )
	, m_indexFootL( -1 )
	, m_indexToeL( -1 )
	, m_indexThighR( -1 )
	, m_indexShinR( -1 )
	, m_indexFootR( -1 )
	, m_indexToeR( -1 )
	, m_pelvisUpDir( A_X )
	, m_torso1UpDir( A_X )
	, m_torso2UpDir( A_X )
	, m_torso3UpDir( A_X )
{

}

TCrInstance* TCrDefinition::CreateRig( const TCrPropertySet* propertySet ) const
{
	TCrInstance* instance = new TCrInstance( propertySet );
	return instance;
}

Bool TCrDefinition::IsValid() const
{
	return m_indexRoot != -1 && m_indexPelvis != -1 && m_indexTorso1 != -1 && m_indexTorso2 != -1 && m_indexTorso3 != -1 &&
		m_indexShoulderL != -1 && m_indexBicepL != -1 && m_indexHandL != -1 && 
		m_indexShoulderR != -1 && m_indexBicepR != -1 && m_indexHandR != -1;
}

void TCrDefinition::SetRigFromPoseLS( const RedQsTransform* poseIn, Uint32 poseInNum, TCrInstance* rigOut ) const
{
	ASSERT( IsValid() );

	RedQsTransform* rigPoseLS = rigOut->AccessBonesLS();

	rigPoseLS[ TCrB_Root ] = poseIn[ m_indexRoot ];				rigPoseLS[ TCrB_Root ].Rotation.Normalize();
	rigPoseLS[ TCrB_Pelvis ] = poseIn[ m_indexPelvis ];			rigPoseLS[ TCrB_Pelvis ].Rotation.Normalize();
	rigPoseLS[ TCrB_Torso1 ] = poseIn[ m_indexTorso1 ];			rigPoseLS[ TCrB_Torso1 ].Rotation.Normalize();
	rigPoseLS[ TCrB_Torso2 ] = poseIn[ m_indexTorso2 ];			rigPoseLS[ TCrB_Torso2 ].Rotation.Normalize();
	rigPoseLS[ TCrB_Torso3 ] = poseIn[ m_indexTorso3 ];			rigPoseLS[ TCrB_Torso3 ].Rotation.Normalize();

	rigPoseLS[ TCrB_ShoulderL ] = poseIn[ m_indexShoulderL ];	rigPoseLS[ TCrB_ShoulderL ].Rotation.Normalize();
	rigPoseLS[ TCrB_BicepL ] = poseIn[ m_indexBicepL ];			rigPoseLS[ TCrB_BicepL ].Rotation.Normalize();
	rigPoseLS[ TCrB_ForearmL ] = poseIn[ m_indexForearmL ];		rigPoseLS[ TCrB_ForearmL ].Rotation.Normalize();
	rigPoseLS[ TCrB_HandL ] = poseIn[ m_indexHandL ];			rigPoseLS[ TCrB_HandL ].Rotation.Normalize();
	rigPoseLS[ TCrB_WeaponL ] = poseIn[ m_indexWeaponL ];		rigPoseLS[ TCrB_WeaponL ].Rotation.Normalize();

	rigPoseLS[ TCrB_ShoulderR ] = poseIn[ m_indexShoulderR ];	rigPoseLS[ TCrB_ShoulderR ].Rotation.Normalize();
	rigPoseLS[ TCrB_BicepR ] = poseIn[ m_indexBicepR ];			rigPoseLS[ TCrB_BicepR ].Rotation.Normalize();
	rigPoseLS[ TCrB_ForearmR ] = poseIn[ m_indexForearmR ];		rigPoseLS[ TCrB_ForearmR ].Rotation.Normalize();
	rigPoseLS[ TCrB_HandR ] = poseIn[ m_indexHandR ];			rigPoseLS[ TCrB_HandR ].Rotation.Normalize();
	rigPoseLS[ TCrB_WeaponR ] = poseIn[ m_indexWeaponR ];		rigPoseLS[ TCrB_WeaponR ].Rotation.Normalize();

	rigOut->MarkSyncLS();
}

void TCrDefinition::SetPoseLSFromRig( const TCrInstance* rigIn, RedQsTransform* poseOut, Uint32 poseOutNum ) const
{
	ASSERT( IsValid() );

	const RedQsTransform* rigPoseLS = rigIn->GetBonesLS();

	poseOut[ m_indexRoot ] = rigPoseLS[ TCrB_Root ];				poseOut[ m_indexRoot ].Rotation.Normalize();
	poseOut[ m_indexPelvis ] = rigPoseLS[ TCrB_Pelvis ];			poseOut[ m_indexPelvis ].Rotation.Normalize();
	poseOut[ m_indexTorso1 ] = rigPoseLS[ TCrB_Torso1 ];			poseOut[ m_indexTorso1 ].Rotation.Normalize();
	poseOut[ m_indexTorso2 ] = rigPoseLS[ TCrB_Torso2 ];			poseOut[ m_indexTorso2 ].Rotation.Normalize();
	poseOut[ m_indexTorso3 ] = rigPoseLS[ TCrB_Torso3 ];			poseOut[ m_indexTorso3 ].Rotation.Normalize();

	poseOut[ m_indexShoulderL ] = rigPoseLS[ TCrB_ShoulderL ];		poseOut[ m_indexShoulderL ].Rotation.Normalize();
	poseOut[ m_indexBicepL ] = rigPoseLS[ TCrB_BicepL ];			poseOut[ m_indexBicepL ].Rotation.Normalize();
	poseOut[ m_indexForearmL ] = rigPoseLS[ TCrB_ForearmL ];		poseOut[ m_indexForearmL ].Rotation.Normalize();
	poseOut[ m_indexHandL ] = rigPoseLS[ TCrB_HandL ];				poseOut[ m_indexHandL ].Rotation.Normalize();
	poseOut[ m_indexWeaponL ] = rigPoseLS[ TCrB_WeaponL ];			poseOut[ m_indexWeaponL ].Rotation.Normalize();

	poseOut[ m_indexShoulderR ] = rigPoseLS[ TCrB_ShoulderR ];		poseOut[ m_indexShoulderR ].Rotation.Normalize();
	poseOut[ m_indexBicepR ] = rigPoseLS[ TCrB_BicepR ];			poseOut[ m_indexBicepR ].Rotation.Normalize();
	poseOut[ m_indexForearmR ] = rigPoseLS[ TCrB_ForearmR ];		poseOut[ m_indexForearmR ].Rotation.Normalize();
	poseOut[ m_indexHandR ] = rigPoseLS[ TCrB_HandR ];				poseOut[ m_indexHandR ].Rotation.Normalize();
	poseOut[ m_indexWeaponR ] = rigPoseLS[ TCrB_WeaponR ];			poseOut[ m_indexWeaponR ].Rotation.Normalize();
}

void TCrDefinition::SetRigFromPoseLS( const SBehaviorGraphOutput& poseIn, TCrInstance* rigOut ) const
{
	SetRigFromPoseLS( poseIn.m_outputPose, poseIn.m_numBones, rigOut );
}

void TCrDefinition::SetPoseLSFromRig( const TCrInstance* rigIn, SBehaviorGraphOutput& poseOut ) const
{
	SetPoseLSFromRig( rigIn, poseOut.m_outputPose, poseOut.m_numBones );
}

Bool TCrDefinition::IsRigBone( Int32 index ) const
{
	return index != -1 && (
		index == m_indexPelvis ||
		index == m_indexPelvis ||
		index == m_indexTorso1 ||
		index == m_indexTorso2 ||
		index == m_indexTorso3 ||
		index == m_indexNeck ||
		index == m_indexHead ||
		index == m_indexShoulderL ||
		index == m_indexBicepL ||
		index == m_indexForearmL ||
		index == m_indexHandL ||
		index == m_indexWeaponL ||
		index == m_indexThighL ||
		index == m_indexShinL ||
		index == m_indexFootL ||
		index == m_indexToeL ||
		index == m_indexShoulderR ||
		index == m_indexBicepR ||
		index == m_indexForearmR ||
		index == m_indexHandR ||
		index == m_indexWeaponR ||
		index == m_indexThighR ||
		index == m_indexShinR ||
		index == m_indexFootR ||
		index == m_indexToeR );
}

void TCrDefinition::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	CSkeleton* s = FindParent< CSkeleton >();
	if ( s )
	{
		#define CacheIKBone( name, index )\
		if ( property->GetName() == TXT( #name ) )\
		{\
			index = s->FindBoneByName( m_##name );\
		}

		CacheIKBone( root, m_indexRoot );
		CacheIKBone( pelvis, m_indexPelvis );
		CacheIKBone( torso1, m_indexTorso1 );
		CacheIKBone( torso2, m_indexTorso2 );
		CacheIKBone( torso3, m_indexTorso3 );
		CacheIKBone( neck, m_indexNeck );
		CacheIKBone( head, m_indexHead );
		CacheIKBone( shoulderL, m_indexShoulderL );
		CacheIKBone( bicepL, m_indexBicepL );
		CacheIKBone( forearmL, m_indexForearmL );
		CacheIKBone( handL, m_indexHandL );
		CacheIKBone( weaponL, m_indexWeaponL );
		CacheIKBone( shoulderR, m_indexShoulderR );
		CacheIKBone( bicepR, m_indexBicepR );
		CacheIKBone( forearmR, m_indexForearmR );
		CacheIKBone( handR, m_indexHandR );
		CacheIKBone( weaponR, m_indexWeaponR );
		CacheIKBone( thighL, m_indexThighL );
		CacheIKBone( shinL, m_indexShinL );
		CacheIKBone( footL, m_indexFootL );
		CacheIKBone( toeL, m_indexToeL );
		CacheIKBone( thighR, m_indexThighR );
		CacheIKBone( shinR, m_indexShinR );
		CacheIKBone( footR, m_indexFootR );
		CacheIKBone( toeR, m_indexToeR );

		#undef CacheIKBone
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CControlRigSettings );

void CControlRigSettings::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == TXT("fkBonesNames_l1" ) )
	{
		CacheFKBones();
	}
	else if ( property->GetName() == TXT("ikBonesNames" ) )
	{
		CacheIKBones();
	}
}

void CControlRigSettings::CacheFKBones()
{
	CSkeleton* s = FindParent< CSkeleton >();
	if ( s )
	{
		m_fkBones_l1.Resize( m_fkBonesNames_l1.Size() );

		for ( Uint32 i=0; i<m_fkBonesNames_l1.Size(); ++i )
		{
			m_fkBones_l1[ i ] =  s->FindBoneByName( m_fkBonesNames_l1[ i ] );
		}
	}
}

void CControlRigSettings::CacheIKBones()
{
	CSkeleton* s = FindParent< CSkeleton >();
	if ( s )
	{
		m_ikBones.Resize( m_ikBonesNames.Size() );

		for ( Uint32 i=0; i<m_ikBonesNames.Size(); ++i )
		{
			m_ikBones[ i ] =  s->FindBoneByName( m_ikBonesNames[ i ] );
		}
	}
}
