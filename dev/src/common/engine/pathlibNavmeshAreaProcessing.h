/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibAreaProcessingJob.h"
#include "pathlibNavmeshArea.h"

namespace PathLib
{

class CNavmeshProcessingJob : public CAreaProcessingJob
{
	typedef CAreaProcessingJob Super;
protected:
	CNavmeshAreaDescription*					GetArea()										{ return static_cast< CNavmeshAreaDescription* >( m_area ); }
public:
	CNavmeshProcessingJob( CNavmeshAreaDescription* naviArea )
		: Super( naviArea )																		{}
};


class CNavmeshDetermineNavmeshNeighbours : public CNavmeshProcessingJob
{
	typedef CNavmeshProcessingJob Super;
public:
	CNavmeshDetermineNavmeshNeighbours( CNavmeshAreaDescription* naviArea )
		: Super( naviArea )																		{}

	Bool										ProcessPathLibTask() override;

	void										DescribeTask( String& task ) override;
};

};			// namespace PathLib