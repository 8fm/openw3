/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma once

class CPackageTool
{
private:
	String m_exeAbsolutePath;

public:
	CPackageTool( const String& exeAbsolutePath );

protected:
	const String& GetExeAbsolutePath() const { return m_exeAbsolutePath; }

protected:
	Bool RunExe( const String& commandLine, const String& workingDirectory = String(TXT(".")) ) const;
};