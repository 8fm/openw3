/**
* Copyright © 2013 CDProjekt Red, Inc. All Rights Reserved.
*/

#pragma once

#include "..\..\common\redSystem\logCommonOutputDevice.h"


class CWccTTYWriter : public Red::System::Log::CommonOutputDevice
{
public:
	CWccTTYWriter ();
	virtual ~CWccTTYWriter ();

	void SetSilent(const Red::Bool silent);
	void SetVerbose(const Red::Bool verbose);

private:
	Red::Bool		m_verbose;
	Red::Bool		m_silent;

	HANDLE			m_hStdOut;		// direct console access

	Char			m_statusText[256];

	void UpdateStatusText( const Char* txt );

	virtual void Flush();
	virtual void Write( const Red::System::Log::Message& message );
	virtual void WriteFormatted( const Red::System::UniChar* message );
};
