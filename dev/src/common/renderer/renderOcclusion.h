/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../engine/renderObject.h"
#include "../engine/umbraStructures.h"
#include "../engine/umbraIncludes.h"

struct CharacterOcclusionInfo
{
	DECLARE_STRUCT_MEMORY_POOL_ALIGNED( MemoryPool_Default, MC_RenderOcclusionData, 16 );

	Box		m_boundingBox;
	Vector	m_position;
	Bool	m_positionVisible;
};

#ifdef USE_UMBRA

class CRenderSceneEx;
class UmbraOcclusionBuffer;
class UmbraRenderer;

class UmbraOcclusionBuffer : public Umbra::OcclusionBuffer, public IRenderObject
{

};

namespace Umbra
{
	class TomeCollection;
	class OcclusionBuffer;
}

#endif // USE_UMBRA


// TODO : This could be some basic Results class, and CRenderOcclusionData could inherit it? Keeping em separate for simplicity.
class ExtraOcclusionQueryResults : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderOcclusionData )

private:
#ifdef USE_UMBRA
	UmbraOcclusionBuffer*				m_occlusionBuffer;
#endif // USE_UMBRA
	TVisibleChunksIndices				m_visibleObjectsIndices;

public:
#ifdef USE_UMBRA
	ExtraOcclusionQueryResults( TVisibleChunksIndices&& visibleObjects, UmbraOcclusionBuffer* occlusionBuffer );
#else
	ExtraOcclusionQueryResults( TVisibleChunksIndices&& visibleObjects );
#endif
	virtual ~ExtraOcclusionQueryResults();

	const TVisibleChunksIndices& GetVisibleStatics() const { return m_visibleObjectsIndices; }

	RED_INLINE Bool IsDynamicObjectVisible( const Box& bBox ) const;
	RED_INLINE Bool IsDynamicObjectVisible( const Vector3& bbMin, const Vector3& bbMax ) const;
	RED_INLINE Uint32 IsDynamicObjectVisibleFullTest( const Box& bBox ) const;
};


#ifdef USE_UMBRA
class CRenderOcclusionData : public IRenderObject
{
	DECLARE_RENDER_OBJECT_MEMORYCLASS( MC_RenderOcclusionData )

public:
	static const	TObjectIdType	NONSTATIC_OBJECT_ID = 0xFFFFFFFF;
	static			Uint8*			s_extraOcclusionQueryAdditionalMemory;
	static const	MemSize			s_extraOcclusionQueryAdditionalMemorySize;

	static Bool		InitExtraMemory();
	static Bool		ShutdownExtraMemory();

public:
	CRenderOcclusionData();
	CRenderOcclusionData( const CUmbraScene* scene );
	virtual ~CRenderOcclusionData();

public:
	RED_INLINE Float						GetQueryThreshold() const				{ return m_queryThreshold; }
	RED_INLINE void							SetQueryThreshold( Float val )			{ m_queryThreshold = val; }
	RED_INLINE Float						GetDataDistanceThresholdSquared() const	{ return m_dataDistanceThresholdSquared; }
	RED_INLINE Bool							HasOcclusionDataReady() const			{ return m_occlusionBuffer != nullptr; }
	RED_INLINE const Umbra::TomeCollection* GetTomeCollection() const				{ return m_tomeCollection; }
	RED_INLINE Bool							IsInCutsceneMode() const				{ return m_isCutscene; }

	RED_INLINE Bool	IsDynamicObjectVisible( const Box& bBox ) const;
	RED_INLINE Bool	IsDynamicObjectVisible( const Vector3& bbMin, const Vector3& bbMax ) const;
	RED_INLINE Uint32 IsDynamicObjectVisibleFullTest( const Box& bBox ) const;
	RED_INLINE Bool IsDynamicSphereVisible( const Vector& center, const Float radius ) const;
	RED_INLINE void SetCutsceneModeForGates( Bool isCutscene ) { m_isCutscene = isCutscene; }
	RED_INLINE void SetCascadesData( const CFrustum* cascades, Uint32 numberOfCascades )
	{
		m_numberOfCascades = numberOfCascades;
		for ( Uint32 i = 0; i < numberOfCascades; ++i )
		{
			m_cascades[i] = cascades[i];
		}
	}

	RED_INLINE Bool IsDynamicObjectShadowVisibleInCascade( const Box& box, const Uint32 cascadeNum ) const
	{
		RED_FATAL_ASSERT( cascadeNum < m_numberOfCascades, "Invalid cascade" );
		return m_cascades[ cascadeNum ].TestBox( box ) > 0;
	}

#ifndef RED_FINAL_BUILD
	RED_INLINE Double						GetQueryTime() const			{ return m_queryTime; }
#endif

public:
	// Perform an occlusion query. Does not change any internal state, does not provide results to render element map. Just submits
	// the query and returns a Results object, from which visible objects can be requested.
	// If doOcclusion is false, only a simple frustum test will be used.
	ExtraOcclusionQueryResults* PerformExtraOcclusionQuery( CRenderSceneEx* scene, CRenderFrame* frame, Bool doOcclusion = true ) const;

	void PerformOcclusionQuery( CRenderSceneEx* scene, CRenderFrame* frame );
	void SetGateState( TObjectIdType objectId, Bool opened );
	Bool PerformLineQuery( const Vector& start, const Vector& end );
	void PerformLineQueries( const Vector& end, CharacterOcclusionInfo* infos, Int32 numberOfEntries ) const;

private:
	Umbra::QueryExt*					m_query;
	Umbra::TomeCollection*				m_tomeCollection;
	Uint32								m_maxObjectsCount;
	UmbraOcclusionBuffer*				m_occlusionBuffer;
	Umbra::Visibility*					m_visibility;

	TVisibleChunksIndices				m_visibleObjectsIndices;
	Float								m_queryThreshold;
	Float								m_dataDistanceThresholdSquared;

	Bool								m_lastQuerySucceeded;
	Bool								m_isCutscene;

	// hold two vectors of gates, one is used currently, we perform state changes on the other one
	Umbra::GateStateVector*				m_gates;
	void*								m_gatesStorage;
	Umbra::GateStateVector*				m_cutsceneGates;
	void*								m_cutsceneGatesStorage;
	CFrustum							m_cameraFrustum;
	CFrustum							m_cascades[ MAX_CASCADES ];
	Uint32								m_numberOfCascades;
	
#ifndef RED_FINAL_BUILD
	UmbraRenderer*						m_debugRenderer;
	Double								m_queryTime;
#endif
};

RED_INLINE Bool CRenderOcclusionData::IsDynamicObjectVisible( const Box& bBox ) const
{
	if ( !m_occlusionBuffer || !m_lastQuerySucceeded )
	{
		// it's better to show everything than nothing in case of error
		return true;
	}
	Umbra::OcclusionBuffer::VisibilityTestResult result = 
		m_occlusionBuffer->testAABBVisibility(  (Umbra::Vector3&)bBox.Min.A, 
												(Umbra::Vector3&)bBox.Max.A,
												0 );
	return result != Umbra::OcclusionBuffer::OCCLUDED;
}

RED_INLINE Bool CRenderOcclusionData::IsDynamicObjectVisible( const Vector3& bbMin, const Vector3& bbMax ) const
{
	if ( !m_occlusionBuffer || !m_lastQuerySucceeded )
	{
		// it's better to show everything than nothing in case of error
		return true;
	}
	auto result = m_occlusionBuffer->testAABBVisibility( (Umbra::Vector3&)bbMin.A, (Umbra::Vector3&)bbMax.A, 0 );
	return result != Umbra::OcclusionBuffer::OCCLUDED;
}

RED_INLINE Uint32 CRenderOcclusionData::IsDynamicObjectVisibleFullTest( const Box& bBox ) const
{
	if ( !m_occlusionBuffer || !m_lastQuerySucceeded )
	{
		// it's better to show everything than nothing in case of error
		return Umbra::OcclusionBuffer::FULLY_VISIBLE;
	}

	return m_occlusionBuffer->testAABBVisibility(	(Umbra::Vector3&)bBox.Min.A, 
													(Umbra::Vector3&)bBox.Max.A,
													Umbra::OcclusionBuffer::TEST_FULL_VISIBILITY );
}

RED_INLINE Bool CRenderOcclusionData::IsDynamicSphereVisible( const Vector& center, const Float radius ) const
{
	RED_FATAL_ASSERT( m_query != nullptr, "" );
	Int32 outputIndex;
	Umbra::IndexList outVisibleIndices( &outputIndex, 1 );
	Umbra::SphereLight sphereLight;
	sphereLight.center = (Umbra::Vector3&)center.A;
	sphereLight.radius = radius;
	Umbra::Query::ErrorCode code = m_query->queryLocalLights( outVisibleIndices, 0, &sphereLight, 1, *m_visibility->getOutputClusters() );
	if ( code != Umbra::Query::ERROR_OK )
	{
		return true;
	}
	return outVisibleIndices.getSize() > 0;
}

RED_INLINE Bool ExtraOcclusionQueryResults::IsDynamicObjectVisible( const Box& bBox ) const
{
	if ( !m_occlusionBuffer )
	{
		// it's better to show everything than nothing in case of error
		return true;
	}
	auto result = m_occlusionBuffer->testAABBVisibility( (Umbra::Vector3&)bBox.Min.A, (Umbra::Vector3&)bBox.Max.A, 0 );
	return result != Umbra::OcclusionBuffer::OCCLUDED;
}

RED_INLINE Bool ExtraOcclusionQueryResults::IsDynamicObjectVisible( const Vector3& bbMin, const Vector3& bbMax ) const
{
	if ( !m_occlusionBuffer )
	{
		// it's better to show everything than nothing in case of error
		return true;
	}
	auto result = m_occlusionBuffer->testAABBVisibility( (Umbra::Vector3&)bbMin.A, (Umbra::Vector3&)bbMax.A, 0 );
	return result != Umbra::OcclusionBuffer::OCCLUDED;
}

RED_INLINE Uint32 ExtraOcclusionQueryResults::IsDynamicObjectVisibleFullTest( const Box& bBox ) const
{
	if ( !m_occlusionBuffer )
	{
		// it's better to show everything than nothing in case of error
		return Umbra::OcclusionBuffer::FULLY_VISIBLE;
	}

	return m_occlusionBuffer->testAABBVisibility( (Umbra::Vector3&)bBox.Min.A, (Umbra::Vector3&)bBox.Max.A, Umbra::OcclusionBuffer::TEST_FULL_VISIBILITY );
}

#endif // USE_UMBRA
