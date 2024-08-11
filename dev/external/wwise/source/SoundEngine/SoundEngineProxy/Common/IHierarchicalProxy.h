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
#ifndef AK_OPTIMIZED


#pragma once

#include "IParameterNodeProxy.h"

class IHierarchicalProxy : virtual public IParameterNodeProxy
{
	DECLARE_BASECLASS( IParameterNodeProxy );
public:
	virtual void AddChild( WwiseObjectIDext in_id ) = 0;
	virtual void RemoveChild( WwiseObjectIDext in_id ) = 0;
	virtual void RemoveAllChildren() = 0;

	enum MethodIDs
	{
		MethodAddChild = __base::LastMethodID,
		MethodRemoveChild,
		MethodRemoveAllChildren,

		LastMethodID
	};
};
#endif // #ifndef AK_OPTIMIZED
