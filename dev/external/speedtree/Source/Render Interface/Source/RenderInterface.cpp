///////////////////////////////////////////////////////////////////////
//  RenderInterface.cpp
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

#include "RenderInterface/ForestRI.h"
using namespace SpeedTree;


///////////////////////////////////////////////////////////////////////
//  Static variables

#ifdef SPEEDTREE_INTEL_GPA
	__itt_domain* CScopeTrace::m_pDomain = NULL;
#endif


