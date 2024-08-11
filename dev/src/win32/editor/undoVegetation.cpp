/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */

#include "build.h"
#include "undoVegetation.h"
#include "../../common/engine/foliageEditionController.h"

IMPLEMENT_ENGINE_CLASS( CUndoVegetationExistance )

CUndoVegetationExistance::CUndoVegetationExistance()
{
}

CUndoVegetationExistance::CUndoVegetationExistance( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene, const String& stepName )
	: IUndoStep( undoManager )
	, m_foliageScene( foliageScene )
	, m_stepName( stepName )
	, m_area( Box::EMPTY )
{
}

/*static*/ 
void CUndoVegetationExistance::CreateStep( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene, const String& stepName )
{
	foliageScene->FlushTransaction();
	foliageScene->PushTransaction();

	CUndoVegetationExistance* step = new CUndoVegetationExistance( undoManager, foliageScene, stepName );
	step->PushStep();
}

/*virtual*/ 
void CUndoVegetationExistance::DoUndo() /*override*/
{
	m_foliageScene->UndoTransaction();
}

/*virtual*/ 
void CUndoVegetationExistance::DoRedo() /*override*/
{
	m_foliageScene->RedoTransaction();
}

/*virtual*/ 
String CUndoVegetationExistance::GetName() /*override*/
{
	return m_stepName;
}

// ------------------------------------------------------------------------------

IMPLEMENT_ENGINE_CLASS( CUndoVegetationSize )

CUndoVegetationSize::CUndoVegetationSize( CEdUndoManager& undoManager, CFoliageEditionController* foliageScene )
	: IUndoStep( undoManager )
	, m_foliageScene( foliageScene )
{
}

/*static*/ 
void CUndoVegetationSize::PrepareStep( CEdUndoManager& undoManager, CFoliageEditionController* foliageScene )
{
	CUndoVegetationSize* step = undoManager.SafeGetStepToAdd< CUndoVegetationSize >();
	if ( !step )
	{
		step = new CUndoVegetationSize( undoManager, foliageScene );
		undoManager.SetStepToAdd( step );
	}
}

/*static*/ 
void CUndoVegetationSize::AddStroke( CEdUndoManager& undoManager, const CSRTBaseTree* tree, const Vector& center, Float radius, Float value, Bool shrink )
{
	CUndoVegetationSize* step = undoManager.SafeGetStepToAdd< CUndoVegetationSize >();
	ASSERT ( step != NULL );
	if( step != nullptr )
	{
		Info info = { tree, center, radius, value, shrink };
		step->m_infos.PushBack( info );
	}
}

/*static*/ 
void CUndoVegetationSize::FinalizeStep( CEdUndoManager& undoManager )
{
	if ( CUndoVegetationSize* step = undoManager.SafeGetStepToAdd< CUndoVegetationSize >() )
	{
		step->PushStep();
	}
}

void CUndoVegetationSize::DoStep()
{
	for ( auto infoIt = m_infos.Begin(); infoIt != m_infos.End(); ++infoIt )
	{
		infoIt->m_shrink = !infoIt->m_shrink;
		m_foliageScene->ResizeInstances( infoIt->m_tree, infoIt->m_center, infoIt->m_radius, infoIt->m_value, infoIt->m_shrink );
	}
}

/*virtual*/ 
void CUndoVegetationSize::DoUndo() /*override*/
{
	DoStep();
}

/*virtual*/ 
void CUndoVegetationSize::DoRedo() /*override*/
{
	DoStep();
}

/*virtual*/ 
String CUndoVegetationSize::GetName() /*override*/
{
	return TXT("paint size");
}
