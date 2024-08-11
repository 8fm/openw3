#include "build.h"

#include "behTreeScriptedNode.h"

#include "behTreeInstance.h"
#include "behTreeLog.h"
#include "behTreeVars.h"

#include "baseDamage.h"
#include "aiLog.h"

RED_DISABLE_WARNING_MSC(4355)


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeScriptTerminalDefinition
////////////////////////////////////////////////////////////////////////
const String CBehTreeNodeScriptTerminalDefinition::NULL_TASK_NODE_NAME( TXT("Terminal: EMPTY SCRIPT") );
const String CBehTreeNodeScriptTerminalDefinition::CAPTION_PREFIX( TXT("Terminal: ") );

String CBehTreeNodeScriptTerminalDefinition::GetNodeCaption() const
{
	if( m_taskOrigin )
	{
		static String str;			
		m_taskOrigin->GetTaskName( str );
		return CAPTION_PREFIX + str;
	}
	else
		return NULL_TASK_NODE_NAME;
}
Bool CBehTreeNodeScriptTerminalDefinition::IsValid() const
{
	if ( !m_taskOrigin )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
CBehTreeNodeScriptTerminalDefinition::eEditorNodeType CBehTreeNodeScriptTerminalDefinition::GetEditorNodeType() const
{
	return NODETYPE_SCRIPTED;
}
IBehTreeTaskDefinition* CBehTreeNodeScriptTerminalDefinition::GetTask() const
{
	return m_taskOrigin;
}
Bool CBehTreeNodeScriptTerminalDefinition::SetTask( IBehTreeTaskDefinition* task )
{
	m_taskOrigin = task;
	if ( task )
	{
		task->Initialize();
	}
	
	return true;
}
Bool CBehTreeNodeScriptTerminalDefinition::IsSupportingTasks() const
{
	return true;
}
IBehTreeNodeInstance* CBehTreeNodeScriptTerminalDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeScriptTerminalInstance
////////////////////////////////////////////////////////////////////////
CBehTreeNodeScriptTerminalInstance::CBehTreeNodeScriptTerminalInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, CBehTreeScriptedInstance( context, def.m_taskOrigin, this, def.m_taskFlags, def.m_runMainOnActivation )
	, m_skipIfActive( def.m_skipIfActive )
{
}
CBehTreeNodeScriptTerminalInstance::~CBehTreeNodeScriptTerminalInstance()
{

}

void CBehTreeNodeScriptTerminalInstance::Update()
{
	eUpdateResult result = ScriptUpdate();
	if ( result != U_CONTINUE )
	{
		Complete( eTaskOutcome( result ) );
	}
}
Bool CBehTreeNodeScriptTerminalInstance::Activate()
{
	if ( ScriptActivate() )
	{
		return Super::Activate();
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeScriptTerminalInstance::Deactivate()
{
	ScriptDeactivate();
	Super::Deactivate();
}
void CBehTreeNodeScriptTerminalInstance::Complete( eTaskOutcome outcome )
{
	ScriptOnCompletion( outcome );

	Super::Complete( outcome );
}

Bool CBehTreeNodeScriptTerminalInstance::IsAvailable()
{
	if ( m_skipIfActive && m_isActive )
	{
		return true;
	}

	if( ScriptIsAvailable() )
	{
		return true;
	}

	DebugNotifyAvailableFail();
	return false;
}

Int32 CBehTreeNodeScriptTerminalInstance::Evaluate()
{
	if ( m_isActive && m_skipIfActive && (m_taskFlags & IBehTreeTask::FLAG_HAS_EVALUATE) == 0 )
	{
		if( m_priority <= 0 )
		{
			DebugNotifyAvailableFail();
		}

		return m_priority;
	}

	Int32 tmp = ScriptEvaluate( m_priority );
	if( tmp <= 0 )
	{
		DebugNotifyAvailableFail();
	}

	return tmp;
}

void CBehTreeNodeScriptTerminalInstance::OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context )
{
	ScriptInitialize( static_cast< const Definition& >( def ).GetTask(), context );
}

void CBehTreeNodeScriptTerminalInstance::OnDestruction()
{
	ScriptOnDestruction();
	Super::OnDestruction();
}
Bool CBehTreeNodeScriptTerminalInstance::OnEvent( CBehTreeEvent& e )
{
	return ScriptOnEvent( e );
}
Bool CBehTreeNodeScriptTerminalInstance::OnListenedEvent( CBehTreeEvent& e )
{
	return ScriptOnListenedEvent( e );
}
CBehTreeInstance* CBehTreeNodeScriptTerminalInstance::ScriptGetOwner() const
{
	return m_owner;
}
IBehTreeNodeInstance* CBehTreeNodeScriptTerminalInstance::ScriptGetNode()
{
	return this;
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeScriptDecoratorDefinition
////////////////////////////////////////////////////////////////////////
const String CBehTreeNodeScriptDecoratorDefinition::NULL_TASK_NODE_NAME( TXT("Decorator: EMPTY SCRIPT") );
const String CBehTreeNodeScriptDecoratorDefinition::CAPTION_PREFIX( TXT("Decorator: ") );

String CBehTreeNodeScriptDecoratorDefinition::GetNodeCaption() const
{
	if( m_taskOrigin )
	{
		static String str;			
		m_taskOrigin->GetTaskName( str );
		return CAPTION_PREFIX + str;
	}
	else
	{
		return NULL_TASK_NODE_NAME;
	}
}
Bool CBehTreeNodeScriptDecoratorDefinition::IsValid() const
{
	if ( !m_taskOrigin )
	{
		return false;
	}
	return TBaseClass::IsValid();
}
CBehTreeNodeScriptDecoratorDefinition::eEditorNodeType CBehTreeNodeScriptDecoratorDefinition::GetEditorNodeType() const
{
	return NODETYPE_SCRIPTED;
}
IBehTreeTaskDefinition* CBehTreeNodeScriptDecoratorDefinition::GetTask() const
{
	return m_taskOrigin;
}
Bool CBehTreeNodeScriptDecoratorDefinition::SetTask( IBehTreeTaskDefinition* task )
{
	m_taskOrigin = task;
	if ( task )
	{
		task->Initialize();
	}
	return true;
}
Bool CBehTreeNodeScriptDecoratorDefinition::IsSupportingTasks() const
{
	return true;
}
IBehTreeNodeDecoratorInstance* CBehTreeNodeScriptDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeScriptDecoratorInstance
////////////////////////////////////////////////////////////////////////

CBehTreeNodeScriptDecoratorInstance::CBehTreeNodeScriptDecoratorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeDecoratorInstance( def, owner, context, parent )
	, CBehTreeScriptedInstance( context, def.m_taskOrigin, this, def.m_taskFlags, def.m_runMainOnActivation )
	, m_forwardAvailability( def.m_forwardAvailability )
	, m_forwardTestIfNotAvailable( def.m_forwardTestIfNotAvailable )
	, m_invertAvailability( def.m_invertAvailability )
	, m_skipIfActive ( def.m_skipIfActive )
{
}
CBehTreeNodeScriptDecoratorInstance::~CBehTreeNodeScriptDecoratorInstance()
{
}
void CBehTreeNodeScriptDecoratorInstance::Update()
{
	eUpdateResult result = ScriptUpdate();
	if ( result != U_CONTINUE )
	{
		Complete( eTaskOutcome( result ) );
	}
	else
	{
		Super::Update();
	}
}
Bool CBehTreeNodeScriptDecoratorInstance::Activate()
{
	if ( !ScriptActivate() )
	{
		DebugNotifyActivationFail();
		return false;
	}

	if ( !Super::Activate() )
	{
		// child node problem during activation - script must also get notice on deactivation
		ScriptDeactivate();
		DebugNotifyActivationFail();
		return false;
	}

	return true;
}
void CBehTreeNodeScriptDecoratorInstance::Deactivate()
{
	ScriptDeactivate();

	Super::Deactivate();
}
void CBehTreeNodeScriptDecoratorInstance::Complete( eTaskOutcome outcome )
{
	ScriptOnCompletion( outcome );

	Super::Complete( outcome );
}
Bool CBehTreeNodeScriptDecoratorInstance::OnEvent( CBehTreeEvent& e )
{
	Bool ret = ScriptOnEvent( e );
	return m_child->OnEvent( e ) || ret;
}
Bool CBehTreeNodeScriptDecoratorInstance::OnListenedEvent( CBehTreeEvent& e )
{
	return ScriptOnListenedEvent( e );
}
Bool CBehTreeNodeScriptDecoratorInstance::IsAvailable()
{
	if ( m_taskFlags & (IBehTreeTask::FLAG_HAS_IS_AVAILABLE | IBehTreeTask::FLAG_HAS_EVALUATE) )
	{
		if ( m_taskFlags & IBehTreeTask::FLAG_HAS_IS_AVAILABLE )
		{
			Bool available;
			if ( m_skipIfActive && m_isActive )
			{
				available = true; 
			}
			else
			{
				available = ScriptIsAvailable();
				if ( m_invertAvailability )
					available = !available;
			}

			if ( available )
			{
				if ( m_forwardAvailability )
				{
					return Super::IsAvailable();
				}
				return true;
			}
			else
			{
				if ( m_forwardTestIfNotAvailable )
				{
					return Super::IsAvailable();
				}

				DebugNotifyAvailableFail();
				return false;
			}
		}
		// m_taskFlags & IBehTreeTask::FLAG_HAS_EVALUATE
		if ( m_skipIfActive && m_isActive )
		{
			return true;
		}

		if( ScriptEvaluate( m_priority ) > 0 )
		{
			return true;
		}
		else
		{
			DebugNotifyAvailableFail();
			return false;
		}
	}

	return Super::IsAvailable();
}

Int32 CBehTreeNodeScriptDecoratorInstance::Evaluate()
{
	if ( m_taskFlags & (IBehTreeTask::FLAG_HAS_IS_AVAILABLE | IBehTreeTask::FLAG_HAS_EVALUATE) )
	{
		if ( m_taskFlags & IBehTreeTask::FLAG_HAS_EVALUATE )
		{
			Int32 tmp = ScriptEvaluate( m_priority );
			if( tmp <= 0 )
			{
				DebugNotifyAvailableFail();
			}

			return tmp;
		}

		// m_taskFlags & IBehTreeTask::FLAG_HAS_IS_AVAILABLE
		Bool available;
		if ( m_skipIfActive && m_isActive )
		{
			available = true;
		}
		else
		{
			available = ScriptIsAvailable();
			if ( m_invertAvailability )
			{
				available = !available;
			}
		}

		if ( !available )
		{
			if ( m_forwardTestIfNotAvailable )
			{
				return m_child->Evaluate();
			}

			DebugNotifyAvailableFail();
			return -1;
		}

		if ( !m_forwardAvailability )
		{
			if( m_priority <= 0 )
			{
				DebugNotifyAvailableFail();
			}

			return m_priority;
		}
	}

	return m_child->Evaluate();
}

void CBehTreeNodeScriptDecoratorInstance::OnSpawn( const IBehTreeNodeDefinition& def, CBehTreeSpawnContext& context )
{
	ScriptInitialize( static_cast< const Definition& >( def ).GetTask(), context );
}

void CBehTreeNodeScriptDecoratorInstance::OnDestruction()
{
	ScriptOnDestruction();
	Super::OnDestruction();
}

CBehTreeInstance* CBehTreeNodeScriptDecoratorInstance::ScriptGetOwner() const
{
	return m_owner;
}
IBehTreeNodeInstance* CBehTreeNodeScriptDecoratorInstance::ScriptGetNode()
{
	return this;
}

////////////////////////////////////////////////////////////////////////
// BehTree scripted conditional decorator
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeScriptConditionalDecoratorDefinition::IsSupportingTaskClass( const CClass* classId ) const
{
	return classId->IsA( IBehTreeConditionalTaskDefinition::GetStaticClass() );
}
