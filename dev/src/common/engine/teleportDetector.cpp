/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "teleportDetector.h"
#include "teleportDetectorData.h"
#include "skeleton.h"
#include "renderFrame.h"
#include "baseEngine.h"

#ifdef DEBUG_ANIMS
#pragma optimize("",off)
#endif

CTeleportDetector::CTeleportDetector( const CSkeleton* sk, const Matrix& l2w )
	: m_poseChanged( false )
	, m_pelvisIndex( -1 )
	, m_pelvisPreviousPosWS( Vector::ZERO_3D_POINT )
	, m_pelvisPreviousPosMS( Vector::ZERO_3D_POINT )
	, m_pelvisPreviousDirectionsLS( 1.f, 0.f, 0.f )
	, m_angleDetectionThreshold( 15.f )
	, m_pelvisTranslationThresholdSqr( 0.025f )
	, m_pelvisChangedMS( false )
	, m_pelvisChangedWS( false )
	, m_enabledFlags( 0 )
	, m_requestedFlagsToReset( 0 )
	, m_forceUpdate( false )
#ifndef NO_EDITOR
	, m_lastUpdateTick( 0 )
#endif
{
	if ( const CTeleportDetectorData* skData = sk->GetTeleportDetectorData() )
	{
		m_angleDetectionThreshold = skData->GetTeleportAngleThreshold();
		const TDynArray< STeleportBone >& teleportedBones = skData->GetTeleportedBones();

		CachePelvisData( sk, l2w, skData );

		const Uint32 size = teleportedBones.Size();
		m_previousBonesTransLS.Resize( size );
		m_bonesIndices.Resize( size );
		m_bonesDirectionsLS.Resize( size );

#ifndef NO_EDITOR
		m_bonesChangedFlag.Resize( size );
		m_bonesChangedAngle.Resize( size );
#endif

		for ( Uint32 i = 0; i<size; ++i )
		{
			const Int32 boneId = sk->FindBoneByName( teleportedBones[i].m_boneName );
			m_bonesIndices[ i ] = boneId;
			m_previousBonesTransLS[ i ] = sk->GetBoneLS( boneId );

			switch( teleportedBones[ i ].m_teleportDetectionAxisLS )
			{
				case A_X	:	m_bonesDirectionsLS[ i ] = AnimVector4(  1.f,  0.f,  0.f, 1.f ); break;
				case A_Y	:	m_bonesDirectionsLS[ i ] = AnimVector4(  0.f,  1.f,  0.f, 1.f ); break;
				case A_Z	:	m_bonesDirectionsLS[ i ] = AnimVector4(  0.f,  0.f,  1.f, 1.f ); break;

				case A_NX	:	m_bonesDirectionsLS[ i ] = AnimVector4( -1.f,  0.f,  0.f, 1.f ); break;
				case A_NY	:	m_bonesDirectionsLS[ i ] = AnimVector4(  0.f, -1.f,  0.f, 1.f ); break;
				case A_NZ	:	m_bonesDirectionsLS[ i ] = AnimVector4(  0.f,  0.f, -1.f, 1.f ); break;

				default		:	m_bonesDirectionsLS[ i ] = AnimVector4(  1.f,  0.f,  0.f, 1.f ); break;
			}
		}
	}
}

void CTeleportDetector::SetEnabled( Uint32 enabledModeFlag )
{
	const Bool wasEnabled = IsEnabled();

	m_enabledFlags |= enabledModeFlag;
	RED_ASSERT( m_requestedFlagsToReset == 0 ); // Do not create overcomplicated cases

	const Bool isEnabled = IsEnabled();
	RED_ASSERT( isEnabled );

	if ( wasEnabled != isEnabled )
	{
		// Reset internal data
		m_poseChanged = false;
		m_pelvisChangedWS = false;
		m_pelvisChangedMS = false;
		m_pelvisPreviousPosWS = Vector::ZERO_3D_POINT;
		m_pelvisPreviousPosMS = Vector::ZERO_3D_POINT;

		const Uint32 size = m_bonesIndices.Size();
		for( Uint32 i=0; i<size; ++i )
		{
			m_previousBonesTransLS[ i ] = AnimQsTransform::IDENTITY;
#ifndef NO_EDITOR
			m_bonesChangedFlag[ i ] = false;
			m_bonesChangedAngle[ i ] = 0.f;
#endif
		}
	}
}

void CTeleportDetector::ResetEnabled( Uint32 enabledModeFlag )
{
	m_enabledFlags &= ~enabledModeFlag;
}

void CTeleportDetector::RequestSetDisabled( Uint32 enabledModeFlag )
{
	m_requestedFlagsToReset |= enabledModeFlag;
}

void CTeleportDetector::CheckTeleport( const AnimQsTransform* bonesLS, const Uint32 numBones, const TDynArray< Matrix >& bonesMS, const Matrix& currentL2w )
{
#ifndef NO_EDITOR
	const Uint64 currTick = GEngine->GetCurrentEngineTick();
	//RED_ASSERT( currTick != m_lastUpdateTick );
	m_lastUpdateTick = currTick;
#endif

	//clear flag
	m_poseChanged = false;
#ifndef NO_EDITOR
	const Uint32 size = m_bonesIndices.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		// clear those flags in editor mode only
		m_bonesChangedFlag[ i ] = false;
		m_bonesChangedAngle[ i ] = 0.f;
	}
#endif

	// Update if enabled
	if ( IsEnabled() || m_forceUpdate )
	{
		// clear force update flag
		m_forceUpdate = false;

		const Float cosAngleMax = MCos( DEG2RAD( m_angleDetectionThreshold ) );

		// first check pelvis
		DetectPelvisTeleport( bonesLS, numBones, bonesMS, currentL2w, cosAngleMax );

		const Uint32 size = m_bonesIndices.Size();
		for ( Uint32 i=0; i<size; ++i )
		{
			const Int32 boneIndexToCheck = m_bonesIndices[i];
			RED_ASSERT( boneIndexToCheck < (Int32)numBones );

			if( boneIndexToCheck != -1 && boneIndexToCheck < (Int32)numBones )
			{ 
				const AnimQsTransform& currBoneTrans = bonesLS[ boneIndexToCheck ];
				AnimQsTransform& prevBoneTrans = m_previousBonesTransLS[ i ];

#ifdef NO_EDITOR
				if ( !m_poseChanged )
#endif
				{
					const AnimVector4& boneDirLS = m_bonesDirectionsLS[ i ];
					
					AnimVector4 currDirLS;
					AnimVector4 prevDirLS;
					currDirLS.RotateDirection( currBoneTrans.Rotation, boneDirLS );
					prevDirLS.RotateDirection( prevBoneTrans.Rotation, boneDirLS );

					//dot3 = prevBoneTrans.Rotation.Quat.Dot3( currBoneTrans.Rotation.Quat );

					const Float dot3 = Dot3( currDirLS, prevDirLS );
					if ( dot3 < cosAngleMax )
					{
						m_poseChanged = true;
					}
#ifndef NO_EDITOR
					m_bonesChangedFlag[ i ] = dot3 < cosAngleMax;
					m_bonesChangedAngle[ i ] = RAD2DEG( MAcos_safe( dot3 ) );
#endif
				}
				prevBoneTrans = currBoneTrans;
			}
		}
	}

	// Process request to reset on the end update func because we want to maintain the same behavior until current frame ends
	if ( m_requestedFlagsToReset != 0 )
	{
		ResetEnabled( m_requestedFlagsToReset );
		m_requestedFlagsToReset = 0;
	}
}

#ifndef NO_EDITOR_FRAGMENTS
void CTeleportDetector::OnGenerateEditorFragments( CRenderFrame* frame, const TDynArray< Matrix >& poseWS ) const
{
	const Uint32 size = m_bonesIndices.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const Int32 boneIndexToCheck = m_bonesIndices[i];

		if ( boneIndexToCheck != -1 && boneIndexToCheck < poseWS.SizeInt() )
		{
			const Vector& point = poseWS[ boneIndexToCheck ].GetTranslationRef();

#ifndef NO_EDITOR
			const Color color = m_bonesChangedFlag[ i ] ? Color::GREEN : Color::RED;
			const Float radius = m_bonesChangedFlag[ i ] ? 0.04f : 0.02f;
#else
			const Color color = m_poseChanged ? Color::GREEN : Color::RED;
			const Float radius = m_poseChanged ? 0.04f : 0.02f;
#endif
			const String text = String::Printf( TXT("%.2f"), m_angleDetectionThreshold );

			frame->AddDebugText( point, text, 15, 0, true, color );
			frame->AddDebugSphere( point, radius, Matrix::IDENTITY, color, true, true );
		}
	}

	//do extra for pelvis
	{
		if ( m_pelvisIndex != -1 && m_pelvisIndex < poseWS.SizeInt() )
		{
			const Vector& point = poseWS[ m_pelvisIndex ].GetTranslationRef();

			const Color color = (m_pelvisChangedMS || m_pelvisChangedWS) ? Color::GREEN : Color::RED;
			const Float radius = (m_pelvisChangedMS || m_pelvisChangedWS) ? 0.04f : 0.02f;

			const String text = String::Printf( TXT("%.2f"), m_angleDetectionThreshold );

			frame->AddDebugText( point, text, 15, 0, true, color );
			frame->AddDebugSphere( point, radius, Matrix::IDENTITY, color, true, true );

			// state print
			const String state = IsEnabled() ? TXT("Enabled") : TXT("Disabled");
			const Color stateCol = IsEnabled() ? Color::GREEN : Color::RED;
			frame->AddDebugText( point, state, 0, -15, true, stateCol );
		}
	}
}
#endif //NO_EDITOR_FRAGMENTS

void CTeleportDetector::DetectPelvisTeleport( const AnimQsTransform* bonesLS, const Uint32 numBones, const TDynArray< Matrix >& bonesMS, const Matrix& currentL2w, const Float angleThreshold )
{
	m_poseChanged = false;
	m_pelvisChangedMS = false;
	m_pelvisChangedWS = false;
	//pelvis teleport check
	if( m_pelvisIndex != -1 && m_pelvisIndex < (Int32)numBones )
	{
		// gather some info about pelvis position
		const Vector& currentPelvisMS = bonesMS[ m_pelvisIndex ].GetTranslationRef();
		const Vector& currentPelvisWS = currentL2w.TransformPoint( currentPelvisMS );

		// model space position check
		if ( currentPelvisMS.DistanceSquaredTo( m_pelvisPreviousPosMS ) > m_pelvisTranslationThresholdSqr )
		{
			m_pelvisChangedMS = true;
		}
		// world space position check
		if ( currentPelvisWS.DistanceSquaredTo( m_pelvisPreviousPosWS ) > m_pelvisTranslationThresholdSqr )
		{
			m_pelvisChangedWS = true;
		}

		m_pelvisPreviousPosMS = currentPelvisMS;
		m_pelvisPreviousPosWS = currentPelvisWS;

		// rotation
		const AnimQsTransform& currBoneTrans = bonesLS[ m_pelvisIndex ];
		AnimQsTransform& prevBoneTrans = m_pelvisPreviousTransLS;
		const AnimVector4& boneDirLS = m_pelvisPreviousDirectionsLS;

		AnimVector4 currDirLS;
		AnimVector4 prevDirLS;
		currDirLS.RotateDirection( currBoneTrans.Rotation, boneDirLS );
		prevDirLS.RotateDirection( prevBoneTrans.Rotation, boneDirLS );

		const Float dot3 = Dot3( currDirLS, prevDirLS );
		if ( dot3 < angleThreshold )
		{
			m_poseChanged = true;
		}
		prevBoneTrans = currBoneTrans;
	}
}

void CTeleportDetector::CachePelvisData( const CSkeleton* sk, const Matrix& l2w, const CTeleportDetectorData* data )
{
	if ( sk && data )
	{
		const STeleportBone pelvisData = data->GetTeleportPelvisData();

		//pelvis case caching
		const Float pelvisTranslationThreshold = data->GetTeleportPositionThreshold();
		m_pelvisTranslationThresholdSqr = pelvisTranslationThreshold * pelvisTranslationThreshold;
		m_pelvisIndex = sk->FindBoneByName( pelvisData.m_boneName );
		const Vector pelvisPosMS = sk->GetBoneMatrixMS( m_pelvisIndex ).GetTranslation();
		m_pelvisPreviousPosMS = pelvisPosMS;
		m_pelvisPreviousPosWS = l2w.TransformPoint( pelvisPosMS );
		switch( pelvisData.m_teleportDetectionAxisLS )
		{
		case A_X	:	m_pelvisPreviousDirectionsLS = AnimVector4(  1.f,  0.f,  0.f, 1.f ); break;
		case A_Y	:	m_pelvisPreviousDirectionsLS = AnimVector4(  0.f,  1.f,  0.f, 1.f ); break;
		case A_Z	:	m_pelvisPreviousDirectionsLS = AnimVector4(  0.f,  0.f,  1.f, 1.f ); break;
		case A_NX	:	m_pelvisPreviousDirectionsLS = AnimVector4( -1.f,  0.f,  0.f, 1.f ); break;
		case A_NY	:	m_pelvisPreviousDirectionsLS = AnimVector4(  0.f, -1.f,  0.f, 1.f ); break;
		case A_NZ	:	m_pelvisPreviousDirectionsLS = AnimVector4(  0.f,  0.f, -1.f, 1.f ); break;
		default		:	m_pelvisPreviousDirectionsLS = AnimVector4(  1.f,  0.f,  0.f, 1.f ); break;
		}
		//end of pelvis caching
	}
}

#ifdef DEBUG_ANIMS
#pragma optimize("",on)
#endif
