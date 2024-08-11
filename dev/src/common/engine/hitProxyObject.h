/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once
#include "hitProxyId.h"

class CHitProxyMap;

/// Base hit proxy object
class CHitProxyObject
{
protected:
	CHitProxyID		m_id;				// ID of hit proxy object
	CObject*		m_object;			// Referenced object 

public:
	// Constructor from object reference
	CHitProxyObject( CObject *object );
	virtual ~CHitProxyObject();

	// Get hit proxy ID
	RED_INLINE const CHitProxyID& GetID() const { return m_id; }

	// Get object that owns this hit proxy
	RED_INLINE CObject* GetHitObject() const { return m_object; }

public:
	//!Bind hit proxy object to hit proxy map at given ID
	void BindToMap( CHitProxyMap &map, const CHitProxyID& id );
};
