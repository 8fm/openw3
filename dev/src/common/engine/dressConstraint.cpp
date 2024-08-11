#include "build.h"
#include "dressConstraint.h"
#include "skeleton.h"
#include "renderFrame.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Dress );

CAnimDangleConstraint_Dress::CAnimDangleConstraint_Dress()
	: m_ofweight( 0.8f )
	, m_p1( 0.1f, -0.11f, 0.f )
	, m_p2( 0.06f, -0.02f, 0.0f )
	, m_p3( 0.f, 0.0f, 0.0f )
	, m_r1( 0.f, 0.0f, 2.7f )
	, m_r2( 0.f, 0.0f, 12.8f )
	, m_r3( 0.f, 0.0f, -9.4f )
	, m_thighBoneWeight( 0.2f )
	, m_shinBoneWeight( 0.4f )
	, m_kneeRollBoneWeight( 0.8f )
{
}

CAnimDangleConstraint_Dress::~CAnimDangleConstraint_Dress()
{

}

void CAnimDangleConstraint_Dress::CacheBones( const CComponent* parent, const CSkeleton* skeleton )
{
	m_cachedAllBones = false;
	const CSkeleton* skel = GetSkeleton();
	if ( parent && skel )
	{
		m_indicesCache[0] = skel->FindBoneByName( "l_thigh" );
		m_indicesCache[1] = skel->FindBoneByName( "l_shin" );
		m_indicesCache[2] = skel->FindBoneByName( "l_kneeRoll" );
		m_indicesCache[3] = skel->FindBoneByName( "r_thigh" );
		m_indicesCache[4] = skel->FindBoneByName( "r_shin" );
		m_indicesCache[5] = skel->FindBoneByName( "r_kneeRoll" );
		m_indicesCache[6] = skel->FindBoneByName( "pelvis" );
		if ( skel )
		{
			m_skeletonModelSpace.Resize( skel->GetBonesNum() );
			m_skeletonWorldSpace.Resize( skel->GetBonesNum() );
			m_dirtyBones.Reserve( skel->GetBonesNum() );
		}
		else
		{
			m_skeletonModelSpace.Clear();
			m_skeletonWorldSpace.Clear();
			m_dirtyBones.Clear();
		}
		if( m_indicesCache[0]>=0 &&
			m_indicesCache[1]>=0 &&
			m_indicesCache[2]>=0 &&
			m_indicesCache[3]>=0 &&
			m_indicesCache[4]>=0 &&
			m_indicesCache[5]>=0 &&
			m_indicesCache[6]>=0 )
		{
			m_cachedAllBones = true;
		}
		else
		{
			m_cachedAllBones = false;
		}
	}
}

Bool CAnimDangleConstraint_Dress::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Dress::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	if ( GetSkeleton() && m_cachedAllBones )
	{
		for(Int32 i=0;i<7;i++)
		{
			if( m_indicesCache[i]>=0 )
			{
				Matrix mat = GetBoneWorldSpace( m_indicesCache[i] );
				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.2f, Color(0,255,0), true, true );
			}
		}
	}
}

Matrix CAnimDangleConstraint_Dress::RollMatrix( const Matrix & mat, Float val )
{
	Matrix rol = Matrix::IDENTITY;
	rol.SetRow( 1, Vector( 0.0f, cosf(val), sinf(val), 0.0f ) );
	rol.SetRow( 2, Vector( 0.0f, -sinf(val), cosf(val), 0.0f ) );
	return Matrix::Mul( mat, rol );
}

void CAnimDangleConstraint_Dress::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsWS( parent, poseLS, poseMS, poseWS, bones );
	if ( m_cachedAllBones && HasSkeletalMatrices() )
	{
		StartUpdateWithBoneAlignment( poseMS, poseWS, bones );
		{
			Vector pelv = GetBoneModelSpace( m_indicesCache[BI_Pelvis] ).GetRow( 0 );
			Vector left = GetBoneModelSpace( m_indicesCache[BI_ThighLeft] ).GetRow( 0 );
			Vector righ = GetBoneModelSpace( m_indicesCache[BI_ThighRight] ).GetRow( 0 );
			Float diff = -(Vector::Dot3( left, pelv ) - Vector::Dot3( righ, pelv ) );

			RedVector4 ltightLoc = poseLS->m_outputPose[ m_indicesCache[BI_ThighLeft]  ].ConvertToMatrix().GetRow(0);
			RedVector4 rtightLoc = poseLS->m_outputPose[ m_indicesCache[BI_ThighRight] ].ConvertToMatrix().GetRow(0);

			RedVector4 lshinLoc = poseLS->m_outputPose[ m_indicesCache[BI_ShinLeft]  ].ConvertToMatrix().GetRow(0);
			RedVector4 rshinLoc = poseLS->m_outputPose[ m_indicesCache[BI_ShinRight] ].ConvertToMatrix().GetRow(0);

			Vector target ( -1.0f, -1.0f, -1.0f, -1.0f );
			Vector current( ltightLoc.X, rtightLoc.X, lshinLoc.Y, rshinLoc.Y );
			Vector delta = target - current;
			Float add_weight = Vector::Dot3( delta, delta )*m_ofweight;

			add_weight = add_weight<0.0f ? 0.0f : add_weight;
			add_weight = add_weight>1.0f ? 1.0f : add_weight;
			add_weight = 1.0f - add_weight;


			EulerAngles rot1( m_r1.X*add_weight, m_r1.Y*add_weight, m_r1.Z*add_weight );
			EulerAngles rot2( m_r2.X*add_weight, m_r2.Y*add_weight, m_r2.Z*add_weight );
			EulerAngles rot3( m_r3.X*add_weight, m_r3.Y*add_weight, m_r3.Z*add_weight );

			Matrix mrot1 = rot1.ToMatrix(); mrot1.SetTranslation( m_p1.X*add_weight, m_p1.Y*add_weight, m_p1.Z*add_weight );
			Matrix mrot2 = rot2.ToMatrix(); mrot2.SetTranslation( m_p2.X*add_weight, m_p2.Y*add_weight, m_p2.Z*add_weight );
			Matrix mrot3 = rot3.ToMatrix(); mrot3.SetTranslation( m_p3.X*add_weight, m_p3.Y*add_weight, m_p3.Z*add_weight );



			Matrix tl = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_ThighLeft]    ),  mrot1) , diff*m_thighBoneWeight );
			Matrix tr = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_ThighRight]   ),  mrot1) , diff*m_thighBoneWeight );
			Matrix sl = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_ShinLeft]     ),  mrot2) , diff*m_shinBoneWeight );
			Matrix sr = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_ShinRight]    ),  mrot2) , diff*m_shinBoneWeight );
			Matrix kl = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_KneeRollLeft] ),  mrot3) , diff*m_kneeRollBoneWeight );
			Matrix kr = RollMatrix( Matrix::Mul( GetBoneModelSpace( m_indicesCache[BI_KneeRollRight]),  mrot3) , diff*m_kneeRollBoneWeight );

			m_dirtyBones.Reserve( BI_Size );

			SetBoneModelSpace( m_indicesCache[BI_ThighLeft], tl );
			SetBoneModelSpace( m_indicesCache[BI_ThighRight], tr );
			SetBoneModelSpace( m_indicesCache[BI_ShinLeft], sl );
			SetBoneModelSpace( m_indicesCache[BI_ShinRight], sr );
			SetBoneModelSpace( m_indicesCache[BI_KneeRollLeft], kl );
			SetBoneModelSpace( m_indicesCache[BI_KneeRollRight], kr );
		}
		EndUpdate( poseMS, poseWS, bones );
	}
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
