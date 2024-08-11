/**
 * Copyright © 2010-2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CSpawnTreeInstance;
class ICreatureDefinitionContainer;


enum ESpawnTreeType
{
	STT_default,
	STT_gameplay,
};

BEGIN_ENUM_RTTI( ESpawnTreeType )
	ENUM_OPTION( STT_default );
	ENUM_OPTION( STT_gameplay );
END_ENUM_RTTI()

struct SBudgetingStats
{
	struct TemplateData
	{
		THandle< CEntityTemplate >					m_template;
		TDynArray< const CEntityAppearance* >		m_usedApperances;
		Bool										m_isUsingLimitedApperances;
		Uint32										m_maxSpawnLimit;
	};

	TDynArray< TemplateData >					m_usedTemplates;
};

class IEdSpawnTreeNode
{
protected:
	static const Uint8 HIDEN_NOT_INITIALIZED = 0xff;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	Int32	m_graphPosX;
	Int32	m_graphPosY;
	String	m_comment;
	Uint8	m_hidden;
#endif
	Uint64	m_id;
	
	Bool RemoveNullChildren();

public:
	enum EDebugState
	{
		EDEBUG_ACTIVE,
		EDEBUG_DEACTIVATED,
		EDEBUG_NOT_RELEVANT,
		EDEBUG_INVALID
	};
	typedef TDynArray< String > SpecialOptionsList;
	
	IEdSpawnTreeNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Get graph position X
	Int32 GetGraphPosX() const { return m_graphPosX; }

	//! Get graph position Y
	Int32 GetGraphPosY() const { return m_graphPosY; }

	//! Set graph position
	void SetGraphPosition( Int32 x, Int32 y );

	//! Get comment
	const String& GetComment() const { return m_comment; }

	//! Check if the branch is hidden
	Bool IsHidden() const { return m_hidden != false; }

	//! Set branch hidden state
	void SetHidden( Bool state ) { m_hidden = state; }

	//! Set all kids visible/hidden
	void SetHiddenRec( Bool state );

	//! Show/hides according to default IsHiddenByDefault
	Bool SetDefaultVisibilityRec( Bool effectOnlyUnitialized = true );

#endif
	//! Gets the possible children classes to be spawned under the node (with friendly names)
	void GetPossibleChildClasses( TArrayMap< String, CClass * >& result, ESpawnTreeType spawnTreeType, Bool sort = false ) const;

	//! Checks if the class is a possible children for the node
	Bool IsPossibleChildClass( CClass* nodeClass, ESpawnTreeType spawnTreeType );

	//! Get as CObject
	virtual CObject* AsCObject() = 0;

	//! Get the CObject to be edited in the properties editor
	virtual IScriptable* GetObjectForPropertiesEdition();

	//! Get parent node
	virtual IEdSpawnTreeNode* GetParentNode() const;

	//! Can add child node
	virtual Bool CanAddChild() const;

	//! Add child node
	virtual void AddChild( IEdSpawnTreeNode* node );

	//! Remove given child node
	virtual void RemoveChild( IEdSpawnTreeNode* node );

	//! Get number of children
	virtual Int32 GetNumChildren() const;

	//! Get child node
	virtual IEdSpawnTreeNode* GetChild( Int32 index ) const;

	//! Updates children order, may also affect the positions, returns true if modification occurs
	virtual Bool UpdateChildrenOrder();

	//! Should be hidden by default?
	virtual Bool IsHiddenByDefault() const;

	//! Can the branch be hidden?
	virtual Bool CanBeHidden() const;

	//! Can the node be copied to the clipboard?
	virtual Bool CanBeCopied() const;

	//! Is it locked for editing?
	virtual Bool IsLocked() const;

	//! Get block color
	virtual Color GetBlockColor() const;

	//! Get block caption
	virtual String GetBlockCaption() const;

	//! Get friendly class name
	virtual String GetEditorFriendlyName() const;

	//! Get the resource name of the bitmap drawn next to the block
	virtual String GetBitmapName() const;

	//! Called when structure has been modified
	virtual void PreStructureModification();

	//! Debugger support - debug state
	virtual EDebugState GetDebugState( const CSpawnTreeInstance* instanceBuffer ) const;

	virtual String GetBlockDebugCaption( const CSpawnTreeInstance& instanceBuffer ) const;

	//! Debugger support - returns true for nodes that should hold instance buffer
	virtual Bool HoldsInstanceBuffer() const;

	//! Debugger support - returns internal instance buffer that applies to node's children
	virtual CSpawnTreeInstance* GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer = NULL );
	
	//! Debugger support - returns internal instance buffer that applies to node's children - helper const version
	const CSpawnTreeInstance* GetInstanceBuffer( const CSpawnTreeInstance* parentBuffer = NULL ) const 
		{ return const_cast< IEdSpawnTreeNode* >( this )->GetInstanceBuffer( parentBuffer ); } ;

	//! Collect special right click menu options supported by this node
	virtual void GetContextMenuSpecialOptions( SpecialOptionsList& outOptions );

	//! Collect special right click menu options supported by this node
	virtual void RunSpecialOption( Int32 option );

	//! Collect special right click menu options supported by this node
	virtual void GetContextMenuDebugOptions( CSpawnTreeInstance& instanceBuffer, TDynArray< String >& outOptions );

	//! Collect special right click menu options supported by this node
	virtual void RunDebugOption( CSpawnTreeInstance& instanceBuffer, Int32 option );

	//! Do some automated stuff when editor is opened
	virtual void OnEditorOpened();

	virtual ICreatureDefinitionContainer* AsCreatureDefinitionContainer();

	//! Returns the root class for children
	virtual void GetRootClassForChildren( TDynArray< CClass* >& rootClasses, ESpawnTreeType spawnTreeType ) const;

	//! Gathering the stats for the appearances
	virtual void GatherBudgetingStats( ICreatureDefinitionContainer* container, SBudgetingStats& stats );

	//! Generate a random uint64 id for this node
	void GenerateId();

	//! Regenerate random uint64 id for this node and all her children
	virtual Bool GenerateIdsRecursively();

	//! Xoring m_id and parent hash and passing further to hash all branches
	virtual void GenerateHashRecursively( Uint64 parentHash, CSpawnTreeInstance* parentBuffer = nullptr );
	
	//! Get randomly generated node id 
	Uint64 GetId() const { return m_id; }

protected:
	//! Checks if the object of particular class can be added as a child
	virtual Bool CanSpawnChildClass( const CClass* classId, ESpawnTreeType spawnTreeType ) const { return true; }
};