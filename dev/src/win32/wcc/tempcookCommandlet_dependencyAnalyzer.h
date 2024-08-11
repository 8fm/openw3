/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma once

/// Analyzer of file dependencies
class CCookerDependencyAnalyzer
{
protected:
	/// File dependency
	struct File
	{
		String					m_depotPath;		//!< Target file ( as was names by the caller )
		Uint64					m_fileTime;			//!< Cached file time
		TDynArray< File* >		m_dependencies;		//!< Cached dependencies
		Bool					m_visited;			//!< File was visited during the file walk
		Bool					m_recursed;			//!< Recursive dependecies were checked for this file
		Int32					m_index;			//!< Dependency index ( save only )
	};

public:
	TDynArray< File* >			m_dependencies;		//!< All dependencies
	THashMap< String, File* >	m_fileMap;			//!< Mapping of files
	IFile*						m_cacheFile;		//!< Cache file
	size_t						m_allocBudget;		//!< Memory allocation budget
	Bool						m_useSoftHandles;	//!< Use soft handles when generating dependencies

public:
	//! allocBudget    -- if > 0, will automatically run GC when bytes allocated reaches this.
	//! useSoftHandles -- whether soft handles should be followed when collecting dependencies.
	CCookerDependencyAnalyzer( size_t allocBudget = 0, Bool useSoftHandles=false );
	~CCookerDependencyAnalyzer();

	//! Get all dependencies for file
	Bool GetSingleFileDependencies( const String& depotPath, TDynArray< String >& referencedFiles );

	//! Get all dependencies for list of files
	Bool GetFilesDependencies( const TDynArray< String > depotPaths, TDynArray< String >& referencedFiles );

	//! Clear dependency cache
	void ClearCache();

protected:
	//! Get single list of file dependencies
	Bool GetFileDependencies( File* file, TDynArray< File* >& referencedFiles );

	//! Allocate or get dependency
	File* GetDependencyFile( const String& depotPath );

	//! Get the recursive dependency try of given file
	void GetRecursiveDependencies( File* file, Int32 level, TDynArray< String >& referencedFiles );
};
