/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#pragma once
#ifndef _RED_RESOURCE_DATABASE_GRAPH_GENERATION_H_
#define _RED_RESOURCE_DATABASE_GRAPH_GENERATION_H_

#include "resourcedatabase.h"

namespace Red
{
	namespace Core
	{
		namespace ResourceManagement
		{
			// This class is an interface so users can customise the graph output
			class IResourceDatabaseGraphFormatter
			{
			public:
				// Generate an identifier to be used in the graph
				virtual void GenerateLabel( CResourceBuildDatabase& buildDb, CResourceBuildDatabase::ResourceIndex resIndex, TString< AnsiChar >& output ) = 0;

				// Generate any GraphViz formatting required for a resource node
				virtual void GenerateResourceFormattingString( CResourceBuildDatabase& buildDb, CResourceBuildDatabase::ResourceIndex resIndex, TString< AnsiChar >& output ) = 0;

				// Generate any graph-wide formatting options
				virtual void GenerateGraphFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output ) = 0;

				// Generate any per-node formatting options
				virtual void GenerateNodeFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output ) = 0;

				// Generate any per-edge formatting options
				virtual void GenerateEdgeFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output ) = 0;
			};

			// This interface is used to filter / prune nodes from the graph
			class IResourceGraphFiltering
			{
			public:
				// Used to filter dependencies when doing the graph traversal. Return true = add the current node to the graph, false = cull the node from the graph
				virtual Red::System::Bool FilterGraphNode( Red::Core::ResourceManagement::CResourceBuildDatabase& buildDb, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex parentNode, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex currentNode ) = 0;
			};

			// This class writes GraphViz compatible graph files so the database can be visualised
			class CResourceDatabaseGraphGenerator
			{
			public:
				CResourceDatabaseGraphGenerator( CResourceBuildDatabase& sourceDatabase, IResourceDatabaseGraphFormatter* graphFormatter = nullptr, IResourceGraphFiltering* graphFilter = nullptr );
		
				// Output the entire database to a single graph
				Red::System::Bool WriteGraph( const String& destinationFilename );

				// Output a specific set of resources in the database
				Red::System::Bool WriteGraph( const String& destinationFilename, const TDynArray< CResourceBuildDatabase::ResourceIndex >& targetResources );

			private:
				CResourceDatabaseGraphGenerator( const CResourceDatabaseGraphGenerator& other ) : m_buildDb( other.m_buildDb ) {  }
				void operator=( const CResourceDatabaseGraphGenerator& other ) { RED_UNUSED( other ); }

				// Graph generation
				void StartGraph();
				void EndGraph();

				// In order to keep interfaces consistent, everything starts with a list of target indices
				void GetEntireDatabaseTargetList( TDynArray< CResourceBuildDatabase::ResourceIndex >& targetList );

				// From the target list, we run filters to determine which nodes we actually want from the entire heirarchy
				void FilterTargets( CResourceBuildDatabase::ResourceIndex rootNode, const TSortedSet< CResourceBuildDatabase::ResourceIndex >& children, TSortedSet< CResourceBuildDatabase::ResourceIndex >& outputList );

				// Write resource descriptors
				void WriteResourceHeaders( const TSortedSet< CResourceBuildDatabase::ResourceIndex >& targetList );
				void WriteResourceDependencies( const TSortedSet< CResourceBuildDatabase::ResourceIndex >& targetList );

				// File stuff
				Red::System::Bool OpenFileForWriting( const String& destinationFilename );
				void FinishWriting();
			
				CResourceBuildDatabase& m_buildDb;
				IFile* m_fileWriter;
				IResourceDatabaseGraphFormatter* m_graphFormatter;
				IResourceGraphFiltering* m_nodeFilter;			
			};
		}
	}
}

#endif