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
// AkKeyDef.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _KEYDEF_H_
#define _KEYDEF_H_

template <class T_KEY, class T_ITEM> 
struct MapStruct
{
	T_KEY	key; 
	T_ITEM	item;
	bool operator ==(const MapStruct& in_Op) const
	{
		return ( (key == in_Op.key) /*&& (item == in_Op.item)*/ );
	}
};

#endif //_KEYDEF_H_
