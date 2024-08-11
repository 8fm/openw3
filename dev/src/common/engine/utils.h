/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CResourceObtainer
{
public:
	CResourceObtainer()		{	}
	~CResourceObtainer()	{	}

	static const CResource* GetResource( const CEntity* entity );
	static const CResource* GetResource( const CComponent* component );
	static const CResource* GetResource( const CNode* node );
	static const CResource* GetResource( const CObject* object );

};


