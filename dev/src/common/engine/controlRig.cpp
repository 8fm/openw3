
#include "build.h"
#include "controlRig.h"
#include "controlRigPropertySet.h"
#include "behaviorGraphUtils.inl"

TCrInstance::TCrInstance( const TCrPropertySet* propertySet )
	: m_propertySet( propertySet )
	, m_poseSyncLS( false )
	, m_poseSyncMS( false )
	, m_poseSyncWS( false )
	, m_weaponOffsetForHandL( 0.f )
	, m_weaponOffsetForHandR( 0.f )
	, m_localToWorld( RedQsTransform::IDENTITY )
	, m_worldToLocal( RedQsTransform::IDENTITY )
{
	const Uint32 bSize = ARRAY_COUNT( m_bonesLS );
	for ( Uint32 i=0; i<bSize; ++i )
	{
		m_bonesLS[ i ].SetIdentity();
		m_bonesMS[ i ].SetIdentity();
		m_bonesWS[ i ].SetIdentity();
	}

	ResetAllEffectors();
}

TCrInstance::~TCrInstance()
{

}

void TCrInstance::Solve()
{
	ASSERT( IsSyncLS() );
	ASSERT( IsSyncMS() );

	m_solver.Solve( this );
}

RedQsTransform* TCrInstance::AccessBonesLS()
{
	m_poseSyncLS = false;
	return m_bonesLS;
}

RedQsTransform* TCrInstance::AccessBonesMS()
{
	m_poseSyncMS = false;
	return m_bonesMS;
}

RedQsTransform* TCrInstance::AccessBonesWS()
{
	m_poseSyncWS = false;
	return m_bonesWS;
}

const RedQsTransform* TCrInstance::GetBonesLS() const
{
	ASSERT( m_poseSyncLS );
	return m_bonesLS;
}

const RedQsTransform* TCrInstance::GetBonesMS() const
{
	ASSERT( m_poseSyncMS );
	return m_bonesMS;
}

const RedQsTransform* TCrInstance::GetBonesWS() const
{
	ASSERT( m_poseSyncWS );
	return m_bonesWS;
}

Bool TCrInstance::IsSyncLS() const
{
	return m_poseSyncLS;
}

Bool TCrInstance::IsSyncMS() const
{
	return m_poseSyncMS;
}

Bool TCrInstance::IsSyncWS() const
{
	return m_poseSyncWS;
}

void TCrInstance::MarkSyncLS()
{
	m_poseSyncLS = true;
}

void TCrInstance::SyncLSFromMS()
{
	m_bonesLS[ TCrB_Root ] = m_bonesMS[ TCrB_Root ];

	m_bonesLS[ TCrB_Pelvis ].SetMulInverseMul( m_bonesMS[ TCrB_Root ], m_bonesMS[ TCrB_Pelvis ] );
	m_bonesLS[ TCrB_Torso1 ].SetMulInverseMul( m_bonesMS[ TCrB_Pelvis ], m_bonesMS[ TCrB_Torso1 ] );
	m_bonesLS[ TCrB_Torso2 ].SetMulInverseMul( m_bonesMS[ TCrB_Torso1 ], m_bonesMS[ TCrB_Torso2 ] );
	m_bonesLS[ TCrB_Torso3 ].SetMulInverseMul( m_bonesMS[ TCrB_Torso2 ], m_bonesMS[ TCrB_Torso3 ] );

	m_bonesLS[ TCrB_ShoulderL ].SetMulInverseMul( m_bonesMS[ TCrB_Torso3 ], m_bonesMS[ TCrB_ShoulderL ] );
	m_bonesLS[ TCrB_BicepL ].SetMulInverseMul( m_bonesMS[ TCrB_ShoulderL ], m_bonesMS[ TCrB_BicepL ] );
	m_bonesLS[ TCrB_ForearmL ].SetMulInverseMul( m_bonesMS[ TCrB_BicepL ], m_bonesMS[ TCrB_ForearmL ] );
	m_bonesLS[ TCrB_HandL ].SetMulInverseMul( m_bonesMS[ TCrB_ForearmL ], m_bonesMS[ TCrB_HandL ] );
	m_bonesLS[ TCrB_WeaponL ].SetMulInverseMul( m_bonesMS[ TCrB_HandL ], m_bonesMS[ TCrB_WeaponL ] );

	m_bonesLS[ TCrB_ShoulderR ].SetMulInverseMul( m_bonesMS[ TCrB_Torso3 ], m_bonesMS[ TCrB_ShoulderR ] );
	m_bonesLS[ TCrB_BicepR ].SetMulInverseMul( m_bonesMS[ TCrB_ShoulderR ], m_bonesMS[ TCrB_BicepR ] );
	m_bonesLS[ TCrB_ForearmR ].SetMulInverseMul( m_bonesMS[ TCrB_BicepR ], m_bonesMS[ TCrB_ForearmR ] );
	m_bonesLS[ TCrB_HandR ].SetMulInverseMul( m_bonesMS[ TCrB_ForearmR ], m_bonesMS[ TCrB_HandR ] );
	m_bonesLS[ TCrB_WeaponR ].SetMulInverseMul( m_bonesMS[ TCrB_HandR ], m_bonesMS[ TCrB_WeaponR ] );

	m_poseSyncLS = true;
}

void TCrInstance::SyncMSFromWS()
{

}

void TCrInstance::SyncMSFromLS()
{
	m_bonesMS[ TCrB_Root ] = m_bonesLS[ TCrB_Root ];

	m_bonesMS[ TCrB_Pelvis ].SetMul( m_bonesMS[ TCrB_Root ], m_bonesLS[ TCrB_Pelvis ] );
	m_bonesMS[ TCrB_Torso1 ].SetMul( m_bonesMS[ TCrB_Pelvis ], m_bonesLS[ TCrB_Torso1 ] );
	m_bonesMS[ TCrB_Torso2 ].SetMul( m_bonesMS[ TCrB_Torso1 ], m_bonesLS[ TCrB_Torso2 ] );
	m_bonesMS[ TCrB_Torso3 ].SetMul( m_bonesMS[ TCrB_Torso2 ], m_bonesLS[ TCrB_Torso3 ] );

	m_bonesMS[ TCrB_ShoulderL ].SetMul( m_bonesMS[ TCrB_Torso3 ], m_bonesLS[ TCrB_ShoulderL ] );
	m_bonesMS[ TCrB_BicepL ].SetMul( m_bonesMS[ TCrB_ShoulderL ], m_bonesLS[ TCrB_BicepL ] );
	m_bonesMS[ TCrB_ForearmL ].SetMul( m_bonesMS[ TCrB_BicepL ], m_bonesLS[ TCrB_ForearmL ] );
	m_bonesMS[ TCrB_HandL ].SetMul( m_bonesMS[ TCrB_ForearmL ], m_bonesLS[ TCrB_HandL ] );
	m_bonesMS[ TCrB_WeaponL ].SetMul( m_bonesMS[ TCrB_HandL ], m_bonesLS[ TCrB_WeaponL ] );

	m_bonesMS[ TCrB_ShoulderR ].SetMul( m_bonesMS[ TCrB_Torso3 ], m_bonesLS[ TCrB_ShoulderR ] );
	m_bonesMS[ TCrB_BicepR ].SetMul( m_bonesMS[ TCrB_ShoulderR ], m_bonesLS[ TCrB_BicepR ] );
	m_bonesMS[ TCrB_ForearmR ].SetMul( m_bonesMS[ TCrB_BicepR ], m_bonesLS[ TCrB_ForearmR ] );
	m_bonesMS[ TCrB_HandR ].SetMul( m_bonesMS[ TCrB_ForearmR ], m_bonesLS[ TCrB_HandR ] );
	m_bonesMS[ TCrB_WeaponR ].SetMul( m_bonesMS[ TCrB_HandR ], m_bonesLS[ TCrB_WeaponR ] );

	m_poseSyncMS = true;
}

void TCrInstance::SyncWSFromMS()
{
	const Uint32 size = ARRAY_COUNT( m_bonesWS );
	for ( Uint32 i=0; i<size; ++i )
	{
		m_bonesWS[ i ].SetMul( m_localToWorld, m_bonesMS[ i ] );
	}
	
	m_poseSyncWS = true;
}

void TCrInstance::SetLocalToWorld( const Matrix& l2w )
{
	m_localToWorld = MatrixToAnimQsTransform( l2w );
	m_worldToLocal.SetInverse( m_localToWorld );
}

void TCrInstance::SetTranslationActive( ETCrEffectorId id, Float weight )
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	ASSERT( id < TCrEffector_Last && id >= 0 );
	m_effectorsPos[ id ] = weight;
}

void TCrInstance::SetRotationActive( ETCrEffectorId id, Float weight )
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	ASSERT( id < TCrEffector_Last && id >= 0 );
	m_effectorsRot[ id ] = weight;
}

void TCrInstance::SetEffectorWS( ETCrEffectorId id, const RedQsTransform& transformWS )
{
	ASSERT( id < TCrEffector_Last && id >= 0 );

	m_effectors[ id ].SetMul( m_worldToLocal, transformWS );
}

void TCrInstance::SetEffectorWS( ETCrEffectorId id, const Vector& positionWS )
{
	ASSERT( id < TCrEffector_Last && id >= 0 );

	m_effectors[ id ].Translation.SetTransformedPos( m_worldToLocal, reinterpret_cast< const RedVector4& >( positionWS ) );
}

void TCrInstance::GetEffectorWS( ETCrEffectorId id, RedQsTransform& transformWS ) const
{
	ASSERT( id < TCrEffector_Last && id >= 0 );
	transformWS.SetMul( m_localToWorld, m_effectors[ id ] );
}

Bool TCrInstance::IsAnyEffectorSet() const
{
	for ( Int32 i=TCrEffector_First; i<TCrEffector_Last; ++i )
	{
		if ( m_effectorsPos[ i ] > 0.f )
		{
			return true;
		}
		else if ( m_effectorsRot[ i ] > 0.f )
		{
			return true;
		}
	}

	return false;
}

#ifndef NO_EDITOR
void TCrInstance::GetEffectorDefaultWS( Int32 id, RedQsTransform& transformWS ) const
{
	ASSERT( id < TCrEffector_Last && id >= 0 );
	ASSERT( IsSyncWS() );

	switch ( id )
	{
	case TCrEffector_HandL:
		transformWS = m_bonesWS[ TCrB_HandL ];
		break;

	case TCrEffector_HandR:
		transformWS = m_bonesWS[ TCrB_HandR ];
		break;

	default:
		transformWS = m_bonesWS[ TCrB_Root ];
		ASSERT( 0 );
		break;
	}
}

void TCrInstance::DrawSkeleton( CRenderFrame *frame, Bool overlay )
{
	Color color( 255, 255, 255 );

#define DRAW_BONE( a, b ) frame->AddDebugLine( AnimVectorToVector( m_bonesWS[ a ].Translation ), AnimVectorToVector( m_bonesWS[ b ].Translation ), color, overlay );

	DRAW_BONE( TCrB_Pelvis, TCrB_Torso1 );
	DRAW_BONE( TCrB_Torso1, TCrB_Torso2 );
	DRAW_BONE( TCrB_Torso2, TCrB_Torso3 );
	DRAW_BONE( TCrB_Torso3, TCrB_Neck );
	DRAW_BONE( TCrB_Neck, TCrB_Head );

	DRAW_BONE( TCrB_Torso3, TCrB_ShoulderL );
	DRAW_BONE( TCrB_ShoulderL, TCrB_BicepL );
	DRAW_BONE( TCrB_BicepL, TCrB_ForearmL );
	DRAW_BONE( TCrB_ForearmL, TCrB_HandL );
	DRAW_BONE( TCrB_Pelvis, TCrB_ThighL );
	DRAW_BONE( TCrB_ThighL, TCrB_ShinL );
	DRAW_BONE( TCrB_ShinL, TCrB_FootL );
	DRAW_BONE( TCrB_FootL, TCrB_ToeL );

	DRAW_BONE( TCrB_Torso3, TCrB_ShoulderR );
	DRAW_BONE( TCrB_ShoulderR, TCrB_BicepR );
	DRAW_BONE( TCrB_BicepR, TCrB_ForearmR );
	DRAW_BONE( TCrB_ForearmR, TCrB_HandR );
	DRAW_BONE( TCrB_Pelvis, TCrB_ThighR );
	DRAW_BONE( TCrB_ThighR, TCrB_ShinR );
	DRAW_BONE( TCrB_ShinR, TCrB_FootR );
	DRAW_BONE( TCrB_FootR, TCrB_ToeR );

#undef DRAW_BONE
}

#endif

void TCrInstance::ResetAllEffectors()
{
	const Uint32 eSize = ARRAY_COUNT( m_effectors );
	for ( Uint32 i=0; i<eSize; ++i )
	{
		m_effectors[ i ].SetIdentity();

		m_effectorsPos[ i ] = 0.f;
		m_effectorsRot[ i ] = 0.f;
	}
}

void TCrInstance::SetBoneLS( ETCrBoneId bone, const RedQsTransform& transform )
{
	ASSERT( bone < TCrB_Last && bone >= 0 );
	m_bonesLS[ bone ] = transform;
}

void TCrInstance::GetBoneLS( ETCrBoneId bone, RedQsTransform& transform ) const
{
	ASSERT( bone < TCrB_Last && bone >= 0 );
	transform = m_bonesLS[ bone ];
}

void TCrInstance::GetBoneMS( ETCrBoneId bone, RedQsTransform& transform ) const
{
	ASSERT( bone < TCrB_Last && bone >= 0 );
	transform = m_bonesMS[ bone ];
}

void TCrInstance::GetBoneWS( ETCrBoneId bone, RedQsTransform& transform ) const
{
	ASSERT( bone < TCrB_Last && bone >= 0 );
	transform = m_bonesWS[ bone ];
}

void TCrInstance::SetWeaponOffsetForHandLeft( Float weight )
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	m_weaponOffsetForHandL = weight;
}

void TCrInstance::SetWeaponOffsetForHandRight( Float weight )
{
	ASSERT( weight >= 0.f && weight <= 1.f );
	m_weaponOffsetForHandR = weight;
}

void TCrInstance::GenerateFragments( CRenderFrame* frame ) const
{
	m_solver.GenerateFragments( frame );
}
