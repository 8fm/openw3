
#include "build.h"
#include "collarConstraint.h"
#include "skeleton.h"
#include "renderFrame.h"
#include "behaviorIncludes.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Collar );

RED_DEFINE_STATIC_NAME( l_collar_01 )
RED_DEFINE_STATIC_NAME( l_collar_02 )
RED_DEFINE_STATIC_NAME( b_collar )
RED_DEFINE_STATIC_NAME( r_collar_01 )
RED_DEFINE_STATIC_NAME( r_collar_02 )
RED_DEFINE_STATIC_NAME( f_collar )

CAnimDangleConstraint_Collar::CAnimDangleConstraint_Collar() 
	: CAnimSkeletalDangleConstraint()
	, m_offset( 0.13f, 0.08f, 0.0f, 1.0f )
	, m_radius( 0.16f )
	, m_offset2( 0.13f, 0.08f, 0.0f, 1.0f )
	, m_radius2( 0.16f )
	, m_cachedAllBones( false )
{
}

Bool CAnimDangleConstraint_Collar::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Collar::CacheBones( const CComponent* parentComp, const CSkeleton* skeleton )
{
	const ISkeletonDataProvider* parent = parentComp ? parentComp->QuerySkeletonDataProvider() : nullptr;
	if ( parent && skeleton )
	{
		m_headIndex = parent->FindBoneByName( CNAME( head ) );

		m_colloar[0] = skeleton->FindBoneByName( CNAME( l_collar_01 ) );
		m_colloar[1] = skeleton->FindBoneByName( CNAME( l_collar_02 ) );
		m_colloar[2] = skeleton->FindBoneByName( CNAME( b_collar ) );
		m_colloar[3] = skeleton->FindBoneByName( CNAME( r_collar_01 ) );
		m_colloar[4] = skeleton->FindBoneByName( CNAME( r_collar_02 ) );
		m_colloar[5] = skeleton->FindBoneByName( CNAME( f_collar ) );

		m_cachedAllBones = m_headIndex != -1;

		for ( Uint32 i=0; i<6; ++i )
		{
			if ( m_colloar[i] != -1 )
			{
				const Int32 parentIdx = skeleton->GetParentBoneIndex( m_colloar[i] );
				if( parentIdx != -1 )
				{
					const String parentName = skeleton->GetBoneName( parentIdx );
					m_colloarParents[i] = parent->FindBoneByName( parentName.AsChar() );
				}
				else
				{
					m_cachedAllBones = false;
				}
			}
			else
			{
				m_cachedAllBones = false;
			}
		}
	}
	else
	{
		m_headIndex = -1;

		for( Uint32 i=0; i<6; ++i )
		{
			m_colloar[i] = -1;
			m_colloarParents[i] = -1;
		}

		m_cachedAllBones = false;
	}
}

void CAnimDangleConstraint_Collar::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_cachedAllBones )
	{
		frame->AddDebugSphere( m_spherePos, m_radius, Matrix::IDENTITY, Color(255,0,0) );
		frame->AddDebugSphere( m_spherePos2, m_radius2, Matrix::IDENTITY, Color(255,0,0) );

		for( Uint32 i=0; i<6; ++i )
		{
			ASSERT( m_colloar[i] != -1 );

			if ( m_colloar[i] < (Int32)GetNumBonesWorldSpace() )
			{
				const Matrix& boneWS = GetBoneWorldSpace( m_colloar[i] );
				frame->AddDebug3DArrow( boneWS.GetTranslation(), boneWS.GetAxisX(), 0.1f, 0.001f, 0.002f, 0.01f, Color(255,0,0), Color(255,0,0) );
			}
		}
	}
}

void CAnimDangleConstraint_Collar::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsWS(parent, poseLS, poseMS, poseWS, bones );
	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );

	const CSkeleton* skeleton = GetSkeleton();

	if ( m_cachedAllBones && HasSkeletalMatrices() && poseMS && skeleton )
	{
		ASSERT( m_headIndex != -1 );

		if ( m_headIndex < poseMS->SizeInt() )
		{
			const Matrix& headMS = (*poseMS)[ m_headIndex ];
			m_spherePos =  headMS.TransformPoint( m_offset );
			m_spherePos2 = headMS.TransformPoint( m_offset2 );

			for( Uint32 i=0; i<6; ++i )
			{
				const Int32 boneIdx = m_colloar[i];

				ASSERT( boneIdx != -1 );

				const AnimQsTransform qstm = skeleton->GetBoneLS( boneIdx );
				Matrix mat;

#ifdef USE_HAVOK_ANIMATION
				HavokTransformToMatrix_Renormalize( qstm, &mat );
#else
				RedMatrix4x4 convertedMatrix = qstm.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( convertedMatrix );
#endif

				m_localBones[ i ] = mat;

				const Matrix newBoneMS = CalculateOffsets( Matrix::Mul( (*poseMS)[m_colloarParents[i]], m_localBones[i] ), m_spherePos, m_radius, m_spherePos2, m_radius2 );
				ASSERT( newBoneMS.IsOk() );
				SetBoneModelSpace( boneIdx, newBoneMS );
			}
		}
	}

	EndUpdate( poseMS, poseWS, bones );
}

Matrix CAnimDangleConstraint_Collar::CalculateOffsets( const Matrix & boneTM, const Vector & sphPos, Float r, const Vector & sphPos2, Float r2 ) const
{
	Matrix mat( boneTM );

	//check weather its inside

	const Vector del =  mat.GetRow(3) - sphPos;
	const Vector del2 = mat.GetRow(3) - sphPos2;
	if ( del.SquareMag3() < r*r || del.SquareMag3() < r2*r2 )
	{
		Matrix rot( Matrix::IDENTITY );

		rot.V[0].X = cosf( 1.0472f );
		rot.V[0].Y = sinf( 1.0472f );

		rot.V[1].X = -rot.V[0].Y;
		rot.V[1].Y = rot.V[0].X;

		return Matrix::Mul( boneTM, rot );
	}
	else
	{
		mat.Invert();
		// Invertion of modelspace bone transform

		const Vector p = mat.TransformPoint( sphPos );
		const Vector p2 = mat.TransformPoint( sphPos2 );
		// This is Position of the sliced circle

		const Float rad = sqrt( Max( 0.0f, (r*r) - (p.Z*p.Z ) ) );
		const Float rad2 = sqrt( Max( 0.0f, (r2*r2) - (p2.Z*p2.Z ) ) );
		// This is radius of the sliced pice

		const Vector toSphere = Vector( p.X, p.Y, 0.0f, 1.0f);
		const Vector toSphere2 = Vector( p2.X, p2.Y, 0.0f, 1.0f);
		const Float z = toSphere.Mag3();
		const Float z2 = toSphere2.Mag3();
		//Projection of sphere position on xy axis, z is length of this projection

		Matrix wyn = boneTM;

		Float ang = 0.0f;
		Float ang2 = 0.0f;
		if ( p.Y < 0.0f && p.Y > -rad )
		{
			Float w = sqrt( Max( 0.0f, (z*z) - (rad*rad) ) );
			Float ang1 = acos( w/z );
			Float ang2 = acos( p.X/z );
			Float anga = ( ang1 - ang2 );
			Float angb = ( ang2 - ang1 );
			ang = anga>angb ? -angb : -anga;
			ang = ang>1.0472f ? 1.0472f : ang;
		}
		else if ( p.Y >= 0.0f )
		{
			Float w = sqrt( Max( 0.0f, (z*z) - (rad*rad) ) );
			Float ang1 = -acos( w/z );
			Float ang2 = acos( p.X/z );
			Float anga = ( ang1 - ang2 );
			Float angb = ( ang2 - ang1 );
			ang = anga>angb ? -angb : -anga;
			ang = ang>1.0472f ? 1.0472f : ang;
		}

		if ( p2.Y < 0.0f && p2.Y > -rad2 )
		{
			Float w2 = sqrt( Max( 0.0f, (z2*z2) - (rad2*rad2) ) );
			Float ang12 = acos( w2/z2 );
			Float ang22 = acos( p2.X/z2 );
			Float anga2 = ( ang12 - ang22 );
			Float angb2 = ( ang22 - ang12 );
			ang2 = anga2>angb2 ? -angb2 : -anga2;
			ang2 = ang2>1.0472f ? 1.0472f : ang2;
		}
		else if ( p2.Y >= 0.0f )
		{
			Float w2 = sqrt( Max( 0.0f, (z2*z2) - (rad2*rad2) ) );
			Float ang12 = -acos( w2/z2 );
			Float ang22 = acos( p2.X/z2 );
			Float anga2 = ( ang12 - ang22 );
			Float angb2 = ( ang22 - ang12 );
			ang2 = anga2>angb2 ? -angb2 : -anga2;
			ang2 = ang2>1.0472f ? 1.0472f : ang2;
		}

		if( fabs(ang)>fabs(ang2) )
		{
			Matrix rot = Matrix::IDENTITY;
			rot.V[0].X = cosf( ang );
			rot.V[0].Y = sinf( ang );
			rot.V[1].X = -rot.V[0].Y;
			rot.V[1].Y = rot.V[0].X;
			wyn = Matrix::Mul( boneTM, rot );
		}
		else
		{
			Matrix rot = Matrix::IDENTITY;
			rot.V[0].X = cosf( ang2 );
			rot.V[0].Y = sinf( ang2 );
			rot.V[1].X = -rot.V[0].Y;
			rot.V[1].Y = rot.V[0].X;
			wyn = Matrix::Mul( boneTM, rot );
		}

		return wyn;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Pusher );

CAnimDangleConstraint_Pusher::CAnimDangleConstraint_Pusher() 
	: CAnimSkeletalDangleConstraint()
	, m_offset( 0.4f, 0.0f, 0.0f, 1.0f )
	, m_radius( 0.16f )
	, m_cachedAllBones( false )
	, m_maxAngle( 90.0f )
{
}

Bool CAnimDangleConstraint_Pusher::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Pusher::CacheBones( const CComponent* parentComp, const CSkeleton* skeleton )
{
	const CSkeleton* skel = GetSkeleton();
	if ( skel )
	{
		m_index = skel->FindBoneByName( ( m_boneName.AsChar() ) );
		m_collindex = skel->FindBoneByName( ( m_collisionName.AsChar() ) );
		m_cachedAllBones = false;
		if( m_index>=0 && m_collindex>=0 )
		{
			m_skeletonModelSpace.Resize( skel->GetBonesNum() );
			m_skeletonWorldSpace.Resize( skel->GetBonesNum() );
			m_dirtyBones.Reserve( skel->GetBonesNum() );
			m_cachedAllBones = true;
		}
	}
	else
	{
		m_index = -1;
		m_collindex = -1;
		m_cachedAllBones = false;
	}
}

void CAnimDangleConstraint_Pusher::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_cachedAllBones )
	{
		frame->AddDebugSphere( m_spherePos, m_radius, Matrix::IDENTITY, Color(255,0,0) );
		//Matrix rot = m_boneTransform;
		//Vector pos = rot.GetTranslation();
		//rot.SetTranslation( 0.0f, 0.0f, 0.f );
		//frame->AddDebugAxis( pos, rot, 1.0f, Color(255,255,0) );
		frame->AddDebug3DArrow( m_boneTransform.GetTranslation(), m_boneTransform.GetAxisX(), 0.5f, 0.01f, 0.02f, 0.1f, Color(255,0,0), Color(255,0,0) );
	}
}

void CAnimDangleConstraint_Pusher::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	StartUpdateWithBoneAlignmentFull( poseMS, poseWS, bones );

	const CSkeleton* skeleton = GetSkeleton();
	if ( m_cachedAllBones && HasSkeletalMatrices() && poseMS && skeleton )
	{
		const Matrix& collMS = m_skeletonModelSpace[ m_collindex ];
		m_spherePos = collMS.TransformPoint( m_offset );
		m_boneTransform = CalculateOffsets( m_skeletonModelSpace[ m_index ], m_spherePos, m_radius );
		SetBoneModelSpace( m_index, m_boneTransform );
	}

	EndUpdate( poseMS, poseWS, bones );
}

Matrix CAnimDangleConstraint_Pusher::CalculateOffsets( const Matrix & boneTM, const Vector & sphPos, Float r ) const
{
	Matrix mat( boneTM );

	//check weather its inside
	Float maxa = DEG2RAD( m_maxAngle );

	const Vector del = mat.GetRow(3) - sphPos;
	if ( del.SquareMag3() < r*r )
	{
		Matrix rot( Matrix::IDENTITY );

		rot.V[0].X = cosf( maxa );
		rot.V[0].Y = sinf( maxa );

		rot.V[1].X = -rot.V[0].Y;
		rot.V[1].Y = rot.V[0].X;

		return Matrix::Mul( boneTM, rot );
	}
	else
	{
		mat.Invert();
		// Invertion of modelspace bone transform

		const Vector p = mat.TransformPoint( sphPos );
		// This is Position of the sliced circle

		const Float rad = sqrt( Max( 0.0f, (r*r) - (p.Z*p.Z ) ) );
		// This is radius of the sliced pice

		const Vector toSphere = Vector( p.X, p.Y, 0.0f, 1.0f);
		const Float z = toSphere.Mag3();
		//Projection of sphere position on xy axis, z is length of this projection

		Matrix wyn = boneTM;

		if ( p.Y < 0.0f && p.Y > -rad )
		{
			Float w = sqrt( Max( 0.0f, (z*z) - (rad*rad) ) );
			Float ang1 = acos( w/z );
			Float ang2 = acos( p.X/z );
			Float anga = ( ang1 - ang2 );
			Float angb = ( ang2 - ang1 );

			Float ang = anga>angb ? -angb : -anga;
			ang = ang>maxa ? maxa : ang;

			Matrix rot = Matrix::IDENTITY;
			rot.V[0].X = cosf( ang );
			rot.V[0].Y = sinf( ang );

			rot.V[1].X = -rot.V[0].Y;
			rot.V[1].Y = rot.V[0].X;

			wyn = Matrix::Mul( boneTM, rot );
		}
		else if ( p.Y >= 0.0f )
		{
			Float w = sqrt( Max( 0.0f, (z*z) - (rad*rad) ) );
			Float ang1 = -acos( w/z );
			Float ang2 = acos( p.X/z );
			Float anga = ( ang1 - ang2 );
			Float angb = ( ang2 - ang1 );

			Float ang = anga>angb ? -angb : -anga;
			ang = ang>maxa ? maxa : ang;

			Matrix rot = Matrix::IDENTITY;
			rot.V[0].X = cosf( ang );
			rot.V[0].Y = sinf( ang );

			rot.V[1].X = -rot.V[0].Y;
			rot.V[1].Y = rot.V[0].X;

			wyn = Matrix::Mul( boneTM, rot );
		}
		return wyn;
	}
}

//////////////////////////////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_NobleDressFix );

CAnimDangleConstraint_NobleDressFix::CAnimDangleConstraint_NobleDressFix() 
	: IAnimDangleConstraint()
	, m_boneNameA( TXT("dress_back_01") )
	, m_boneNameB( TXT("dress_back_02") )
	, m_boneAxisA( A_Z )
	, m_boneAxisB( A_Z )
	, m_boneValueA( -12.f )
	, m_boneValueB( -11.2f )
	, m_boneIdxA( -1 )
	, m_boneIdxB( -1 )
	, m_cachedBones( false )
{
}

void CAnimDangleConstraint_NobleDressFix::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	m_cachedBones = false;
	m_boneIdxA = -1;
	m_boneIdxB = -1;
}

Uint32 CAnimDangleConstraint_NobleDressFix::GetRuntimeCacheIndex() const
{
	return 0;
}

const struct SSkeletonSkeletonCacheEntryData* CAnimDangleConstraint_NobleDressFix::GetSkeletonMaping( const ISkeletonDataProvider* parentSkeleton ) const
{
	return &SSkeletonSkeletonCacheEntryData::FAKE_DATA;
}

namespace
{
	void RotateBone( const EAxis axis, const Float angle, RedQsTransform& boneLS )
	{
		const RedVector4 rAxis = BehaviorUtils::RedVectorFromAxis( axis );

		const RedQuaternion quat( rAxis, DEG2RAD( angle ) );
		const RedQsTransform trans( RedVector4(0.0f, 0.0f, 0.0f, 0.0f ), quat, RedVector4(1.0f, 1.0f, 1.0f, 1.0f ) );

		boneLS.SetMul( boneLS, trans );
	}
}

void CAnimDangleConstraint_NobleDressFix::AddCorrection( SBehaviorGraphOutput* poseLS )
{
	if ( !m_cachedBones )
	{
		m_boneIdxA = -1;
		m_boneIdxB = -1;

		const CComponent* parent = GetComponent();
		while ( const CAnimatedAttachment* att = Cast< const CAnimatedAttachment >( parent->GetTransformParent() ) )
		{
			const CComponent* attPar = att->GetParent()->AsComponent();
			if ( attPar )
			{
				parent = attPar;
			}
		}
		if ( const ISkeletonDataProvider* prov = parent->QuerySkeletonDataProvider() )
		{
			m_boneIdxA = prov->FindBoneByName( m_boneNameA.AsChar() );
			m_boneIdxB = prov->FindBoneByName( m_boneNameB.AsChar() );
		}

		m_cachedBones = true;
	}

	if ( m_boneIdxA != -1 && m_boneIdxB != -1 )
	{
		RedQsTransform& boneA = poseLS->m_outputPose[ m_boneIdxA ];
		RedQsTransform& boneB = poseLS->m_outputPose[ m_boneIdxB ];

		RotateBone( m_boneAxisA, m_boneValueA, boneA );
		RotateBone( m_boneAxisB, m_boneValueB, boneB );
	}
}

void CAnimDangleConstraint_NobleDressFix::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	AddCorrection( poseLS );
}

void CAnimDangleConstraint_NobleDressFix::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	AddCorrection( poseLS );
}

void CAnimDangleConstraint_NobleDressFix::OnParentUpdatedAttachedAnimatedObjectsLSSync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	AddCorrection( poseLS );
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
