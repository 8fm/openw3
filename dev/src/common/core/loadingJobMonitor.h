/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

/// Debug monitor for the job system
class ILoadingJobMonitor
{
public:
	virtual ~ILoadingJobMonitor() {};

	// Report job states
	virtual void OnJobIssued( class ILoadJob* job ) = 0;
	virtual void OnJobStarted( class ILoadJob* job ) = 0;
	virtual void OnJobFinished( class ILoadJob* job, const EJobResult result ) = 0;
	virtual void OnJobCanceled( class ILoadJob* job ) = 0;
};