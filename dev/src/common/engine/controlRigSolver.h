
#pragma once

#include "controlRigAlgorithms.h"

class TCrInstance;
class CRenderFrame;

class TCrSolver
{
public:
	void Solve( TCrInstance* cr );

	void GenerateFragments( CRenderFrame* frame ) const;

private:
	Ik2Solver::Input	m_handL_ik2Input;
	Ik2Solver::Output	m_handL_ik2Output;

	Ik2Solver::Input	m_handR_ik2Input;
	Ik2Solver::Output	m_handR_ik2Output;
};
