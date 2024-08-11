/**
* Copyright © 2008 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Value evaluator
class IEvaluator : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IEvaluator, CObject );

public:
	//! Get curve group related to this evaluator, NULL if evaluator does not use curves
	virtual class CurveParameter* GetCurves();

	//! Returns true if evaluator is a function of input variable
	virtual Bool IsFunction() const;
};

BEGIN_CLASS_RTTI( IEvaluator );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();