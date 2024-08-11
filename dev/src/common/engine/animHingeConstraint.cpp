
#include "build.h"
#include "animHingeConstraint.h"
#include "skeleton.h"
#include "renderFrame.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Hinge );

CAnimDangleConstraint_Hinge::CAnimDangleConstraint_Hinge() 
	: CAnimSkeletalDangleConstraint()
	, m_radius( 0.16f )
	, m_cachedAllBones( false )
	, m_limit( 20.0f )
	, m_bounce( 0.9f )
	, m_damp( 0.99f )
	, m_min( -0.5f )
	, m_max(  0.5f )
	, m_inertia( 1.0f )
	, m_gravity( 1.0f )
	, m_spring( 0.0f )
	, m_forceReset( false )
{
}

Bool CAnimDangleConstraint_Hinge::HasCachedBones() const
{
	return m_cachedAllBones;
}

#ifndef NO_EDITOR
void CAnimDangleConstraint_Hinge::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( GetParentComponent() && GetSkeleton() )
	{
		Int32 ind = GetSkeleton()->FindBoneByName( m_name.AsChar() );
		Int32 pind = GetSkeleton()->GetParentBoneIndex( ind );
		if( ind>=0 && pind>=0 )
		{
			const AnimQsTransform qtm = GetSkeleton()->GetBoneLS( ind );
			RedMatrix4x4 convertedMatrix = qtm.ConvertToMatrixNormalized();
			m_hinge.Setup( ind, pind, reinterpret_cast< Matrix& >( convertedMatrix ) );
			m_cachedAllBones = true;
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
#endif

void CAnimDangleConstraint_Hinge::CacheBones( const CComponent* parent, const CSkeleton* skeleton )
{
	if ( skeleton )
	{
		Int32 ind = skeleton->FindBoneByName( m_name.AsChar() );
		Int32 pind = skeleton->GetParentBoneIndex( ind );
		if( ind>=0 && pind>=0 )
		{
			const AnimQsTransform qtm = skeleton->GetBoneLS( ind );
			RedMatrix4x4 convertedMatrix = qtm.ConvertToMatrixNormalized();
			m_hinge.Setup( ind, pind, reinterpret_cast< Matrix& >( convertedMatrix ) );
			m_cachedAllBones = true;
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

void CAnimDangleConstraint_Hinge::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );
	m_dt = dt*1.0f;
}

void CAnimDangleConstraint_Hinge::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLSAsync( dt, poseLS, bones );
	m_dt = dt*1.0f;
}

void CAnimDangleConstraint_Hinge::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_cachedAllBones )
	{
		Float scale = m_radius;
		const Matrix& boneWS = GetBoneWorldSpace( (m_hinge.getParentIndex()) );
		const Matrix& boneLS = m_hinge.getLocalTransform();
		Matrix global = Matrix::Mul( boneWS, boneLS );
		frame->AddDebugAxis( global.GetTranslation(), global, 0.5f*scale );

		Vector a( -sinf(m_hinge.GetValue())*scale, cosf(m_hinge.GetValue())*scale, 0.0f );
		Vector p( cosf(m_hinge.GetValue())*scale, sinf(m_hinge.GetValue())*scale, 0.0f );
		Vector d = p+a*m_hinge.GetAcc();

		frame->AddDebugSphere( global.TransformPoint( p ), 0.03f*scale, Matrix::IDENTITY, Color(255,255,255) );
		draw_ellipse( global, m_hinge.GetLowwerBound(), m_hinge.GetUpperBound(), frame, scale );

		frame->AddDebugLine( global.TransformPoint( p ), global.TransformPoint( d ), Color(255,255,255) );

		Matrix mat = Matrix::IDENTITY;
		mat.V[0].A[0] = cosf(m_hinge.GetValue());
		mat.V[0].A[1] = sinf(m_hinge.GetValue());

		mat.V[1].A[0] = -sinf(m_hinge.GetValue());
		mat.V[1].A[1] = cosf(m_hinge.GetValue());

		Matrix wyn = Matrix::Mul( global, mat );

		frame->AddDebugAxis( wyn.GetTranslation(), wyn, 0.5f*scale );
		
	}
}

void CAnimDangleConstraint_Hinge::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );

	const CSkeleton* skeleton = GetSkeleton();

	bool forceReset = m_forceReset;
	m_forceReset = false;

	if( forceReset )
	{
		m_hinge.reset();
	}

	if ( m_cachedAllBones && HasSkeletalMatrices() && poseMS && skeleton )
	{
		const Matrix& boneWS = GetBoneModelSpace( (m_hinge.getParentIndex()) );
		const Matrix& boneLS = m_hinge.getLocalTransform();
		Matrix global = Matrix::Mul( boneWS, boneLS );

		m_hinge.Values( m_limit*100.0f, m_bounce, m_damp, m_min, m_max, m_inertia*100.0f, m_gravity*100.0f, m_spring*100.0f );
		Matrix mat = m_hinge.Evaluate( m_dt, global, forceReset );
		SetBoneModelSpace( m_hinge.getIndex(), Matrix::Mul( global, mat ) );
	}

	EndUpdate( poseMS, poseWS, bones );
}

void CAnimDangleConstraint_Hinge::draw_ellipse( const Matrix & mat, Float from, Float to, CRenderFrame* frame, Float scale )
{
	Vector p0 = mat.TransformPoint(Vector( cosf(from)*scale, sinf(from)*scale, 0.0f ) );
	frame->AddDebugLine( p0, mat.GetTranslation(), Color::YELLOW );
	Int32 num = Int32( (fabs(to-from)/0.1f)+1.0f );
	Float delta = (fabs(to-from)/Float(num));
	for( Float a = from; a <= to; a += delta )
	{
		Vector p = mat.TransformPoint(Vector( cosf(a)*scale, sinf(a)*scale, 0.0f ));
		frame->AddDebugLine( p0, p, Color::YELLOW );
		p0 = p;
	}
	Vector p2 = mat.TransformPoint(Vector( cosf(to)*scale, sinf(to)*scale, 0.0f ) );
	frame->AddDebugLine( p0, p2, Color::YELLOW );
	frame->AddDebugLine( p2, mat.GetTranslation(), Color::YELLOW );
}

void CAnimDangleConstraint_Hinge::ForceReset()
{
	m_forceReset = true;
}
void CAnimDangleConstraint_Hinge::ForceResetWithRelaxedState()
{
	m_forceReset = true;
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
