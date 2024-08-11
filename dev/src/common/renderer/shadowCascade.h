/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "renderElementMeshChunk.h"
#include "renderElementApex.h"

class CRenderElement_Apex;
class CRenderFrameInfo;
class CCascadeShadowResources;

// Simple look at matrix
class LookMatrix2 : public Matrix
{
public:
	RED_INLINE LookMatrix2( const Vector& dir, const Vector& up )
	{
		Vector right = Vector::Cross( dir, up );
		V[0] = Vector::Cross( dir, up ).Normalized3();
		V[1] = dir;
		V[2] = Vector::Cross( right, dir ).Normalized3();
		V[3] = Vector::EW;
	}
};

/// Older, slower collector of elements that casts global shadows
struct SShadowCascade : public Red::System::NonCopyable
{
private:
	TDynArray< CRenderElement_MeshChunk* >		m_mergedSolidElements;	//!< Render merged shadow elements
	TDynArray< CRenderElement_MeshChunk* >		m_solidElements;		//!< Render elements that are solid ( faster pipe )
	TDynArray< CRenderElement_MeshChunk* >		m_discardElements;		//!< Render elements that have discards
	TDynArray< CRenderElement_Apex* >			m_apexElements;			//!< Apex proxies casting shadows

public:
	const TDynArray< CRenderElement_MeshChunk* >& GetMergedSolidElements() const { return m_mergedSolidElements; }
	const TDynArray< CRenderElement_MeshChunk* >& GetSolidElements() const { return m_solidElements; }
	const TDynArray< CRenderElement_MeshChunk* >& GetDiscardElements() const { return m_discardElements; }
	const TDynArray< CRenderElement_Apex* >& GetApexElements()	const { return m_apexElements; }

public:
	CRenderCamera								m_camera;
	CRenderCamera								m_jitterCamera;
	Uint8										m_cascadeIndex;			//!< Index of this cascade
	Float										m_pixelSize;			//!< Shadowmap meters per single pixels
	Float										m_invPixelSize;			//!< Shadowmap pixels per single meter
	Float										m_edgeFade;				//!< Percent of cascade to be faded

	SShadowCascade( const CRenderCamera& camera );
	SShadowCascade( );
	~SShadowCascade( );

	//! Reset
	void Reset();

	//! Push merged shadow element
	RED_INLINE void PushMergedShadowElement( CRenderElement_MeshChunk* element )
	{
		// Add fragment
		m_mergedSolidElements.PushBack( (CRenderElement_MeshChunk*) element );
	}

	//! Push shadow element
	RED_INLINE void PushShadowElement( CRenderElement_MeshChunk* element, Bool hasDiscards )
	{
		// Add to cascade fragments
		if ( hasDiscards )
		{
			m_discardElements.PushBack( (CRenderElement_MeshChunk*) element );
		}
		else
		{
			m_solidElements.PushBack( (CRenderElement_MeshChunk*) element );
		}
	}

	//! Push shadow element
	RED_INLINE void PushShadowElement( CRenderElement_Apex* element )
	{
		// Add fragment
		m_apexElements.PushBack( element );
	}
};

/// Optimized cascade shadows collector
struct SMergedShadowCascades
{
	CRenderCollector*	m_collector;					//!< The render collector
	Uint16				m_numCascades;					//!< Number of cascades to collect
	CFrustum			m_frustums[ MAX_CASCADES ];		//!< Cascade frustums, overlapping
	SShadowCascade		m_cascades[ MAX_CASCADES ];		//!< Output shadow cascades
	Uint8				m_frustumJitter;				//!< Jitter index used to flip frustum every frame

	// number of shadow cascades that are collected normally
	static const Uint16 MAX_NORMAL_CASCADES;

	// Minimal filter texel size prevents from subpixel filtering - sampling disk cannot be 
	// inside one texel, bcoz this will give regular linear sampling pattern.
	static const Float MIN_FILTER_SIZE;

	SMergedShadowCascades();

	void Init( CRenderCollector* collector, const CRenderFrameInfo &info, const CCascadeShadowResources &cascadeShadowResources );
	void Reset();

private:
	void CalculateData( const CRenderFrameInfo &info, const CCascadeShadowResources &cascadeShadowResources );
};