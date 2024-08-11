#include "build.h"
#include "boidSpecies.h"
#include "definitionsManager.h"
#include "boidLairEntity.h"
#include "baseBoidNode.h"
#include "pointOfInterestSpeciesConfig.h"
#include "swarmSound.h"


void CBoidSpecies::AddParams(CBoidLairParams *const params)
{
	m_boidLairParamsArray.PushBack(params);
}

CBoidSpecies::CBoidSpecies()
{

}
CBoidSpecies::~CBoidSpecies()
{
	CBoidLairParamsArray::const_iterator end	= m_boidLairParamsArray.End();
	CBoidLairParamsArray::const_iterator it;
	for ( it = m_boidLairParamsArray.Begin();  it !=  end; ++ it )
	{
		const CBoidLairParams *const params = *it;
		delete params;
	}

	for (Uint32 i = 0; i < m_baseBoidNodeArray.Size(); ++i)
	{
		delete m_baseBoidNodeArray[ i ];
	}

	for ( Uint32 i = 0; i < m_pointOfInterestConfigArray.Size(); ++i )
	{
		delete m_pointOfInterestConfigArray[ i ];
	}

	for ( Uint32 i = 0; i < m_soundConfigArray.Size(); ++i )
	{
		delete m_soundConfigArray[ i ];
	}
}


const CBoidLairParams *const CBoidSpecies::GetParamsByName( CName name )const
{
	CBoidLairParamsArray::const_iterator end	= m_boidLairParamsArray.End();
	CBoidLairParamsArray::const_iterator it;
	for (it = m_boidLairParamsArray.Begin();  it !=  end; ++ it )
	{
		const CBoidLairParams *const params = *it;
		if (params->m_name == name)
		{
			if ( params->m_boidTemplateHandle.GetPath().Empty()  )
			{
				RED_LOG_ERROR( RED_LOG_CHANNEL( SwarmAI ), TXT("boid species missing boidTemplate !") );
				return NULL;
			}
			return params;
		}
	}
	return NULL;
}

void CBoidSpecies::PreloadTemplates()
{
	auto itParams	= m_boidLairParamsArray.Begin();
	auto endParams	= m_boidLairParamsArray.End();
	while ( itParams != endParams )
	{
		CBoidLairParams * const boidLairParams		= *itParams++;
		CEntityTemplate *const boidEntityTemplate	= boidLairParams->m_boidTemplateHandle.Get();
		if ( boidEntityTemplate )
		{
			boidEntityTemplate->AddToRootSet();
		}
	}
}

void CBoidSpecies::UnloadTemplates()
{
	auto itParams	= m_boidLairParamsArray.Begin();
	auto endParams	= m_boidLairParamsArray.End();
	while ( itParams != endParams )
	{
		CBoidLairParams * const boidLairParams		= *itParams++;
		CEntityTemplate *const boidEntityTemplate	= boidLairParams->m_boidTemplateHandle.Get();
		if ( boidEntityTemplate )
		{
			boidEntityTemplate->RemoveFromRootSet();
		}
		boidLairParams->m_boidTemplateHandle.Release();
	}
}

void CBoidSpecies::InitParams(CDefinitionsManager *const definitionManager)
{
	Bool errorInXML = true;
	const TDynArray< SCustomNode >& nodes						= definitionManager->GetCustomNodes();
	const TDynArray< SCustomNode >::const_iterator customEnd	= nodes.End();
	TDynArray< SCustomNode >::const_iterator  customIt;
	for(  customIt = nodes.Begin();  customIt !=  customEnd; ++ customIt )
	{
		const SCustomNode & defNode = * customIt;
		if ( defNode.m_nodeName != CNAME( boidSpeciesDef ) )
		{
			continue;
		}

		ParseSpecies(defNode);		
		errorInXML = false;
	}
	if (errorInXML)
	{
		ERR_GAME(TXT("Error: no species declared in boid_species.xml, all swarm creatures will be disabled !"));
	}
}
void CBoidSpecies::ParseSpecies(const SCustomNode & defNode, const CBoidLairParams *const parentParams)
{
	if ( defNode.m_attributes.Size() == 0 )
	{
		return;
	}

	CName typeName;

	const TDynArray< SCustomNodeAttribute >::const_iterator attEnd	= defNode.m_attributes.End();
	TDynArray< SCustomNodeAttribute >::const_iterator		attIt;
	for (attIt = defNode.m_attributes.Begin();  attIt !=  attEnd; ++ attIt )
	{
		const SCustomNodeAttribute & att	= *attIt;
		if ( att.m_attributeName == CNAME( type_name ) )
		{
			typeName			= att.m_attributeValueAsCName;
		}
	}
	if (typeName == CName::NONE)
	{
		typeName = parentParams->m_typeName;
	}
	CBoidLairParams *const params = NewBoidSpeciesParams( typeName ); 
	if (NULL == params)
	{
		return;
	}
	// copying parent class params if it exists
	// this enables species to inherit from each other :
	if (parentParams)
	{
		if (parentParams->CopyTo(params) == false)
		{
			delete params;
			return;
		}
	}

	if ( false == params->ParseXML( defNode, this ) )
	{
		delete params;
		return;
	}
	m_boidLairParamsArray.PushBack(params);

	const TDynArray< SCustomNode >::const_iterator subSpeciesEnd	= defNode.m_subNodes.End();
	TDynArray< SCustomNode >::const_iterator  subSpeciesIt;
	for(  subSpeciesIt = defNode.m_subNodes.Begin();  subSpeciesIt !=  subSpeciesEnd; ++ subSpeciesIt )
	{
		const SCustomNode & subSpeciesNode = *subSpeciesIt;
		if (subSpeciesNode.m_nodeName == CNAME( subSpecies ))
		{
			const TDynArray< SCustomNode >::const_iterator subDefEnd	= subSpeciesNode.m_subNodes.End();
			TDynArray< SCustomNode >::const_iterator  subDefIt;
			for(  subDefIt = subSpeciesNode.m_subNodes.Begin();  subDefIt !=  subDefEnd; ++ subDefIt )
			{
				const SCustomNode & subdefNode = *subDefIt;
				if (subdefNode.m_nodeName == CNAME( boidSpeciesDef ))
				{
					ParseSpecies(subdefNode, params);
				}
			}
		}
	}
}

void CBoidSpecies::GetBoidTemplatePaths(TDynArray<String>& output)
{
	auto itParams	= m_boidLairParamsArray.Begin();
	auto endParams	= m_boidLairParamsArray.End();
	while ( itParams != endParams )
	{
		CBoidLairParams * const boidLairParams		= *itParams++;
		output.PushBackUnique( boidLairParams->m_boidTemplateHandle.GetPath() );
	}
}

