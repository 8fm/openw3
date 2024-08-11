/**
* Copyright © 2014 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once

#if defined(RED_PLATFORM_WINPC) || defined(RED_PLATFORM_DURANGO)

#include "..\..\common\redSystem\logCommonOutputDevice.h"


class CGatTTYWriter : public Red::System::Log::CommonOutputDevice
{
public:
	CGatTTYWriter ();
	virtual ~CGatTTYWriter ();

	void SetSilent(const Red::Bool silent);
	void SetVerbose(const Red::Bool verbose);

private:
	Red::Bool		m_verbose;
	Red::Bool		m_silent;

	HANDLE			m_hStdOut;		// direct console access

	Red::System::Char			m_statusText[256];

	void UpdateStatusText( const Red::System::Char* txt );

	virtual void WriteFormatted( const Red::System::AnsiChar* message );
	virtual void WriteFormatted( const Red::System::UniChar* message );
};

#endif