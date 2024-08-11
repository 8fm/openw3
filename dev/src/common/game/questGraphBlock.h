/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once


class CQuestGraphBlock;
class CQuestThread;

struct SBlockDesc
{
	DECLARE_RTTI_STRUCT( SBlockDesc )

	CQuestGraphBlock*							block;
	CName										inputName;
	SBlockDesc( CQuestGraphBlock* _block = NULL, const CName& _inputName = CName::NONE )
		: block( _block )
		, inputName( _inputName )
	{}
};
BEGIN_CLASS_RTTI( SBlockDesc );
	PROPERTY( block );
	PROPERTY( inputName );
END_CLASS_RTTI();

struct SCachedConnections
{
	DECLARE_RTTI_STRUCT( SCachedConnections )

	CName								m_socketId;
	TDynArray< SBlockDesc >				m_blocks;

	SCachedConnections( const CName& sockedId = CName::NONE )
		: m_socketId( sockedId )
	{}
};
BEGIN_CLASS_RTTI( SCachedConnections );
	PROPERTY( m_socketId );
	PROPERTY( m_blocks );
END_CLASS_RTTI();

class IQuestContentCollector
{
public:
	virtual ~IQuestContentCollector() {};
	virtual void CollectResource( const String& depotPath, const CName contentChunkOverride = CName::NONE ) = 0;
};

class CQuestGraphBlock : public CGraphBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CQuestGraphBlock, CGraphBlock );

	friend class CUndoQuestGraphBlockIO;

#ifndef NO_DEBUG_SERVER
	friend class CQuestDebuggerPlugin;
#endif

public:
	enum EState
	{
		ST_ACTIVATING,
		ST_ACTIVE,
		ST_INACTIVE
	};

protected:
	String												m_name;					//!< Block name
	String												m_comment;				//!< Block comment
	CGUID												m_guid;					//!< Internal block GUID						
	TDynArray< SCachedConnections >						m_cachedConnections;	//! A list of connections to other blocks
	Bool												m_hasPatchOutput;		//! Does the block have a patch output defined?
	Bool												m_hasTerminationInput;	//! Does the block have a termination input defined?
	Bool												m_forceKeepLoadingScreen; //! HACK: loading screen can't drop while this block is active

	TInstanceVar< CName >								i_activeOutput;
	TInstanceVar< TDynArray< CName > >					i_activeInputs;
	TInstanceVar< Bool >								i_wasOutputActivated;
	TInstanceVar< Bool >								i_canDeactivate;
	TInstanceVar< Bool >								i_canActivate;
	TInstanceVar< Bool >								i_isEnabled;
	TInstanceVar< Int32 >								i_state;
	TInstanceVar< String >								i_errorMsg;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	TInstanceVar< Bool >								i_isVisited;
#endif //NO_EDITOR_GRAPH_SUPPORT

public:
	virtual ~CQuestGraphBlock() {}

	//! CGraphBlock interface
	virtual String GetBlockName() const { return m_name; }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return false; }

	// returns the block's comment
	RED_INLINE String GetComment() const { return m_comment; }

	// The method checks if the block should be updated by the user
	virtual Bool NeedsUpdate() const { return false; }

	// Should we save thread owned by this block to the save game
	virtual Bool ShouldSaveThread() const { return true; }

	// Tells if the thread can activate block's inputs when the game state is loaded from a save game
	virtual Bool CanActivateInputsOnLoad( CQuestGraphBlock::EState activationState ) const { return true; }

	// Returns the unique id assigned to the block instance
	RED_INLINE const CGUID& GetGUID() const { return m_guid; }

	//! Returns all GUIDs
	virtual void GetGUIDs( TDynArray< CGUID >& outGUIDs ) const;

	//! Checks if the block can be identified with the specified GUID
	virtual Bool MatchesGUID( const CGUID& guid ) const { return m_guid == guid; }

	//! Updates the block's GUID
	void UpdateGUID();

	//! Returns the names of all outputs on the block
	void GetAllOutputNames( TDynArray< CName >& outNames ) const;

	//! Get debug name of the block (resource path::root block::block caption)
	virtual String GetDebugName() const;

#ifndef NO_EDITOR
	virtual String GetSearchCaption() const { return GetCaption(); };
#endif

	//! Collect content referenced in block (very manual)
	virtual void CollectContent( IQuestContentCollector& collector ) const;

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------

	// Called when a data layout for the graph the block is a part of is being built.
	// A block should register all its instance variables with the specified compiler.
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	// Called when a graph instance is initialized. A block should 
	// initialize its instance data here
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;

	// Get all active inputs of the block
	const TDynArray< CName >&  GetActiveInputs( InstanceBuffer& instanceData ) const;

	// Called in order to activate the block. 
	Bool Activate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

	// Called in order to deactivate the bock instance.
	void Deactivate( InstanceBuffer& data ) const;

	// Called by the instanced execution context to execute the block-related
	// action
	Bool Execute( InstanceBuffer& data, TDynArray< SBlockDesc >& outBlocks ) const;

	// Activates one of the block's outputs, indicating to the execution context
	// that the block related action has been finished.
	Bool ActivateOutput( InstanceBuffer& data, const CName& outSocket, Bool onlyIfOutputExists = false ) const;

	// Activates one of the block's outputs, but don't deactivate the block.
	Bool ActivateOutputWithoutExiting( InstanceBuffer& data, const CName& outSocket, Bool onlyIfOutputExists = false ) const;
	
	// Deactivates the block, sending an error message to the debugger
	void ThrowError( InstanceBuffer& data, const Char* errorMsg, ... ) const;

	// Deactivates the block, sending an error message to the debugger, and activating the specified output
	void ThrowErrorNonBlocking( InstanceBuffer& data, const CName& outputName, const Char* errorMsg, ... ) const;

	// Disables the block in a particular instance context. If an active block is
	// being disabled, it immediately gets deactivated. If canActivate flag is set
	// to false, whenever the control flow reaches it, it won't activate the block. 
	void Disable( InstanceBuffer& data, Bool canActivate = false ) const;

	// If canActivate flag is set to false block won't be activated when the control flow reaches it.
	void SetCanActivate( InstanceBuffer& data, Bool canActivate ) const;

	// ------------------------------------------------------------------------
	// Block status accessors
	// ------------------------------------------------------------------------
	// Returns the block's activation status
	EState GetActivationState( InstanceBuffer& data ) const;

	// Tells if the block can be activated
	Bool IsBlockEnabled( InstanceBuffer& data ) const;

	// Tells whether a block had any output activated
	Bool WasOutputActivated( InstanceBuffer& data ) const;

	// Returns the name of the activated output
	CName GetActivatedOutputName( InstanceBuffer& data ) const;

	// Returns an error message
	const String& GetErrorMsg( InstanceBuffer& data ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	// Tells if block was ever visited
	Bool IsBlockVisited( InstanceBuffer& data ) const;
#endif //NO_EDITOR_GRAPH_SUPPORT

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	//! Sorts alphabetically output blocks
	virtual void SortOutputBlocks() {}

	//! Caches the block connections, removing the actual connections and memorizing only the blocks they are connected to
	virtual void CacheConnections();

	//! Removes all block connections
	virtual void CleanupConnections();

	// Called when the block's connections get rebuilt
	virtual void OnRebuildSockets();

	//! Define a patch output
	void AddPatchOutput();

	//! Removes a patch output
	void RemovePatchOutput();

	//! Define a termination input
	void AddTerminationInput();

	//! Removes a termination input
	void RemoveTerminationInput();

	//! An optional secondary string to display next to a quest block (for clarity)
	virtual String GetBlockAltName() const { return String::EMPTY; }

	//! Collect special right click menu options supported by this node
	typedef TDynArray< String > SpecialOptionsList;
	virtual void GetContextMenuSpecialOptions( SpecialOptionsList& outOptions );

	//! Collect special right click menu options supported by this node
	virtual void RunSpecialOption( Int32 option );

	//! Removes unnecessary object from the resource for cooking purposes
	virtual void CleanupSourceData();
#endif

	//! Does the block have a patch output defined
	RED_INLINE Bool HasPatchOutput() const { return m_hasPatchOutput; }

	//! Does the block have a termination input defined
	RED_INLINE Bool HasTerminationInput() const { return m_hasTerminationInput; }

	//!< Prevent the loading scren from dropping while the block is active
	Bool ForceKeepLoadingScreen() const { return m_forceKeepLoadingScreen; }

	// Called before the block the block functionality activation
	// in order to give it some time to initialize itself
	virtual Bool OnProcessActivation( InstanceBuffer& data ) const { return true; }

protected:
	CQuestGraphBlock();

	// Called when the block is activated from an instanced context.
	// Override to do some implementation specific execution stuff.
	virtual void OnActivate( InstanceBuffer& data, const CName& inputName, CQuestThread* parentThread ) const;

	// Called when the block action is executed. Override to do some implementation
	// specific execution stuff.
	virtual void OnExecute( InstanceBuffer& data ) const {}

	// Called when the block is deactivated. Override to do some implementation
	// specific execution stuff.
	virtual void OnDeactivate( InstanceBuffer& data ) const {}

public:
	// Checks if journal paths are null or invalid
	virtual Bool IsValid() const { return true; }

protected:
#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! Block was spawned in editor
	virtual void OnSpawned( const GraphBlockSpawnInfo& info );

	//! Block was pasted
	virtual void OnPasted( Bool wasCopied );

#endif

	//! Returns a list of connected blocks
	void GetConnectedBlocks( const CName& socketId, TDynArray< SBlockDesc >& outBlocks ) const;

	//! Checks if an output socket with the specified id exist
	Bool DoesSocketExist( const CName& socketId ) const;
};

BEGIN_CLASS_RTTI( CQuestGraphBlock );
	PARENT_CLASS( CGraphBlock );
	PROPERTY_EDIT( m_name, TXT( "Block name" ) );
	PROPERTY_EDIT( m_comment, TXT( "Block comment" ) );
	PROPERTY_EDIT( m_forceKeepLoadingScreen, TXT("Keep the loading screen on while this block is active") );
	PROPERTY_RO( m_guid, TXT( "Block GUID" ) );
	PROPERTY( m_cachedConnections );
	PROPERTY( m_hasPatchOutput );
	PROPERTY( m_hasTerminationInput );

END_CLASS_RTTI();

