/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibAreaDescription.h"
#include "pathlibStreamingItem.h"

#ifndef NO_EDITOR_PATHLIB_SUPPORT

namespace PathLib
{

class CSpecialZonesMap;

class CAreaProcessingJob : public IGenerationManagerBase::CAsyncTask
{
protected:
	CAreaDescription::CStreamingRequest			m_loadRequest;
	CAreaDescription*							m_area;
public:
	CAreaProcessingJob( CAreaDescription* area );
	~CAreaProcessingJob();

	CAreaDescription*							GetTargetArea() const override;
	IGenerationManagerBase::CAsyncTask*			PostProcessingSync() override;
};


class CAreaLoadPreSynchronization : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
public:
	CAreaLoadPreSynchronization( CAreaDescription* area );

	Bool										PreProcessingSync() override;
	Bool										ProcessPathLibTask() override;
	IGenerationManagerBase::CAsyncTask*			PostProcessingSync() override;
	void										DescribeTask( String& task ) override;
};

class CAreaGenerationJob : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
protected:
	Uint16										m_flags;
	Bool										m_runPreProcessingSync;
	Bool										m_runSyncProcessingAfter;
		
public:
	CAreaGenerationJob( CAreaDescription* area, Uint16 taskFlags, Bool runPrePreocessingSync = true, Bool runSyncProcessingAfter = true );

	Bool										ShouldTerminate() override;
	Bool										ProcessPathLibTask() override;
	Bool										PreProcessingSync() override;
	IGenerationManagerBase::CAsyncTask*			PostProcessingSync() override;
	void										DescribeTask( String& task ) override;

	Uint16										GetGenerationFlags() const						{ return m_flags; }
};

class CSpecialZonesProcessing : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
protected:
	CGlobalWater*								m_water;
	CSpecialZonesMap*							m_specialZones;

public:
	CSpecialZonesProcessing( CAreaDescription* area, CSpecialZonesMap* specialZones );

	Bool										PreProcessingSync() override;
	Bool										ProcessPathLibTask() override;

	void										DescribeTask( String& task ) override;
};

//class CAreaCoherentRegionsComputationJob : public CAreaProcessingJob
//{
//	typedef CAreaProcessingJob Super;
//public:
//	CAreaCoherentRegionsComputationJob( CAreaDescription* area )
//		: Super( area )																		{}
//
//	Bool ProcessPathLibTask() override;
//	Bool PreProcessingSync() override;
//	IGenerationManagerBase::CAsyncTask* PostProcessingSync() override;
//	void DescribeTask( String& task ) override;
//};

};			// namespace PathLib

#endif
