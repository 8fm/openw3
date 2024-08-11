/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once


#include "behTreeRtti.h"


#define IMPLEMENT_BEHTREE_NODE( _class )																	\
	IMPLEMENT_ENGINE_CLASS( _class );																		\
	_class::_class##Info::_class##Info()																	\
	{ CBehNodesManager::GetInstance().Register( this ); }													\
	struct _class::_class##Info _class::sm_Info;															\
	CName _class::GetNodeName() const { return StaticNodeName(); }

