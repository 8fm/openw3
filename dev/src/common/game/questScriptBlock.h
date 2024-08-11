/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once



struct LatentScriptData;

///////////////////////////////////////////////////////////////////////////////

// Quest scene save mode
enum EQuestScriptSaveMode
{
	QSCSM_SaveBlocker,			//!< Game will not be saved if this script is running
	QSCSM_Restart,				//!< Restart script after loading
};

BEGIN_ENUM_RTTI( EQuestScriptSaveMode );
	ENUM_OPTION( QSCSM_SaveBlocker );
	ENUM_OPTION( QSCSM_Restart );
END_ENUM_RTTI();

///////////////////////////////////////////////////////////////////////////////

struct QuestScriptParam
{
	DECLARE_RTTI_STRUCT( QuestScriptParam );

	CName		m_name;
	CVariant	m_value;
	Bool		m_softHandle;

	RED_INLINE QuestScriptParam() {};

	RED_INLINE QuestScriptParam( CName propName, CName valueTypeName )
		: m_name( propName )
		, m_value( valueTypeName, NULL )
		, m_softHandle( false )
	{};

	RED_INLINE QuestScriptParam( CName propName, const CVariant& propValue )
		: m_name( propName )
		, m_value( propValue )
		, m_softHandle( false )
	{};

};

BEGIN_CLASS_RTTI( QuestScriptParam );
	PROPERTY_RO( m_name, TXT("Property name") );
	PROPERTY_EDIT( m_value, TXT("Value") );
	PROPERTY( m_softHandle );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CQuestScriptBlock :	public CQuestGraphBlock, 
									public IDynamicPropertiesSupplier
{
	DECLARE_ENGINE_CLASS( CQuestScriptBlock, CQuestGraphBlock, 0 )


protected:
	// editable data
	CName												m_functionName;
	EQuestScriptSaveMode								m_saveMode;


	// static data
	TDynArray< QuestScriptParam >						m_parameters;
	Bool												m_choiceOutput;
	String												m_caption;

#ifndef NO_EDITOR
	String												m_searchCaption;
#endif

	// runtime data
	TInstanceVar< TGenericPtr >							i_runtimeData;
	TInstanceVar< int >									i_saveLock;
	TInstanceVar< int >									i_functionStarted;
	TInstanceVar< TDynArray<TSoftHandle<CResource>> >	i_softHandles;

public:
	CQuestScriptBlock();

#ifndef NO_EDITOR_GRAPH_SUPPORT

	//! CObject interface
	virtual void OnPropertyPostChange( IProperty* property );

	virtual void OnPostLoad();

	//! CGraphBlock interface
	virtual String GetCaption() const { return m_caption; }

#ifndef NO_EDITOR
	virtual String GetSearchCaption() const { return m_searchCaption; }
#endif

	virtual void OnRebuildSockets();
	virtual EGraphBlockShape GetBlockShape() const { return GBS_Default; }
	virtual Color GetClientColor() const { return Color( 151, 111, 255 ); }
	virtual String GetBlockCategory() const { return TXT( "Scripting" ); }
	virtual Bool CanBeAddedToGraph( const CQuestGraph* graph ) const { return true; }

	//! Special function for animation reporter
	Bool ReadDynamicPropForEditor( const CName& propName, CVariant& propValue ) const;

#endif

	//! Returns the name of a script function the block executes
	RED_INLINE const CName& GetFunctionName() const { return m_functionName; }

	// ------------------------------------------------------------------------
	// Block related action execution methods
	// ------------------------------------------------------------------------
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( InstanceBuffer& instanceData ) const;
	virtual void OnActivate( InstanceBuffer& data, const CName& inputNamee, CQuestThread* parentThread ) const;
	virtual void OnExecute( InstanceBuffer& data ) const;
	virtual void OnDeactivate( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Runtime data access
	// ------------------------------------------------------------------------
	// Returns the thread the function is running, if there's one
	const CScriptThread* const GetScriptThread( InstanceBuffer& data ) const;
	const Bool CheckIfAllDataLoaded( InstanceBuffer& data ) const;

	// ------------------------------------------------------------------------
	// Saving the game state
	// ------------------------------------------------------------------------
	virtual void SaveGame( InstanceBuffer& data, IGameSaver* saver ) const;
	virtual void LoadGame( InstanceBuffer& data, IGameLoader* loader ) const;

protected:
	// -------------------------------------------------------------------------
	// IDynamicPropertiesSupplier implementation
	// -------------------------------------------------------------------------
	virtual IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier();
	virtual const IDynamicPropertiesSupplier* QueryDynamicPropertiesSupplier() const;
	virtual void GetDynamicProperties( TDynArray< CName >& properties ) const;
	virtual Bool ReadDynamicProperty( const CName& propName, CVariant& propValue ) const;
	virtual Bool WriteDynamicProperty( const CName& propName, const CVariant& propValue );
	virtual void SerializeDynamicPropertiesForGC( IFile& file );

private:
	CFunction* GetFunction() const;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void UpdateSockets( CFunction* function );
	void UpdateCaption( CFunction* function );
	void UpdateParameters( CFunction* function );
#endif
	void SetupFunctionCallstack( CFunction* function, void* stackData, InstanceBuffer& data ) const;
	Bool RunImmediateFunction( CFunction* function, void* stackData ) const;
	void RunLatentFunction( InstanceBuffer& data, CFunction* function, void* stackData ) const;
	void Exit( InstanceBuffer& data, Bool result = true ) const;
};

BEGIN_CLASS_RTTI( CQuestScriptBlock )
	PARENT_CLASS( CQuestGraphBlock )
	PROPERTY_CUSTOM_EDIT( m_functionName, TXT("Name of the quest function"), TXT("QuestFunctionList") );
	PROPERTY_EDIT( m_saveMode, TXT("Save mode for script block") );
	PROPERTY( m_parameters )
	PROPERTY( m_choiceOutput )
	PROPERTY( m_caption )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

struct LatentScriptData : public CScriptThread::IListener
{
	CFunction*					m_function;
	CScriptStackFrame*			m_frame;
	CScriptThread*				m_thread;
	void*						m_stackData;
	CPropertyDataBuffer*		m_scriptReturnValue;
	Bool						m_isFinished;
	Bool						m_result;

	// Constructor.
	LatentScriptData( CFunction* scriptFunction, CScriptStackFrame* frame, CScriptThread* scriptThread, void* stackData, CPropertyDataBuffer* scriptReturnValue );
	virtual ~LatentScriptData();

	// -------------------------------------------------------------------------
	// CScriptThread::IListener implementation
	// -------------------------------------------------------------------------
	virtual void OnScriptThreadKilled( CScriptThread* thread, Bool finished );
	virtual String GetDebugName() const { return String(TXT("QuestScriptData")); }
};
