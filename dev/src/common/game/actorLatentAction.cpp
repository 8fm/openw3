#include "build.h"
#include "actorLatentAction.h"

#include "aiActionParameters.h"
#include "behTree.h"
#include "behTreeNode.h"

#include "../../common/core/depot.h"

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IActorLatentAction );

IBehTreeNodeInstance* IActorLatentAction::ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
{
	return NULL;
}

THandle< IAIActionTree > IActorLatentAction::ConvertToActionTree( const THandle< IScriptable >& parentObj )
{
	static CName FUN( TXT("ConvertToActionTree") );

	THandle< IAIActionTree > ret;

	CallFunctionRet< THandle< IAIActionTree >, THandle< IScriptable > >( this, FUN, parentObj, ret );

	return ret;
}

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( CCustomBehTreeActorLatentAction );

CCustomBehTreeActorLatentAction::CCustomBehTreeActorLatentAction()
	: m_behTree( NULL )
{
}

IBehTreeNodeInstance* CCustomBehTreeActorLatentAction::ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
{
	if ( m_behTree )
	{
		return m_behTree->SpawnInstance( owner, context, parent );
	}
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
IMPLEMENT_ENGINE_CLASS( IPresetActorLatentAction );

IPresetActorLatentAction::IPresetActorLatentAction()
	: m_res( NULL )
	, m_def( NULL )
{

}

IBehTreeNodeInstance* IPresetActorLatentAction::ComputeAction( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
{
	if( !m_res )
	{
		if( !m_resName.Empty() )
		{
			CResource* res = GDepot->LoadResource( m_resName );
			m_res = Cast< CBehTree >( res );
			if( !m_res )
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}

	IBehTreeNodeDefinition* def = m_res->GetRootNode();
	if ( !def )
	{
		return NULL;
	}

	context.Push( this );
	IBehTreeNodeInstance* instance = def->SpawnInstance( owner, context, parent );
	context.Pop( this );
	return instance;
}
