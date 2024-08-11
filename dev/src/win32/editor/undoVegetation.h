/**
  * Copyright © 2013 CD Projekt Red. All Rights Reserved.
  */

#pragma once

#include "undoManager.h"

class CFoliageManager;
class CSRTBaseTree;
class CFoliageEditionController;

class CUndoVegetationExistance : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoVegetationExistance, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene, const String& stepName = TXT("paint vegetation") );

private:
	CUndoVegetationExistance();
	CUndoVegetationExistance( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene, const String& stepName );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CFoliageEditionController * m_foliageScene;
	
	TDynArray< const CSRTBaseTree* > m_trees;
	Box m_area;
	String m_stepName;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoVegetationExistance, IUndoStep );

// -----------------------------------------------------------------------------------

class CUndoVegetationSize : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoVegetationSize, IUndoStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene );

	static void AddStroke( CEdUndoManager& undoManager, const CSRTBaseTree* tree, const Vector& center, Float radius, Float value, Bool shrink );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoVegetationSize() {}
	CUndoVegetationSize( CEdUndoManager& undoManager, CFoliageEditionController * foliageScene );

	void DoStep();

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	struct Info
	{
		const CSRTBaseTree* m_tree;
		Vector m_center;
		Float  m_radius;
		Float  m_value;
		Bool   m_shrink;
	};

	CFoliageEditionController * m_foliageScene;
	TDynArray< Info > m_infos;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoVegetationSize, IUndoStep );
