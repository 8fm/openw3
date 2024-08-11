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
// AkSIS.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _SIS_H_
#define _SIS_H_

#include <AK/Tools/Common/AkObject.h>
#include "ITransitionable.h"
#include "AkPropBundle.h"

class CAkParameterNodeBase;
class CAkTransition;
class CAkRegisteredObj;

struct AkSISValue
{
	AkReal32 fValue;
	CAkTransition* pTransition;
};

typedef AkPropBundle<AkSISValue> AkSISValues;

// This class contains the Specific Information
class CAkSIS : public ITransitionable
{

public:
	inline CAkSIS( CAkParameterNodeBase* in_Parent, AkUInt8 in_bitsFXBypass, CAkRegisteredObj * in_pGameObj = NULL )
		: m_pParamNode( in_Parent )
		, m_pGameObj( in_pGameObj )
		, m_bitsFXBypass( in_bitsFXBypass )
	{
	}

	~CAkSIS();

	CAkParameterNodeBase*	m_pParamNode;
	CAkRegisteredObj*		m_pGameObj;
	AkSISValues				m_values;
	AkUInt8					m_bitsFXBypass; // 0-3 is effect-specific, 4 is bypass all

	virtual void TransUpdateValue(
		AkIntPtr in_eTarget,
		AkReal32 in_fValue,
		bool in_bIsTerminated
		);
};
#endif
