
#pragma once

#include "undoStep.h"
#include "../../common/engine/meshChunk.h"
#include "../../common/engine/mesh.h"


class CEdMaterialListManager;

class CUndoMeshMaterialInstanceExistance : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoMeshMaterialInstanceExistance, IUndoStep, 0 );

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index );

private:
	CUndoMeshMaterialInstanceExistance() {}
	CUndoMeshMaterialInstanceExistance( CEdUndoManager& undoManager, CEdMaterialListManager* editor, Int32 index, Bool creating );

	void DoStep( Bool create );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	virtual void OnSerialize( class IFile& file ) override;

	CEdMaterialListManager* m_editor;
	CMaterialInstance* m_instance;
	Int32    m_index;
	CObject* m_originalParent;
	Bool     m_creating;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoMeshMaterialInstanceExistance, IUndoStep );

// NOTE: the way the steps track changes is quite brute-force (storing the whole chunks). If it becomes a problem,
// flushing out to disk may be added (by implementing serialization)
class CUndoMeshChunksChanged : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoMeshChunksChanged, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CMesh* mesh, const String& stepName );

private:
	CUndoMeshChunksChanged() {}
	CUndoMeshChunksChanged( CEdUndoManager& undoManager, CMesh* mesh, const String& stepName );

	void DoStep();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CMesh* m_mesh;
	class CMeshData* m_data;

	String m_stepName;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoMeshChunksChanged, IUndoStep );


class CUndoMeshMaterialsChanged : public IUndoStep
{
    DECLARE_ENGINE_CLASS( CUndoMeshMaterialsChanged, IUndoStep, 0 );

public:
	static void CreateStep( CEdUndoManager& undoManager, CMeshTypeResource* mesh, const String& stepName );

private:
	CUndoMeshMaterialsChanged() {}
	CUndoMeshMaterialsChanged( CEdUndoManager& undoManager, CMeshTypeResource* mesh, const String& stepName );

	void DoStep();
	void StoreData();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	virtual void OnSerialize( class IFile& file ) override;

	CMeshTypeResource* m_mesh;
	
	CMesh::TMaterials   m_materials;
	TDynArray< String > m_materialNames;
	TDynArray< Int32 >  m_chunkMaterialIds;

	String m_stepName;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoMeshMaterialsChanged, IUndoStep );
