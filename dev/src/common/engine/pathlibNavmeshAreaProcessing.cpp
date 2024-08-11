/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibNavmeshAreaProcessing.h"


namespace PathLib
{

Bool CNavmeshDetermineNavmeshNeighbours::ProcessPathLibTask()
{
	CNavmeshAreaDescription* naviArea = GetArea();
	naviArea->DetermineNeighbourAreas();
	return true;
}

void CNavmeshDetermineNavmeshNeighbours::DescribeTask( String& task )
{
	task = String::Printf( TXT("Determine instance %04x neighbours"), GetArea()->GetId() );
}

};				// namespace PathLib

