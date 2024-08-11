///////////////////////////////////////////////////////////////////////  
//  MyStatsReporter.cpp
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      http://www.idvinc.com


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#include "MyStatsReporter.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////  
//  CMyStatsReporter::CMyStatsReporter

CMyStatsReporter::CMyStatsReporter( ) :
	m_nUpdatesSinceLastReport(0),
	m_nNumReports(0),
	m_fLastReportTime(-1.0f),
	m_fReportInterval(1.0f)
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyStatsReporter::~CMyStatsReporter

CMyStatsReporter::~CMyStatsReporter( )
{
}


///////////////////////////////////////////////////////////////////////  
//  CMyStatsReporter::Update

st_bool CMyStatsReporter::Update(st_float32 fCurrentTime, st_float32 fReportInterval)
{
	st_bool bUpdate = false;

	++m_nUpdatesSinceLastReport;

	if (m_fLastReportTime == -1.0f)
		m_fLastReportTime = fCurrentTime;
	else
	{
		st_float32 fInterval = fCurrentTime - m_fLastReportTime;
		if (fInterval >= fReportInterval)
		{
			m_fReportInterval = fInterval;

			m_fLastReportTime = fCurrentTime;
			bUpdate = true;
		}
	}

	return bUpdate;
}


///////////////////////////////////////////////////////////////////////  
//  GetTriangleReport

CReportString GetTriangleReport(st_int32 nTriangleCount)
{
	if (nTriangleCount < 1000)
		return CReportString::Format("%d tris", nTriangleCount);
	else
		return CReportString::Format("%.1fK tris", nTriangleCount / 1000.0f);
}


///////////////////////////////////////////////////////////////////////  
//  CMyStatsReporter::Report

const char* CMyStatsReporter::Report(const CRenderStats& cStats)
{
	#ifdef SPEEDTREE_RENDER_STATS
		// compute frame rate
		const st_float32 c_fFramesPerSec = m_nUpdatesSinceLastReport / m_fReportInterval;

		// gather stat totals, sorted by SpeedTree and non-SpeedTree
		st_int32 nSpeedTreeTriangles = 0, nOtherTriangles = 0;
		st_int32 nSpeedTreeDraws = 0, nOtherDraws = 0;
		st_int32 nSpeedTreeShaderBinds = 0, nOtherShaderBinds = 0;
		const CArray<CRenderStats::SObjectStats>& aObjectStats = cStats.GetObjectsArray( );
		for (st_int32 i = 0; i < st_int32(aObjectStats.size( )); ++i)
		{
			const CRenderStats::SObjectStats& sObjStats = aObjectStats[i];
			if (sObjStats.m_strName != "Terrain" && sObjStats.m_strName != "Sky")
			{
				nSpeedTreeTriangles += sObjStats.m_nNumTrianglesRendered;
				nSpeedTreeDraws += sObjStats.m_nNumDrawCalls;
				nSpeedTreeShaderBinds += sObjStats.m_nNumShaderBinds;
			}
			else
			{
				nOtherTriangles += sObjStats.m_nNumTrianglesRendered;
				nOtherDraws += sObjStats.m_nNumDrawCalls;
				nOtherShaderBinds += sObjStats.m_nNumShaderBinds;
			}
		}

		// speedtree
		st_float32 fSpeedTreeCullAndUpdateTime = cStats.m_fSpeedTreeCullAndUpdateTime;

		// other
		const st_float32 c_fOtherCullAndUpdateTime = cStats.m_fOtherCullAndUpdateTime;

		// overall
		const st_float32 c_fMsPerFrame = 1000.0f / c_fFramesPerSec;
		const st_float32 c_fMtrisPerSec = (nSpeedTreeTriangles + nOtherTriangles) * c_fFramesPerSec / 1e6f;
		const st_float32 c_fOverallCullAndUpdateTime = cStats.m_fOverallCullAndUpdateTime;
		const st_float32 c_fTotalKtris = (nSpeedTreeTriangles + nOtherTriangles) / 1000.0f;
		const st_int32 c_nOverallDrawCalls = nSpeedTreeDraws + nOtherDraws;

		m_strReportText.clear( );

		// print column headings every 4th report
		if ((m_nNumReports % 4) == 0)
			m_strReportText = "            [cull(ms)] [K tris] [drw cls] [ fps ] [ms/f] [Mtris/s]\n";

		m_strReportText += CReportString::Format("  speedtree    %4.2f    %7.2f     %3d\n",
								fSpeedTreeCullAndUpdateTime, 
								nSpeedTreeTriangles / 1000.0f, 
								nSpeedTreeDraws);
		m_strReportText += CReportString::Format("      other    %4.2f    %7.2f     %3d\n",
								c_fOtherCullAndUpdateTime, 
								nOtherTriangles / 1000.0f, 
								nOtherDraws);
		m_strReportText += CReportString::Format("    overall    %4.2f    %7.2f     %3d    %6.1f   %5.2f   %5.2f\n",
								c_fOverallCullAndUpdateTime, 
								c_fTotalKtris, 
								c_nOverallDrawCalls,
								c_fFramesPerSec,
								c_fMsPerFrame,
								c_fMtrisPerSec);

		// reset for next report
		m_nUpdatesSinceLastReport = 0;
		++m_nNumReports;

	#else
		(cStats);
	#endif

	return m_strReportText.c_str( );
}


///////////////////////////////////////////////////////////////////////  
//  CMyStatsReporter::LastReport

const char* CMyStatsReporter::LastReport(void)
{
	return m_strReportText.c_str( );
}
