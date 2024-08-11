/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphInstance.h"
#include "node.h"
#include "characterConstraint.h"
#include "animatedComponent.h"
#include "entity.h"

IAnimationConstraint::IAnimationConstraint( const CName controlVariable, const CName variableToControl, Float timeout )
	: m_controlVariable( controlVariable )
	, m_variableToControl( variableToControl )
	, m_timeout( timeout )
	, m_timer( 0.f )
{
	ASSERT( m_controlVariable != CName::NONE );
	ASSERT( m_variableToControl != CName::NONE );
}

IAnimationConstraint::~IAnimationConstraint()
{}

Bool IAnimationConstraint::IsUnderControlBy( const CName controlVariable ) const 
{ 
	return ( controlVariable == m_controlVariable ); 
}

void IAnimationConstraint::Deactivate( CBehaviorGraphInstance& instance ) const 
{
	instance.SetFloatValue( m_controlVariable, 0.f );
}

Vector IAnimationConstraint::GetControledValue( CBehaviorGraphInstance& instance ) const 
{ 
	return instance.GetVectorValue( m_variableToControl ); 
}

void IAnimationConstraint::Update( CBehaviorGraphInstance& instance, Float dt )
{
	if ( m_timeout > 0.f )
	{
		m_timer += dt;
	}

	// Update control variable
	instance.SetFloatValue( m_controlVariable, 1.f );

	// Update controlled variable
	UpdateVariable( instance );
}

Bool IAnimationConstraint::IsFinished() const
{
	return m_timeout > 0.f && m_timer >= m_timeout ? true : false;
}

//////////////////////////////////////////////////////////////////////////

CAnimationConstraint::CAnimationConstraint( const CNode* target, 
											const CName controlVariable, const CName variableToControl, 
											Float timeout )
	: IAnimationConstraint( controlVariable, variableToControl, timeout )
	, m_target( target ) 
{
	
}

void CAnimationConstraint::UpdateVariable( CBehaviorGraphInstance& instance )
{
	const CNode* target = m_target.Get();
	if ( target )
	{
		Matrix entityMatrix;
		instance.GetAnimatedComponent()->GetEntity()->CalcLocalTransformMatrix(entityMatrix); 
		Matrix targetInMS = target->GetLocalToWorld() * entityMatrix.FullInverted();

		if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Position )
		{
			instance.SetVectorValue( m_variableToControl, targetInMS.GetTranslation() );
		}
		else if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Rotation )
		{
			EulerAngles angles = targetInMS.ToEulerAnglesFull();
			instance.SetVectorValue( m_variableToControl, Vector(angles.Roll, angles.Pitch, angles.Yaw) );
		}
	}
}

Bool CAnimationConstraint::IsFinished() const
{
	if ( !m_target.Get() )
	{
		return true;
	}

	return IAnimationConstraint::IsFinished();
}

String CAnimationConstraint::ToString() const
{
	const CNode* target = m_target.Get();

	if ( target )
	{	
		return String::Printf( TXT("Animation constraint - target %s, controlVar %s, var %s, timeout %f"), 
			m_target.Get()->GetName().AsChar(), m_controlVariable.AsChar(), m_variableToControl.AsChar(), m_timeout );
	}
	else
	{
		return TXT("Target is empty!!!");
	}
}

//////////////////////////////////////////////////////////////////////////

CAnimationBoneConstraint::CAnimationBoneConstraint( const CAnimatedComponent* target, Int32 boneIndex,
												    const CName controlVariable, const CName variableToControl,
													Bool useOffset, const Matrix& offsetMatrix,
												    Float timeout )
	: IAnimationConstraint( controlVariable, variableToControl, timeout )
	, m_target( target )
	, m_boneIndex( boneIndex )
	, m_offsetMatrix( offsetMatrix )
	, m_useOffset( useOffset )
{
	ASSERT( m_boneIndex != -1 );
}

void CAnimationBoneConstraint::UpdateVariable( CBehaviorGraphInstance& instance )
{
	const CAnimatedComponent* target = m_target.Get();
	if ( target && m_boneIndex != -1 )
	{
		Matrix entityMatrix;
		instance.GetAnimatedComponent()->GetEntity()->CalcLocalTransformMatrix( entityMatrix ); 
		Matrix boneMatrix = target->GetBoneMatrixWorldSpace( m_boneIndex );
		
		if( m_useOffset )
		{
			boneMatrix = Matrix::Mul( boneMatrix, m_offsetMatrix );
		}

		Matrix targetInMS = boneMatrix * entityMatrix.FullInverted();

		if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Position )
		{
			instance.SetVectorValue( m_variableToControl, targetInMS.GetTranslation() );
		}
		else if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Rotation )
		{
			EulerAngles angles = targetInMS.ToEulerAnglesFull();
			instance.SetVectorValue( m_variableToControl, Vector(angles.Roll, angles.Pitch, angles.Yaw) );
		}
	}
}

Bool CAnimationBoneConstraint::IsFinished() const
{
	if ( !m_target.Get() )
	{
		return true;
	}

	return IAnimationConstraint::IsFinished();
}

String CAnimationBoneConstraint::ToString() const
{
	const CAnimatedComponent* target = m_target.Get();

	if ( target )
	{	
		return String::Printf( TXT("Animation bone constraint - target %s, bone %d, controlVar %s, var %s, timeout %f, offset %s, %s"), 
			m_target.Get()->GetName().AsChar(), m_boneIndex, m_controlVariable.AsChar(), m_variableToControl.AsChar(), m_timeout, ::ToString( m_useOffset ).AsChar(), ::ToString( m_offsetMatrix ).AsChar() );
	}
	else
	{
		return TXT("Target is empty!!!");
	}
}

//////////////////////////////////////////////////////////////////////////

CAnimationConstraintStatic::CAnimationConstraintStatic( const Vector &target, 
														const CName controlVariable, const CName variableToControl, 
														Float timeout )
	: IAnimationConstraint( controlVariable, variableToControl, timeout )
	, m_target( target )
{

}

void CAnimationConstraintStatic::UpdateVariable( CBehaviorGraphInstance& instance )
{
	Matrix entityMatrix;
	instance.GetAnimatedComponent()->GetEntity()->CalcLocalTransformMatrix( entityMatrix ); 

	if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Position )
	{
		instance.SetVectorValue( m_variableToControl, entityMatrix.FullInverted().TransformPoint( m_target ) );
	}
	else if ( instance.GetVectorVariableType( m_variableToControl ) == VVT_Rotation )
	{
		Matrix targetInWS = EulerAngles( m_target.X, m_target.Y, m_target.Z ).ToMatrix();
		/*Matrix targetInWS;
		targetInWS.SetRotX33( m_target.X );
		targetInWS.SetRotY33( m_target.Y );
		targetInWS.SetRotZ33( m_target.Z );*/
		targetInWS.SetTranslation( Vector::ZERO_3D_POINT );

		Matrix targetInMS( targetInWS * entityMatrix.FullInverted() );
		EulerAngles angles( targetInMS.ToEulerAnglesFull() );
		instance.SetVectorValue( m_variableToControl, Vector( angles.Roll, angles.Pitch, angles.Yaw ) );
	}
}

String CAnimationConstraintStatic::ToString() const
{
	String vecStr = ::ToString( m_target );
	return String::Printf( TXT("Animation static constraint - target %s, controlVar %s, var %s, timeout %f"), 
		vecStr.AsChar(), m_controlVariable.AsChar(), m_variableToControl.AsChar(), m_timeout );
}
