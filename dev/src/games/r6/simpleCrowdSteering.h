/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "build.h"
#include "crowdSpaceImplementation.h"

#include "simpleSolver.h"
#include "orvoSolver.h"

typedef ORVOSolver CSolverImpl;
struct SCrowdAgent;

class CSimpleCrowdSteering
{
private:
	CSolverImpl m_solver;	

public :
	void UpdateLOD1( SCrowdAgent* input, Int16 amountOfAgents, CCrowdManager* mgr, const Float timeDelta  );

};