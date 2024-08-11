/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behaviorGraphActorTilt.h"
#include "behaviorGraphInstance.h"
#include "behaviorGraphOutput.h"
#include "../core/instanceDataLayoutCompiler.h"

IMPLEMENT_ENGINE_CLASS( CBehaviorGraphActorTiltNode );

CBehaviorGraphActorTiltNode::CBehaviorGraphActorTiltNode()
	: m_scaleFactor( 0.005f )
	, m_scaleAxis( A_X )
	, m_leftThighWeight( 0.3f )
	, m_leftShinWeight( 1.f )
	, m_rightThighWeight( 0.3f )
	, m_rightShinWeight( 1.f )
	, m_leftThighBone( TXT("l_thigh") )
	, m_rightThighBone( TXT("r_thigh") )
	, m_leftShinBone( TXT("l_shin") )
	, m_rightShinBone( TXT("r_shin") )
{

}

void CBehaviorGraphActorTiltNode::OnBuildDataLayout( InstanceDataLayoutCompiler& compiler )
{
	TBaseClass::OnBuildDataLayout( compiler );

	compiler << i_leftThighBone;
	compiler << i_rightThighBone;
	compiler << i_leftShinBone;
	compiler << i_rightShinBone;
}

void CBehaviorGraphActorTiltNode::OnInitInstance( CBehaviorGraphInstance& instance ) const
{
	TBaseClass::OnInitInstance( instance );

	instance[ i_leftThighBone ] = FindBoneIndex( m_leftThighBone, instance );
	instance[ i_rightThighBone ] = FindBoneIndex( m_rightThighBone, instance );
	instance[ i_leftShinBone ] = FindBoneIndex( m_leftShinBone, instance );
	instance[ i_rightShinBone ] = FindBoneIndex( m_rightShinBone, instance );
}

void CBehaviorGraphActorTiltNode::Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const
{
	TBaseClass::Sample( context, instance, output );

	const Float currentAngle = instance[ i_currentAngle ];

	const Int32 leftThighBone = instance[ i_leftThighBone ];
	const Int32 rightThighBone = instance[ i_rightThighBone ];
	const Int32 leftShinBone = instance[ i_leftShinBone ];
	const Int32 rightShinBone = instance[ i_rightShinBone ];
	
	if ( leftThighBone != -1 && rightThighBone != -1 &&
		 leftShinBone != -1 && rightShinBone != -1 && currentAngle != 0.0f )
	{
#ifdef USE_HAVOK_ANIMATION
		output.m_outputPose[ leftThighBone ].m_translation( m_scaleAxis )	+= currentAngle * m_leftThighWeight * m_scaleFactor;
		output.m_outputPose[ rightThighBone ].m_translation( m_scaleAxis )	-= currentAngle * m_rightThighWeight * m_scaleFactor;
		output.m_outputPose[ leftShinBone ].m_translation( m_scaleAxis )	+= currentAngle * m_leftShinWeight * m_scaleFactor;
		output.m_outputPose[ rightShinBone ].m_translation( m_scaleAxis )	-= currentAngle * m_rightShinWeight * m_scaleFactor;
#else
		output.m_outputPose[ leftThighBone ].Translation.f[ m_scaleAxis ] += currentAngle * m_leftThighWeight * m_scaleFactor;
		output.m_outputPose[ rightThighBone ].Translation.f[ m_scaleAxis ] -= currentAngle * m_rightThighWeight * m_scaleFactor;
		output.m_outputPose[ leftShinBone ].Translation.f[ m_scaleAxis ] += currentAngle * m_leftShinWeight * m_scaleFactor;
		output.m_outputPose[ rightShinBone ].Translation.f[ m_scaleAxis ] -= currentAngle * m_rightShinWeight * m_scaleFactor;

#endif
	}
}
