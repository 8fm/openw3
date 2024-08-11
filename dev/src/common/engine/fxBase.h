/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Base element of FX definition
class CFXBase : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CFXBase, CObject );

public:
	CFXBase();

	//! Get object name
	virtual String GetName() const=0;

	//! Change FX object name
	virtual void SetName( const String &name )=0;

	//! Remove from parent structure
	virtual void Remove()=0;		
};

BEGIN_ABSTRACT_CLASS_RTTI( CFXBase );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();