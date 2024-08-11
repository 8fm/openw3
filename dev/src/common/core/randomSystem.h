/*
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CRandomSystem
{
public:
	static Uint32 GetRandomIntFromSeed( Uint32& seed );
	static Float GetRandomFloatFromSeed( Uint32& seed );
};
