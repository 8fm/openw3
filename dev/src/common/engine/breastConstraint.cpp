#include "build.h"
#include "breastConstraint.h"
#include "animatedAttachment.h"
#include "skeleton.h"
#include "renderFrame.h"
#include "baseEngine.h"

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",off)
#endif

IMPLEMENT_RTTI_ENUM( EBreastPreset );

namespace AnimDangleConstraints
{
////////////////////////////////////////
// simulator

simulator::simulator()
{
	m_velDamp = 1.0f;
	m_bounceDamp = 1.0f;
	m_a = 1.0f;
	m_blackHoleGravity = 1.0f;
	m_shape = Vector( 0.0f, 0.0f, 0.3f, 0.3f );
	m_pointPosVel = Vector( 0.0f, 0.0001f, 0.0f, 0.0001f );
	m_velClamp = 4.0f;
	m_inertiaScale = 10.0f;
}

void simulator::Simulate( Float dt )
{
	const Float pot = CalcPotential();
	if ( pot > 0.f )
	{
		Float nx, ny;
		CalcNormal( nx, ny );
		m_pointPosVel.Z += nx/dt*m_a;
		m_pointPosVel.W += ny/dt*m_a;
	}
	Float xl = sqrt((m_shape.X - m_pointPosVel.X)*(m_shape.X - m_pointPosVel.X) + (m_shape.Y - m_pointPosVel.Y) * (m_shape.Y - m_pointPosVel.Y));
	if ( xl > 0.f )
	{
		Float tempLen = xl*xl;
		m_pointPosVel.Z += ((0.0f - m_pointPosVel.X) / xl) * m_blackHoleGravity * tempLen / dt * 100.0f;
		m_pointPosVel.W += ((0.0f - m_pointPosVel.Y) / xl) * m_blackHoleGravity * tempLen / dt * 100.0f;
	}
	m_pointPosVel.Z *= m_velDamp;
	m_pointPosVel.W *= m_velDamp;
	const Float currentVel = sqrt( m_pointPosVel.Z*m_pointPosVel.Z + m_pointPosVel.W*m_pointPosVel.W );
	if ( currentVel > m_velClamp )
	{
		m_pointPosVel.Z = (m_pointPosVel.Z / currentVel)*m_velClamp;
		m_pointPosVel.W = (m_pointPosVel.W / currentVel)*m_velClamp;
	}
	MovePoint( dt );
}

void simulator::ApplyAcc( Float gx, Float gy )
{
	m_pointPosVel.Z += gx;
	m_pointPosVel.W += gy;
}

Float simulator::Collision( )
{
	const Float a = m_shape.Z;
	const Float b = m_shape.W;
	const Float px = m_pointPosVel.X - m_shape.X;
	const Float py = m_pointPosVel.Y - m_shape.Y;
	const Float dx = m_pointPosVel.Z;
	const Float dy = m_pointPosVel.W;
	const Float av = b*b * dx*dx + a*a * dy*dy;
	const Float bv = 2.f*b*b*px*dx + 2.f*a*a*py*dy;
	const Float cv = b*b * px*px + a*a * py*py - a*a * b*b;
	const Float del = bv*bv - 4.f*av*cv;
	if ( del > 0.0f )
	{
		Float x1 = ( (-bv - sqrt( del ))/(2.f*av) );
		Float x2 = ( (-bv + sqrt( del ))/(2.f*av) );
		Float res = ( x2>x1 ) ? x2 : x1;
		if ( res >= 0.f )
		{
			return res;
		}
		else
		{
			return -1.f;
		}
	}
	return -1.f;
}

void simulator::Bounce( Float nx, Float ny )
{
	const Float velx = -m_pointPosVel.Z;
	const Float vely = -m_pointPosVel.W;
	const Float fakeDot = nx*velx + ny*vely;
	const Float vx = velx + 2.f*(nx*fakeDot-velx);
	const Float vy = vely + 2.f*(ny*fakeDot-vely);
	m_pointPosVel.Z = vx*m_bounceDamp;
	m_pointPosVel.W = vy*m_bounceDamp;
}

void simulator::MovePoint( Float dt )
{
	const Float a = m_shape.Z;
	const Float b = m_shape.W;
	RED_UNUSED( a );
	RED_UNUSED( b );
	const Float collTime = Collision();
	if ( collTime > 0.f && collTime <= dt )
	{
		m_pointPosVel.X += m_pointPosVel.Z*collTime;
		m_pointPosVel.Y += m_pointPosVel.W*collTime;

		Float restTime = dt - collTime;
		Float nx;
		Float ny;
		CalcNormal( nx, ny );
		Bounce( nx, ny );
		MovePoint( restTime );
	}
	else
	{
		m_pointPosVel.X += m_pointPosVel.Z*dt;
		m_pointPosVel.Y += m_pointPosVel.W*dt;
	}
}

Float simulator::CalcPotential()
{
	const Float a = m_shape.Z;
	const Float b = m_shape.W;
	const Float p1 = powf( (m_pointPosVel.X - m_shape.X), 2.f) / (a*a);
	const Float p2 = powf( (m_pointPosVel.Y - m_shape.Y), 2.f) / (b*b);
	return p1+p2-1.f;
}

void simulator::CalcNormal( Float &nx, Float &ny )
{
	Float nnx = nx;
	Float nny = ny;
	const Float a = m_shape.Z;
	const Float b = m_shape.W;
	nnx = 2.f * (m_pointPosVel.X - m_shape.X) / (a*a);
	nny = 2.f * (m_pointPosVel.Y - m_shape.Y) / (b*b);
	const Float len = sqrt( nnx*nnx + nny*nny );
	if ( len > 0.f )
	{
		nnx /= -len;
		nny /= -len;
	}
	nx = nnx;
	ny = nny;
}

///////////////////////////////////////////////////
//BoneSimulator

BoneSimulator::BoneSimulator()
	: m_simulator()
	, m_index( -1 )
	, m_parent_index( -1 )
	, m_screen( Vector( 0.0f, 0.0f, 1.0f, 0.0f ), Vector( 0.0f, -1.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f, 0.0f ), Vector( 1.0f, 0.0f, 0.0f, 1.0f ) )
{
}

BoneSimulator::~BoneSimulator()
{

}

void BoneSimulator::set( const Matrix & loc, Int32 ind, Int32 par )
{
	m_index = ind;
	m_parent_index = par;
	m_local = loc;
	m_global = loc;
}

void BoneSimulator::Reset( const Matrix& loc )
{
	m_global = loc;
	m_position = m_global.GetTranslation();
	m_velocity = Vector::ZEROS;
	m_acceleration = Vector::ZEROS;
}

void BoneSimulator::evaluate( Matrix& out, const Matrix & parMS, Float dt, bool reset, Float startSimPointOffset, Int32 numIt )
{
	if ( dt > 0.0f )
	{
		out = Matrix::Mul( parMS, m_local );
		m_global = Matrix::Mul( out, m_screen );

		Vector vel = m_global.GetTranslation() - m_position;
		m_position = m_global.GetTranslation();
		m_acceleration = vel - m_velocity;
		m_velocity = vel;

		Float g = m_simulator.m_gravity;
		Float grx = m_global.GetRow(0).Z*g;
		Float gry = m_global.GetRow(1).Z*g;
		m_gravity = Vector(grx,gry,0.0f);

		if( reset )
		{
			m_simulator.m_pointPosVel = Vector::ZEROS;
			m_simulator.m_pointPosVel.X = 0.0f;
			m_simulator.m_pointPosVel.Y = -startSimPointOffset;
			m_simulator.m_pointPosVel.W = 0.0001f;
			m_velocity = Vector::ZEROS;
			m_acceleration = Vector::ZEROS;
		}

		if( !reset )
		{
			Float accx = -m_simulator.m_inertiaScale*( m_acceleration.X*m_global.GetRow(0).X + m_acceleration.Y*m_global.GetRow(0).Y + m_acceleration.Z*m_global.GetRow(0).Z );
			Float accy = -m_simulator.m_inertiaScale*( m_acceleration.X*m_global.GetRow(1).X + m_acceleration.Y*m_global.GetRow(1).Y + m_acceleration.Z*m_global.GetRow(1).Z );

			m_inertia = Vector( accx, accy, 0.0f);
			m_simulator.ApplyAcc( accx/dt, accy/dt );
		}
		
		Int32 k;
		for( k=0;k<numIt;++k )
		{
			Float ndt = numIt==1 ? dt : 0.1f;
			m_simulator.ApplyAcc( grx/ndt, gry/ndt );
			m_simulator.Simulate( ndt );
		}

		m_target = m_global.TransformPoint( Vector( m_simulator.m_pointPosVel.X, m_simulator.m_pointPosVel.Y+m_simulator.m_simPointOffset, 0.0f ) );

		Vector off( 0.0f, 0.0f, m_simulator.m_simPointOffset, 0.0f );
		out.SetRow( 0, out.GetRow(0) + (out.GetRow(2)*m_simulator.m_pointPosVel.X + out.GetRow(1)*-m_simulator.m_pointPosVel.Y)*m_simulator.m_rotationWeight );
		out.SetRow( 0, out.GetRow(0) + off );
		out.SetRow( 3, out.GetRow(3) + (out.GetRow(2)*m_simulator.m_pointPosVel.X + out.GetRow(1)*(-m_simulator.m_pointPosVel.Y-m_simulator.m_simPointOffset))*m_simulator.m_translationWeight );
	}
	else
	{
		ASSERT( dt <= 0.0f );
	}
}
}
///////////////////////////////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Breast );

CAnimDangleConstraint_Breast::CAnimDangleConstraint_Breast() 
	: CAnimSkeletalDangleConstraint()
	, m_simTime( 1.0f )
	, m_elA( 0.0f,0.0f,0.3f,0.3f )
	, m_velDamp ( 1.0f )
	, m_bounceDamp( 1.0f )
	, m_inAcc( 1.0f )
	, m_inertiaScaler( 1.0f )
	, m_blackHole( 1.0f )
	, m_velClamp( 4.0f )
	, m_gravity( -0.327f )
	, m_movementBoneWeight( 1.0f )
	, m_rotationBoneWeight( 1.0f )
	, m_startSimPointOffset( 0.0f )
	, m_forceReset( false )
	, m_cachedAllBones( false ) 
{
}
void CAnimDangleConstraint_Breast::SetPresetValues( Uint32 p )
{
	m_simTime = breastPreset[p].simTime;
	m_elA.X = breastPreset[p].elAX;
	m_elA.Y = breastPreset[p].elAY;
	m_elA.Z = breastPreset[p].elAZ;
	m_elA.W = breastPreset[p].elAW;
	m_velDamp = breastPreset[p].velDamp;
	m_bounceDamp = breastPreset[p].bounceDamp;
	m_inAcc = breastPreset[p].inAcc;
	m_inertiaScaler = breastPreset[p].inertiaScaler;
	m_blackHole = breastPreset[p].blackHole;
	m_velClamp = breastPreset[p].velClamp;
	m_gravity = breastPreset[p].grav;
	m_movementBoneWeight = breastPreset[p].moveWeight;
	m_rotationBoneWeight = breastPreset[p].rotWeight;
	m_startSimPointOffset = breastPreset[p].offset;

	m_bones[0].load_preset( p );
	m_bones[1].load_preset( p );

}

#ifndef NO_EDITOR
void CAnimDangleConstraint_Breast::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if( m_cachedAllBones )
	{
		if( m_preset == CUSTOM_PRESET )
		{
			m_bones[0].params( m_elA, m_velDamp, m_bounceDamp, m_inAcc, m_blackHole, m_velClamp, m_gravity, m_movementBoneWeight, m_rotationBoneWeight, m_inertiaScaler, m_startSimPointOffset );
			m_bones[1].params( m_elA, m_velDamp, m_bounceDamp, m_inAcc, m_blackHole, m_velClamp, m_gravity, m_movementBoneWeight, m_rotationBoneWeight, m_inertiaScaler, m_startSimPointOffset );
		}
		else
		{
			SetPresetValues( m_preset );
		}
	}
}
#endif

Bool CAnimDangleConstraint_Breast::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Breast::CacheBones( const CComponent* parent, const CSkeleton* skeleton )
{
	m_cachedAllBones = false;
	if ( parent && skeleton )
	{
		Int32 booblind = skeleton->FindBoneByName( "l_boob" );
		Int32 boobrind = skeleton->FindBoneByName( "r_boob" );

		Int32 booblindpar = skeleton->GetParentBoneIndex( booblind );
		Int32 boobrindpar = skeleton->GetParentBoneIndex( boobrind );

		if( booblind>=0 && boobrind>=0 && booblindpar>=0 && boobrindpar>=0 )
		{
			const AnimQsTransform qstml = skeleton->GetBoneLS( booblind );
			const AnimQsTransform qstmr = skeleton->GetBoneLS( boobrind );

			RedMatrix4x4 convertedMatrixl = qstml.ConvertToMatrixNormalized();
			RedMatrix4x4 convertedMatrixr = qstmr.ConvertToMatrixNormalized();

			m_bones[0].set( reinterpret_cast< const Matrix& >( convertedMatrixl ), booblind, booblindpar );
			m_bones[1].set( reinterpret_cast< const Matrix& >( convertedMatrixr ), boobrind, boobrindpar );

			m_bones[0].params( m_elA, m_velDamp, m_bounceDamp, m_inAcc, m_blackHole, m_velClamp, m_gravity, m_movementBoneWeight, m_rotationBoneWeight, m_inertiaScaler, m_startSimPointOffset );
			m_bones[1].params( m_elA, m_velDamp, m_bounceDamp, m_inAcc, m_blackHole, m_velClamp, m_gravity, m_movementBoneWeight, m_rotationBoneWeight, m_inertiaScaler, m_startSimPointOffset );

			m_cachedAllBones = true;
		}
	}
}

void CAnimDangleConstraint_Breast::SetBlendToAnimationWeight( Float w )
{
}

void CAnimDangleConstraint_Breast::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	if ( m_cachedAllBones )
	{
		const CComponent* c = GetParentComponent();
		const Matrix& l2w = c ? c->GetLocalToWorld() : GetComponent()->GetLocalToWorld();

		for( int i=0;i<2;i++)
		{
			const Matrix m = Matrix::Mul( l2w, m_bones[i].m_global );

			const Matrix & m1 = m_bones[i].m_local;
			Matrix t = Matrix::Mul( m_parentWS, m1 );
			frame->AddDebugAxis( t.GetTranslation(), t, 0.5f );

			frame->AddDebugLine( m.GetAxisX()+m.GetTranslation(), m.GetAxisX()*-0.2f+m.GetTranslation(), Color::RED );
			frame->AddDebugLine( m.GetAxisY()+m.GetTranslation(), m.GetAxisY()*-0.2f+m.GetTranslation(), Color::GREEN );
			frame->AddDebugSphere( l2w.TransformPoint( m_bones[i].m_target ), 0.005f, Matrix::IDENTITY, Color::YELLOW  );
			Float grx = m_bones[i].m_gravity.X;
			Float gry = m_bones[i].m_gravity.Y;
			frame->AddDebugLine( m.GetTranslation(), m.GetTranslation()+m.GetRow(0)*grx+m.GetRow(1)*gry, Color::YELLOW );
			//Float accx = m_bones[i].m_inertia.X*0.1f;
			//Float accy = m_bones[i].m_inertia.Y*0.1f;

			DrawEllipse( m, m_bones[i].m_simulator.m_shape.Z, m_bones[i].m_simulator.m_shape.W, m_bones[i].m_simulator.m_shape.X, m_bones[i].m_simulator.m_shape.Y, frame );
		}
	}
}

void CAnimDangleConstraint_Breast::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );

	// using engine dt
	m_dt = dt*m_simTime;
	//m_dt = m_simTime;
}

void CAnimDangleConstraint_Breast::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLSAsync( dt, poseLS, bones );

	// using engine dt
	m_dt = dt*m_simTime;
	//m_dt = m_simTime;
}

void CAnimDangleConstraint_Breast::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
#ifdef DEBUG_ANIM_CONSTRAINTS
	CTimeCounter timer;
#endif

	if ( !m_firstUpdate )
	{
		m_dt = GEngine->GetLastTimeDelta()*m_simTime;
		m_forceReset = true;
	}

	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsWS(parent, poseLS, poseMS, poseWS, bones );

	// This is a simple version - we check only base skeleton
	if ( m_forceReset || m_forceRelaxedState )
	{
		if ( const CAnimatedComponent* ac = CAnimatedComponent::GetRootAnimatedComponentFromAttachment( GetComponent() ) )
		{
			if ( const CTeleportDetector* detector = ac->GetTeleportDetector() )
			{
#ifndef NO_EDITOR
				RED_ASSERT( detector->GetLastTick() == GEngine->GetCurrentEngineTick() );
#endif
				const Bool poseChanged = detector ? detector->DoesPoseChanged() : false;
				const Bool pelvisChangedMS = detector ? detector->DoesPelvisChangedMS() : false;

				///////////////////////////////////
				//
				//	So we have 4 cases when teleport request appears
				//
				//	1. req: poseChanged=0 && pelvisChanged=0 => do nothing
				//	2. req: poseChanged=1 && pelvisChanged=0 => reset relax
				//	3. req: poseChanged=1 && pelvisChanged=1 => reset relax 
				//	4. req: poseChanged=0 && pelvisChanged=1 => do nothing
				//
				///////////////////////////////////

				if ( poseChanged || pelvisChangedMS )
				{
					// case 2 and 3 doesnt matter if pelvis changed
					m_forceRelaxedState = true;
					m_forceReset = true;
				}
				else
				{
					// case 1 and case 4 do nothing
					// otherwise please clear flags
					m_forceRelaxedState = false;
					m_forceReset = false;
				}
			}
		}
	}

	const Uint32 numIt = m_forceRelaxedState ? 30 : 1;

	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );

	if ( m_cachedAllBones && poseMS && HasSkeletalMatrices() && !bones.Empty() )
	{
		if ( bones.SizeInt() > m_bones[0].m_parent_index )
		{
			const Int32 boneIndex = bones[ m_bones[0].m_parent_index ].m_boneB;
			m_parentWS = (*poseWS)[ boneIndex ];
		}

		for( Uint32 i=0;i<2;i++)
		{
			const Int32 boneIndex = bones[ m_bones[i].m_parent_index ].m_boneB;
			if ( bones.SizeInt() > boneIndex )
			{
				if ( m_forceReset )
				{
					m_bones[i].Reset( (*poseMS)[ boneIndex ] );
				}

				Matrix out = Matrix::IDENTITY;
				m_bones[i].evaluate( out, (*poseMS)[ boneIndex ], m_dt, m_forceReset, m_startSimPointOffset, numIt );
				SetBoneModelSpace( m_bones[i].m_index, out );
			}
		}
	}

	m_forceRelaxedState = false;
	m_forceReset = false;

	EndUpdate( poseMS, poseWS, bones );

#ifdef DEBUG_ANIM_CONSTRAINTS
	const Float timeElapsed = timer.GetTimePeriod();
	if ( timeElapsed > 0.0005f )
	{
		GScreenLog->PerfWarning( timeElapsed, TXT("ANIM DANGLES"), TXT("Slow anim dangle update CAnimDangleConstraint_Breast for agent '%ls'"), GetComponent()->GetEntity()->GetFriendlyName().AsChar() );
	}

	BEH_LOG( TXT("CAnimDangleConstraint_Breast: %1.5f"), timeElapsed );
#endif
}

void CAnimDangleConstraint_Breast::DrawEllipse( const Matrix & mat, Float scaleX, Float scaleY, Float posX, Float posY, CRenderFrame* frame )
{
	Vector p0 = mat.TransformPoint(Vector( scaleX + posX,posY,0.0f));
	Float ang = 30.0f;
	for( Float a = ang; a <= 360.0f; a += ang )
	{
		Float rad = DEG2RAD(a);
		Vector p = mat.TransformPoint(Vector( cosf(rad)*scaleX + posX, sinf(rad)*scaleY + posY, 0.0f ));
		frame->AddDebugLine( p0, p, Color::YELLOW );
		p0 = p;
	}
}

void CAnimDangleConstraint_Breast::ForceReset()
{
	m_forceReset = true;
}
void CAnimDangleConstraint_Breast::ForceResetWithRelaxedState()
{
	m_forceRelaxedState = true;
	ForceReset();
}

#ifdef DEBUG_ANIM_DANGLES
#pragma optimize("",on)
#endif
