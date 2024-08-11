/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "undoStripe.h"
#include "stripeEdit.h"

IMPLEMENT_ENGINE_CLASS( CUndoStripePoint );

CUndoStripePoint::CUndoStripePoint( CEdUndoManager& undoManager, CEdStripeEdit* editor, CStripeComponent* stripe, const String & name )
	: IUndoStep( undoManager )
	, m_editor( editor )
	, m_stripe( stripe )
	, m_name( name )
{
}

void CUndoStripePoint::CreateStep( CEdUndoManager& undoManager, CEdStripeEdit* editor, CStripeComponent* stripe, const String & name )
{
	CUndoStripePoint* step = new CUndoStripePoint( undoManager, editor, stripe, name );
	step->m_points = stripe->m_points;
	step->PushStep();
}

void CUndoStripePoint::DoStep()
{
	Swap( m_points, m_stripe->m_points );
	m_stripe->UpdateStripeProxy();
	m_editor->RebuildVertexEntities();
}

void CUndoStripePoint::DoUndo()
{
	DoStep();
}

void CUndoStripePoint::DoRedo()
{
	DoStep();
}

String CUndoStripePoint::GetName()
{
	if ( m_name.Empty() )
	{
		return TXT("stripe node added");
	}
	else
	{
		return m_name;
	}
}

void CUndoStripePoint::OnObjectRemoved( CObject *object )
{
	if ( object == m_stripe || object == m_editor )
	{
		PopStep();
		return;
	}
}
