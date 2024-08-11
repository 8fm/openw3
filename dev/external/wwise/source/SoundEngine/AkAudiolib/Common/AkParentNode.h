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
// AkParentNode.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _AK_PARENT_NODE_H_
#define _AK_PARENT_NODE_H_

#include "AkKeyArray.h"
#include "AkParameterNode.h"
#include "AkAudioLibIndex.h"
#include <AK/Tools/Common/AkBankReadHelpers.h>

/// Get policy for AkSortedKeyArray of type AkMapChildID.
struct AkChildIDValueGetKey
{
	/// Default policy.
	static AkForceInline AkUniqueID& Get( CAkParameterNodeBase* & in_item ) 
	{
		return in_item->key;
	}
};

typedef AkSortedKeyArray<AkUniqueID, CAkParameterNodeBase*, ArrayPoolDefault, AkChildIDValueGetKey, 1> AkMapChildID;

template <class T> class CAkParentNode : public T
{
public:
	//Constructor
	CAkParentNode(AkUniqueID in_ulID)
	:T(in_ulID)
	{
	}

	//Destructor
	virtual ~CAkParentNode()
	{
		m_mapChildId.Term();
	}

	AKRESULT Init()
	{
		return T::Init();
	}

	// Get the total number of children
    //
    // Return - AkUInt16 - Number of inputs
    virtual AkUInt16 Children()
	{
		return static_cast<AkUInt16>( m_mapChildId.Length() );
	}

	// Check if the specified child can be connected
    //
    // Return - bool - True if possible
    virtual AKRESULT CanAddChild(
        CAkParameterNodeBase * in_pAudioNode // Audio node to connect on
        ) = 0;

    // Add a Child
    //
    // Return - AKRESULT - AK_NotCompatible : The output is not compatible with the Node
	//					 - AK_Success if everything succeed
    //                   - AK_MaxReached if the maximum of Input is reached
    virtual AKRESULT AddChildInternal( CAkParameterNodeBase* pAudioNode )
	{
		AKRESULT eResult = CanAddChild(pAudioNode);
		if(eResult == AK_Success)
		{
			CAkParameterNodeBase** ppNode = m_mapChildId.AddNoSetKey( pAudioNode->ID() );
			if( !ppNode )
			{
				eResult = AK_Fail;
			}
			else
			{
				*ppNode = pAudioNode;
				pAudioNode->Parent(this);
				this->AddRef();
			}
		}
		pAudioNode->Release();
		return eResult;
	}

	// Remove a child from the list
	//
    // Return - AKRESULT - AK_Success if all went right
    virtual void RemoveChild( CAkParameterNodeBase* in_pChild )
	{
		// IMPORTANT NOTICE :: CAkRanSeqCntr Has ist own version of RemoveChild() implemented, not calling this one
		// The reason for that is that the this->Release() must be the last line of the last function called on the
		// Object.
		AKASSERT( in_pChild );

		if( in_pChild->Parent() == this )
		{
			in_pChild->Parent( NULL );
			m_mapChildId.Unset( in_pChild->ID() );
			this->Release();
		}
	}

	virtual void RemoveChild( WwiseObjectIDext in_ulID )
	{
		AKASSERT(in_ulID.id);
		CAkParameterNodeBase** l_pANPtr = m_mapChildId.Exists(in_ulID.id);
		if( l_pANPtr )
		{
			RemoveChild(*l_pANPtr);
		}
	}

	AKRESULT SetChildren( AkUInt8*& io_rpData, AkUInt32& io_rulDataSize )
	{
		AKRESULT eResult = AK_Success;
		//Process Child list
		AkUInt32 ulNumChilds = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
		if( ulNumChilds )
		{
			eResult = m_mapChildId.Reserve( ulNumChilds );
			if( eResult != AK_Success )
				return eResult;
				
			for( AkUInt32 i = 0; i < ulNumChilds; ++i )
			{
				AkUInt32 ulChildID = READBANKDATA( AkUInt32, io_rpData, io_rulDataSize );
				WwiseObjectID wwiseId( ulChildID );
				eResult = this->AddChild( wwiseId );
				if( eResult != AK_Success )
				{
					break;
				}
			}
		}
		return eResult;
	}

	virtual void GetChildren( AkUInt32& io_ruNumItems, AkObjectInfo* out_aObjectInfos, AkUInt32& in_uIndex_Out, AkUInt32 in_uDepth )
	{
		AkMapChildID::Iterator iter = m_mapChildId.Begin();
		for( ; iter != m_mapChildId.End(); ++iter )
		{
			if( in_uIndex_Out < io_ruNumItems )
			{
				out_aObjectInfos[in_uIndex_Out].objID = AkChildIDValueGetKey::Get( *iter );
				out_aObjectInfos[in_uIndex_Out].parentID = (*iter)->Parent()->ID();
				out_aObjectInfos[in_uIndex_Out].iDepth = in_uDepth;
			}
			++in_uIndex_Out;

			if( in_uIndex_Out == io_ruNumItems )
				break; //exit loop

			(*iter)->GetChildren( io_ruNumItems, out_aObjectInfos, in_uIndex_Out, in_uDepth + 1 );

			if( in_uIndex_Out == io_ruNumItems )
				break; //exit loop
		}
	}

protected:
	AkMapChildID m_mapChildId; // List of nodes connected to this one

};

#endif
