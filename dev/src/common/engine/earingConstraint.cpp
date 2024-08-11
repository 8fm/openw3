
#include "build.h"
#include "earingConstraint.h"
#include "skeleton.h"
#include "renderVertices.h"
#include "renderFrame.h"

RedQuaternion operator*(const RedQuaternion & a, const RedQuaternion & b )
{
	RedVector4 av(a.Quat.X,a.Quat.Y,a.Quat.Z,0.0f);
	RedVector4 bv(b.Quat.X,b.Quat.Y,b.Quat.Z,0.0f);
	RedVector4 v = Add(Cross(av,bv), Add(Mul(av,b.Quat.W), Mul(bv,a.Quat.W)));
	return RedQuaternion( v.X, v.Y, v.Z, (a.Quat.W*b.Quat.W)-(a.Quat.X*b.Quat.X+a.Quat.Y*b.Quat.Y+a.Quat.Z*b.Quat.Z) );
}

RedVector4 operator*( const RedVector4 & f, const RedQuaternion & q )
{
	RedQuaternion w( f.X, f.Y, f.Z, 0.0f );
	RedQuaternion s( -q.Quat.X, -q.Quat.Y, -q.Quat.Z, q.Quat.W );
	RedQuaternion wyn = s*w*q;
	return RedVector4( wyn.Quat.X, wyn.Quat.Y, wyn.Quat.Z, 0.0f );
}

namespace AnimDangleConstraints
{

bonePendulum::bonePendulum() 
	: targetPosition( 0.0f, 0.0f, -1.0f, 0.0f)
	, targetVelocity( 0.0f, 0.0f, 0.0f, 0.0f )
	, index(-1)
	, dir1( -1.0f, 0.0f, 0.0f )
	, dir2( 0.0f, 0.0f, 0.0f )
	, k1( 0.0f )
	, k2( 0.0f )
{


}

Float bonePendulum::collision( const Vector & norm, Float k )
{
	Float u = targetPosition.X*norm.X + targetPosition.Y*norm.Y + targetPosition.Z*norm.Z;

	if( u<=k )
	{
		Float v = targetVelocity.X*norm.X + targetVelocity.Y*norm.Y + targetVelocity.Z*norm.Z;
		Float p = -k*k+u*u+v*v;
		if( (k+u)!=0.0f && p>0.0f  )
		{
			Float t = 2.0f*atanf( (v-sqrt(p))/(k+u) );
			if( t>0.0f )
			{
				return t;
			}
		}
	}
	else
	{
		targetVelocity += norm*-10.1f;
		correct_speed();
	}
	return 1000.0f;
}

bool bonePendulum::collisions( Float sp, Vector & nor, Float & t, Float & k )
{
	Float t1 = collision( dir1, k1 );
	Float t2 = collision( dir2, k2 );
	if( t1<sp && t1<t2 )
	{
		nor = dir1;
		t = t1;
		k = k1;
		return true;
	}
	if( t2<sp && t2<t1 )
	{
		nor = dir2;
		t = t2;
		k = k2;
		return true;
	}
	return false;
}

bool bonePendulum::move( Float speed, Float dt )
{
	Float sp = speed*dt;
	bool collided = false;
	if( sp>0.0001f )
	{
		correct_speed();
		Vector norm;
		Float t;
		Float k;
		collided = collisions( sp, norm, t, k );

		if( collided )
		{
			//Float a = targetVelocity.X*norm.X + targetVelocity.Y*norm.Y + targetVelocity.Z*norm.Z;
			targetPosition = targetPosition*cosf( t ) + targetVelocity*sinf( t );
			targetVelocity = (targetPosition*-sinf( t ) + targetVelocity*cosf( t ));
			Vector temp = targetVelocity*-1.0f;
			targetVelocity = temp + ( norm*(temp.X*norm.X + temp.Y*norm.Y + temp.Z*norm.Z) - temp ) * 2.0f;
			move( (sp-t), dt );
		}
		else
		{
			targetPosition = targetPosition*cosf( sp ) + targetVelocity*sinf( sp );
			targetVelocity = (targetPosition*-sinf( sp ) + targetVelocity*cosf( sp ));
		}
	}
	/*
	else
	{
		targetPosition = targetPosition*cosf( 0.00000001f ) + targetVelocity*sinf( 0.00000001f );
		targetVelocity = (targetPosition*-sinf( 0.00000001f ) + targetVelocity*cosf( 0.00000001f ));
	}
	*/
	return collided;
}

void bonePendulum::correct_speed()
{
	Float dotRV = targetPosition.X*targetVelocity.X + targetPosition.Y*targetVelocity.Y + targetPosition.Z*targetVelocity.Z;
	if( fabs(dotRV)>0.0001f)
	{
		targetVelocity = targetVelocity - ( targetPosition*dotRV );
		//targetVelocity.Normalize3();
	}
}

void bonePendulum::evaluate( Float dt, Matrix & out, Float damp, const Vector& s, Float rad, bool usecolls ) // this applies movement
{
	targetPosition.Normalize3();
	correct_speed();
	Float len = targetVelocity.Normalize3();
	if( len>3.1416f ){ len = 3.1416f; }
	{
		if( move( len, dt ) )
		{
			damp = 0.9f;  
		}
		targetVelocity = targetVelocity*len*damp;
	}

	if(targetPosition.X<0.997f)
	{
		Vector os = Vector::Cross( Vector(1.0f,0.0f,0.0f), targetPosition );
		os.Normalize3();
		Float han = acosf( targetPosition.X )*0.5f;
		Float shan = sinf(han);

		RedVector4 wr(os.X*shan,os.Y*shan,os.Z*shan,cosf(han));
		RedMatrix4x4 wyn = BuildFromQuaternion( wr );
		out.SetRow( 0, Vector(wyn.GetRow(0).X,wyn.GetRow(0).Y,wyn.GetRow(0).Z,0.0f) );
		out.SetRow( 1, Vector(wyn.GetRow(1).X,wyn.GetRow(1).Y,wyn.GetRow(1).Z,0.0f) );
		out.SetRow( 2, Vector(wyn.GetRow(2).X,wyn.GetRow(2).Y,wyn.GetRow(2).Z,0.0f) );
		out.SetRow( 3, Vector(0.0f,0.0f,0.0f,1.0f) );
	}
}

void bonePendulum::apply_force( const Vector & a, Float dt, Float mass )
{
	targetVelocity += (a);
}

void bonePendulum::system_moved_to( const Matrix & del, Float mass )
{
	/*
	targetPosition.Normalize3();
	Vector delta = (del.GetTranslation()-globalPosition.GetTranslation())/(1.0f/30.0f);
	globalPosition.Invert();
	globalPosition.SetTranslation( 0.0f,0.0f,0.0f );
	delta = globalPosition.TransformPoint( delta );

	Matrix rot = 

	globalPosition = del;
	
	globalAcceleration = delta - globalVelocity;
	globalVelocity = delta;

	targetVelocity += globalAcceleration*-1.0*mass;
	targetVelocity += globalVelocity*mass*-0.1f;
	*/


	globalPosition.Invert();
	Matrix delta = Matrix::Mul( globalPosition, del );
	Vector linear = delta.GetTranslation();
	globalAcceleration = linear - globalVelocity;
	globalVelocity = linear;
	globalPosition = del;
	/*
	RedQuaternion q;
	RedMatrix3x3 rot;
	rot.SetRow( 0, RedVector3( delta.GetRow(0).X, delta.GetRow(0).Y, delta.GetRow(0).Z ) );
	rot.SetRow( 1, RedVector3( delta.GetRow(1).X, delta.GetRow(1).Y, delta.GetRow(1).Z ) );
	rot.SetRow( 2, RedVector3( delta.GetRow(2).X, delta.GetRow(2).Y, delta.GetRow(2).Z ) );
	q.ConstructFromMatrix( rot );

	RedVector4 v;
	if( fabs( q.Quat.W ) == 1.0f )
	{
		v.X = 0.0f;
		v.Y = 0.0f;
		v.Z = 1.0f;
		v.W = 0.0f;
	}
	Float d = sqrt( 1.0f-q.Quat.W*q.Quat.W );
	v.X = (q.Quat.X/d);
	v.Y = (q.Quat.Y/d);
	v.Z = (q.Quat.Z/d);
	v.W = (2.0f*acos(q.Quat.W));
	Vector axis(v.X,v.Y,v.Z,0.0f);
	Vector force = targetPosition - (axis*(axis.X*targetPosition.X,axis.Y*targetPosition.Y,axis.Z*targetPosition.Z));
	targetVelocity += (force * v.W)*1.0f/mass;
	*/
	//targetVelocity += globalAcceleration*-0./mass;
	targetVelocity += globalVelocity*-0.5f/mass;

}


// this tracks the specified bones transform, velocity and acceleration


boneTracer::boneTracer() : m_position( 0.0f, 0.0f, 0.0f, 0.0f ), m_rotation( 0.0f, 0.0f, 0.0f, 1.0f ), m_delta(0.0f)
{

}
void boneTracer::quaternionToAngleAxis( const RedQuaternion & q, RedVector4 & v, Float dt )
{
	if( fabs( q.Quat.W ) == 1.0f )
	{
		v.X = 0.0f;
		v.Y = 0.0f;
		v.Z = 1.0f;
		v.W = 0.0f;
		return;
	}
	Float d = sqrt( 1.0f-q.Quat.W*q.Quat.W );
	v.X = (q.Quat.X/d);
	v.Y = (q.Quat.Y/d);
	v.Z = (q.Quat.Z/d);
	v.W = (2.0f*acos(q.Quat.W))/dt;
}
void boneTracer::attach( const RedVector4 & np, const RedQuaternion & nr )
{
	m_position = np;
	m_rotation = nr;
}
void boneTracer::motion( const RedVector4 & np, const RedQuaternion & nr, Float dt )
{
	if( m_delta > 0.01f )
	{

		RedVector4 delta_position = Sub( np, m_position );
		RedQuaternion delta_rotation; delta_rotation.SetInverseMul( nr, m_rotation );

		m_position = np;
		m_rotation = nr;

		// calculated delta

		RedVector4 linVel = Mul(delta_position, 1.0f / m_delta);
		RedVector4 angVel;
		quaternionToAngleAxis( delta_rotation, angVel, m_delta );

		// calculated velocities

		RedVector4 r0( m_angularVelocity.X, m_angularVelocity.Y, m_angularVelocity.Z, 0.0f );
		RedVector4 r1( angVel.X, angVel.Y, angVel.Z, 0.0f );
		SetMul( r0, m_angularVelocity.W );
		SetMul( r1, angVel.W );
		RedVector4 rdel = Sub( r1, r0 );
		Float rlen = rdel.Length3();

		m_linearAcceleration = Sub( linVel, m_linearVelocity );
		if( rlen>0.0f )
		{
			m_angularAcceleration = RedVector4( rdel.X/rlen, rdel.Y/rlen, rdel.Z/rlen, rlen );
		}
		else
		{
			m_angularAcceleration = RedVector4( 0.0f, 0.0f, 1.0f, 0.0f );
		}
		m_linearVelocity = linVel;
		m_angularVelocity = angVel;
		m_delta = 0.0f;
	}
	else
	{
		m_delta += dt;
	}
}

RedVector4 boneTracer::forceAtPoint( const RedVector4 & p, Float mulVel, Float mulAcc, Float mulMov )
{
	RedVector4 ax( m_angularAcceleration.X, m_angularAcceleration.Y, m_angularAcceleration.Z, 0.0f );
	Float an = m_angularAcceleration.W;
	Float cen = ax.X*p.X + ax.Y*p.Y + ax.Z*p.Z; 
	RedVector4 cenp = Mul( ax, cen );
	RedVector4 r = Sub( p, cenp );
	//Float an2 = m_angularVelocity.W;
	RedVector4 vel = Mul( Mul( Cross(ax,r),-an ), mulVel );
	RedVector4 acc = Mul( Mul(r, ((an*an))), mulAcc );
	return Add(vel, Add( acc, Mul( m_linearAcceleration, mulMov ) ) );
}

}

/////////////////////////

IMPLEMENT_ENGINE_CLASS( CAnimDangleConstraint_Earing );

CAnimDangleConstraint_Earing::CAnimDangleConstraint_Earing() 
	: CAnimSkeletalDangleConstraint()
	, m_cachedAllBones( false )
	, m_dampening( 0.97f )
	, m_inertia( 10.0f )
	, m_lastDt( 0.01f )
	, m_collision1( 0.0f, 0.0f, 0.0f, 0.0f )
	, m_collision2( 90.0f, 0.0f, 0.0f, 0.0f )
	, m_display( 0.1f )
	, m_usecolls( true )
{

}

Bool CAnimDangleConstraint_Earing::HasCachedBones() const
{
	return m_cachedAllBones;
}

void CAnimDangleConstraint_Earing::CacheBones( const CAnimatedComponent* parent, const CSkeleton* skeleton )
{
	if ( parent && skeleton )
	{
		m_bones[0].index = skeleton->FindBoneByName( "l_earing" );
		m_bones[1].index = skeleton->FindBoneByName( "r_earing" );
		m_head_index = skeleton->FindBoneByName( "head" );

		//m_head_tracer.attach();

		for(int i=0;i<2;i++)
		{
			AnimQsTransform qtm = skeleton->GetBoneLS( m_bones[i].index );	
			RedVector4 p = qtm.Translation;
			RedQuaternion lr = qtm.Rotation;
			//m_bones[i].set( p, lr );
		}

		m_cachedAllBones = true;
	}
	else
	{
		m_cachedAllBones = false;
	}
}

void CAnimDangleConstraint_Earing::DrawPlane( CRenderFrame* frame, const Matrix & m, const Vector & d, Float s, Float k )
{
	Vector os = Vector::Cross( Vector(0.0f,0.0f,1.0f), d );
	os.Normalize3();
	Float han = acosf( d.Z )*0.5f;
	Float shan = sinf(han);

	Vector lig( 1.0f,1.0f,-0.5f ); // for now
	lig.Normalize3(); // sure

	RedVector4 wr(os.X*shan,os.Y*shan,os.Z*shan,cosf(han));
	RedMatrix4x4 wyn = BuildFromQuaternion( wr );
	Matrix out;
	out.SetRow( 0, Vector(wyn.GetRow(0).X,wyn.GetRow(0).Y,wyn.GetRow(0).Z,0.0f) );
	out.SetRow( 1, Vector(wyn.GetRow(1).X,wyn.GetRow(1).Y,wyn.GetRow(1).Z,0.0f) );
	out.SetRow( 2, Vector(wyn.GetRow(2).X,wyn.GetRow(2).Y,wyn.GetRow(2).Z,0.0f) );
	out.SetRow( 3, Vector(0.0f,0.0f,0.0f,1.0f) );

	Matrix fin = Matrix::Mul( m, out );

	Float dot = fin.GetRow(2).X * lig.X + fin.GetRow(2).Y * lig.Y + fin.GetRow(2).Z * lig.Z;
	dot = (dot*0.5f)+0.5f;
	Color col = Color(Uint8(dot*255.0f),Uint8(dot*255.0f),0);

	DebugVertex pts[4];
	Uint16 inds[6] = { 0,1,2, 0,2,3 };
	pts[0].Set( Vector(  -s,-s, k*s  ), col );
	pts[1].Set( Vector(   s,-s, k*s  ), col );
	pts[2].Set( Vector(   s, s, k*s  ), col );
	pts[3].Set( Vector(  -s, s, k*s  ), col );
	frame->AddDebugTriangles( fin, pts, 4, inds, 6, col, false );
	frame->AddDebug3DArrow( fin.GetRow(3), fin.GetRow(2)*-1.0f, 1.0f*s, 0.01f*s, 0.02f*s, 0.01f*s, Color(255,255,0), Color(255,255,0) );
}
void CAnimDangleConstraint_Earing::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	//frame->AddDebug3DArrow( mat.GetRow(3),(m_draw), m_draw.Mag3()*10.0f, 0.001f, 0.002f, 0.01f, Color(0,255,0), Color(0,255,0) );
	//frame->AddDebug3DArrow( mat.GetRow(3),rot.TransformPoint(m_draw), m_draw.Mag3()*10.0f, 0.001f, 0.002f, 0.01f, Color(0,255,0), Color(0,255,0) );
	Float s = m_display;
	if ( m_cachedAllBones )
	{
		Matrix head = GetBoneWorldSpace( m_head_index );
		for(int i=0;i<2;i++)
		{
			const CSkeleton* skeleton = GetSkeleton();

			//drawing constraint
			AnimQsTransform local = ( skeleton->GetBoneLS( m_bones[i].index ) );
			RedMatrix4x4 convertedMatrix = local.ConvertToMatrixNormalized();
			Matrix ear;
			ear.SetRow( 0, Vector( convertedMatrix.GetRow(0).X, convertedMatrix.GetRow(0).Y, convertedMatrix.GetRow(0).Z, 0.0f ) );
			ear.SetRow( 1, Vector( convertedMatrix.GetRow(1).X, convertedMatrix.GetRow(1).Y, convertedMatrix.GetRow(1).Z, 0.0f ) );
			ear.SetRow( 2, Vector( convertedMatrix.GetRow(2).X, convertedMatrix.GetRow(2).Y, convertedMatrix.GetRow(2).Z, 0.0f ) );
			ear.SetRow( 3, Vector( convertedMatrix.GetRow(3).X, convertedMatrix.GetRow(3).Y, convertedMatrix.GetRow(3).Z, 1.0f ) );
			//Matrix earWS = Matrix::Mul(head, ear);

			Matrix mat = Matrix::Mul(head, ear);
			Matrix mat2 = GetBoneWorldSpace( m_bones[i].index );

			frame->AddDebug3DArrow( mat2.GetRow(3), mat2.GetRow(0), 1.0f*s, 0.01f*s, 0.02f*s, 0.01f*s, Color(255,0,0), Color(255,0,0) );
			frame->AddDebugSphere( mat.TransformPoint( m_bones[i].targetPosition*s), 0.1f*s, Matrix::IDENTITY, Color(0,0,255) );

			if( m_usecolls )
			{
				DrawPlane( frame, mat, m_bones[i].dir1, s, m_bones[i].k1 );
				DrawPlane( frame, mat, m_bones[i].dir2, s, m_bones[i].k2 );
			}
		}
	}
}

void CAnimDangleConstraint_Earing::OnParentUpdatedAttachedAnimatedObjectsLS( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );

	m_lastDt = dt;
}

void CAnimDangleConstraint_Earing::OnParentUpdatedAttachedAnimatedObjectsLSAsync( Float dt, SBehaviorGraphOutput* poseLS, const BoneMappingContainer& bones )
{
	TBaseClass::OnParentUpdatedAttachedAnimatedObjectsLS( dt, poseLS, bones );

	m_lastDt = dt;
}

void CAnimDangleConstraint_Earing::OnParentUpdatedAttachedAnimatedObjectsWS( const CAnimatedComponent* parent, SBehaviorGraphOutput* poseLS, TDynArray< Matrix >* poseMS, TDynArray< Matrix >* poseWS, const BoneMappingContainer& bones )
{
	if ( !m_cachedAllBones )
	{
		return;
	}

	StartUpdateWithBoneAlignment( poseMS, poseWS, bones );

	Float dt = m_lastDt;

	// track heads acceleration
	const Matrix& boneWS = GetBoneModelSpace( m_head_index ); //head

	RedQuaternion q;
	RedMatrix3x3 rot;
	rot.SetRow( 0, RedVector3( boneWS.GetRow(0).X, boneWS.GetRow(0).Y, boneWS.GetRow(0).Z ) );
	rot.SetRow( 1, RedVector3( boneWS.GetRow(1).X, boneWS.GetRow(1).Y, boneWS.GetRow(1).Z ) );
	rot.SetRow( 2, RedVector3( boneWS.GetRow(2).X, boneWS.GetRow(2).Y, boneWS.GetRow(2).Z ) );
	q.ConstructFromMatrix( rot );

	m_head_tracer.motion( RedVector4(boneWS.GetRow(3).X, boneWS.GetRow(3).Y, boneWS.GetRow(3).Z, 1.0f), q, dt );

	const CSkeleton* skeleton = GetSkeleton();

	Matrix omat;
	for(Int32 i=0;i<2;i++)
	{
		Float a = DEG2RAD( m_collision1.X );
		Float b = DEG2RAD( m_collision1.Y );
		m_bones[i].dir1 = Vector( sinf(a)*cosf(b), sinf(a)*sinf(b), cosf(a) );
		m_bones[i].k1 = m_collision1.Z*0.01f;

		a = DEG2RAD( m_collision2.X );
		b = DEG2RAD( m_collision2.Y );
		m_bones[i].dir2 = Vector( sinf(a)*cosf(b), sinf(a)*sinf(b), cosf(a) );
		m_bones[i].k2 = m_collision2.Z*0.01f;

		AnimQsTransform local = ( skeleton->GetBoneLS( m_bones[i].index ) );
		RedMatrix4x4 convertedMatrix = local.ConvertToMatrixNormalized();
		Matrix ear;
		ear.SetRow( 0, Vector( convertedMatrix.GetRow(0).X, convertedMatrix.GetRow(0).Y, convertedMatrix.GetRow(0).Z, 0.0f ) );
		ear.SetRow( 1, Vector( convertedMatrix.GetRow(1).X, convertedMatrix.GetRow(1).Y, convertedMatrix.GetRow(1).Z, 0.0f ) );
		ear.SetRow( 2, Vector( convertedMatrix.GetRow(2).X, convertedMatrix.GetRow(2).Y, convertedMatrix.GetRow(2).Z, 0.0f ) );
		ear.SetRow( 3, Vector( convertedMatrix.GetRow(3).X, convertedMatrix.GetRow(3).Y, convertedMatrix.GetRow(3).Z, 1.0f ) );
		omat = Matrix::Mul(boneWS, ear);

		Vector lpf = ear.GetRow(3);

		RedVector4 kor = m_head_tracer.forceAtPoint( RedVector4( lpf.X,lpf.Y,lpf.Z,lpf.W), 0.01f,0.1f,0.01f );
		Vector force(kor.X,kor.Y,kor.Z,kor.W);

		//m_bones[i].system_moved_to( omat, m_inertia );

		Matrix omatinv = omat;
		omatinv.Invert();
		omatinv.SetTranslation(0.0f,0.0f,0.0f);
		Matrix wyn = Matrix::IDENTITY;

		m_bones[i].apply_force( omatinv.TransformPoint( Vector(0.0f,0.0f,-0.627f,0.0f) /*+ force*0.01f*/ ), dt, m_inertia );
		m_bones[i].evaluate( dt, wyn, m_dampening, Vector() , 0.0f, m_usecolls );

		SetBoneModelSpace( m_bones[i].index, Matrix::Mul(omat,wyn) );
	}

	EndUpdate( poseMS, poseWS, bones );
}
