/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

//----

/// Error reporter stub for cooker
class CCookerErrorReporter
{
public:
	CCookerErrorReporter();

	void DumpErrorsToLog();
	void DumpErrorsToReport();

	void ReportError( const CResource* resource, const CObject* object, const String& txt );
	void ReportWarning( const CResource* resource, const CObject* object, const String& txt );

private:
	struct Message
	{
		String	m_resource;
		String	m_object;
		String	m_text;
	};

	typedef TDynArray< Message >	TMessages;
	typedef Red::Threads::CMutex	Mutex;

	TMessages		m_errors;
	TMessages		m_warnings;
	Mutex			m_lock;
};

//----

/// Dependency tracker
class CCookerDependencyTracker
{
public:
	CCookerDependencyTracker();

	void ReportHardDependency( const String& depotPath );
	void ReportSoftDependency( const String& depotPath );

	void GetHardDependencies( TDynArray< String >& outPaths ) const;
	void GetSoftDependencies( TDynArray< String >& outPaths ) const;

private:
	typedef	THashSet< String >		TDependencies;
	TDependencies	m_softDependencies;
	TDependencies	m_hardDependencies;
};

//----

/// Cooker stat collector
class ICookerStatCollector
{
public:
	virtual ~ICookerStatCollector() {};
	virtual void ReportCookingTime( const CClass* objectClass, const Double time ) = 0;
};

/// Cooker framework helper object
class CCookerFramework : public ICookerFramework
{
public:
	CCookerFramework( const CResource* resource, const ECookingPlatform platform, CCookerErrorReporter* errorReporter, CCookerDependencyTracker* dependencyTracker, ICookerStatCollector* statCollector );

	// interface
	virtual ECookingPlatform GetPlatform() const override { return m_platform; }
	virtual void CookingError( const CObject* subObject, const Char* message, ... ) override;
	virtual void CookingWarning( const CObject* subObject, const Char* message, ... ) override;
	virtual void ReportHardDependency( const String& depotPath ) override;
	virtual void ReportSoftDependency( const String& depotPath ) override;
	virtual void ReportCookingTime( const CClass* objectClass, const Double timeTaken ) override;

private:
	static const Uint32 MAX_MESSAGE_LENGTH = 1024;

	const CResource*			m_cookedResource;

	ECookingPlatform			m_platform;
	CCookerErrorReporter*		m_errorReporter;
	CCookerDependencyTracker*	m_dependencyTracker;
	ICookerStatCollector*		m_statCollector;
};

//----