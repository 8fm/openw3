
#include "build.h"
#include "expSlider.h"

void ExpSimpleSlider::Setup( Float transStart, Float transEnd, Float rotStart, Float rotEnd )
{
	m_startTrans = transStart;
	m_endTrans = transEnd;

	m_startRot = rotStart;
	m_endRot = rotEnd;
}

ESliderResult ExpSimpleSlider::Update( AnimQsTransform& delta, const AnimQuaternion& exRotation, const AnimVector4& exTranslation, Float prevTime, Float currTime, const CNode* node, const Vector& targetPoint, Float targetYaw, Float ifYawDiffExceeds, Bool allowGoingBeyondEnd )
{
#ifdef USE_HAVOK_ANIMATION
	delta.setIdentity();
#else
	delta.SetIdentity();
#endif
	ESliderResult slided = SR_NotSliding;

	// Translation
	if( currTime > m_startTrans && ( allowGoingBeyondEnd || prevTime < m_endTrans ) )
	{
		const Vector& nodePos = node->GetWorldPositionRef();
#ifdef USE_HAVOK_ANIMATION
		const Vector& exTrans = TO_CONST_VECTOR_REF( exTranslation );
#else
		const Vector& exTrans = reinterpret_cast< const Vector& >( exTranslation );
#endif
		Matrix localToWorld;
		node->GetLocalToWorld( localToWorld );
		const Vector finalExPos = nodePos + localToWorld.TransformVector( exTrans );

		const Vector remainingTrans = targetPoint - finalExPos;

		const Float timeDeltaTrans = ( currTime < m_endTrans ? currTime : m_endTrans ) - ( prevTime > m_startTrans ? prevTime : m_startTrans );
		const Float remainingDurationTrans = m_endTrans - ( prevTime > m_startTrans ? prevTime : m_startTrans );
		// allow sliding after end time to adjust to moving object
		//ASSERT( remainingDurationTrans > 0.f );

		const Float ratio = remainingDurationTrans > 0.0f? timeDeltaTrans / remainingDurationTrans : 1.0f;
		const Vector deltaTrans = remainingTrans * ratio;

		Matrix worldToLocal;
		node->GetWorldToLocal( worldToLocal );
#ifdef USE_HAVOK_ANIMATION
		delta.m_translation.setXYZ( TO_CONST_HK_VECTOR_REF( worldToLocal.TransformVector( deltaTrans ) ) );
#else
		Vector tempVecStore = worldToLocal.TransformVector( deltaTrans );
		RedVector4 tempRedVecStore = reinterpret_cast< const RedVector4& >( tempVecStore );
		delta.Translation.Set( tempRedVecStore.X, tempRedVecStore.Y, tempRedVecStore.Z, 0.0f);
#endif
		if( ratio == 1.f )
		{
			slided = SR_FinishedSliding;
		}
		else
		{
			slided = SR_Sliding;
		}
	}

	// Rotation
	if( m_endRot > 0.f && currTime > m_startRot && ( allowGoingBeyondEnd || prevTime < m_endRot ) )
	{
		const Float nodeYaw = node->GetWorldYaw();
#ifdef USE_HAVOK_ANIMATION
		const Float exYaw = HavokQuaternionToYaw( exRotation );
#else
		const Float exYaw = exRotation.GetYaw();
#endif
		
		const Float finalExYaw = EulerAngles::NormalizeAngle( nodeYaw + exYaw );

		Float remainingYaw = EulerAngles::AngleDistance( finalExYaw, targetYaw );
		if ( remainingYaw > ifYawDiffExceeds )
		{
			remainingYaw -= ifYawDiffExceeds;
		}
		else if ( remainingYaw < -ifYawDiffExceeds )
		{
			remainingYaw += ifYawDiffExceeds;
		}
		else
		{
			remainingYaw = 0.0f;
		}

		const Float timeDeltaRot = ( currTime < m_endRot ? currTime : m_endRot ) - ( prevTime > m_startRot ? prevTime : m_startRot );
		const Float remainingDurationRot = m_endRot - ( prevTime > m_startRot ? prevTime : m_startRot );
		// allow sliding after end time to adjust to moving object
		//ASSERT( remainingDurationRot > 0.f );

		const Float ratio = remainingDurationRot > 0.0f? timeDeltaRot / remainingDurationRot : 1.0f;
		
		const Float deltaYaw = remainingYaw * ratio;
#ifdef USE_HAVOK_ANIMATION
		delta.m_rotation.setAxisAngle( hkVector4( 0.f, 0.f, 1.f ), DEG2RAD( deltaYaw ) );
#else
		delta.Rotation.SetAxisAngle( RedVector4( 0.0f, 0.0f, 1.0f, 0.0f ), DEG2RAD( deltaYaw ) );
#endif
		// Hmm... Lets say its temporary :) I don't care about the rotation right now...
		if( slided != SR_FinishedSliding )
		{
			slided = SR_Sliding;
		}
	}

	return slided;
}