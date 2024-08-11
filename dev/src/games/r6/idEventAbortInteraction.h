/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "idEvent.h"

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEventAbortInteraction : public CIdEvent
{	
	DECLARE_ENGINE_CLASS( CIdEventAbortInteraction, CIdEvent, 0 );

	//------------------------------------------------------------------------------------------------------------------
	// Variables
	//------------------------------------------------------------------------------------------------------------------
private:
	CName	m_entityTag;

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance );
};

BEGIN_CLASS_RTTI( CIdEventAbortInteraction )
	PARENT_CLASS( CIdEvent )
	PROPERTY_EDIT( m_entityTag, TXT("") )
	END_CLASS_RTTI()