#include "build.h"
#include "behTreeTask.h"

#include "baseDamage.h"
#include "behTreeCustomMoveData.h"
#include "behTreeInstance.h"
#include "behTreeScriptedNode.h"
#include "behTreeVarsUtils.h"
#include "behTreeReactionData.h"

#include "../core/scriptingSystem.h"

//#define PROFILE_BEHTREE_SCRIPT_TASKS

#ifdef PROFILE_BEHTREE_SCRIPT_TASKS

struct BehTreeTaskProfilerRegister
{
	BehTreeTaskProfilerRegister( const AnsiChar* n )
		: m_n( n ) {}

	CProfilerHandle* Get( CClass* c )
	{
		CProfilerHandle*& p = m_h[ c ];
		if ( p == nullptr )
		{
			m_strings.PushBack( m_n + c->GetName().AsAnsiChar() );
			p = UnifiedProfilerManager::GetInstance().RegisterHandler( m_strings.Back().AsChar(), 1, PBC_CPU );
		}
		return p;
	}

	THashMap< CClass*, CProfilerHandle* >			m_h;
	StringAnsi										m_n;
	TDynArray< StringAnsi >							m_strings;
};

#define BEHTREE_TASK_PROFILER_REG( profilerName ) static BehTreeTaskProfilerRegister s_Profiler##profilerName( #profilerName );
#define BEHTREE_TASK_PROFILER_SCOPE( profilerName ) CProfilerBlock __profilerBlock_( s_Profiler##profilerName.Get( m_taskInstance->GetClass() ) );

#else

#define BEHTREE_TASK_PROFILER_REG( profilerName )
#define BEHTREE_TASK_PROFILER_SCOPE( profilerName )

#endif

////////////////////////////////////////////////////////////////////////
// CBehTreeScriptedInstance
////////////////////////////////////////////////////////////////////////
CBehTreeScriptedInstance::CBehTreeScriptedInstance( CBehTreeSpawnContext& context, IBehTreeTaskDefinition* taskDefinition, IBehTreeNodeInstance* nodeInstance, Uint16& flags, Bool runMainOnActivation )
	: m_thread( NULL )
	, m_scriptStateFlags( SS_DEFAULT )
{
	if ( taskDefinition )
	{
		m_taskInstance = taskDefinition->SpawnInstance( context, nodeInstance, this );
		IBehTreeTask* behTreeTask = m_taskInstance.Get();
		if ( behTreeTask )
		{ 
			behTreeTask->SetInstanceData( this );
			if ( flags == IBehTreeTask::FLAGS_UNINITIALIZED )
			{
				flags = behTreeTask->ComputeTaskFlags();
			}
			m_taskFlags = flags;
		}
	}

	if ( !m_taskInstance )
	{
		AI_LOG( TXT("Problem while spawning IBehTreeTask instance!") );
		m_taskFlags = 0;
	}
	m_autoRunMain = runMainOnActivation && (m_taskFlags & IBehTreeTask::FLAG_HAS_MAIN) != 0;
}
CBehTreeScriptedInstance::~CBehTreeScriptedInstance()
{
	ASSERT( !m_thread, TXT("Critical AI consistency problem. Node didn't get deactivated so it leads to many many different shitty behaviors. Don't shield against this crash otherwise you will get even harder to spot errors later on during the production.") );
}

void CBehTreeScriptedInstance::ScriptInitialize( const IBehTreeTaskDefinition* taskDefinition, CBehTreeSpawnContext& context )
{
	if ( taskDefinition && taskDefinition->HasOnSpawn() )
	{
		taskDefinition->OnSpawn( context, this, m_taskInstance );
	}
	
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_INITIALIZE )
	{
		IBehTreeTask* task = m_taskInstance.Get();

		CallFunction( task, CNAME( Initialize ) );
	}
}

CBehTreeScriptedInstance::eUpdateResult CBehTreeScriptedInstance::ScriptUpdate()
{
	if ( m_scriptStateFlags & ( SS_RETURNED | SS_RUNMAIN ) )
	{
		if ( m_scriptStateFlags & SS_RETURNED )
		{
			return (m_scriptStateFlags & SS_FAILED) ? U_FAILURE : U_SUCCESS;
		}
		else if ( m_scriptStateFlags & SS_RUNMAIN )
		{
			ASSERT( !m_thread );
			IBehTreeTask* behTreeTask = m_taskInstance.Get();
			IScriptable* context = behTreeTask;
			const CFunction* fun = behTreeTask->FindFunction( context, CNAME( Main ) );
			IRTTIType* returnType = fun->GetReturnValue()->GetType();
			m_threadReturnValue.Reset( returnType );
			const Uint32 stackSize = fun->GetStackSize();

			void* stackData = RED_MEMORY_ALLOCATE_HYBRID( MemoryPool_Default, MC_ScriptObject, stackSize );

			CScriptStackFrame* frame = new CScriptStackFrame( NULL, context, fun, stackData, stackData );
			m_thread = GScriptingSystem->CreateThreadUseGivenFrame( context, fun, *frame, m_threadReturnValue.Data() );
			m_thread->SetListener( this );
			m_scriptStateFlags = SS_RUNNING;
		}
	}
	return U_CONTINUE;
}

BEHTREE_TASK_PROFILER_REG( Activate )

Bool CBehTreeScriptedInstance::ScriptActivate()
{
	IBehTreeTask* behTreeTask = m_taskInstance.Get();
	if ( behTreeTask )
	{
		behTreeTask->SetActive( true );
	}
	m_scriptStateFlags = SS_DEFAULT;
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_ACTIVATE )
	{
		EBTNodeStatus status;

		BEHTREE_TASK_PROFILER_SCOPE( Activate );

		if( !CallFunctionRet< EBTNodeStatus >( behTreeTask, CNAME( OnActivate ), status ) )
		{
			status = BTNS_Active;
		}
		switch ( status )
		{
		case BTNS_Active:
			m_scriptStateFlags = SS_DEFAULT;
			break;
		case BTNS_Completed:
			m_scriptStateFlags = SS_RETURNED | SS_NOABORT;
			behTreeTask->SetActive( false );
			return true;
		case BTNS_Failed:
		default:
			behTreeTask->SetActive( false );
			return false;
		}
	}
	
	if ( m_autoRunMain )
	{
		m_scriptStateFlags |= SS_RUNMAIN;

	}
	return true;
}

BEHTREE_TASK_PROFILER_REG( Deactivate )
void CBehTreeScriptedInstance::ScriptDeactivate()
{
	IBehTreeTask* behTreeTask = m_taskInstance.Get();
	if ( behTreeTask )
	{
		behTreeTask->SetActive( false );
		behTreeTask->ClearBehTreeEvent();
	}
	if ( m_thread )
	{
		m_thread->SetListener( NULL );

		if ( m_scriptStateFlags & SS_RUNNING )
		{
			m_thread->ForceKill();
		}

		m_thread = NULL;
	}

	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_DEACTIVATE && (m_scriptStateFlags & SS_NOABORT) == 0 )
	{
		BEHTREE_TASK_PROFILER_SCOPE( Deactivate );
		CallFunction( behTreeTask, CNAME( OnDeactivate ) );
	}

	m_scriptStateFlags = SS_DEFAULT;

}

BEHTREE_TASK_PROFILER_REG( Available );
Bool CBehTreeScriptedInstance::ScriptIsAvailable()
{
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_IS_AVAILABLE )
	{
		Bool available = false;
		BEHTREE_TASK_PROFILER_SCOPE( Available );
		CallFunctionRet< Bool >( m_taskInstance.Get(), CNAME( IsAvailable ), available );
		return available;
	}

	return true;
}

BEHTREE_TASK_PROFILER_REG( OnCompletion );

void CBehTreeScriptedInstance::ScriptOnCompletion( IBehTreeNodeInstance::eTaskOutcome e )
{
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_COMPLETION )
	{
		BEHTREE_TASK_PROFILER_SCOPE( OnCompletion );
		CallFunction< Bool >( m_taskInstance.Get(), CNAME( OnCompletion ), e == IBehTreeNodeInstance::BTTO_SUCCESS );
	}
}

BEHTREE_TASK_PROFILER_REG( Evaluate );

Int32 CBehTreeScriptedInstance::ScriptEvaluate( Int32 defaultPriority )
{
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_EVALUATE )
	{
		BEHTREE_TASK_PROFILER_SCOPE( Evaluate );
		Int32 eval = false;
		CallFunctionRet< Int32 >( m_taskInstance.Get(), CNAME( Evaluate ), eval );
		return eval;
	}
	else if ( m_taskFlags & IBehTreeTask::FLAG_HAS_IS_AVAILABLE )
	{
		BEHTREE_TASK_PROFILER_SCOPE( Available );
		Bool available = false;
		CallFunctionRet< Bool >( m_taskInstance.Get(), CNAME( IsAvailable ), available );
		if ( !available )
			return -1;
	}

	return defaultPriority;
}

BEHTREE_TASK_PROFILER_REG( OnListenedEvent );

Bool CBehTreeScriptedInstance::ScriptOnListenedEvent( CBehTreeEvent& e )
{
	if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_LISTENED_GAMEPLAY_EVENT )
	{
		Bool result = false;
		IBehTreeTask* task = m_taskInstance;
		if ( task )
		{
			BEHTREE_TASK_PROFILER_SCOPE( OnListenedEvent );

			task->SetBehTreeEvent( const_cast<CBehTreeEvent&>(e) );
			CallFunctionRet< Bool >( task, CNAME( OnListenedGameplayEvent ), e.m_eventName, result );
			task->ClearBehTreeEvent();
		}
		else
		{
			WARN_GAME( TXT("Handle to task instance for lost") );
		}

		return result;
	}
	else
	{
		return ScriptOnEvent( e );
	}
}

BEHTREE_TASK_PROFILER_REG( OnAnimEvent );
BEHTREE_TASK_PROFILER_REG( OnEvent );

Bool CBehTreeScriptedInstance::ScriptOnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventType != BTET_GameplayEvent )
	{
		// anim event
		if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_ANIM_EVENT )
		{
			BEHTREE_TASK_PROFILER_SCOPE( OnAnimEvent );

			Bool result = false;
			CallFunctionRet< Bool >( m_taskInstance.Get(), CNAME( OnAnimEvent ), e.m_eventName, CBehTreeEvent::ConvertToAnimationEvent( e.m_eventType ), e.m_animEventData.m_eventAnimInfo? *e.m_animEventData.m_eventAnimInfo : SAnimationEventAnimInfo(), result );
			return result;
		}
	}
	else
	{
		// gameplay event
		if ( m_taskFlags & IBehTreeTask::FLAG_HAS_ON_GAMEPLAY_EVENT )
		{
			BEHTREE_TASK_PROFILER_SCOPE( OnEvent );

			Bool result = false;
			IBehTreeTask* task = m_taskInstance;
			task->SetBehTreeEvent( const_cast<CBehTreeEvent&>(e) );
			CallFunctionRet< Bool >( task, CNAME( OnGameplayEvent ), e.m_eventName, result );
			task->ClearBehTreeEvent();
			return result;
		}
	}
	return false;
}

void CBehTreeScriptedInstance::ScriptOnDestruction()
{
	CBehTreeInstance* owner = ScriptGetOwner();
	const Uint32 &listenerCount = m_listenedEventList.Size();
	if ( listenerCount != 0 )
	{
		IBehTreeNodeInstance *const behTreeNodeInstance = ScriptGetNode();
		for ( Uint32 i = 0; i < listenerCount; ++i )
		{
			owner->RemoveEventListener( m_listenedEventList[ i ],  behTreeNodeInstance );
		}
	}
}


void CBehTreeScriptedInstance::ReviveMain()
{
	if ( ( m_scriptStateFlags & SS_RUNNING ) == 0 && (m_taskFlags & IBehTreeTask::FLAG_HAS_MAIN) )
	{
		m_scriptStateFlags |= SS_RUNMAIN;
	}
}

void CBehTreeScriptedInstance::OnScriptThreadKilled( CScriptThread* thread, Bool finished )
{
	if ( finished )
	{
		EBTNodeStatus status = *( EBTNodeStatus* ) m_threadReturnValue.Data();
		switch ( status )
		{
		default:
		case BTNS_Failed:
			m_scriptStateFlags = SS_RETURNED | SS_FAILED;
			break;
		case BTNS_Completed:
			m_scriptStateFlags = SS_RETURNED;
			break;
		case BTNS_Active:
			m_scriptStateFlags = SS_DEFAULT;
			break;
		} 
	}
	m_thread = NULL;
}
String CBehTreeScriptedInstance::GetDebugName() const
{
	return String(TXT("BehaviorTreeScriptedInstance"));
}
void CBehTreeScriptedInstance::ScriptCompletedItself( Bool success )
{
	if ( m_thread )
	{
		m_thread->SetListener( NULL );

		if ( m_scriptStateFlags & SS_RUNNING )
		{
			m_thread->ForceKill();
		}

		m_thread = NULL;
	}
	m_scriptStateFlags = SS_RETURNED;
	if ( !success )
	{
		m_scriptStateFlags |= SS_FAILED;
	}
}
void CBehTreeScriptedInstance::ScriptOnEventListenerAdded( const SBehTreeEvenListeningData & event )
{
	m_listenedEventList.PushBack( event );
}
////////////////////////////////////////////////////////////////////////
// IBehTreeTask
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeTask );

RED_DEFINE_STATIC_NAME( GetLabel );

IBehTreeTask::IBehTreeTask()
	: m_instance( NULL )
	, m_event ( NULL )
	, m_isActive( false )
{
	EnableReferenceCounting();
}

IBehTreeTask::~IBehTreeTask()
{}

Uint16 IBehTreeTask::ComputeTaskFlags() const
{
	Uint16 ret = 0;
	{
		IScriptable* context = const_cast< IBehTreeTask* >( this );
		const CFunction* fun = FindFunction( context, CNAME( Main ) );
		if( fun && fun->IsLatent() )
		{			
			CProperty* retVal = fun->GetReturnValue();
			if ( retVal && retVal->GetType() && retVal->GetType()->GetName() == GetTypeName< EBTNodeStatus >() )
			{
				ret |= FLAG_HAS_MAIN;
			}
		}
	}
	auto funCheck =
		[ this ]( const CName& c ) -> Bool
	{
		IScriptable* context = const_cast< IBehTreeTask* >( this );
		const CFunction* fun = this->FindFunction( context, c );
		return fun != NULL;
	};


	if ( funCheck( CNAME( OnActivate ) ) )
	{
		ret |= FLAG_HAS_ON_ACTIVATE;
	}
	if ( funCheck( CNAME( OnDeactivate ) ) )
	{
		ret |= FLAG_HAS_ON_DEACTIVATE;
	}
	if ( funCheck( CNAME( OnCompletion ) ) )
	{
		ret |= FLAG_HAS_ON_COMPLETION;
	}
	if ( funCheck( CNAME( OnAnimEvent ) ) )
	{
		ret |= FLAG_HAS_ON_ANIM_EVENT;
	}
	if ( funCheck( CNAME( OnGameplayEvent ) ) )
	{
		ret |= FLAG_HAS_ON_GAMEPLAY_EVENT;
	}
	if ( funCheck( CNAME( OnListenedGameplayEvent ) ) )
	{
		ret |= FLAG_HAS_ON_LISTENED_GAMEPLAY_EVENT;
	}
	if ( funCheck( CNAME( IsAvailable ) ) )
	{
		ret |= FLAG_HAS_IS_AVAILABLE;
	}
	if ( funCheck( CNAME( Evaluate ) ) )
	{
		ret |= FLAG_HAS_EVALUATE;
	}
	if ( funCheck( CNAME( Initialize ) ) )
	{
		ret |= FLAG_HAS_INITIALIZE;
	}

	return ret;
}

CName& IBehTreeTask::GetEventParamCName( CName& defaultVal )
{
	if ( m_event && m_event->m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)m_event->m_gameplayEventData.m_customDataType;
		SGameplayEventParamCName* data = Cast < SGameplayEventParamCName > ( classId, m_event->m_gameplayEventData.m_customData );
		if ( data)
		{
			return data->m_value;
		}
	}

	return defaultVal;
}

Int32 IBehTreeTask::GetEventParamInt( Int32 defaultVal )
{
	if ( m_event && m_event->m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)m_event->m_gameplayEventData.m_customDataType;
		SGameplayEventParamInt* data = Cast < SGameplayEventParamInt > ( classId, m_event->m_gameplayEventData.m_customData );
		if ( data)
		{
			return data->m_value;
		}
	}

	return defaultVal;
}

Float IBehTreeTask::GetEventParamFloat( Float defaultVal )
{
	if ( m_event && m_event->m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)m_event->m_gameplayEventData.m_customDataType;
		SGameplayEventParamFloat* data = Cast < SGameplayEventParamFloat > ( classId, m_event->m_gameplayEventData.m_customData );
		if ( data)
		{
			return data->m_value;
		}
	}

	return defaultVal;
}

THandle< IScriptable > IBehTreeTask::GetEventParamCObject()
{
	if ( m_event && m_event->m_gameplayEventData.m_customDataType && m_event->m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)m_event->m_gameplayEventData.m_customDataType;
		SGameplayEventParamObject* data = Cast < SGameplayEventParamObject > ( classId, m_event->m_gameplayEventData.m_customData );
		if ( data )
		{
			return data->m_value;
		}
	}

	return NULL;
}

void IBehTreeTask::GetTaskName( String& str ) const
{
	static String temp,label;		
	static const String BEGINING = TXT("CBTTask");
	temp = GetFriendlyName();

	label.Clear();
	CallFunctionRef( const_cast<IBehTreeTask*>(this), CNAME( GetLabel ), label );
	if( temp.BeginsWith( BEGINING ) )
	{
		str = temp.MidString( BEGINING.GetLength() );
		if( label.GetLength() > 0 )
			str += label;
	}
	else
	{
		str = temp;
		if( label.GetLength() > 0 )
			str += label;
	}	
}
void IBehTreeTask::funcSetEventRetvalCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, val, CName::NONE );
	FINISH_PARAMETERS;

	Bool success = false;

	if ( m_event && m_event->m_eventType == BTET_GameplayEvent )
	{
		SGameplayEventParamCName* data = m_event->m_gameplayEventData.Get< SGameplayEventParamCName >();
		if ( data )
		{
			data->m_value = val;
			success = true;
		}
	}

	RETURN_BOOL( success );
}
void IBehTreeTask::funcSetEventRetvalFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, val, 0.f );
	FINISH_PARAMETERS;

	Bool success = false;

	if ( m_event && m_event->m_eventType == BTET_GameplayEvent )
	{
		SGameplayEventParamFloat* data = m_event->m_gameplayEventData.Get< SGameplayEventParamFloat >();
		if ( data )
		{
			data->m_value = val;
			success = true;
		}
	}

	RETURN_BOOL( success );
}
void IBehTreeTask::funcSetEventRetvalInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, val, 0 );
	FINISH_PARAMETERS;

	Bool success = false;

	if ( m_event && m_event->m_eventType == BTET_GameplayEvent )
	{
		SGameplayEventParamInt* data = m_event->m_gameplayEventData.Get< SGameplayEventParamInt >();
		if ( data )
		{
			data->m_value = val;
			success = true;
		}
	}

	RETURN_BOOL( success );
}


void IBehTreeTask::funcGetEventParamBaseDamage( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	THandle< CBaseDamage > damageData;

	if ( m_event && m_event->m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)m_event->m_gameplayEventData.m_customDataType;
		CBaseDamage* data = Cast < CBaseDamage > ( classId, m_event->m_gameplayEventData.m_customData );
		if ( data)
		{
			damageData = data;
		}
	}

	RETURN_HANDLE( CBaseDamage, damageData );
}
void IBehTreeTask::funcGetEventParamCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, defaultVal, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_NAME( GetEventParamCName( defaultVal ) );
}
void IBehTreeTask::funcGetEventParamFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Float, defaultVal, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetEventParamFloat( defaultVal ) );
}
void IBehTreeTask::funcGetEventParamInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, defaultVal, 0 );
	FINISH_PARAMETERS;

	RETURN_INT( GetEventParamInt( defaultVal ) );
}
void IBehTreeTask::funcUnregisterFromAnimEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_instance )
	{
		IBehTreeNodeInstance* node = m_instance->ScriptGetNode();
		SBehTreeEvenListeningData e;
		e.m_eventName = eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_ANIM;

		node->GetOwner()->RemoveEventListener( e, node );
	}
}
void IBehTreeTask::funcUnregisterFromGameplayEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_instance )
	{
		IBehTreeNodeInstance* node = m_instance->ScriptGetNode();
		SBehTreeEvenListeningData e;
		e.m_eventName = eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;

		node->GetOwner()->RemoveEventListener( e, node );
	}
}

void IBehTreeTask::funcSetIsInCombat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, isCombat, false );
	FINISH_PARAMETERS;

	if ( m_instance )
	{
		CBehTreeInstance* instance = m_instance->ScriptGetOwner();
		instance->SetIsInCombat( isCombat );
	}
}

void IBehTreeTask::funcSetCustomTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	GET_PARAMETER( Float, heading, 0.f );
	FINISH_PARAMETERS;

	Bool b = false;

	if ( m_instance )
	{
		CBehTreeInstance* owner = m_instance->ScriptGetOwner();
		CAIStorageItem* item = owner->GetItem( CBehTreeCustomMoveData::DefaultStorageName() );
		if ( item )
		{
			CBehTreeCustomMoveData* data = item->GetPtr< CBehTreeCustomMoveData >();
			if ( data )
			{
				data->SetTarget( position );
				data->SetHeading( heading );
				b = true;
			}
		}
	}
	RETURN_BOOL( b );
}

void IBehTreeTask::funcGetCustomTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Vector, position, Vector::ZEROS );
	GET_PARAMETER_REF( Float, heading, 0.f );
	FINISH_PARAMETERS;

	Bool b = false;

	if ( m_instance )
	{
		CBehTreeInstance* owner = m_instance->ScriptGetOwner();
		CAIStorageItem* item = owner->GetItem( CBehTreeCustomMoveData::DefaultStorageName() );
		if ( item )
		{
			CBehTreeCustomMoveData* data = item->GetPtr< CBehTreeCustomMoveData >();
			if ( data )
			{
				position = data->GetTarget();
				heading = data->GetHeading();
				b = true;
			}
		}
	}
	RETURN_BOOL( b );
}


void IBehTreeTask::funcGetEventParamObject( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	THandle< IScriptable > obj( GetEventParamCObject() );

	RETURN_HANDLE( IScriptable, obj );
}
void IBehTreeTask::funcGetActor( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_instance->ScriptGetOwner()->GetActor() );
}

void IBehTreeTask::funcGetNPC( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_OBJECT( m_instance->ScriptGetOwner()->GetNPC() );
}
void IBehTreeTask::funcGetLocalTime( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_FLOAT( m_instance->ScriptGetOwner()->GetLocalTime() );
}

void IBehTreeTask::funcGetNamedTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, targetName, CName::NONE );
	FINISH_PARAMETERS;
	THandle< CNode > node = m_instance->ScriptGetOwner()->GetNamedTarget( targetName );
	RETURN_HANDLE( CNode, node );
}
void IBehTreeTask::funcSetNamedTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, targetName, CName::NONE );
	GET_PARAMETER( THandle< CNode >, node, NULL );
	FINISH_PARAMETERS;
	m_instance->ScriptGetOwner()->SetNamedTarget( targetName, node );
}

void IBehTreeTask::funcGetActionTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	const THandle< CNode >& node = m_instance->ScriptGetOwner()->GetActionTarget();
	RETURN_HANDLE( CNode, node );
}
void IBehTreeTask::funcSetActionTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, NULL );
	FINISH_PARAMETERS;
	m_instance->ScriptGetOwner()->SetActionTarget( node );
}
void IBehTreeTask::funcGetCombatTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	const THandle< CActor >& node = m_instance->ScriptGetOwner()->GetCombatTarget();
	RETURN_HANDLE( CActor, node );
}
void IBehTreeTask::funcSetCombatTarget( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CActor >, node, NULL );
	FINISH_PARAMETERS;
	m_instance->ScriptGetOwner()->SetCombatTarget( node );
}
void IBehTreeTask::funcRunMain( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	m_instance->ReviveMain();
}
void IBehTreeTask::funcComplete( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, success, true );
	FINISH_PARAMETERS;
	m_instance->ScriptCompletedItself( success );
}

void IBehTreeTask::funcGetReactionEventInvoker( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	THandle< CEntity > invoker;

	if ( m_event != nullptr && m_event->m_eventType == BTET_GameplayEvent )
	{
		if ( m_event->m_gameplayEventData.m_customDataType == CBehTreeReactionEventData::GetStaticClass() )
		{
			CBehTreeReactionEventData* data = reinterpret_cast< CBehTreeReactionEventData* >( m_event->m_gameplayEventData.m_customData );
			if ( data != nullptr )
			{
				invoker = data->GetInvoker();
			}
		}
	}
	RETURN_HANDLE( CEntity, invoker );
}

void IBehTreeTask::funcRequestStorageItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( CName, itemType, CName::NONE );
	FINISH_PARAMETERS;
	THandle< IScriptable > scriptable = m_instance->ScriptGetOwner()->ScriptableStorageRequestItem( itemName, itemType );
	RETURN_HANDLE( IScriptable, scriptable );
}

void IBehTreeTask::funcFindStorageItem( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, itemName, CName::NONE );
	GET_PARAMETER( CName, itemType, CName::NONE );
	FINISH_PARAMETERS;
	THandle< IScriptable > scriptable = m_instance->ScriptGetOwner()->ScriptableStorageFindItem( itemName, itemType );
	RETURN_HANDLE( IScriptable, scriptable );
}

////////////////////////////////////////////////////////////////////////
// IBehTreeObjectDefinition
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeObjectDefinition );

Bool IBehTreeObjectDefinition::OnPropertyTypeMismatch( CName propertyName, IProperty* existingProperty, const CVariant& readValue )
{
	if ( IScriptable::OnPropertyTypeMismatch( propertyName, existingProperty, readValue ) )
	{
		return true;
	}
	if ( BehTreeVarsUtils::OnPropertyTypeMismatch( this, propertyName, existingProperty, readValue )  )
	{
		return true;
	}
	
	return false;
}

void IBehTreeObjectDefinition::OnScriptReloaded()
{
	InitializeTranslation();

	IScriptable::OnScriptReloaded();
}

CClass* IBehTreeObjectDefinition::GetInstancedClass()
{
	return SRTTI::GetInstance().FindClass( m_instanceClass );
}

Bool IBehTreeObjectDefinition::IsPropertyTranslated( CProperty* property ) const
{
	if ( property->GetName() == CNAME( instanceClass ) )
	{
		return false;
	}

	return true;
}

void IBehTreeObjectDefinition::InitializeTranslation()
{
	m_translation.ClearFast();

	CClass* insClass = SRTTI::GetInstance().FindClass( m_instanceClass );
	if ( !insClass )
	{
		// invalid object what so ever
		return;
	}

	CClass* defClass = GetClass();

	const CClass::TPropertyList& allDefProperties = defClass->GetCachedProperties();
	const CClass::TPropertyList& allInsProperties = insClass->GetCachedProperties();

	TArrayMap< CName, IProperty* > defProperties;
	TArrayMap< CName, IProperty* > insProperties;

	auto funFilterProperties =
		[ this ] ( const CClass::TPropertyList& input, TArrayMap< CName, IProperty* >& output )
		{
			output.Reserve( input.Empty() ? 0 : input.Size()-1 );
			for ( auto it = input.Begin(), end = input.End(); it != end; ++it )
			{
				IProperty* property = *it;
				if ( property )
				{
					if ( !this->IsPropertyTranslated( property ) )
					{
						continue;
					}
					
					output.PushBack( TArrayMap< CName, IProperty* >::value_type( property->GetName(), property ) );
				}
			}
			output.Sort();
		};

	// collect & sort priorities to process them lineary
	funFilterProperties( allDefProperties, defProperties );
	funFilterProperties( allInsProperties, insProperties );

	// move through sorted lists and compute matching properties
	auto itDef = defProperties.Begin(), endDef = defProperties.End();
	auto itIns = insProperties.Begin(), endIns = insProperties.End();

	while ( itDef != endDef && itIns != endIns )
	{
		if ( itDef->m_first == itIns->m_first )
		{
			if ( BehTreeVarsUtils::IsPropertyTranslatable( itDef->m_second->GetType(), itIns->m_second->GetType() ) )
			{
				PropertyTranslation def;
				def.m_propertyDefinition = itDef->m_second;
				def.m_propertyInstance = itIns->m_second;
				m_translation.PushBack( def );
			}

			++itDef;
			++itIns;
		}
		else if ( itDef->m_first < itIns->m_first )
		{
			++itDef;
		}
		else
		{
			++itIns;
		}
	}

	// translation is cool. But lets also cache fact we have OnSpawn function
	{
		IScriptable* context = const_cast< IBehTreeObjectDefinition* >( this );
		const CFunction* fun = this->FindFunction( context, CNAME( OnSpawn ) );
		m_hasOnSpawn = fun != nullptr;
	}
}

void IBehTreeObjectDefinition::Translate( IScriptable* objPtr, CBehTreeSpawnContext& context ) const
{
	for ( auto it = m_translation.Begin(), end = m_translation.End(); it != end; ++it )
	{
		BehTreeVarsUtils::Translate( this, objPtr, it->m_propertyDefinition, it->m_propertyInstance, context );
	}
}

THandle< IScriptable > IBehTreeObjectDefinition::SpawnInstanceBase( CBehTreeSpawnContext& context ) const
{
	CClass* classId = SRTTI::GetInstance().FindClass( m_instanceClass );
	if ( !classId )
	{
		return nullptr;
	}

	THandle< IScriptable > obj = classId->CreateObject< IScriptable >();
	IScriptable* objPtr = obj.Get();
	if ( !objPtr )
	{
		return nullptr;
	}

	Translate( objPtr, context );

	return obj;
}

void IBehTreeObjectDefinition::OnPostLoad()
{
	InitializeTranslation();

	IScriptable::OnPostLoad();
}

#ifndef NO_EDITOR

void IBehTreeObjectDefinition::OnCreatedInEditor()
{
	InitializeTranslation();

	IScriptable::OnCreatedInEditor();
}

void IBehTreeObjectDefinition::Refactor()
{
	IScriptable* context = this;
	::CallFunction( context, CNAME( Refactor ) );
}

void IBehTreeObjectDefinition::RefactorAll()
{
	TDynArray< THandle< IScriptable > > allScriptables;
	IScriptable::CollectAllScriptableObjects( allScriptables );

	for ( auto it = allScriptables.Begin(), end = allScriptables.End(); it != end; ++it )
	{
		IBehTreeObjectDefinition* task = Cast< IBehTreeObjectDefinition >( it->Get() );
		if ( task )
		{
			task->Refactor();
		}
	}
}

#endif


void IBehTreeObjectDefinition::funcGetValFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValFloat, var, CBehTreeValFloat() );
	FINISH_PARAMETERS;

	Float ret = m_context ? var.GetVal( *m_context ) : var.m_value;
	RETURN_FLOAT( ret );
}
void IBehTreeObjectDefinition::funcGetValInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValInt, var, CBehTreeValInt() );
	FINISH_PARAMETERS;

	Int32 ret = m_context ? var.GetVal( *m_context ) : var.m_value;
	RETURN_INT( ret );
}
void IBehTreeObjectDefinition::funcGetValEnum( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( THandle<IBehTreeValueEnum>, var, NULL );
	FINISH_PARAMETERS;

	Int32 ret = var.Get() ? (m_context ? var.Get()->GetVal( *m_context ) : var.Get()->GetValue()) : 0;
	RETURN_INT( ret );
}
void IBehTreeObjectDefinition::funcGetValString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValString, var, CBehTreeValString() );
	FINISH_PARAMETERS;

	String ret;
	if ( !m_context || !var.GetValRef( *m_context, ret ) )
	{
		ret = var.m_value;
	}
	RETURN_STRING( ret );
}
void IBehTreeObjectDefinition::funcGetValCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValCName, var, CBehTreeValCName() );
	FINISH_PARAMETERS;

	CName ret = m_context ? var.GetVal( *m_context ) : var.m_value;
	RETURN_NAME( ret );
}
void IBehTreeObjectDefinition::funcGetValBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValBool, var, CBehTreeValBool() );
	FINISH_PARAMETERS;

	Bool ret = m_context ? var.GetVal( *m_context ) : var.m_value;
	RETURN_BOOL( ret );
}

void IBehTreeObjectDefinition::funcGetObjectByVar( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, var, CName::NONE );
	FINISH_PARAMETERS;

	THandle< IScriptable > obj;
	if ( m_context )
	{
		THandle< CObject > obj2;
		m_context->GetValRef< THandle< CObject > >( var, obj2 );

		if ( obj2.IsValid() )
		{
			obj = Cast< IScriptable >( obj2 );
		}
		else
		{
			m_context->GetValRef< THandle< IScriptable > >( var, obj );
		}
	}

	RETURN_HANDLE( IScriptable, obj );
}

void IBehTreeObjectDefinition::funcGetAIParametersByClassName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	IAIParameters * aiParams = nullptr;
	if ( m_context )
	{
		aiParams = m_context->GetAiParametersByClassName( className );
	}
	THandle< IAIParameters > obj( aiParams );
	RETURN_HANDLE( IAIParameters, obj );
}


void IBehTreeObjectDefinition::funcSetValFloat( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValFloat, var, CBehTreeValFloat() );
	GET_PARAMETER( Float, val, 0.f );
	FINISH_PARAMETERS;

	var.m_value = val;
}
void IBehTreeObjectDefinition::funcSetValInt( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValInt, var, CBehTreeValInt() );
	GET_PARAMETER( Int32, val, 0 );
	FINISH_PARAMETERS;

	var.m_value = val;
}
void IBehTreeObjectDefinition::funcSetValString( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValString, var, CBehTreeValString() );
	GET_PARAMETER( String, val, String::EMPTY );
	FINISH_PARAMETERS;

	var.m_value = val;
}
void IBehTreeObjectDefinition::funcSetValCName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValCName, var, CBehTreeValCName() );
	GET_PARAMETER( CName, val, CName::NONE );
	FINISH_PARAMETERS;

	var.m_value = val;
}
void IBehTreeObjectDefinition::funcSetValBool( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( CBehTreeValBool, var, CBehTreeValBool() );
	GET_PARAMETER( Bool, val, false );
	FINISH_PARAMETERS;

	var.m_value = val;
}



////////////////////////////////////////////////////////////////////////
// IBehTreeTaskDefinition
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeTaskDefinition );

Bool IBehTreeTaskDefinition::IsPropertyTranslated( CProperty* property ) const
{
	if ( property->GetName() == CNAME( listenToGameplayEvents ) 
		|| property->GetName() == CNAME( listenToAnimEvents ) )
	{
		return false;
	}
	return IBehTreeObjectDefinition::IsPropertyTranslated( property );
}

void IBehTreeTaskDefinition::Initialize()
{
	CallFunction( const_cast<IBehTreeTaskDefinition*>(this), CNAME( Initialize ) );
}
void IBehTreeTaskDefinition::GetTaskNameStatic( CClass* classId, String& outName )
{
	static const Int32 PREXIES_COUNT = 4;
	static const Int32 SUFIXES_COUNT = 2;
	static const String PREFIXES[ PREXIES_COUNT ] =
	{
		TXT("CBehTree"),
		TXT("IBehTree"),
		TXT("CBT"),
		TXT("BT")
	};
	static const String SUFIXES[ SUFIXES_COUNT ] =
	{
		TXT("Def"),
		TXT("Definition")
	};
	String name = classId->GetName().AsString();
	for( Int32 i = 0; i < PREXIES_COUNT; ++i )
	{
		if ( name.LeftString( PREFIXES[i].GetLength() ) == PREFIXES[ i ] )
		{
			name = name.MidString( PREFIXES[i].GetLength() );
			break;
		}
	}
	for( Int32 i = 0; i < SUFIXES_COUNT; ++i )
	{
		if ( name.RightString( SUFIXES[i].GetLength() ) == SUFIXES[ i ] )
		{
			name = name.LeftString( name.GetLength() - SUFIXES[i].GetLength() );
			break;
		}
	}
	outName = name;
}
void IBehTreeTaskDefinition::GetTaskName( String& outName ) const
{
	GetTaskNameStatic( GetClass(), outName );
}


THandle< IBehTreeTask > IBehTreeTaskDefinition::SpawnInstance( CBehTreeSpawnContext& context, IBehTreeNodeInstance* instance, CBehTreeScriptedInstance* scriptInstance ) const
{
	THandle< IScriptable > obj = SpawnInstanceBase( context );
	THandle< IBehTreeTask > taskHandle = Cast< IBehTreeTask >( obj );
	if ( !taskHandle )
	{
		return nullptr;
	}

	for ( auto it = m_listenToGameplayEvents.Begin(), end = m_listenToGameplayEvents.End(); it != end; ++it )
	{
		SBehTreeEvenListeningData e; 
		e.m_eventName = *it;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;

		context.AddEventListener( e, instance );
		scriptInstance->ScriptOnEventListenerAdded( e );
	}

	for ( auto it = m_listenToAnimEvents.Begin(), end = m_listenToAnimEvents.End(); it != end; ++it )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = *it;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_ANIM;

		context.AddEventListener( e, instance );
		scriptInstance->ScriptOnEventListenerAdded( e );
	}

	return taskHandle;
}

void IBehTreeTaskDefinition::OnSpawn( CBehTreeSpawnContext& context, CBehTreeScriptedInstance* scriptedInstance, THandle< IBehTreeTask >& task ) const
{
	m_context = &context;
	m_nodeScriptInstance = scriptedInstance;
	m_nodeInstance = scriptedInstance->ScriptGetNode();
	IScriptable* funContext = const_cast< IBehTreeTaskDefinition* >( this );
	CallFunction( funContext, CNAME( OnSpawn ), task );
	m_nodeInstance = nullptr;
	m_nodeScriptInstance = nullptr;
	m_context = nullptr;
}

void IBehTreeTaskDefinition::OnSerialize( IFile &file )
{
	IBehTreeObjectDefinition::OnSerialize( file );
}

void IBehTreeTaskDefinition::funcListenToAnimEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_nodeInstance && m_context )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_ANIM;

		m_context->AddEventListener( e, m_nodeInstance );
		m_nodeScriptInstance->ScriptOnEventListenerAdded( e );
	}
}
void IBehTreeTaskDefinition::funcListenToGameplayEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_nodeInstance && m_context )
	{
		SBehTreeEvenListeningData e;
		e.m_eventName = eventName;
		e.m_eventType = SBehTreeEvenListeningData::TYPE_GAMEPLAY;

		m_context->AddEventListener( e, m_nodeInstance );
		m_nodeScriptInstance->ScriptOnEventListenerAdded( e );
	}
}


//void IBehTreeTaskDefinition::funcGetDefaultPriority( CScriptStackFrame& stack, void* result )
//{
//	FINISH_PARAMETERS;
//	Int32 priority = -1;
//	if ( m_nodeInstance )
//	{
//		priority = m_nodeInstance->GetDefaultPriority();
//	}
//	RETURN_INT( priority );
//}


////////////////////////////////////////////////////////////////////////
// IBehTreeConditionTask
////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IBehTreeConditionalTaskDefinition );
