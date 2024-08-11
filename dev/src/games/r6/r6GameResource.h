	/**
* Copyright c 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#include "../../common/game/commonGameResource.h"

class CR6GameResource : public CCommonGameResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CR6GameResource, CCommonGameResource, "r6game", "R6 game definition" );

public:
	CR6GameResource();

	RED_INLINE CName GetTraitDataPath() { return m_traitDataPath; }
	RED_INLINE String GetCharacterDBPath() { return m_characterDBRootDir; }


private:
	CName			m_traitDataPath;
	String			m_characterDBRootDir;
};

BEGIN_CLASS_RTTI( CR6GameResource );
	PARENT_CLASS( CCommonGameResource );
	PROPERTY_EDIT( m_traitDataPath, TXT( "Trait data path" ) );
	PROPERTY_CUSTOM_EDIT( m_characterDBRootDir, TXT( "Character database root directory" ), TXT("DirectorySelectionEditor") );
END_CLASS_RTTI();
