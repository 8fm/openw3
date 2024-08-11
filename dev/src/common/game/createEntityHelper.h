#pragma once

class CCreateEntityHelper : public IScriptable
{
	DECLARE_RTTI_SIMPLE_CLASS( CCreateEntityHelper );
public :
	// User interface
	CCreateEntityHelper();
	~CCreateEntityHelper();

	Bool			IsCreating()const;
	void			Reset();
	CEntity *const	GetCreatedEntity();
	void			SetPostAttachedScriptCallback( IScriptable *const caller, CName funcName );

	// script interface
	void funcIsCreating( CScriptStackFrame& stack, void* result );
	void funcReset( CScriptStackFrame& stack, void* result );
	void funcGetCreatedEntity( CScriptStackFrame& stack, void* result );
	void funcSetPostAttachedCallback( CScriptStackFrame& stack, void* result );

public:
	// System interface
	class CScriptSpawnEventHandler : public ISpawnEventHandler
	{
	public :
		CScriptSpawnEventHandler( IScriptable *const caller, CName funcName );
		void CallOnPostSpawnCallback( CEntity* entity );
	private:
		THandle<IScriptable>	m_caller;
		CName					m_funcName;
	protected:
		void OnPostAttach( CEntity* entity ) override;
		
	};

	// called when helper starts processing
	virtual void BeginProcessing();

	// Update spawn processing
	virtual Bool Update( CCreateEntityManager* manager );

	// Called when helper becomes obsolate (spawn failed or completed)
	virtual void Discard( CCreateEntityManager* manager );

	// Starts entity creation job
	Bool StartSpawnJob( EntitySpawnInfo&& spawnInfo );

	// If the entity is created anew this will be called before the spawn job is pushed
	void OnPreSpawn( EntitySpawnInfo & entitySpawnInfo );

	// Registers info about the spawn job so that it can be listened to
	void OnSpawnJobStarted( IJobEntitySpawn*	jobSpawnEntity );

	// Listen to spawn Job And do stuff once it's finished
	Bool UpdateSpawnJob();

	// Converts the entity to a stray entity once it is spawned and releases the spawn job
	void OnSpawnJobFinished();

	// manual accessor in case there is no need to spawn the entity 
	CScriptSpawnEventHandler* GetScriptSpawnEventHandler()const					{ return m_scriptSpawnEventHandler; }
	CScriptSpawnEventHandler* StealScriptSpawnEventHandler()					{ CScriptSpawnEventHandler* ret = m_scriptSpawnEventHandler; m_scriptSpawnEventHandler = nullptr; return ret; }

	
	IJobEntitySpawn*			m_jobSpawnEntity;
	CEntity*					m_entity;
	CScriptSpawnEventHandler *	m_scriptSpawnEventHandler;
	Bool						m_isCreating;
	
	
};
BEGIN_CLASS_RTTI( CCreateEntityHelper );
	PARENT_CLASS( IScriptable );
	NATIVE_FUNCTION( "IsCreating", funcIsCreating );
	NATIVE_FUNCTION( "Reset", funcReset );
	NATIVE_FUNCTION( "GetCreatedEntity", funcGetCreatedEntity );
	NATIVE_FUNCTION( "SetPostAttachedCallback", funcSetPostAttachedCallback );
END_CLASS_RTTI();