/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

// Used to build input action (activator) entry in string format
class CInputMappingActionStringBuilder
{
public:
	CInputMappingActionStringBuilder();

	RED_INLINE void SetActionName( const String& name ) { m_actionName = name; }
	RED_INLINE void SetState( const String& state ) { m_state = state; }
	RED_INLINE void SetIdleTime( const Float time ) { m_idleTime = time; }
	RED_INLINE void SetReprocess( const Bool reprocess ) { m_reprocess = reprocess; }

	void Reset();
	String Construct();

private:
	void BeginConstruct(String& result);
	void EndConstruct(String& result);
	void ConstructAction(String &result);
	void ConstructState(String &result);
	void ConstructActionValue(String& result);
	void ConstructIdleTime(String& result);
	void ConstructAxisValue(String& result);
	void ConstructReprocess(String& result);

private:
	String m_actionName;
	String m_state;
	Float m_idleTime;
	Bool m_reprocess;

};

// Forward Declarations
enum EInputKey : Int32;
namespace Config
{
	namespace Legacy
	{
		class CConfigLegacyFile;
	}

	extern TConfigVar<String> cvInputMappingSettingsSectionName;
}

// Used to create input mapping file
class CInputMappingSaver
{
public:
	CInputMappingSaver( const String& filename, const String& fileAbsolutePath, const Int32 version );
	~CInputMappingSaver();
	void AddEntry( const CName& context, const EInputKey key, const String& action );
	void AddEmptyContext( const CName& context );
	void Save();

private:
	String GetKeyAsString(const EInputKey key);

private:
	Config::Legacy::CConfigLegacyFile* m_file;

};
