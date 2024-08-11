
/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#ifndef _RESOURCE_VERIFIER_ERROR_SYSTEM_H_
#define _RESOURCE_VERIFIER_ERROR_SYSTEM_H_

class CResourceVerifierErrorSystem : public CWin32ErrorSystem
{
protected:

	EAssertAction AssertImp( const Char *file, Uint line, const Char *message );
	void ErrorImp( EErrorType type, const Char *message );

public: 
	CResourceVerifierErrorSystem();

	static void GetCallStackStringStatic(Char* buffer, Uint size, Uint framesToSkip, EXCEPTION_POINTERS* pExcPtrs = NULL, const Char *indent = TXT(""), const Char *newLine = TXT("\r\n"));
};

extern CResourceVerifierErrorSystem GResVerifierErrorSystem;

#endif