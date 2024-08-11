/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintNodeCameraLookAt.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeCameraLookAt );

CBehaviorGraphConstraintNodeCameraLookAt::CBehaviorGraphConstraintNodeCameraLookAt()
{
}

void CBehaviorGraphConstraintNodeCameraLookAt::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
}

void CBehaviorGraphConstraintNodeCameraLookAt::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_bone, instance );
}

void CBehaviorGraphConstraintNodeCameraLookAt::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
}

#ifdef USE_HAVOK_ANIMATION
hkQsTransform CBehaviorGraphConstraintNodeCameraLookAt::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	return hkQsTransform::getIdentity();
}
#else
RedQsTransform CBehaviorGraphConstraintNodeCameraLookAt::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	return RedQsTransform::IDENTITY;
}
#endif

void CBehaviorGraphConstraintNodeCameraLookAt::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{	
	TBaseClass::Sample( context, instance, output );

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const Int32 boneIndex = instance[ i_boneIndex ];

	if ( !IsConstraintActive( instance ) || !ac ) 
		return;

#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton *havokSkeleton = ac->GetSkeleton()->GetHavokSkeleton();
	if ( boneIndex != -1 && m_targetObject)
	{
		const hkQsTransform& targetTransform = GetCurrentConstraintTransform( instance );
		if ( !targetTransform.isOk() || targetTransform.m_translation.equals3( hkVector4( 0.f, 0.f, 0.f ) ) ) return;

		Int32 par = havokSkeleton->m_parentIndices[ boneIndex ];
		hkQsTransform ptr = output.GetBoneModelTransform( ac, par );

		//hkQsTransform tr = output.GetBoneModelTransform( ac, boneIndex );
		hkQsTransform tr;
		tr.setMul( ptr, output.m_outputPose[ boneIndex ] );

		Vector cel;
		cel.X=targetTransform.m_translation(0);
		cel.Y=targetTransform.m_translation(1);
		cel.Z=targetTransform.m_translation(2);
		cel.W=targetTransform.m_translation(3);

		Matrix mat;
		Matrix pmat;
		HavokTransformToMatrix(tr, &mat);
		HavokTransformToMatrix(ptr, &pmat);

		pmat = pmat.FullInvert();

		Vector r3 = Vector(0.0f,0.0f,1.0f);
		Vector r2 = Vector::Sub4(cel, mat.V[3]);
		r2.Normalize3();

		Vector r1 = Vector::Cross( r2, r3 );
		r1.Normalize3();
		r3 = Vector::Cross(r1, r2);
		r3.Normalize3();

		mat.V[0]=r1;
		mat.V[1]=r2;
		mat.V[2]=r3;

		hkQsTransform wyn, hmat, phmat;
		MatrixToHavokQsTransform( mat, hmat);
		MatrixToHavokQsTransform( pmat, phmat);
		wyn.setMul( phmat, hmat );
		output.m_outputPose[ boneIndex ] = wyn;
	}
#else
	if ( boneIndex != -1 && m_targetObject)
	{
		const RedQsTransform& targetTransform = GetCurrentConstraintTransform( instance );
		if ( !targetTransform.IsOk() || targetTransform.Translation.AsVector3().IsZero() )
        {
			return;
		}

		const Int32 par = ac->GetSkeleton()->GetParentBoneIndex(boneIndex);
		RedQsTransform ptr = output.GetBoneModelTransform( ac, par );

		AnimQsTransform tr;
		tr.SetMul( ptr, output.m_outputPose[ boneIndex ] );

		Vector cel;
		cel.X = targetTransform.Translation.X;
		cel.Y = targetTransform.Translation.Y;
		cel.Z = targetTransform.Translation.Z;
		cel.W = targetTransform.Translation.W;

		Matrix mat;
		Matrix pmat;
		{
			RedMatrix4x4 conversionMatrix;
			conversionMatrix = tr.ConvertToMatrix();
			mat = reinterpret_cast< const Matrix& >( conversionMatrix ); 
			conversionMatrix = ptr.ConvertToMatrix();
			pmat = reinterpret_cast< const Matrix& >( conversionMatrix );
		}
		pmat = pmat.FullInvert();

		Vector r3 = Vector(0.0f,0.0f,1.0f);
		Vector r2 = Vector::Sub4(cel, mat.V[3]);
		r2.Normalize3();

		Vector r1 = Vector::Cross( r2, r3 );
		r1.Normalize3();
		r3 = Vector::Cross(r1, r2);
		r3.Normalize3();

		mat.V[0]=r1;
		mat.V[1]=r2;
		mat.V[2]=r3;


		RedQsTransform wyn;
		RedQsTransform hmat;
		RedQsTransform phmat;
		{
			RedMatrix4x4 conversionMatrix;
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( mat );
			hmat.Set( conversionMatrix );
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( pmat );
			phmat.Set( conversionMatrix );
		}
		
		wyn.SetMul( phmat, hmat );
		output.m_outputPose[ boneIndex ] = wyn;
	}
#endif
}

String CBehaviorGraphConstraintNodeCameraLookAt::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Camera Look at - %s"), m_name.AsChar() );
	else
		return String( TXT("Camera Look at") );
}
