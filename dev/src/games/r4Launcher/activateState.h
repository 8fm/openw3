/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "../../common/core/gameApplication.h"
#include "../../common/core/types.h"
#include "../../common/core/string.h"

class CActivateState : public IGameState
{
public:
	virtual const Char* GetName() const override { return TXT("Activate"); };

public:

	CActivateState( const wchar_t * commandLine );

	virtual Red::System::Bool OnTick( IGameApplication & application ) override;

private:

	String m_commandLine;
	const wchar_t * m_rawCommandLine;
};
