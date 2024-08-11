/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "renderDissolve.h"
#include "renderMesh.h"
#include "renderHelpers.h"
#include "../core/math.h"
#include "../core/configVar.h"

namespace Config
{
	TConfigVar< Float >		cvDissolveSpeed( "Rendering/Dissolve", "DissolveSpeed", 500.0f ); // full dissolve range is 255 ticks, 500 ticks is around ~0.5s for full dissolve
}

using namespace DissolveHelpers;

//----------------------------------------------------------------------

/// HOW THE DISSOLVE VALUE IS CALCULATED
///
/// Dissolve value represents the visibility factor
///    Value of 0 - fully dissolved object, not visible
///    Value of 255 - not dissolved object, fully visible (no alphatested pixels)
///
/// The CRenderDissolveSynchronizer is used for synchronization and is keeping the global timing.
/// The time base used is normalized to the 256 units, the dissolve speed is specified in units/second.
/// Fractional part of the time base is kept only in the synchronizer.
///
/// Basic idea of the fading is to remember the time stamp of the fadeout/fadein event and from that 
/// (and the current time base value) calculate the actual alpha. To indicate if we are fading in or out
/// we use the sign of the time stamp value.
///   Value is positive/zero - we are fading in
///    Value is negative - we are fading out

/// Kamil's change:
/// I have changed distance to quantized Uint32, bcoz float and integers are binary monotony and
/// might be fast compared

//----------------------------------------------------------------------

CRenderDissolveSynchronizer::CRenderDissolveSynchronizer()
	: m_timeIndex( 1000 )
	, m_leftOver( 0.0f )
{
}

void CRenderDissolveSynchronizer::Advance( const Float dt )
{
	// advance by the integer part
	if ( dt > 0.0f )
	{
		const Float adv = dt * Config::cvDissolveSpeed.Get();
		m_leftOver += adv;
		
		const Int32 leftOverI = (Int32) m_leftOver;
		m_timeIndex += leftOverI;

		m_leftOver = m_leftOver - (Float)leftOverI;
	}

	// If current frame used to be invalidated. Its done
	m_requestFinish = false;
}

void CRenderDissolveSynchronizer::Finish()
{
	m_timeIndex += 256;
	m_leftOver = 0.0f;

	// Mark current frame as beeing in instant dissolve state
	m_requestFinish = true;
}

//----------------------------------------------------------------------

void CRenderDissolveValue::StartFadeIn( const CRenderDissolveSynchronizer& sc )
{
	m_marker = sc.GetValue();
}

void CRenderDissolveValue::StartFadeOut( const CRenderDissolveSynchronizer& sc )
{
	m_marker = -sc.GetValue();
}

void CRenderDissolveValue::FadeIn( const CRenderDissolveSynchronizer& sc )
{
	if ( m_marker < 0 )
	{
		const int dm = sc.GetValue() + m_marker;
		const int c = (dm <= 0) ? 0 : ((dm >= 256) ? 256 : dm);
		const int v = 256 - c;
		m_marker = sc.GetValue() - v;
	}
}

void CRenderDissolveValue::FadeOut( const CRenderDissolveSynchronizer& sc )
{
	if ( m_marker >= 0 )
	{
		const int dm = sc.GetValue() - m_marker;
		const int c = (dm <= 0) ? 0 : ((dm >= 256) ? 256 : dm);
		const int v = c;
		m_marker = -sc.GetValue() + 256 - v;
	}
}

//----------------------------------------------------------------------

CRenderLODSelector::CRenderLODSelector()
	: m_currentLod( -2 )
	, m_pendingLod( -2 )
	, m_numLODs( 0 )
{
	m_dissolve.SetAlphaOne();
}

void CRenderLODSelector::SetupFromMesh( const CRenderMesh* mesh, const Float autoHideDistance, const Float autoHideDistanceSquared )
{
	RED_FATAL_ASSERT( mesh != nullptr, "No mesh specified" );

	// Reset LODs
	m_numLODs = 0;

	// Copy LODs that are closer than the auto hide
	const auto& lods = mesh->GetLODs();
	for ( Uint32 lodIndex = 0; lodIndex < lods.Size() && lodIndex < MAX_LODS; ++lodIndex )
	{
		const Float distance = lods[lodIndex].m_distance;

		// there's no point in including LOD that is not going to be visible
		if ( distance > autoHideDistance )
			break;
		
		const Float distanceSq = distance*distance;

		m_lodDistancesSq[ lodIndex ] = reinterpret_cast<const Uint32&>(distanceSq);
		m_numLODs += 1;
	}

	// Last entry is always the autohide squared
	m_lodDistancesSq[ m_numLODs ] = reinterpret_cast<const Uint32&>( autoHideDistanceSquared );
}

void CRenderLODSelector::SetupSingle( const Float autoHideDistanceSquared )
{
	m_numLODs = 1;
	m_lodDistancesSq[0] = 0;
	m_lodDistancesSq[1] = reinterpret_cast<const Uint32&>( autoHideDistanceSquared );
}

const Int8 CRenderLODSelector::ComputeBestLOD( const Float distSq, const Float visDistSq ) const
{
	// LOD distance table:
	//  [0]: sq distance of LOD0
	//  [1]: sq distance of LOD1
	//  [.]: sq distance of LODx
	//  [N]: auto hide distance sq
	// returned:
	//  N such that (dist >= LOD[N]) && (dist < LOD[N-1])

	// Visibility range
	// W3 Hack: this is needed because our beloved environment artists want the LOD to be selected based on the DIFFERENT position than the visibility :)
	const Uint32 hideDistanceSq = m_lodDistancesSq[ m_numLODs ];
	if ( reinterpret_cast<const Uint32&>( visDistSq ) > hideDistanceSq )
		return -1;

	const Uint32 distSqQuantized = reinterpret_cast<const Uint32&>(distSq);

	if( distSqQuantized < m_lodDistancesSq[0] )
		return -1;

	// LOD selection
	for ( Uint8 i=0; i < m_numLODs; ++i )
	{
		if ( distSqQuantized < m_lodDistancesSq[i+1] )
			return i;
	}

	// no LOD to draw
	return -1;
}

void CRenderLODSelector::ForceLOD( const Int8 lodLevel )
{
	m_dissolve.SetAlphaOne();
	m_currentLod = lodLevel;
	m_pendingLod = lodLevel;
}

void CRenderLODSelector::ComputeLOD( const Float distanceSq, const Float visDistSq, const Bool allowDissolve, const CRenderDissolveSynchronizer& sc, const Int8 minLOD/*=-1*/, const Int8 maxLOD/*=100*/ )
{
	// we are during transition, don't bother
	if ( m_dissolve.IsFading(sc) && allowDissolve )
		return;

	// we finished fading, synchronize LOD
	m_currentLod = m_pendingLod;

	// get the LOD we would like to have, if it's the same don't do anything
	const Int8 requestedLOD = Clamp< Int8 >( ComputeBestLOD( distanceSq, visDistSq ), minLOD, maxLOD );
	RED_FATAL_ASSERT( requestedLOD >= -1 && requestedLOD < m_numLODs, "Invalid LOD index [requestedLOD: %d, m_numLODs: %d ]" , (Uint32)requestedLOD , (Uint32)m_numLODs );

	// Don't dissolve if lodIndex == -2. -2 is set when first adding to the scene, so we want to just instantly select the proper LOD in this case.
	if ( !allowDissolve || m_currentLod == -2 ) 
	{
		// we were not visible last frame, there's no need for any dissolve
		m_currentLod = requestedLOD;
		m_pendingLod = requestedLOD;
		m_dissolve.SetAlphaOne();
		return;
	}

	// make sure the LOD fade alpha is always at 1 when we are not dissolving
	m_dissolve.SetAlphaOne();

	// we are already at the target
	if ( m_currentLod == requestedLOD )
		return;

	// set target LOD and start dissolving
	m_pendingLod = requestedLOD;
	if( sc.IsFinishPending() )
	{
		// If we are in dissolve invalidation frame. Just jump into desired LOD
		m_currentLod = requestedLOD;
	}
	else
	{
		m_dissolve.StartFadeOut( sc ); // fade out current LOD -> next LOD
	}
}

Vector CRenderLODSelector::ComputeDissolveValue( const Int8 renderLodIndex, const CRenderDissolveSynchronizer& sc, const CRenderDissolveAlpha baseAlpha ) const
{
	const auto lodDissolve = m_dissolve.ComputeAlpha( sc ); // progression between m_currentLOD and m_pendingLOD

	Vector ret = Vector::ZEROS;

	if ( renderLodIndex == m_currentLod )
	{
		// fade out
		const auto mergedDissolve = lodDissolve * baseAlpha;
		ret = CalculateBiasVectorPatternPositive( mergedDissolve.ToFraction() );
	}
	else
	{
		// fade in
		const auto mergedDissolve = lodDissolve.Inverted() * baseAlpha;
		ret = CalculateBiasVectorPatternNegative( mergedDissolve.ToFraction() );
	}
	return ret;
}
