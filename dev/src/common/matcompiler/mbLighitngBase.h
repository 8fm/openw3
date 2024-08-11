/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Base block that implements lighting function
class CMaterialBlockLighting : public CMaterialBlock
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMaterialBlockLighting, CMaterialBlock );

public:
};

BEGIN_ABSTRACT_CLASS_RTTI( CMaterialBlockLighting );
	PARENT_CLASS( CMaterialBlock );
END_CLASS_RTTI();