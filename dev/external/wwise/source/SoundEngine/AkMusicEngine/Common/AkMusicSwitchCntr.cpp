/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

//////////////////////////////////////////////////////////////////////
//
// AkMusicRanSeqCntr.cpp
//
// Music Random/Sequence container definition.
//
//////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "AkMusicSwitchCntr.h"
#include "AkMusicSwitchCtx.h"
#include "AkCritical.h"
#include "AkPBI.h"
#include "AkMusicRenderer.h"
#include <AK/SoundEngine/Common/AkSoundEngine.h>

CAkMusicSwitchCntr::CAkMusicSwitchCntr( AkUniqueID in_ulID )
:CAkMusicTransAware( in_ulID )
,m_bIsContinuePlayback( true )
,m_pArguments(NULL)
,m_pGroupTypes(NULL)
{
}

CAkMusicSwitchCntr::~CAkMusicSwitchCntr()
{
    Term();
}

// Thread safe version of the constructor.
CAkMusicSwitchCntr * CAkMusicSwitchCntr::Create(
    AkUniqueID in_ulID
    )
{
    CAkFunctionCritical SpaceSetAsCritical;
	CAkMusicSwitchCntr * pSwitchCntr = AkNew( g_DefaultPoolId, CAkMusicSwitchCntr( in_ulID ) );
    if( pSwitchCntr )
	{
		if( pSwitchCntr->Init() != AK_Success )
		{
			pSwitchCntr->Release();
			pSwitchCntr = NULL;
		}
	}
    return pSwitchCntr;
}

AKRESULT CAkMusicSwitchCntr::SetInitialValues( AkUInt8* in_pData, AkUInt32 in_ulDataSize )
{	
	AKRESULT eResult = SetMusicTransNodeParams( in_pData, in_ulDataSize, false );
	if ( eResult != AK_Success )
		return eResult;

	m_bIsContinuePlayback = (READBANKDATA( AkUInt8, in_pData, in_ulDataSize) != 0);

	AkUInt32 uTreeDepth = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	AkUInt32 uArgumentsSize = uTreeDepth * ( sizeof( AkUniqueID ) );
	AkUInt32 uArgumentsGroupTypesSize = uTreeDepth * ( sizeof( AkUInt8 ) );

	eResult = SetArguments((AkUniqueID*)in_pData, (AkUInt8*)in_pData+uArgumentsSize, uTreeDepth);
	if ( eResult != AK_Success )
		return eResult;

	SKIPBANKBYTES( uArgumentsSize+uArgumentsGroupTypesSize, in_pData, in_ulDataSize );

	// Read decision tree

	AkUInt32 uTreeDataSize = READBANKDATA(AkUInt32, in_pData, in_ulDataSize);

	m_decisionTree.SetMode( READBANKDATA(AkUInt8, in_pData, in_ulDataSize) );

	eResult = m_decisionTree.SetTree( in_pData, uTreeDataSize, uTreeDepth );
	if ( eResult != AK_Success )
		return eResult;

	SKIPBANKBYTES( uTreeDataSize, in_pData, in_ulDataSize );

	CHECKBANKDATASIZE( in_ulDataSize, eResult );

	return eResult;
}

void CAkMusicSwitchCntr::SetAkProp( AkPropID in_eProp, AkReal32 in_fValue, AkReal32 in_fMin, AkReal32 in_fMax )
{
	CAkParameterNode::SetAkProp(in_eProp, in_fValue, in_fMin, in_fMax);
}

void CAkMusicSwitchCntr::SetAkProp( AkPropID in_eProp, AkInt32 in_iValue, AkInt32 in_iMin, AkInt32 in_iMax )
{
	if ( in_eProp == AkPropID_DialogueMode )
		m_decisionTree.SetMode( (AkUInt8) in_iValue );
	else
		CAkParameterNode::SetAkProp(in_eProp, in_iValue, in_iMin, in_iMax);
}

AKRESULT CAkMusicSwitchCntr::SetDecisionTree( void* in_pData, AkUInt32 in_uSize, AkUInt32 in_uDepth )
{
	return m_decisionTree.SetTree( in_pData, in_uSize, in_uDepth );
}

AKRESULT CAkMusicSwitchCntr::SetArguments( AkUniqueID* in_pArgs, AkUInt8* in_pGroupTypes, AkUInt32 in_uNumArgs )
{
	AKRESULT res = AK_Success;

	ReleaseArgments();

	AKASSERT(m_pArguments==NULL);
	AkUInt32 argsSize = in_uNumArgs*sizeof(AkUniqueID);
	m_pArguments = (AkUniqueID *) AkAlloc( g_DefaultPoolId, argsSize );
	if ( m_pArguments )
	{
		memcpy( m_pArguments, in_pArgs, argsSize );

		AKASSERT(m_pGroupTypes==NULL);
		AkUInt32 grouspTypesSize = in_uNumArgs*sizeof(AkUInt8);
		m_pGroupTypes = (AkUInt8*) AkAlloc( g_DefaultPoolId, grouspTypesSize );
		if ( m_pGroupTypes )
		{
			memcpy( m_pGroupTypes, in_pGroupTypes, grouspTypesSize );
		}
		else
		{
			res = AK_InsufficientMemory;
		}
	}
	else
	{
		if ( in_uNumArgs > 0 )
		res = AK_InsufficientMemory;
	}

	return res;
}

void CAkMusicSwitchCntr::ReleaseArgments()
{
	if (m_pArguments)
	{
		AkFree( g_DefaultPoolId, m_pArguments );
		m_pArguments = NULL;
	}
	if (m_pGroupTypes)
	{
		AkFree( g_DefaultPoolId, m_pGroupTypes );
		m_pGroupTypes = NULL;
	}
}

void CAkMusicSwitchCntr::Term()
{
	ReleaseArgments();
}

// Return the node category.
AkNodeCategory CAkMusicSwitchCntr::NodeCategory()
{
    return AkNodeCategory_MusicSwitchCntr;
}

// Override MusicObject::ExecuteAction() to catch Seek actions.
AKRESULT CAkMusicSwitchCntr::ExecuteAction( ActionParams& in_rAction )
{
    if ( ActionParamType_Seek == in_rAction.eType )
	{
		// No need to propagate this action on children; only segments can handle this.
		SeekActionParams & rSeekActionParams = (SeekActionParams&)in_rAction;
		if ( rSeekActionParams.bIsSeekRelativeToDuration )
		{
			CAkMusicRenderer::Get()->SeekPercent( this, rSeekActionParams.pGameObj, rSeekActionParams.fSeekPercent, rSeekActionParams.bSnapToNearestMarker );
		}
		else
		{
			CAkMusicRenderer::Get()->SeekTimeAbsolute( this, rSeekActionParams.pGameObj, rSeekActionParams.iSeekTime, rSeekActionParams.bSnapToNearestMarker );
		}
		return AK_Success;
	}

	return CAkMusicNode::ExecuteAction( in_rAction );
}

// Hierarchy enforcement: Music RanSeq Cntr can only have Segments as parents.
AKRESULT CAkMusicSwitchCntr::CanAddChild(
    CAkParameterNodeBase * in_pAudioNode 
    )
{
    AKASSERT( in_pAudioNode );

	AkNodeCategory eCategory = in_pAudioNode->NodeCategory();

	AKRESULT eResult = AK_Success;	
	if(Children() >= AK_MAX_NUM_CHILD)
	{
		eResult = AK_MaxReached;
	}
	else if(eCategory != AkNodeCategory_MusicSegment &&
            eCategory != AkNodeCategory_MusicRanSeqCntr &&
            eCategory != AkNodeCategory_MusicSwitchCntr )
	{
		eResult = AK_NotCompatible;
	}
	else if(in_pAudioNode->Parent() != NULL)
	{
		eResult = AK_ChildAlreadyHasAParent;
	}
	else if(m_mapChildId.Exists(in_pAudioNode->ID()))
	{
		eResult = AK_AlreadyConnected;
	}
	else if(ID() == in_pAudioNode->ID())
	{
		eResult = AK_CannotAddItseflAsAChild;
	}
	return eResult;	
}

CAkMatrixAwareCtx * CAkMusicSwitchCntr::CreateContext( 
    CAkMatrixAwareCtx * in_pParentCtx,
    CAkRegisteredObj * in_GameObject,
    UserParams &  in_rUserparams
    )
{
    CAkMusicSwitchCtx * pSwitchCntrCtx = AkNew( g_DefaultPoolId, CAkMusicSwitchCtx( 
        this,
        in_pParentCtx ) );
    if ( pSwitchCntrCtx )
    {
		pSwitchCntrCtx->AddRef();
        if ( pSwitchCntrCtx->Init( in_GameObject, 
                                   in_rUserparams
                                    ) == AK_Success )
		{
			pSwitchCntrCtx->Release();
		}
		else
        {
            pSwitchCntrCtx->_Cancel();
			pSwitchCntrCtx->Release();
            pSwitchCntrCtx = NULL;
        }
    }
    return pSwitchCntrCtx;

}

AKRESULT CAkMusicSwitchCntr::PlayInternal( AkPBIParams& in_rPBIParams )
{
    // Create a top-level switch container context (that is, attached to the Music Renderer).
    // OPTIM. Could avoid virtual call.
    CAkMatrixAwareCtx * pCtx = CreateContext( NULL, in_rPBIParams.pGameObj, in_rPBIParams.userParams );
    if ( pCtx )
    {
		// Complete initialization of the context.
		pCtx->EndInit();

        // Do not set source offset: let it start playback at the position specified by the descendant's 
        // transition rules.
        
        AkMusicFade fadeParams;
        fadeParams.transitionTime   = in_rPBIParams.pTransitionParameters->TransitionTime;
		fadeParams.eFadeCurve       = in_rPBIParams.pTransitionParameters->eFadeCurve;
		// Set fade offset to context's silent duration.
        fadeParams.iFadeOffset		= pCtx->GetSilentDuration();
        pCtx->_Play( fadeParams );
		return AK_Success;
    }
    return AK_Fail;
}

//NOTE: Game sync preparation is only supported for single-argument switch containers.
AKRESULT CAkMusicSwitchCntr::ModifyActiveState( AkUInt32 in_stateID, bool in_bSupported )
{
	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount != 0 && GetTreeDepth() == 1)
	{
		AkUniqueID audioNode = m_decisionTree.GetAudioNodeForState(in_stateID);
		if( audioNode != AK_INVALID_UNIQUE_ID )
		{
			if( in_bSupported )
			{
				eResult = PrepareNodeData( audioNode );
			}
			else
			{
				UnPrepareNodeData( audioNode );
			}
		}
	}

	return eResult;
}

//NOTE: Game sync preparation is only supported for single-argument switch containers.
AKRESULT CAkMusicSwitchCntr::PrepareData()
{
 	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation || GetTreeDepth() != 1)
	{
		return CAkMusicNode::PrepareData();
	}

 	AKRESULT eResult = AK_Success;

	if( m_uPreparationCount == 0 )
	{
		eResult = PrepareMusicalDependencies();
		if( eResult == AK_Success )
		{
			AkUInt32 uGroupID = GetSwitchGroup(0);
			AkGroupType eGroupType = GetSwitchGroupType(0);

			CAkPreparedContent* pPreparedContent = GetPreparedContent( uGroupID, eGroupType );
			if( pPreparedContent )
			{
				AkDecisionTree::SwitchNodeAssoc switchNodeAssoc;
				m_decisionTree.GetSwitchNodeAssoc(switchNodeAssoc);

				for( AkDecisionTree::SwitchNodeAssoc::Iterator iter = switchNodeAssoc.Begin(); iter != switchNodeAssoc.End(); ++iter )
				{
					//Always include the fallback argument path.
					if( iter.pItem->key == 0 || pPreparedContent->IsIncluded( iter.pItem->key ) )
					{
						eResult = PrepareNodeData( iter.pItem->item );
					}
					if( eResult != AK_Success )
					{
						// Do not let this half-prepared, unprepare what has been prepared up to now.
						for( AkDecisionTree::SwitchNodeAssoc::Iterator iterFlush = switchNodeAssoc.Begin(); iterFlush != iter; ++iterFlush )
						{
							if( pPreparedContent->IsIncluded( iterFlush.pItem->key ) )
							{
								UnPrepareNodeData( iter.pItem->item );
							}
						}
					}
				}

				switchNodeAssoc.Term();

				if( eResult == AK_Success )
				{
					++m_uPreparationCount;
					eResult = SubscribePrepare( uGroupID, eGroupType );
					if( eResult != AK_Success )
						UnPrepareData();
				}
			}
			else
			{
				eResult = AK_InsufficientMemory;
			}
			if( eResult != AK_Success )
			{
				UnPrepareMusicalDependencies();
			}
		}
	}
	else
	{
		++m_uPreparationCount;
	}
	return eResult;
}

//NOTE: Game sync preparation is only supported for single-argument switch containers.
void CAkMusicSwitchCntr::UnPrepareData()
{
	extern AkInitSettings g_settings;
	if( !g_settings.bEnableGameSyncPreparation || GetTreeDepth() != 1)
	{
		return CAkMusicNode::UnPrepareData();
	}

	if( m_uPreparationCount != 0 ) // must check in case there were more unprepare than prepare called that succeeded.
	{
		if( --m_uPreparationCount == 0 )
		{
			AkUInt32 uGroupID = GetSwitchGroup(0);
			AkGroupType eGroupType = GetSwitchGroupType(0);

			CAkPreparedContent* pPreparedContent = GetPreparedContent( uGroupID, eGroupType );
			if( pPreparedContent )
			{
				AkDecisionTree::SwitchNodeAssoc switchNodeAssoc;
				m_decisionTree.GetSwitchNodeAssoc(switchNodeAssoc);

				for( AkDecisionTree::SwitchNodeAssoc::Iterator iter = switchNodeAssoc.Begin(); iter != switchNodeAssoc.End(); ++iter )
				{
					if( iter.pItem->key == 0 || pPreparedContent->IsIncluded( iter.pItem->key ) )
					{
						UnPrepareNodeData( iter.pItem->item );
					}
				}
				switchNodeAssoc.Term();
			}
			CAkPreparationAware::UnsubscribePrepare( uGroupID, eGroupType );
			UnPrepareMusicalDependencies();
		}
	}
}


