/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "behaviorConstraintCameraFocus.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "../engine/animatedComponent.h"
#include "../engine/skeleton.h"
#include "../core/instanceDataLayoutCompiler.h"


IMPLEMENT_ENGINE_CLASS( CBehaviorGraphConstraintNodeCameraFocus );

CBehaviorGraphConstraintNodeCameraFocus::CBehaviorGraphConstraintNodeCameraFocus()
{
	
}

void CBehaviorGraphConstraintNodeCameraFocus::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_boneIndex;
	compiler << i_boneIndex2;
}

void CBehaviorGraphConstraintNodeCameraFocus::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_boneIndex ] = FindBoneIndex( m_bone, instance );
	instance[ i_boneIndex2 ] = FindBoneIndex( m_bone2, instance );
}

void CBehaviorGraphConstraintNodeCameraFocus::OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const
{
	TBaseClass::OnBuildInstanceProperites( instance, builder );

	INST_PROP_INIT;
	INST_PROP( i_boneIndex );
	INST_PROP( i_boneIndex2 );
}

AnimQsTransform CBehaviorGraphConstraintNodeCameraFocus::CalcTargetFromOutput( const CBehaviorGraphInstance& instance, const SBehaviorGraphOutput &output ) const
{
	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const Int32 boneIndex = instance[ i_boneIndex ];
	const Int32 boneIndex2 = instance[ i_boneIndex2 ];

#ifdef USE_HAVOK_ANIMATION
	const hkaSkeleton *havokSkeleton = ac->GetSkeleton()->GetHavokSkeleton();
	if ( boneIndex != -1 && boneIndex2 != -1 && m_targetObject && havokSkeleton)
	{
		Int32 paro = havokSkeleton->m_parentIndices[ boneIndex2 ];
		hkQsTransform bms = output.GetBoneModelTransform( boneIndex2, havokSkeleton->m_parentIndices );

		Vector destTargetVec = GetTargetEnd( instance ).GetPosition();
		Float distToTarget = ( destTargetVec - TO_CONST_VECTOR_REF( bms.getTranslation() ) ).Mag3();

		hkVector4 cel = hkVector4(0.0f,distToTarget,0.0f,0.0f);

		hkTransform out;
		bms.copyToTransform(out);
		cel._setMul4xyz1(out, cel);

		return hkQsTransform( cel, hkQuaternion::getIdentity(), hkVector4(1,1,1,1) );
	}
#else
	if ( boneIndex != -1 && boneIndex2 != -1 && m_targetObject )
	{
		RedQsTransform bms = output.GetBoneModelTransform( boneIndex2, ac->GetSkeleton()->GetParentIndices() );

		RedVector4 destTargetVec = reinterpret_cast<const RedVector4&>(GetTargetEnd( instance ).GetPosition());
		const Float distToTarget = Sub(destTargetVec, bms.Translation).Length3();

		const RedVector4 cel2(0.0f,distToTarget,0.0f,0.0f);
		
		RedVector4 cel;
		cel.SetTransformedPos(bms, cel2);

		return RedQsTransform( cel, RedQuaternion::IDENTITY, RedVector4::ONES );
	}
#endif

	return AnimQsTransform::IDENTITY;
}

void CBehaviorGraphConstraintNodeCameraFocus::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const CAnimatedComponent* ac = instance.GetAnimatedComponent();
	const Int32 boneIndex = instance[ i_boneIndex ];
	const Int32 boneIndex2 = instance[ i_boneIndex2 ];

	if ( !IsConstraintActive( instance ) || !ac ) 
		return;

	if ( ac && boneIndex != -1 && boneIndex2 != -1 && m_targetObject)
	{
		const AnimQsTransform& targetTransform = GetCurrentConstraintTransform( instance );

#ifdef USE_HAVOK_ANIMATION
		if ( !targetTransform.isOk() ) return;
#else
		if ( !targetTransform.IsOk() )
		{
			return;
		}
#endif
		
		const Int32 paro = ac->GetSkeleton()->GetParentBoneIndex( boneIndex );

		AnimQsTransform tro = output.GetBoneModelTransform( ac, boneIndex );
		AnimQsTransform ptro =  output.GetBoneModelTransform( ac, paro );

		Vector celo;
#ifdef USE_HAVOK_ANIMATION
		celo.X=targetTransform.m_translation(0);
		celo.Y=targetTransform.m_translation(1);
		celo.Z=0;
		celo.W=targetTransform.m_translation(3);
#else
		celo.X = targetTransform.Translation.X;
		celo.Y = targetTransform.Translation.Y;
		celo.Z = 0.0f;
		celo.W = targetTransform.Translation.W;
#endif
		Matrix mato;
		Matrix pmato;
#ifdef USE_HAVOK_ANIMATION
		HavokTransformToMatrix(tro, &mato);
		HavokTransformToMatrix(ptro, &pmato);
#else
		tro.Get4x4ColumnMajor( reinterpret_cast< Float* > ( &mato ) );
		ptro.Get4x4ColumnMajor( reinterpret_cast< Float* >( &pmato ) );
#endif
		pmato = pmato.FullInvert();

		Vector r3o = Vector(0.0f,0.0f,1.0f);
		Vector r2o = Vector::Sub4(celo, mato.V[3]);
		r2o.Normalize3();

		Vector r1o = Vector::Cross( r2o, r3o );
		r1o.Normalize3();
		r3o = Vector::Cross(r1o, r2o);
		r3o.Normalize3();

		mato.V[0]=r1o;
		mato.V[1]=r2o;
		mato.V[2]=r3o;

		if ( Vector::Near3( r1o, Vector::ZERO_3D_POINT ) ||
			 Vector::Near3( r2o, Vector::ZERO_3D_POINT ) ||
			 Vector::Near3( r3o, Vector::ZERO_3D_POINT ) )
		{
			return;
		}
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform wyno, hmato, phmato;
		MatrixToHavokQsTransform( mato, hmato);
		MatrixToHavokQsTransform( pmato, phmato);

		wyno.setMul( phmato, hmato );
		output.m_outputPose[ boneIndex ] = wyno;
#else
		RedQsTransform hmato;
		RedQsTransform wyno;
		RedQsTransform phmato;
		{
			RedMatrix4x4 conversionMatrix;
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( mato );
			hmato.Set( conversionMatrix );
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( pmato );
			phmato.Set( conversionMatrix );
		}
		wyno.SetMul( phmato, hmato );
		output.m_outputPose[ boneIndex ] = wyno;
#endif

		const Int32 par = ac->GetSkeleton()->GetParentBoneIndex( boneIndex2 );
		AnimQsTransform tr = output.GetBoneModelTransform( ac, boneIndex2 );
		AnimQsTransform ptr = output.GetBoneModelTransform( ac, par );

#ifdef USE_HAVOK_ANIMATION
		Vector cel;
		cel.X=targetTransform.m_translation(0);
		cel.Y=targetTransform.m_translation(1);
		cel.Z=targetTransform.m_translation(2);
		cel.W=targetTransform.m_translation(3);
#else
		Vector cel;
		cel.X=targetTransform.Translation.X;
		cel.Y=targetTransform.Translation.Y;
		cel.Z=targetTransform.Translation.Z;
		cel.W=targetTransform.Translation.W;
#endif

		Matrix mat;
		Matrix pmat;
#ifdef USE_HAVOK_ANIMATION
		HavokTransformToMatrix(tr, &mat);
		HavokTransformToMatrix(ptr, &pmat);
#else
		{
			RedMatrix4x4 conversionMatrix;
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( mat );
			tr.Set( conversionMatrix );
			conversionMatrix = reinterpret_cast< const RedMatrix4x4& >( pmat );
			ptr.Set( conversionMatrix );
		}
#endif
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
#ifdef USE_HAVOK_ANIMATION
		hkQsTransform wyn, hmat, phmat;
		MatrixToHavokQsTransform( mat, hmat);
		MatrixToHavokQsTransform( pmat, phmat);
		wyn.setMul( phmat, hmat );
#else
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
#endif
		output.m_outputPose[ boneIndex2 ] = wyn;	
	}
}

String CBehaviorGraphConstraintNodeCameraFocus::GetCaption() const
{
	if ( !m_name.Empty() )
		return String::Printf( TXT("Camera Focus - %s"), m_name.AsChar() );
	else
		return String( TXT("Camera Focus") );
}
