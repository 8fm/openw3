///////////////////////////////////////////////////////////////////////  
//  MyStatsReporter.h
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

#pragma once
#include "RenderInterface/ForestRI.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	typedef CBasicFixedString<4096> CReportString;

	///////////////////////////////////////////////////////////////////////  
	//  Class CMyStatsReporter

	class CMyStatsReporter
	{
	public:
        					CMyStatsReporter( );
	virtual 				~CMyStatsReporter( );

			st_bool			Update(st_float32 fCurrentTime, st_float32 fReportInterval = 1.5f); // returns true if a report should be made; interval is in seconds
			const char*		Report(const CRenderStats& cStats);
			const char*		LastReport(void);

	private:
			st_int32		m_nUpdatesSinceLastReport;
			st_int32		m_nNumReports;
			st_float32		m_fLastReportTime;
			st_float32		m_fReportInterval;
			CReportString	m_strReportText;
	};

} // end namespace SpeedTree

