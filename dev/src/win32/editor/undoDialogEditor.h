
#pragma once

#include "undoManager.h"
#include "undoGraph.h"

class CEdSceneEditorDialogsetPanel;
class CEdStorySceneElementPanel;
class CEdSceneEditorScreenplayPanel;
class CEdSceneSectionPanel;
class CStorySceneDialogsetSlot;
class CEdSceneGraphEditor;
class CEdStorySceneChoicePanel;

// -------------------------------------------------

class CUndoDialogGraphBlockExistance : public CUndoGraphBlockExistance
{
    DECLARE_ENGINE_CLASS( CUndoDialogGraphBlockExistance, CUndoGraphBlockExistance, 0 );

public:
	enum DeleteStrategy
	{
		EDITOR_DELETE_BLOCK,
		GRAPH_REMOVE_BLOCK,
		DO_NOT_DELETE
	};

	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block, DeleteStrategy strategy );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	CUndoDialogGraphBlockExistance() {}
	CUndoDialogGraphBlockExistance( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor );
	static CUndoDialogGraphBlockExistance* PrepareStep( CEdUndoManager& undoManager, CEdSceneGraphEditor* graphEditor, CGraphBlock* block );
	void StoreSectionIndexOn( TDynArray< Int32 >& sections, CGraphBlock* block );

	void DoCreationOn( const TDynArray< Info >& infos, const TDynArray< Int32 >& sections );
	void DoDeletionOn( const TDynArray< Info >& infos );

    virtual void DoUndo() override;
    virtual void DoRedo() override;

	THandle< CStoryScene > m_scene;
	TDynArray< Int32 > m_createdBlocksSections;
	TDynArray< Int32 > m_deletedBlocksSections;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogGraphBlockExistance, CUndoGraphBlockExistance );

// ----------------------------------------------

class CUndoDialogSetStep : public IUndoStep
{
protected:
	CUndoDialogSetStep() {}

	CUndoDialogSetStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* editor, CStorySceneDialogsetInstance* dialogSet )
		: IUndoStep( undoManager ), m_editor( editor ), m_dialogSet( dialogSet )
		{}

	CEdSceneEditorDialogsetPanel* m_editor;
	CStorySceneDialogsetInstance* m_dialogSet;
};

// ----------------------------------------------

class CUndoDialogSetExistance : public CUndoDialogSetStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogSetExistance, CUndoDialogSetStep, 0 );

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet );

private:
	CUndoDialogSetExistance() {}
	CUndoDialogSetExistance( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, Bool creation );

	void DoStep( Bool creating );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	Bool m_creation;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogSetExistance, CUndoDialogSetStep );

// ----------------------------------------------

class CUndoDialogSlotExistance : public CUndoDialogSetStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogSlotExistance, CUndoDialogSetStep, 0 );

public:
	static void CreateCreationStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot );

	static void CreateDeletionStep( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot );

private:
	CUndoDialogSlotExistance() {}
	CUndoDialogSlotExistance( CEdUndoManager& undoManager, CEdSceneEditorDialogsetPanel* panel, CStorySceneDialogsetInstance* dialogSet, CStorySceneDialogsetSlot* slot, Bool creation );

	void DoStep( Bool creating );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CStorySceneDialogsetSlot*	  m_slot;
	Bool m_creation;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogSlotExistance, CUndoDialogSetStep );

// ----------------------------------------------

class CUndoScreenplayStep : public IUndoStep
{
protected:
	CUndoScreenplayStep() {}

	CUndoScreenplayStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor )
		: IUndoStep( undoManager ), m_editor( editor )
		{}

	CEdSceneEditorScreenplayPanel* m_editor;
};

// ----------------------------------------------

class CUndoDialogSectionMove : public CUndoScreenplayStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogSectionMove, CUndoScreenplayStep, 0 );

public:
	static void CreateUpStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CEdSceneSectionPanel* sectionPanel );

	static void CreateDownStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CEdSceneSectionPanel* sectionPanel );

private:
	CUndoDialogSectionMove() {}
	CUndoDialogSectionMove( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CEdSceneSectionPanel* sectionPanel, Bool up );

	void DoStep( Bool up );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CEdSceneSectionPanel* m_sectionPanel;
	Bool m_up;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogSectionMove, CUndoScreenplayStep );

// ----------------------------------------------

class CUndoDialogTextChange : public CUndoScreenplayStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogTextChange, CUndoScreenplayStep, 0 );

public:
	static void PrepareLineContentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode = -1 );

	static void PrepareLineCommentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode = -1 );

	static void PrepareLineVoiceTagStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CAbstractStorySceneLine* element, int keyCode = -1 );

	static void PrepareCommentStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneComment* element, int keyCode = -1 );

	static void PrepareScriptLineStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneScriptLine* element, int keyCode = -1 );

	static void PrepareChoiceLineStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneChoiceLine* element, int keyCode = -1 );

	static void PrepareNameStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CStorySceneSection* section, int keyCode = -1 );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	enum Type
	{
		LINE_CONTENT,
		LINE_COMMENT,
		LINE_VOICE_TAG,
		COMMENT,
		SCRIPT_LINE,
		CHOICE_LINE,
		NAME
	};

	CUndoDialogTextChange() {}
	CUndoDialogTextChange( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CObject* element, Type type );

	static Bool IsSeparator( int keyCode );
	static void PrepareStep( CEdUndoManager& undoManager, CEdSceneEditorScreenplayPanel* editor, CObject* element, Type type, int keyCode );
	void StoreValue();
	void RefreshPanel( CStorySceneElement* element );
	void DoStep();
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CObject* m_element;
	String   m_value;
	Type     m_type;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogTextChange, CUndoScreenplayStep );

// -------------------------------------------------

class CUndoDialogElementExistance : public CUndoScreenplayStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogElementExistance, CUndoScreenplayStep, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel, CStorySceneElement* element, Uint32 index );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel, CStorySceneElement* element, Uint32 index );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	struct Info
	{
		Info( CStorySceneElement* element, Uint32 index );

		CStorySceneElement* m_element;
		Uint32   m_index;
		CObject* m_parent;
	};

	CUndoDialogElementExistance() {}
	CUndoDialogElementExistance( CEdUndoManager& undoManager, CStorySceneSection* section, CEdSceneEditorScreenplayPanel* editor );

	void DoCreationOn( const TDynArray< Info >& infos );
	void DoDeletionOn( const TDynArray< Info >& infos );

	static CUndoDialogElementExistance* PrepareStep( CEdUndoManager& undoManager, CEdSceneSectionPanel* panel );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CStorySceneSection* m_section;
	TDynArray< Info > m_createdElements;
	TDynArray< Info > m_deletedElements;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogElementExistance, CUndoScreenplayStep );

// --------------------------------------

class CUndoDialogChoiceLineExistance : public CUndoScreenplayStep
{
    DECLARE_ENGINE_CLASS( CUndoDialogChoiceLineExistance, CUndoScreenplayStep, 0 );

public:
	static void PrepareCreationStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel, CStorySceneChoiceLine* element, Uint32 index );

	static void PrepareDeletionStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel, CStorySceneChoiceLine* element, Uint32 index );

	static void FinalizeStep( CEdUndoManager& undoManager );

private:
	struct Info
	{
		Info( CStorySceneChoiceLine* element, Uint32 index );

		CStorySceneChoiceLine* m_element;
		Uint32   m_index;
		CObject* m_parent;
	};

	CUndoDialogChoiceLineExistance() {}
	CUndoDialogChoiceLineExistance( CEdUndoManager& undoManager, CStorySceneChoice* choice, CEdSceneEditorScreenplayPanel* editor );

	CEdStorySceneChoicePanel* GetPanel() const;
	void DoCreationOn( const TDynArray< Info >& infos );
	void DoDeletionOn( const TDynArray< Info >& infos );

	static CUndoDialogChoiceLineExistance* PrepareStep( CEdUndoManager& undoManager, CEdStorySceneChoicePanel* panel );
    virtual void DoUndo() override;
    virtual void DoRedo() override;
	virtual String GetName() override;

	CStorySceneChoice*  m_choice;
	TDynArray< Info > m_createdElements;
	TDynArray< Info > m_deletedElements;
};

DEFINE_SIMPLE_RTTI_CLASS( CUndoDialogChoiceLineExistance, CUndoScreenplayStep );
