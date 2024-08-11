/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once

class CIDTopicInstance;

//------------------------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------------------------
class CIdEvent : public CObject
{	
	DECLARE_ENGINE_ABSTRACT_CLASS( CIdEvent, CObject );

	//------------------------------------------------------------------------------------------------------------------
	// Methods
	//------------------------------------------------------------------------------------------------------------------
public:
	virtual void	Activate	( CIDTopicInstance* topicInstance )	= 0;
};

BEGIN_ABSTRACT_CLASS_RTTI( CIdEvent )
	PARENT_CLASS( CObject )
END_CLASS_RTTI()
