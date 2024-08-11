/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "aiParameters.h"

class IAIActionParameters : public CAIParameters
{
	typedef CAIParameters TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAIActionParameters );
public:
	IAIActionParameters(){}
};

BEGIN_CLASS_RTTI( IAIActionParameters )
	PARENT_CLASS( CAIParameters )
END_CLASS_RTTI()

class IAIActionTree : public CAITree
{
	typedef CAITree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( IAIActionTree );
public:
	IAIActionTree(){}
};

BEGIN_CLASS_RTTI( IAIActionTree )
	PARENT_CLASS( CAITree )
END_CLASS_RTTI()

class CAIActionSequence : public IAIActionTree
{
	typedef IAIActionTree TBaseClass;
	DECLARE_RTTI_SIMPLE_CLASS( CAIActionSequence );
protected:
	TDynArray< THandle< IAIActionTree > >			m_actions;

public:
	void AddAction( IAIActionTree* action )								{ m_actions.PushBack( action ); }
};

BEGIN_CLASS_RTTI( CAIActionSequence )
	PARENT_CLASS( IAIActionTree )
	PROPERTY_INLINED( m_actions, TXT("Actions list") )
END_CLASS_RTTI()


