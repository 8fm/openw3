/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#pragma once
//////////////////////////////////////////////////////////////////////////////////////////////
// Keeping the name (W3LockableEntity) even though it should be the more generic 
// CLockableEntity. Changing the name at this point would invalidate a lot of data in 
// entities placed in the levels.
//
// TODO: after W3 is shipped refactor this to CLockableEntity
//////////////////////////////////////////////////////////////////////////////////////////////
class W3LockableEntity : public CGameplayEntity
{
	DECLARE_ENGINE_CLASS( W3LockableEntity, CGameplayEntity, 0 );

private:
	Bool m_isEnabledOnSpawn;
	Bool m_lockedByKey;

public:
	W3LockableEntity();

	Bool IsEnabledOnSpawn()	const	{ return m_isEnabledOnSpawn; }
	Bool IsLocked()	const			{ return m_lockedByKey; }
	virtual void OnStreamIn() override;
};

BEGIN_CLASS_RTTI( W3LockableEntity );
	PARENT_CLASS( CGameplayEntity );
	PROPERTY_EDIT_NAME( m_isEnabledOnSpawn, TXT( "isEnabledOnSpawn" ), TXT( "Enabled when spawned?" ) );
	PROPERTY_EDIT_SAVED( m_lockedByKey, TXT( "Locked or not?" ) );
END_CLASS_RTTI();