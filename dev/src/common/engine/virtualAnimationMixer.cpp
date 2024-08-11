
#include "build.h"
#include "virtualAnimationMixer.h"
#include "controlRig.h"
#include "controlRigDefinition.h"
#include "skeletalAnimationEntry.h"

VirtualAnimationMixer::VirtualAnimationMixer( const IVirtualAnimationContainer* c )
	: m_container( c )
#ifndef NO_EDITOR
	, m_listener( NULL )
#endif
{

}

void VirtualAnimationMixer::CalcMinMaxTime( Float& min, Float& max, EVirtualAnimationTrack track ) const
{
	{
		const TDynArray< VirtualAnimation >& anims = m_container->GetVirtualAnimations( track );

		const Uint32 size = anims.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const VirtualAnimation& anim = anims[ i ];

			if ( anim.m_time > min )
			{
				min = anim.m_time;
			}

			const Float endTime = anim.m_time + anim.GetDuration();
			if ( endTime > max )
			{
				max = endTime;
			}
		}
	}

	{
		const TDynArray< VirtualAnimationPoseFK >& fks = m_container->GetVirtualFKs();

		const Uint32 size = fks.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const VirtualAnimationPoseFK& data = fks[ i ];

			if ( data.m_time > min )
			{
				min = data.m_time;
			}
			if ( data.m_time > max )
			{
				max = data.m_time;
			}
		}
	}

	{
		const TDynArray< VirtualAnimationPoseIK >& iks = m_container->GetVirtualIKs();

		const Uint32 size = iks.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const VirtualAnimationPoseIK& data = iks[ i ];

			if ( data.m_time > min )
			{
				min = data.m_time;
			}
			if ( data.m_time > max )
			{
				max = data.m_time;
			}
		}
	}
}

void VirtualAnimationMixer::CalcMinMaxTime( Float& min, Float& max ) const
{
	min = 0.f;
	max = 0.033f;

	CalcMinMaxTime( min, max, VAT_Base );
	CalcMinMaxTime( min, max, VAT_Additive );
	CalcMinMaxTime( min, max, VAT_Override );
}

AnimQsTransform VirtualAnimationMixer::GetMovementAtTime( Float time ) const
{
	AnimQsTransform motion( AnimQsTransform::IDENTITY );

	motion = GetMovementBetweenTime( 0.f, time );

	return motion;
}

AnimQsTransform VirtualAnimationMixer::GetMovementBetweenTime( Float startTime, Float endTime ) const
{
	AnimQsTransform motion( AnimQsTransform::IDENTITY );

	const TDynArray< VirtualAnimation >& baseAnims = m_container->GetVirtualAnimations( VAT_Base );

	const Uint32 baseSize = baseAnims.Size();
	{
		for ( Uint32 i=0; i<baseSize; ++i )
		{
			const VirtualAnimation& vanim = baseAnims[ i ];
			if ( !vanim.m_useMotion )
			{
				continue;
			}

			Float timeForAnimA = 0.f;
			Float weightA = 0.f;
			Bool validA = false;

			Float timeForAnimB = 0.f;
			Float weightB = 0.f;
			Bool validB = false;

			validA = CalcBlendingParams( startTime, vanim, timeForAnimA, weightA );
			validB = CalcBlendingParams( endTime, vanim, timeForAnimB, weightB );

			// PTom TODO: Rethink this
			Int32 loops = endTime < startTime ? 1 : 0;

			if ( !validA && !validB && IsAnimInside( startTime, endTime, vanim ) )
			{
				validA = CalcBlendingParamsClamped( startTime, vanim, timeForAnimA, weightA );
				validB = CalcBlendingParamsClamped( endTime, vanim, timeForAnimB, weightB );
			}
			else if ( !validA && !validB )
			{
				continue;
			}
			else if ( !validA )
			{
				validA = CalcBlendingParamsClamped( startTime, vanim, timeForAnimA, weightA );
			}
			else if ( !validB )
			{
				validB = CalcBlendingParamsClamped( endTime, vanim, timeForAnimB, weightB );
			}

			if ( !validA || !validB )
			{
				continue;
			}

			ASSERT( timeForAnimA >= 0.f && timeForAnimA <= vanim.m_cachedAnimation->GetAnimation()->GetDuration() );
			ASSERT( timeForAnimB >= 0.f && timeForAnimB <= vanim.m_cachedAnimation->GetAnimation()->GetDuration() );

			AnimQsTransform motionA = vanim.m_cachedAnimation->GetAnimation()->GetMovementAtTime( timeForAnimA );
			AnimQsTransform motionB = vanim.m_cachedAnimation->GetAnimation()->GetMovementAtTime( timeForAnimB );

			if ( loops != 0 )
			{
				AnimQsTransform fullCycle; 
				fullCycle = vanim.m_cachedAnimation->GetAnimation()->GetMovementAtTime( vanim.m_cachedAnimation->GetAnimation()->GetDuration() );

				const Uint32 fabsloops = (loops < 0) ? -loops : loops;
				for (Uint32 l=0; l < fabsloops; l++)
				{
					if ( loops < 0 )
					{
						// Underflow
						AnimQsTransform temp;
#ifdef USE_HAVOK_ANIMATION
						temp.setMulInverseMul( fullCycle, motionB );
#else
						temp.SetMulInverseMul( fullCycle, motionB );
#endif
						motionB = temp;
					}
					else
					{
						// Overflow
						AnimQsTransform temp;
#ifdef USE_HAVOK_ANIMATION
						temp.setMul( fullCycle, motionB );
#else
						temp.SetMul( fullCycle, motionB );
#endif
						motionB = temp;
					}
				}
			}

			AnimQsTransform motionAB;
#ifdef USE_HAVOK_ANIMATION
			motionAB.setMulInverseMul( motionA, motionB );
#else
			motionAB.SetMulInverseMul( motionA, motionB );
#endif

			if ( weightB < 1.f )
			{
#ifdef USE_HAVOK_ANIMATION
				motionAB.setInterpolate4( hkQsTransform::getIdentity(), motionAB, weightB );
#else
				motionAB.Lerp( AnimQsTransform::IDENTITY, motionAB, weightB );
#endif
			}

#ifdef USE_HAVOK_ANIMATION
			if ( motionAB.m_translation( 1 ) < -0.1 )
			{
				LOG_ENGINE(TXT("DUPA"));
			}
			
			motion.setMulEq( motionAB );
#else
			if ( motionAB.Translation.Y < -0.1 )
			{
				LOG_ENGINE(TXT("DUPA"));
			}

			motion.SetMulEq( motionAB );
#endif
		}
	}

	return motion;
}

void VirtualAnimationMixer::ExtractBoneFromPose( Int32 boneToExtract, AnimQsTransform* bones, Uint32 num ) const
{
	ASSERT( num > 0 );
	ASSERT( boneToExtract >= 0 && boneToExtract < (Int32)num );

	AnimQsTransform& boneToExtractTrans = bones[ boneToExtract ];

	// Prevent error scale shit.
#ifdef USE_HAVOK_ANIMATION
	boneToExtractTrans.m_rotation.normalize();
#else
	boneToExtractTrans.Rotation.Normalize();
#endif

	AnimQsTransform boneToExtractTransInv;
#ifdef USE_HAVOK_ANIMATION
	boneToExtractTransInv.setInverse( boneToExtractTrans );
#else
	boneToExtractTransInv.SetInverse( boneToExtractTrans );
#endif

	/*const hkaSkeleton *havokSkeleton = animComponent->GetSkeleton()->GetHavokSkeleton();

	for (Uint32 boneIdx=0; boneIdx<m_numBones; boneIdx++)
	{
		hkQsTransform& bone = bones[boneIdx];
		Int32 parentIdx = (Int32)havokSkeleton->m_parentIndices[boneIdx];

		ASSERT(parentIdx < (Int32)boneIdx);

		if ( parentIdx == 0 )
		{
			bone.setMul( boneToExtractTransInv, bone );
		}
	}*/

	// To jest placeholder, nie potrzebujemy tego mechanizmu wiec to jest do wyjebania
#ifdef USE_HAVOK_ANIMATION
	bones[ 2 ].setMul( boneToExtractTransInv, bones[ 2 ] );
	bones[ 1 ].setIdentity();
	bones[ 0 ].setIdentity();
#else
	bones[ 2 ].SetMul( boneToExtractTransInv, bones[ 2 ] );
	bones[ 1 ].SetIdentity();
	bones[ 0 ].SetIdentity();
#endif
}

Bool VirtualAnimationMixer::CalcBlendingParams( Float time, const VirtualAnimation& vanim, Float& timeForAnim, Float& weight ) const
{
	if ( IsAnimValid( vanim ) && CanUseAnim( vanim, time ) )
	{
		timeForAnim = ( time - vanim.m_time ) * vanim.m_speed + vanim.m_startTime;
		ASSERT( timeForAnim >= vanim.m_startTime );
		ASSERT( timeForAnim <= vanim.m_endTime );
		ASSERT( vanim.m_blendIn > 0.f );
		ASSERT( vanim.m_blendOut > 0.f );

		timeForAnim = Clamp( timeForAnim, vanim.m_startTime, vanim.m_endTime );

		weight = vanim.m_weight;

		if ( vanim.m_blendIn > 0.f && timeForAnim - vanim.m_startTime < vanim.m_blendIn )
		{
			Float p = ( timeForAnim - vanim.m_startTime ) / vanim.m_blendIn;
			ASSERT( p >= 0.f && p <= 1.f );

			p = Clamp( p, 0.f, 1.f );

			weight *= p;
		}
		else if ( vanim.m_blendOut > 0.f && vanim.m_endTime - timeForAnim < vanim.m_blendOut )
		{
			Float p = ( vanim.m_endTime - timeForAnim ) / vanim.m_blendOut;
			ASSERT( p >= 0.f && p <= 1.f );

			p = Clamp( p, 0.f, 1.f );

			weight *= p;
		}

		ASSERT( weight >= 0.f && weight <= 1.f );

		return true;
	}

	timeForAnim = 0.f;
	weight = 0.f;

	return false;
}

Bool VirtualAnimationMixer::CalcBlendingParamsClamped( Float time, const VirtualAnimation& vanim, Float& timeForAnim, Float& weight ) const
{
	time = Clamp( time, vanim.m_time, vanim.m_time + vanim.GetDuration() );

	return CalcBlendingParams( time, vanim, timeForAnim, weight );
}

Bool VirtualAnimationMixer::Sample( Float time, Uint32 boneNumIn, Uint32 tracksNumIn, AnimQsTransform* bonesOut, AnimFloat* tracksOut, VirtualAnimationMixer::ControlRigSetup* csSetup ) const
{
	if ( boneNumIn > 128 || tracksNumIn > 16 )
	{
		return false;
	}
	AnimQsTransform bones[ 128 ];
	AnimFloat tracks[ 16 ];

	// 1.Base
	const TDynArray< VirtualAnimation >& baseAnims = m_container->GetVirtualAnimations( VAT_Base );

	Bool setZero = false;

	Float accWeight = 0.f;

	const Uint32 baseSize = baseAnims.Size();
	for ( Uint32 i=0; i<baseSize; ++i )
	{
		const VirtualAnimation& vanim = baseAnims[ i ];

		const Bool allBones = vanim.m_bones.Size() == 0;
		Float timeForAnim = 0.f;
		Float weight = 0.f;

		if ( CalcBlendingParams( time, vanim, timeForAnim, weight ) )
		{
			ASSERT( timeForAnim >= 0.f && timeForAnim <= vanim.m_cachedAnimation->GetAnimation()->GetDuration() );

			vanim.m_cachedAnimation->GetAnimation()->Sample( timeForAnim, boneNumIn, tracksNumIn, bones, tracks );
			if ( ShouldExtractBone( vanim, boneNumIn ) )
			{
				ExtractBoneFromPose( vanim.m_boneToExtract, bones, boneNumIn );
			}

			if ( !setZero )
			{
				BehaviorUtils::BlendingUtils::SetPoseZero( bonesOut, boneNumIn );
				setZero = true;
			}

			if ( allBones )
			{
				BehaviorUtils::BlendingUtils::BlendPosesNormal( bonesOut, bones, boneNumIn, weight );
			}
			else
			{
				BehaviorUtils::BlendingUtils::BlendPartialPosesNormal( bonesOut, bones, boneNumIn, weight, vanim.m_bones );
			}

			accWeight += weight;
		}
	}

	if ( setZero )
	{
		BehaviorUtils::BlendingUtils::RenormalizePose( bonesOut, boneNumIn, accWeight );
	}

	// 2. Override
	const TDynArray< VirtualAnimation >& overAnims = m_container->GetVirtualAnimations( VAT_Override );
	const Uint32 overSize = overAnims.Size();
	for ( Uint32 i=0; i<overSize; ++i )
	{
		const VirtualAnimation& vanim = overAnims[ i ];

		const Bool allBones = vanim.m_bones.Size() == 0;
		const Bool useWeights = vanim.m_weights.Size() != 0;
		Float timeForAnim = 0.f;
		Float weight = 0.f;

		if ( CalcBlendingParams( time, vanim, timeForAnim, weight ) )
		{
			ASSERT( timeForAnim >= 0.f && timeForAnim <= vanim.m_cachedAnimation->GetAnimation()->GetDuration() );

			vanim.m_cachedAnimation->GetAnimation()->Sample( timeForAnim, boneNumIn, tracksNumIn, bones, tracks );
			if ( ShouldExtractBone( vanim, boneNumIn ) )
			{
				ExtractBoneFromPose( vanim.m_boneToExtract, bones, boneNumIn );
			}

			if ( !setZero )
			{
				if ( !allBones )
				{
					BehaviorUtils::BlendingUtils::SetPoseIdentity( bonesOut, boneNumIn );
				}
				setZero = true;
			}

			if ( !allBones && useWeights )
			{
				ASSERT( vanim.m_weights.Size() == vanim.m_bones.Size() );

				BlendPartialPosesOverride( bonesOut, bones, boneNumIn, weight, vanim.m_bones, vanim.m_weights );
			}
			else if ( !allBones )
			{
				BlendPartialPosesOverride( bonesOut, bones, boneNumIn, weight, vanim.m_bones );
			}
			else
			{
				// Hmm this will be strange...
				BlendPosesOverride( bonesOut, bones, boneNumIn, weight );
			}
		}
	}

	// 3. Additive
	const TDynArray< VirtualAnimation >& addAnims = m_container->GetVirtualAnimations( VAT_Additive );

	const Uint32 addSize = addAnims.Size();
	for ( Uint32 i=0; i<addSize; ++i )
	{
		const VirtualAnimation& vanim = addAnims[ i ];

		const Bool allBones = vanim.m_bones.Size() == 0;
		Float timeForAnim = 0.f;
		Float weight = 0.f;

		if ( CalcBlendingParams( time, vanim, timeForAnim, weight ) )
		{
			ASSERT( timeForAnim >= 0.f && timeForAnim <= vanim.m_cachedAnimation->GetAnimation()->GetDuration() );

			vanim.m_cachedAnimation->GetAnimation()->Sample( timeForAnim, boneNumIn, tracksNumIn, bones, tracks );

			if ( !setZero )
			{
				BehaviorUtils::BlendingUtils::SetPoseIdentity( bonesOut, boneNumIn );
				setZero = true;
			}

			if ( allBones )
			{
				BlendPosesAdditive( bonesOut, bones, boneNumIn, weight );
			}
			else
			{
				BlendPartialPosesAdditive( bonesOut, bones, boneNumIn, weight, vanim.m_bones );
			}
		}
	}

#ifndef NO_EDITOR
	if ( m_listener )
	{
		m_listener->OnAnimationTracksSampled( boneNumIn, tracksNumIn, bones, tracks );
	}
#endif

	// 4. FK
	{
		const VirtualAnimationPoseFK* poseA = NULL;
		const VirtualAnimationPoseFK* poseB = NULL;
		Float weight = 0.f;

		if ( FindFkPoses( time, poseA, poseB, weight ) )
		{
			ASSERT( poseA );
			ASSERT( poseB );
			ASSERT( weight >= 0.f && weight <= 1.f );

			BlendFKPoses( bonesOut, boneNumIn, poseA, poseB, weight );
		}
	}

#ifndef NO_EDITOR
	if ( m_listener )
	{
		m_listener->OnFKTracksSampled( boneNumIn, tracksNumIn, bones, tracks );
	}
#endif

	// 5. IK
	if ( csSetup )
	{
		const VirtualAnimationPoseIK* poseA = NULL;
		const VirtualAnimationPoseIK* poseB = NULL;
		Float weight = 0.f;

		if ( FindIkPoses( time, poseA, poseB, weight ) )
		{
			ASSERT( poseA );
			ASSERT( poseB );
			ASSERT( weight >= 0.f && weight <= 1.f );

			BlendIKPoses( bonesOut, boneNumIn, poseA, poseB, weight, csSetup );
		}
	}

#ifndef NO_EDITOR
	if ( m_listener )
	{
		m_listener->OnIKTracksSampled( boneNumIn, tracksNumIn, bones, tracks );
	}
#endif

	// 6. Motion

#ifndef NO_EDITOR
	if ( m_listener )
	{
		m_listener->OnAllTracksSampled( boneNumIn, tracksNumIn, bones, tracks );
	}
#endif

	return true;
}

Bool VirtualAnimationMixer::Sample( Float time, TDynArray< AnimQsTransform >& bonesOut, TDynArray< AnimFloat >& tracksOut, VirtualAnimationMixer::ControlRigSetup* csSetup ) const
{
	return Sample( time, bonesOut.Size(), tracksOut.Size(), bonesOut.TypedData(), tracksOut.TypedData(), csSetup );
}

Bool VirtualAnimationMixer::FindFkPoses( Float time, const VirtualAnimationPoseFK*& poseA, const VirtualAnimationPoseFK*& poseB, Float& weight ) const
{
	// TODO - sort this array!!!

	const TDynArray< VirtualAnimationPoseFK >& fks = m_container->GetVirtualFKs();

	Int32 left = -1;
	Int32 right = -1;
	Float leftT = -NumericLimits< Float >::Max();
	Float rightT = NumericLimits< Float >::Max();

	const Uint32 fkSize = fks.Size();
	for ( Uint32 i=0; i<fkSize; ++i )
	{
		const VirtualAnimationPoseFK& pose = fks[ i ];

		const Float diff = time - pose.m_time;
		if ( diff >= 0.f && diff > leftT )
		{
			left = (Int32)i;
			leftT = pose.m_time;
		}
		if ( diff <= 0.f && diff < rightT )
		{
			right = (Int32)i;
			rightT = diff;
		}
	}

	if ( left != -1 && right != -1 )
	{
		if ( left != right && leftT != rightT )
		{
			ASSERT( leftT < rightT );

			weight = ( time - leftT ) / ( rightT - leftT ); 

			ASSERT( weight >= 0.f && weight <= 1.f );
		}
		else
		{
			weight = 0.f;
		}

		poseA = &(fks[ left ]);
		poseB = &(fks[ right ]);

		return true;
	}
	else if ( left != -1 )
	{
		poseA = &(fks[ left ]);
		poseB = &(fks[ left ]);

		weight = 1.f;

		return true;
	}
	else if ( right != -1 )
	{
		poseA = &(fks[ right ]);
		poseB = &(fks[ right ]);

		weight = 1.f;

		return true;
	}

	poseA = NULL;
	poseB = NULL;
	weight = 0.f;

	return false;
}

void VirtualAnimationMixer::BlendFKPoses( AnimQsTransform* bonesOut, Uint32 boneNumIn, const VirtualAnimationPoseFK* poseA, const VirtualAnimationPoseFK* poseB, Float weight ) const
{
	// TODO
	const Int32 size = (Int32)boneNumIn;

	const TDynArray< Int32 >& iPoseA = poseA->m_indices;
	const TDynArray< Int32 >& iPoseB = poseB->m_indices;

	const TEngineQsTransformArray& tPoseA = poseA->m_transforms;
	const TEngineQsTransformArray& tPoseB = poseB->m_transforms;

	for ( Int32 i=0; i<size; ++i )
	{
		const Int32 ptrA = static_cast< Int32 >( iPoseA.GetIndex( i ) );
		const Int32 ptrB = static_cast< Int32 >( iPoseB.GetIndex( i ) );
#ifdef USE_HAVOK_ANIMATION
		if ( ptrA != -1 && ptrB != -1 )
		{

			hkQsTransform toAdd;
			toAdd.setInterpolate4( TO_CONST_HK_TRANSFORM_REF( tPoseA[ ptrA ] ), TO_CONST_HK_TRANSFORM_REF( tPoseB[ ptrB ] ), weight );
			bonesOut[ i ].setMul( bonesOut[ i ], toAdd );
		}
		else if ( ptrA != -1 )
		{
			hkQsTransform toAdd;
			toAdd.setInterpolate4( hkQsTransform::getIdentity(), TO_CONST_HK_TRANSFORM_REF( tPoseA[ ptrA ] ), weight );
			bonesOut[ i ].setMul( bonesOut[ i ], toAdd );
		}
		else if ( ptrB != -1 )
		{
			hkQsTransform toAdd;
			toAdd.setInterpolate4( hkQsTransform::getIdentity(), TO_CONST_HK_TRANSFORM_REF( tPoseB[ ptrB ] ), weight );
			bonesOut[ i ].setMul( bonesOut[ i ], toAdd );
		}
#else
		if ( ptrA != -1 && ptrB != -1 )
		{

			RedQsTransform toAdd;
			toAdd.Lerp( reinterpret_cast< const RedQsTransform& >( tPoseA[ ptrA ] ), reinterpret_cast< const RedQsTransform& >( tPoseB[ ptrB ] ), weight );
			bonesOut[ i ].SetMul( bonesOut[ i ], toAdd );
		}
		else if ( ptrA != -1 )
		{
			RedQsTransform toAdd;
			toAdd.Lerp( RedQsTransform::IDENTITY, reinterpret_cast< const RedQsTransform& >( tPoseA[ ptrA ] ), weight );
			bonesOut[ i ].SetMul( bonesOut[ i ], toAdd );
		}
		else if ( ptrB != -1 )
		{
			RedQsTransform toAdd;
			toAdd.Lerp( RedQsTransform::IDENTITY, reinterpret_cast< const RedQsTransform& >( tPoseB[ ptrB ] ), weight );
			bonesOut[ i ].SetMul( bonesOut[ i ], toAdd );
		}
#endif
	}
}

Bool VirtualAnimationMixer::FindIkPoses( Float time, const VirtualAnimationPoseIK*& poseA, const VirtualAnimationPoseIK*& poseB, Float& weight ) const
{
	// TODO - sort this array!!!

	const TDynArray< VirtualAnimationPoseIK >& iks = m_container->GetVirtualIKs();

	Int32 left = -1;
	Int32 right = -1;
	Float leftT = -NumericLimits< Float >::Max();
	Float rightT = NumericLimits< Float >::Max();

	const Uint32 ikSize = iks.Size();
	for ( Uint32 i=0; i<ikSize; ++i )
	{
		const VirtualAnimationPoseIK& pose = iks[ i ];

		const Float diff = time - pose.m_time;
		if ( diff >= 0.f && diff > leftT )
		{
			left = (Int32)i;
			leftT = pose.m_time;
		}
		if ( diff <= 0.f && diff < rightT )
		{
			right = (Int32)i;
			rightT = diff;
		}
	}

	if ( left != -1 && right != -1 )
	{
		if ( left != right && leftT != rightT )
		{
			ASSERT( leftT < rightT );

			weight = ( time - leftT ) / ( rightT - leftT ); 

			ASSERT( weight >= 0.f && weight <= 1.f );
		}
		else
		{
			weight = 0.f;
		}

		poseA = &(iks[ left ]);
		poseB = &(iks[ right ]);

		return true;
	}
	else if ( left != -1 )
	{
		poseA = &(iks[ left ]);
		poseB = &(iks[ left ]);

		weight = 1.f;

		return true;
	}
	else if ( right != -1 )
	{
		poseA = &(iks[ right ]);
		poseB = &(iks[ right ]);

		weight = 1.f;

		return true;
	}

	poseA = NULL;
	poseB = NULL;
	weight = 0.f;

	return false;
}

void VirtualAnimationMixer::BlendIKPoses( AnimQsTransform* bonesOut, Uint32 boneNumIn, const VirtualAnimationPoseIK* poseA, const VirtualAnimationPoseIK* poseB, Float weight, VirtualAnimationMixer::ControlRigSetup* csSetup ) const
{
	if ( !csSetup->m_definition || !csSetup->m_instance )
	{
		return;
	}

	// TODO
	//const Int32 size = (Int32)boneNumIn;

	if ( poseA == poseB )
	{
		const TDynArray< ETCrEffectorId >& ids = poseA->m_ids;
		const Uint32 size = ids.Size();

		if ( size > 0 )
		{
			csSetup->m_definition->SetRigFromPoseLS( bonesOut, boneNumIn, csSetup->m_instance );

			csSetup->m_instance->SyncMSFromLS();
			csSetup->m_instance->SyncWSFromMS();

			for ( Uint32 i=0; i<size; ++i )
			{
				ETCrEffectorId effector = ids[ i ];
				const Vector& position = poseA->m_positionsMS[ i ];
				const Float weight = poseA->m_weights[ i ];

				csSetup->m_instance->SetEffectorWS( effector, position );
				csSetup->m_instance->SetTranslationActive( effector, weight );
				csSetup->m_instance->SetRotationActive( effector, 0.f );
			}

			csSetup->m_instance->Solve();

			//csSetup->m_instance->SyncLSFromMS();

			csSetup->m_definition->SetPoseLSFromRig( csSetup->m_instance, bonesOut, boneNumIn );

			csSetup->m_instance->ResetAllEffectors();
		}
	}
}

void VirtualAnimationMixer::BlendPosesAdditive( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight ) const
{
	const AnimFloat hkWeight = weight;
	if ( weight > 0.99f )
	{
		for ( Uint32 i=0; i<num; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			bonesA[ i ].setMulEq( bonesB[ i ] );
#else
			bonesA[ i ].SetMulEq( bonesB[ i ] );
#endif
		}
	}
	else
	{
		for ( Uint32 i=0; i<num; ++i )
		{
#ifdef USE_HAVOK_ANIMATION
			hkQsTransform temp;
			temp.setInterpolate4( hkQsTransform::getIdentity(), bonesB[ i ], hkWeight );
			bonesA[ i ].setMulEq( temp );
#else
			RedQsTransform temp;
			temp.Lerp( RedQsTransform::IDENTITY, bonesB[ i ], hkWeight );
			bonesA[ i ].SetMulEq( temp );
#endif
		}
	}
}

void VirtualAnimationMixer::BlendPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight ) const
{
#ifdef USE_HAVOK_ANIMATION
	const hkReal hkWeight = weight;
	for ( Uint32 i=0; i<num; ++i )
	{
		bonesA[ i ].setInterpolate4( bonesA[ i ], bonesB[ i ], hkWeight );
	}
#else
	const Float hkWeight = weight;
	for ( Uint32 i=0; i<num; ++i )
	{
		bonesA[ i ].Lerp( bonesA[ i ], bonesB[ i ], hkWeight );
	}
#endif
}

void VirtualAnimationMixer::BlendPartialPosesAdditive( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones ) const
{
	const Uint32 size = bones.Size();
	ASSERT( num < size );
	const AnimFloat hkWeight = weight;

	if ( weight > 0.99f )
	{
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 boneIdx = bones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < (Int32)num );
			ASSERT( boneIdx < (Int32)size );
#ifdef USE_HAVOK_ANIMATION
			bonesA[ boneIdx ].setMulEq( bonesB[ boneIdx ] );
#else
			bonesA[ boneIdx ].SetMulEq( bonesB[ boneIdx ] );
#endif
		}
	}
	else
	{
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 boneIdx = bones[ i ];

			ASSERT( boneIdx != -1 );
			ASSERT( boneIdx < (Int32)num );
			ASSERT( boneIdx < (Int32)size );
#ifdef USE_HAVOK_ANIMATION
			hkQsTransform temp;
			temp.setInterpolate4( hkQsTransform::getIdentity(), bonesB[ boneIdx ], hkWeight );
			bonesA[ boneIdx ].setMulEq( temp );
#else
			RedQsTransform temp;
			temp.Lerp( RedQsTransform::IDENTITY, bonesB[ boneIdx ], hkWeight );
			bonesA[ boneIdx ].SetMulEq( temp );
#endif
		}
	}
}

void VirtualAnimationMixer::BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones ) const
{
	const Uint32 size = bones.Size();
	ASSERT( num < size );
	const AnimFloat hkWeight = weight;
	for ( Uint32 i=0; i<size; ++i )
	{
		const Int32 boneIdx = bones[ i ];

		ASSERT( boneIdx != -1 );
		ASSERT( boneIdx < (Int32)num );
		ASSERT( boneIdx < (Int32)size );
#ifdef USE_HAVOK_ANIMATION
		bonesA[ boneIdx ].setInterpolate4( bonesA[ boneIdx ], bonesB[ boneIdx ], hkWeight );
#else
		bonesA[ boneIdx ].Lerp( bonesA[ boneIdx ], bonesB[ boneIdx ], hkWeight );
#endif
	}
}

void VirtualAnimationMixer::BlendPartialPosesOverride( AnimQsTransform* bonesA, AnimQsTransform* bonesB, Uint32 num, Float weight, const TDynArray< Int32 >& bones, const TDynArray< Float >& boneWeights ) const
{
	const Uint32 size = bones.Size();
	ASSERT( num > size );
	const AnimFloat hkWeight = weight;
	for ( Uint32 i=0; i<size; ++i )
	{
		const AnimFloat boneWeight = hkWeight * boneWeights[ i ];
		const Int32 boneIdx = bones[ i ];

		ASSERT( boneIdx != -1 );
		ASSERT( boneIdx < (Int32)num );
		ASSERT( boneIdx < (Int32)size );
#ifdef USE_HAVOK_ANIMATION
		bonesA[ boneIdx ].setInterpolate4( bonesA[ boneIdx ], bonesB[ boneIdx ], boneWeight );
#else
		bonesA[ boneIdx ].Lerp( bonesA[ boneIdx ], bonesB[ boneIdx ], boneWeight );
#endif
	}
}
