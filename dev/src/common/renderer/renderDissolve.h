#pragma once

//-----------------------------------------------------

typedef Int32 TDissolveMarker;

namespace DissolveHelpers
{

	RED_FORCE_INLINE static Vector CalculateBiasVectorPatternPositive( Float alpha )
	{
		return Vector( 1.0f, -1.0f + alpha, 0.0f, 0.0f );
	}

	RED_FORCE_INLINE static Vector CalculateBiasVectorPatternNegative( Float alpha )
	{
		return Vector( -1.0f, alpha, 0.0f, 0.0f );
	}

	// We need to support both -1 and -2 (initial) lod index
	RED_FORCE_INLINE static Bool IsLodVisible( Int32 lod ) { return lod >= 0; }

}

//-----------------------------------------------------

/// Dissolve synchronizer
class CRenderDissolveSynchronizer
{
public:
	CRenderDissolveSynchronizer();

	// Advance using normal DT
	void Advance( const Float dt );

	// Force dissolves to finish this frame
	void Finish();

	// Get current marker position
	RED_INLINE const TDissolveMarker GetValue() const { return m_timeIndex; }

	RED_INLINE Bool IsFinishPending() const { return m_requestFinish; }
	
private:
	TDissolveMarker		m_timeIndex; // actual time index
	Float				m_leftOver; // factional part
	Bool				m_requestFinish;
};

//-----------------------------------------------------

/// Dissolve alpha
class CRenderDissolveAlpha
{
public:
	RED_FORCE_INLINE CRenderDissolveAlpha()
		: m_value(256)
	{}

	RED_FORCE_INLINE CRenderDissolveAlpha( const Uint16 value )
		: m_value( value )
	{
		RED_FATAL_ASSERT( value <= 256, "Invalid input value" );
	}
	
	RED_FORCE_INLINE CRenderDissolveAlpha operator*( const CRenderDissolveAlpha& other ) const
	{
		RED_FATAL_ASSERT( other.m_value <= 256, "Invalid input value" );
		Uint32 ret = ((Uint32)m_value * (Uint32)other.m_value) >> 8;
		return CRenderDissolveAlpha( (Uint16) ret );
	}

	RED_FORCE_INLINE const Float ToFraction() const
	{
		return m_value / 256.0f;
	}

	RED_FORCE_INLINE const CRenderDissolveAlpha Inverted() const
	{
		return CRenderDissolveAlpha( 256 - m_value );
	}

	RED_FORCE_INLINE const Bool IsZero() const
	{
		return m_value == 0;
	}

	RED_FORCE_INLINE const Bool IsOne() const
	{
		return m_value == 256;
	}

	RED_FORCE_INLINE void SetZero()
	{
		m_value = 0;
	}

	RED_FORCE_INLINE void SetOne()
	{
		m_value = 256;
	}

	RED_FORCE_INLINE CRenderDissolveAlpha GetMappedUpperHalfRange() const
	{
		return CRenderDissolveAlpha( m_value >= 128 ? (m_value - 128) << 1 : 0 );
	}

	RED_FORCE_INLINE CRenderDissolveAlpha GetMappedLowerHalfRange() const
	{
		return CRenderDissolveAlpha( m_value >= 128 ? 256 : m_value << 1 );
	}

private:
	Uint16		m_value;
};

//-----------------------------------------------------

/// Dissolve value helper - tickless fadein/fadeout
class CRenderDissolveValue
{
public:
	RED_INLINE CRenderDissolveValue()
		: m_marker( 0 ) // fully visible
	{}

	// reset - to either full visibility or no visibility
	RED_INLINE void SetAlphaZero() { m_marker = -1; }
	RED_INLINE void SetAlphaOne() { m_marker = 0; }

	// fadeout control
	void StartFadeIn( const CRenderDissolveSynchronizer& sc );
	void StartFadeOut( const CRenderDissolveSynchronizer& sc );
	void FadeIn( const CRenderDissolveSynchronizer& sc );
	void FadeOut( const CRenderDissolveSynchronizer& sc );

	// Finish the current transition
	RED_FORCE_INLINE void Finish()
	{
		const Bool sign = (m_marker >= 0);
		m_marker = sign ? 0 : -1;
	}

	// compute current alpha value (8 bits)
	RED_FORCE_INLINE const CRenderDissolveAlpha ComputeAlpha( const CRenderDissolveSynchronizer& sc ) const
	{
		// (fade in): if marker >= 0: alpha = clamp(0, 256, timebase - abs(marker)) 
		// (fade out): if marker < 0: alpha = 256 - clamp(0, 256, timebase - abs(marker));

		const Bool sign = (m_marker >= 0);
		const Int32 dm = sc.GetValue() - ( sign ? m_marker : -m_marker ); // timebase - abs(marker) 
		const Int32 c = (dm <= 0) ? 0 : ((dm >= 256) ? 256 : dm); // clamp(0, 256, timebase - abs(marker))
		return CRenderDissolveAlpha( sign ? c : 256 - c );
	}

	// are we during transition ? 
	RED_FORCE_INLINE const Bool IsFading( const CRenderDissolveSynchronizer& sc ) const
	{
		const Bool sign = (m_marker >= 0);
		const Int32 dm = sc.GetValue() - ( sign ? m_marker : -m_marker ); // timebase - abs(marker) 
		return dm < 256; // not finished fading
	}

	// are we visible (alpha > 0) ?
	RED_FORCE_INLINE const Bool IsVisible( const CRenderDissolveSynchronizer& sc ) const
	{
		return !ComputeAlpha(sc).IsZero();
	}

private:
	TDissolveMarker	m_marker; // 0 for no dissolve
};

//-----------------------------------------------------

/// LOD calculation helper
class CRenderLODSelector
{
public:
	CRenderLODSelector();

	/// Get number of LODs (does not include the autohide)
	RED_INLINE const Uint32 GetNumLODs() const { return m_numLODs; }

	// Are we during LOD change ?
	RED_INLINE const Bool IsDuringLODChange() const { return m_pendingLod != m_currentLod; }

	// Is object visible based on LODs state
	RED_INLINE const Bool IsVisible() const { return !(m_pendingLod == -1 && m_currentLod == -1); }

	/// Setup LOD distances, LODs with show distance < autoHide are not used
	void SetupFromMesh( const class CRenderMesh* mesh, const Float autoHideDistance, const Float autoHideDistanceSquared );

	/// Setup single LOD
	void SetupSingle( const Float autoHideDistance );

	/// Force a given LOD (resets dissolve)
	void ForceLOD( const Int8 lodLevel );
	
	/// Select best LOD using specified LOD distance table. Note: everything is using square distances
	void ComputeLOD( const Float distSq, const Float visDistSq, const Bool allowDissolve, const CRenderDissolveSynchronizer& sc, const Int8 minLOD=-1, const Int8 maxLOD=100 );

	// Compute final dissolve parameters for rendering given LOD, it can include general alpha of the whole thing
	Vector ComputeDissolveValue( const Int8 renderLodIndex, const CRenderDissolveSynchronizer& sc, const CRenderDissolveAlpha baseAlpha ) const;

	// Select best LOD
	const Int8 ComputeBestLOD( const Float distSq, const Float visDistSq ) const;

public:
	// Get indices of the LODs to render, -1 is specified if the given LOD is not valid, both may be the same
	RED_FORCE_INLINE void GetLODIndices( Int32& outBaseLOD, Int32& outSecondLOD ) const
	{
		RED_FATAL_ASSERT( m_currentLod >= -2 && m_currentLod < m_numLODs, "Invalid LOD index[ currenbtLOD: %d, numLODs: %d ]" , (Uint32)m_currentLod , (Uint32)m_numLODs );
		RED_FATAL_ASSERT( m_pendingLod >= -2 && m_pendingLod < m_numLODs, "Invalid LOD index[ pendingLod: %d, numLODs: %d ]" , (Uint32)m_pendingLod , (Uint32)m_numLODs );

		outBaseLOD = m_currentLod;
		outSecondLOD = m_pendingLod;
	}

private:
	static const Uint32 MAX_LODS = 4;

	Int8						m_pendingLod;	// pending LOD index (during transition)
	Int8						m_currentLod;	// current LOD index
	Uint8						m_numLODs;		// number of LODs
	CRenderDissolveValue		m_dissolve;		// dissolve between LODs

	// square distances of the LODs, the last entry is ALWAYS auto hide distance
	Uint32						m_lodDistancesSq[ MAX_LODS+1 ];	
};

//-----------------------------------------------------
