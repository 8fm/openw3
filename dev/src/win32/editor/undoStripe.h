/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

#include "../../common/engine/stripeComponent.h"
#include "undoManager.h"
#include "undoStep.h"

class CUndoStripePoint : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoStripePoint, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CEdStripeEdit* editor, CStripeComponent * stripe, const String & name = String::EMPTY );

private:
	CUndoStripePoint() {}
	CUndoStripePoint( CEdUndoManager& undoManager, CEdStripeEdit* editor, CStripeComponent * stripe, const String & name );

	void DoStep();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	virtual void OnObjectRemoved( CObject *object ) override;

	CEdStripeEdit* m_editor;
	CStripeComponent* m_stripe;
	TDynArray< SStripeControlPoint > m_points;
	String m_name;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoStripePoint, IUndoStep );
