
#pragma once

#include "undoManager.h"
#include "treeEditor.h"

class CEdTreeEditor;

class CUndoTreeBlockMove : public IUndoStep
{
	CUndoTreeBlockMove() {}
    DECLARE_ENGINE_CLASS( CUndoTreeBlockMove, IUndoStep, 0 );

public:
	static void PrepareStep( CEdUndoManager& undoManager, CEdTreeEditor* editor, IScriptable* owner, const wxPoint& offset, Bool alternate );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoTreeBlockMove( CEdUndoManager& undoManager, CEdTreeEditor* editor );

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;
	void DoStep();

	CEdTreeEditor* m_editor;

	struct Info
	{
		Info() {}
		Info( const wxPoint& offset, Bool alternate )
			: m_offset( offset ), m_alternate( alternate )
			{}
		wxPoint m_offset; 
		Bool	m_alternate;
	};

	THashMap< IScriptable* /*owner*/, Info > m_infos;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoTreeBlockMove, IUndoStep );

//-------------------

class CUndoTreeBlockExistance : public IUndoStep
{
    DECLARE_ENGINE_ABSTRACT_CLASS( CUndoTreeBlockExistance, IUndoStep );

protected:
	CUndoTreeBlockExistance() {}
	CUndoTreeBlockExistance( CEdUndoManager& undoManager );

	void DoPrepareCreationStep( CObject* object );

	void DoPrepareDeletionStep( CObject* object );

	void SetNameOverride( const String& name );

	virtual void SetupCustomRelationship( CObject* object, Bool created ) =0;

	virtual String GetFriendlyBlockName( CObject* object ) =0;

protected:

    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

private:
	struct Info
	{
		Info( CObject* object, CObject* parent, Bool created )
			: m_object( object )
			, m_parent( parent )
			, m_created( created )
			{ }

		CObject*           m_object;
		THandle< CObject > m_parent;
		Bool	           m_created;
	};

	String				m_nameOverride;
	TDynArray< Info >	m_infos;

	void DoBlock( Info& info, Bool backInTime );
	void DoStep( Bool backInTime );

	virtual void OnSerialize( IFile& file ) override;

};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( CUndoTreeBlockExistance, IUndoStep );

//-------------------

class CUndoTreeBlockDecorate : public IUndoStep
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CUndoTreeBlockDecorate, IUndoStep );

protected:
	CUndoTreeBlockDecorate() {}
	CUndoTreeBlockDecorate( CEdUndoManager& undoManager, CObject* object, CObject* decorated );

	virtual void SetupCustomRelationship( CObject* object, Bool created ) =0;

protected:

	virtual void DoUndo() override;
	virtual void DoRedo() override;
	virtual String GetName() override;

private:
	CObject*           m_object;
	THandle< CObject > m_parent;
	THandle< CObject > m_decorated;
	Bool	           m_created;

	virtual void OnSerialize( IFile& file ) override;
};

DEFINE_SIMPLE_ABSTRACT_RTTI_CLASS( CUndoTreeBlockDecorate, IUndoStep );
