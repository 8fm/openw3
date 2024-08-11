/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

enum EQueryResult
{
	EQR_Success,
	EQR_Pending,
	EQR_Error
};

/// Render query ref
class CRenderQuery : public IDynamicRenderResource
{
	typedef TDynArray<GpuApi::QueryRef, MC_RenderData > TQueriesArray;

private:
	TQueriesArray					m_queries;
	Int8							m_queriesCount;
	Int8							m_lastProperQuery;
	Int8							m_nextQuery;
	Bool							m_queryIssued;

public:
	CRenderQuery( GpuApi::eQueryType queryType, Bool nonAutomaticQuery );
	virtual ~CRenderQuery();

	// Describe resource
	virtual CName GetCategory() const;

	// Calculate video memory used by resource
	virtual Uint32 GetUsedVideoMemory() const;

	// Get displayable name
	virtual String GetDisplayableName() const { return TXT("Query"); }

	// Begin query
	void BeginQuery();

	// End query
	void EndQuery();

	// Query is valid?
	Bool IsValid();

	// Get query result
	template< typename RESULT_TYPE >
	EQueryResult GetQueryResult( RESULT_TYPE& outResult, Bool forceImmediate );

	// Device lost/reset
	virtual void OnDeviceLost();
	virtual void OnDeviceReset();
};