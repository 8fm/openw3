#pragma once

#define MAX_SCRIPT_REG_COUNT 10

class CScriptRegistration
{
public:
	CScriptRegistration();
	virtual void RegisterScriptFunctions()const = 0;
};

class CScriptRegistrationManager
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );

public:
	static CScriptRegistrationManager *const GetInstance();
	static void ReleaseInstance();

	void RegisterScriptFunctions();

	void AddScriptRegistration( const CScriptRegistration *const scriptRegistration );

private:
	// this is a singleton so constructor must be called by myself only
	CScriptRegistrationManager();

	// instance of the singleton
	static CScriptRegistrationManager * s_instance;

	TStaticArray< const CScriptRegistration *, MAX_SCRIPT_REG_COUNT >	m_scriptRegistrationArray;	
};