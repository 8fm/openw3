
#include "build.h"
#include "behaviorGraphNodeControlRig.h"
#include "../engine/controlRig.h"
#include "../engine/controlRigDefinition.h"
#include "behaviorGraphInstance.h"
#include "../core/instanceDataLayoutCompiler.h"
#include "../engine/renderFrame.h"
#include "animatedComponent.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphControlRigNode );

CBehaviorGraphControlRigNode::CBehaviorGraphControlRigNode()
	: m_offsetHandLeft( false )
	, m_offsetHandRight( false )
{

}

void CBehaviorGraphControlRigNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_controlRigPtr;
	compiler << i_manualControl;
}

void CBehaviorGraphControlRigNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{	
	RED_FATAL_ASSERT( IsOnReleaseInstanceManuallyOverridden(), "" );

	TBaseClass::OnInitInstance( instance );

	CreateControlRig( instance );

	instance[ i_manualControl ] = false;
}

void CBehaviorGraphControlRigNode::OnReleaseInstance( CBehaviorGraphInstance& instance ) const
{
	DestroyControlRig( instance );

	TBaseClass::OnReleaseInstance( instance );
}

void CBehaviorGraphControlRigNode::OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const
{
	TBaseClass::OnUpdate( context, instance, timeDelta );
}

void CBehaviorGraphControlRigNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Bool isNotManualControl = !instance[ i_manualControl ];

	const Float eHandLeftW = instance.GetFloatValue( m_eHandLeftW );
	const Float eHandRightW = instance.GetFloatValue( m_eHandRightW );

	const Vector eHandLeftP = instance.GetVectorValue( m_eHandLeftP );
	const Vector eHandRightP = instance.GetVectorValue( m_eHandRightP );

	const Float eHandLeftWeaponOffset = instance.GetFloatValue( m_eHandLeftWeaponOffset );
	const Float eHandRightWeaponOffset = instance.GetFloatValue( m_eHandRightWeaponOffset );

	TCrInstance* cr = GetControlRig( instance );
	if ( cr )
	{
		const CSkeleton* s = instance.GetAnimatedComponent()->GetSkeleton();
		const TCrDefinition* def = s->GetControlRigDefinition();

		cr->SetLocalToWorld( instance.GetAnimatedComponent()->GetLocalToWorld() );

		def->SetRigFromPoseLS( output, cr );

		if ( isNotManualControl )
		{
			cr->SetWeaponOffsetForHandLeft( eHandLeftWeaponOffset );
			cr->SetWeaponOffsetForHandRight( eHandRightWeaponOffset );
		}

		cr->SyncMSFromLS();
		cr->SyncWSFromMS();

		if ( isNotManualControl )
		{
			RedQsTransform t( RedQsTransform::IDENTITY );
			t.Translation = reinterpret_cast< const RedVector4& >( eHandLeftP );

			if ( m_offsetHandLeft )
			{
				//if ( eHandLeftWeaponOffset > 0.5f ) 
				{
					cr->GetBoneMS( TCrB_WeaponL, t );
				}
				//else
				//{
				//	cr->GetBoneMS( TCrB_HandL, t );
				//}
				SetAdd( t.Translation , reinterpret_cast< const RedVector4& >( eHandLeftP ) );
			}

			//Matrix mat = instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( def->m_indexHandL );
			//Vector t2 = mat.GetTranslation();

			Vector t1 = reinterpret_cast< const Vector& >(t.Translation );

			cr->SetEffectorWS( TCrEffector_HandL, t );
			cr->SetTranslationActive( TCrEffector_HandL, eHandLeftW );
			cr->SetRotationActive( TCrEffector_HandL, 0.f );
		}

		if ( isNotManualControl )
		{
			RedQsTransform t( RedQsTransform::IDENTITY );
			t.Translation = reinterpret_cast< const RedVector4& >( eHandRightP );

			if ( m_offsetHandRight )
			{
				//if ( eHandRightWeaponOffset > 0.5f ) 
				{
					cr->GetBoneMS( TCrB_WeaponR, t );
				}
				//else
				//{
				//	cr->GetBoneMS( TCrB_HandR, t );
				//}

				SetAdd( t.Translation, reinterpret_cast< const RedVector4& >( eHandRightP ) );
			}

			cr->SetEffectorWS( TCrEffector_HandR, t );
			cr->SetTranslationActive( TCrEffector_HandR, eHandRightW );
			cr->SetRotationActive( TCrEffector_HandR, 0.f );
		}

		cr->Solve();

		//cr->SyncLSFromMS();

		def->SetPoseLSFromRig( cr, output );
	}
	

	/*SControlRigSetup setup;

	setup.m_hipRotation.m_rotationZ = instance.GetFloatValue( m_varHipsAngle.AsChar() );
	setup.m_hipRotation.m_offsetZ = instance.GetFloatValue( m_varHipsOffset.AsChar() );

	setup.m_leftLeg.m_weight = instance.GetFloatValue( m_varLegControl.AsChar() );
	setup.m_leftLeg.m_weightFirst = instance.GetFloatValue( m_varLegFirstWeight.AsChar() );
	setup.m_leftLeg.m_weightSecond = instance.GetFloatValue( m_varLegSecondWeight.AsChar() );
	setup.m_leftLeg.m_weightEnd = instance.GetFloatValue( m_varLegEndWeight.AsChar() );
	setup.m_leftLeg.m_enforceRotation = instance.GetFloatValue( m_varLegEnforceRotation.AsChar() ) > 0.f;
	setup.m_leftLeg.m_cosineMinHingeAngle = MCos( DEG2RAD( m_legKneeMinAngleDeg ) );
	setup.m_leftLeg.m_cosineMaxHingeAngle = MCos( DEG2RAD( m_legKneeMaxAngleDeg ) );
	
	setup.m_rightLeg.m_weight = setup.m_leftLeg.m_weight;
	setup.m_rightLeg.m_weightFirst = setup.m_leftLeg.m_weightFirst;
	setup.m_rightLeg.m_weightSecond = setup.m_leftLeg.m_weightSecond;
	setup.m_rightLeg.m_weightEnd = setup.m_leftLeg.m_weightEnd;
	setup.m_rightLeg.m_enforceRotation = setup.m_leftLeg.m_enforceRotation;
	setup.m_rightLeg.m_cosineMinHingeAngle = setup.m_leftLeg.m_cosineMinHingeAngle;
	setup.m_rightLeg.m_cosineMaxHingeAngle = setup.m_leftLeg.m_cosineMaxHingeAngle;

	const Float chestWeight = instance.GetFloatValue( m_varChestWeight.AsChar() );
	if ( chestWeight > 0.f )
	{
		setup.m_chest.m_boneDirection1 = m_chestDir1;
		setup.m_chest.m_boneDirection2 = m_chestDir2;
		setup.m_chest.m_boneDirection3 = m_chestDir3;
		setup.m_chest.Set( instance.GetVectorValue( m_varChestOffset.AsChar() ) );
		setup.m_chest.m_weight = chestWeight;
		setup.m_chest.m_weightSpine1 = instance.GetFloatValue( m_varChestWeightSpine1.AsChar() );
		setup.m_chest.m_weightSpine2 = instance.GetFloatValue( m_varChestWeightSpine2.AsChar() );
		setup.m_chest.m_weightSpine3 = instance.GetFloatValue( m_varChestWeightSpine3.AsChar() );
	}

	setup.m_leftArm.m_weight = instance.GetFloatValue( m_varArmLeftWeight.AsChar() );
	setup.m_leftArm.m_targetMS = instance.GetVectorValue( m_varArmLeftTarget.AsChar() );
	setup.m_rightArm.m_weight = instance.GetFloatValue( m_varArmRightWeight.AsChar() );
	setup.m_rightArm.m_targetMS = instance.GetVectorValue( m_varArmRightTarget.AsChar() );

	CControlRig* cr = GetControlRig( instance );

	cr->Solve( setup, output );*/
}

TCrInstance* CBehaviorGraphControlRigNode::CreateControlRig( CBehaviorGraphInstance& instance ) const
{
	const CSkeleton* s = instance.GetAnimatedComponent()->GetSkeleton();

	const TCrDefinition* def = s->GetControlRigDefinition();
	if ( !def || !def->IsValid() )
	{
		return NULL;
	}

	const TCrPropertySet* ps = s->GetDefaultControlRigPropertySet();

	TCrInstance* cr = def->CreateRig( ps );
	if ( !cr )
	{
		return NULL;
	}

	instance[ i_controlRigPtr ] = reinterpret_cast< TGenericPtr >( cr );

	return cr;
}

TCrInstance* CBehaviorGraphControlRigNode::GetControlRig( CBehaviorGraphInstance& instance ) const
{
	return reinterpret_cast< TCrInstance* >( instance[ i_controlRigPtr ] );
}

void CBehaviorGraphControlRigNode::DestroyControlRig( CBehaviorGraphInstance& instance ) const
{
	TCrInstance* cr = GetControlRig( instance );
	delete cr;
	instance[ i_controlRigPtr ] = 0;
}

#ifndef NO_EDITOR_GRAPH_SUPPORT

void CBehaviorGraphControlRigNode::OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const
{
	if ( m_generateEditorFragments )
	{
		TCrInstance* cr = GetControlRig( instance );
		if ( cr )
		{
			const TCrDefinition* def = instance.GetAnimatedComponent()->GetSkeleton()->GetControlRigDefinition();

			{
				RedQsTransform trans;

				cr->GetEffectorWS( TCrEffector_HandR, trans );

				frame->AddDebugSphere( reinterpret_cast< const Vector& >( trans.Translation ), 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );

				Matrix mat;
				RedMatrix4x4 conversionMatrix = trans.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( conversionMatrix );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				cr->GetBoneMS( TCrB_HandR, trans );

				conversionMatrix = trans.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( conversionMatrix );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				mat = instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( def->m_indexHandR );
				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				cr->GetBoneMS( TCrB_WeaponR, trans );

				conversionMatrix = trans.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( conversionMatrix );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				mat = instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( def->m_indexWeaponR );
				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );
			}

			{
				RedQsTransform trans;

				cr->GetEffectorWS( TCrEffector_HandL, trans );

				frame->AddDebugSphere( reinterpret_cast< const Vector& >( trans.Translation ), 0.05f, Matrix::IDENTITY, Color( 255, 0, 0 ) );

				Matrix mat;
				RedMatrix4x4 conversionMatrix = trans.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( conversionMatrix );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				cr->GetBoneMS( TCrB_HandL, trans );

				conversionMatrix = trans.ConvertToMatrixNormalized();
				mat = reinterpret_cast< const Matrix& >( conversionMatrix );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );

				mat = instance.GetAnimatedComponent()->GetBoneMatrixWorldSpace( def->m_indexHandL );

				frame->AddDebugAxis( mat.GetTranslation(), mat, 0.1f, true );
			}
		}
	}
}

#endif

void CBehaviorGraphControlRigNode::OnLoadingSnapshot( CBehaviorGraphInstance& instance, InstanceBuffer& snapshotData ) const
{
	TBaseClass::OnLoadingSnapshot( instance, snapshotData );
	// TODO - rig state is not stored at the moment, and if it was in different point in memory, we need to do this
	if ( instance[ i_controlRigPtr ] != snapshotData[ i_controlRigPtr ] )
	{
		DestroyControlRig( instance );
	}
}

void CBehaviorGraphControlRigNode::OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const
{
	TBaseClass::OnLoadedSnapshot( instance, previousData );
	// TODO - rig state is not stored at the moment, and if it was in different point in memory, we need to do this
	if ( instance[ i_controlRigPtr ] != previousData[ i_controlRigPtr ] )
	{
		CreateControlRig( instance );
	}
}

void CBehaviorGraphControlRigNode::SetManualControl( CBehaviorGraphInstance& instance, Bool flag ) const
{
	instance[ i_manualControl ] = flag;
}

//////////////////////////////////////////////////////////////////////////

CBehaviorGraphControlRigInterface::CBehaviorGraphControlRigInterface()
	: m_node( nullptr )
	, m_instance( nullptr )
	, m_instanceH( nullptr )
{

}

CBehaviorGraphControlRigInterface::~CBehaviorGraphControlRigInterface()
{

}

void CBehaviorGraphControlRigInterface::Init( CBehaviorGraphControlRigNode* node, CBehaviorGraphInstance* instance )
{
	m_node = node;
	m_instance = instance;
	m_instanceH = instance;
}

void CBehaviorGraphControlRigInterface::Clear()
{
	m_node = nullptr;
	m_instance = nullptr;
	m_instanceH = nullptr;
}

Bool CBehaviorGraphControlRigInterface::IsValid() const
{
	return m_node && m_instanceH.Get() && m_instance && m_instance->IsBinded();
}

void CBehaviorGraphControlRigInterface::SetManualControl( Bool flag )
{
	if ( IsValid() )
	{
		m_node->SetManualControl( *m_instance, flag );
	}
}

TCrInstance* CBehaviorGraphControlRigInterface::GetControlRig()
{
	return IsValid() ? m_node->GetControlRig( *m_instance ) : nullptr;
}

const TCrInstance* CBehaviorGraphControlRigInterface::GetControlRig() const
{
	return IsValid() ? m_node->GetControlRig( *m_instance ) : nullptr;
}
