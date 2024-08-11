#include "build.h"
#include "resourcedatabasegraphgeneration.h"
#include "fileSys.h"

namespace Red {	namespace Core {
namespace ResourceManagement {

//////////////////////////////////////////////////////////////////////////
// Default graph formatter implementation
class CDefaultResourceGraphFormatter : public IResourceDatabaseGraphFormatter
{
public:
	void GenerateLabel( CResourceBuildDatabase& buildDb, CResourceBuildDatabase::ResourceIndex resIndex, TString< AnsiChar >& output )
	{
		output.ClearFast();
		const AnsiChar* resPath = buildDb.GetResourceSourcePath(resIndex);
		const AnsiChar* resName = buildDb.GetResourceName(resIndex);
		const AnsiChar* typeString = buildDb.GetResourceTypeName(resIndex);
		output += typeString;
		output += + "\\n";
		if( resName )
		{
			TString<AnsiChar> resNameFixed = resName;
			resNameFixed.ReplaceAll( "\\", "\\\\" );
			output += resNameFixed;
		}
		if( resPath ) 
		{
			TString<AnsiChar> resPathFixed = resPath;
			resPathFixed.ReplaceAll( "\\", "\\\\" );
			if( resName )
			{
				output += " ";
			}
			output += "(";
			output += resPathFixed;
			output += ")";
		}
	}

	void GenerateResourceFormattingString( CResourceBuildDatabase& buildDb, CResourceBuildDatabase::ResourceIndex resIndex, TString< AnsiChar >& output )
	{
		RED_UNUSED( buildDb );
		RED_UNUSED( resIndex );
		output += "shape=box";
	}

	virtual void GenerateGraphFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output )
	{
		RED_UNUSED( buildDb );
		output = "ranksep=4.0, rankdir=LR";
	}

	virtual void GenerateNodeFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output )
	{
		RED_UNUSED( buildDb );
		output = "fontsize=12, fontname=Arial";
	}

	virtual void GenerateEdgeFormattingString( CResourceBuildDatabase& buildDb, TString< AnsiChar >& output )
	{
		RED_UNUSED( buildDb );
		output = "color=\"#999999\"";
	}

} g_defaultGraphFormatter;

//////////////////////////////////////////////////////////////////////////
// Default graph filter
class CDefaultResourceGraphFilter : public IResourceGraphFiltering
{
public:
	virtual Red::System::Bool FilterGraphNode( Red::Core::ResourceManagement::CResourceBuildDatabase& buildDb, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex parentNode, Red::Core::ResourceManagement::CResourceBuildDatabase::ResourceIndex currentNode )
	{
		RED_UNUSED( buildDb );
		RED_UNUSED( parentNode );
		RED_UNUSED( currentNode );
		return true;		// All nodes added
	}
} g_defaultNodeFilter;

//////////////////////////////////////////////////////////////////////////
CResourceDatabaseGraphGenerator::CResourceDatabaseGraphGenerator( CResourceBuildDatabase& buildDb, IResourceDatabaseGraphFormatter* graphFormatter, IResourceGraphFiltering* graphFilter )
	: m_buildDb( buildDb )
	, m_fileWriter( nullptr )
	, m_graphFormatter( graphFormatter )
	, m_nodeFilter( graphFilter )
{
	if( m_graphFormatter == nullptr )
	{
		m_graphFormatter = &g_defaultGraphFormatter;
	}
	if( m_nodeFilter == nullptr )
	{
		m_nodeFilter = &g_defaultNodeFilter;
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::FilterTargets( CResourceBuildDatabase::ResourceIndex rootNode, const TSortedSet< CResourceBuildDatabase::ResourceIndex >& children, TSortedSet< CResourceBuildDatabase::ResourceIndex >& outputList )
{
	// This will contain the filtered local node children
	TSortedSet< CResourceBuildDatabase::ResourceIndex > branchOutput;

	// Run through the input set, any that pass the filters get run through again
	for( auto it = children.Begin(); it != children.End(); ++it )
	{
		if( m_nodeFilter->FilterGraphNode( m_buildDb, rootNode, *it ) )
		{
			branchOutput.Insert( *it );
		}
	}

	// Add anything in the branch output to the target list
	for( auto it = branchOutput.Begin(); it != branchOutput.End(); ++it )
	{
		outputList.Insert( *it );

		TSortedSet< CResourceBuildDatabase::ResourceIndex > nodeChildren;
		m_buildDb.GetImmediateDependencies( *it, nodeChildren );

		FilterTargets( *it, nodeChildren, outputList );
	}
}

//////////////////////////////////////////////////////////////////////////
// Output the entire database to a single graph
Red::System::Bool CResourceDatabaseGraphGenerator::WriteGraph( const String& destinationFilename )
{
	// Get the list of resources to output
	TDynArray< CResourceBuildDatabase::ResourceIndex > targetResources;
	GetEntireDatabaseTargetList( targetResources );

	return WriteGraph( destinationFilename, targetResources );
}

//////////////////////////////////////////////////////////////////////////
// Output a specific set of resources in the database
Red::System::Bool CResourceDatabaseGraphGenerator::WriteGraph( const String& destinationFilename, const TDynArray< CResourceBuildDatabase::ResourceIndex >& targetResources )
{
	Red::System::Bool fileOpen = OpenFileForWriting( destinationFilename );
	RED_ASSERT( fileOpen, TXT( "Failed to open graph file '%ls' for writing" ), destinationFilename.AsChar() );
	if( !fileOpen )
	{
		return false;
	}

	// Push the targets to a set, then extract dependencies + apply filters
	TSortedSet< CResourceBuildDatabase::ResourceIndex > resourceList;
	Red::System::Uint32 targetCount = targetResources.Size();
	for( Red::System::Uint32 i=0; i<targetCount; ++i )
	{
		resourceList.Insert( targetResources[i] );
	}

	TSortedSet< CResourceBuildDatabase::ResourceIndex > allDependencies;
	FilterTargets( CResourceBuildDatabase::c_InvalidIndex, resourceList, allDependencies );

	// Now we are ready to rock!
	StartGraph();
	WriteResourceHeaders( allDependencies );
	WriteResourceDependencies( allDependencies );
	EndGraph();

	FinishWriting();
	return true;
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::StartGraph()
{
	static AnsiChar c_GraphProlog[] = "digraph \"resources\" {\r\n";
	m_fileWriter->Serialize( &c_GraphProlog, sizeof( c_GraphProlog ) / sizeof( *c_GraphProlog ) - 1 );

	TString<AnsiChar> graphFormatting;
	TString<AnsiChar> nodeFormatting;
	TString<AnsiChar> edgeFormatting;
	TString<AnsiChar> output;
	m_graphFormatter->GenerateGraphFormattingString( m_buildDb, graphFormatting );
	m_graphFormatter->GenerateNodeFormattingString( m_buildDb, nodeFormatting );
	m_graphFormatter->GenerateEdgeFormattingString( m_buildDb, edgeFormatting );
	if( graphFormatting.Size() > 0 )
	{
		output += TString<AnsiChar>::Printf("\tgraph [%s];\r\n", graphFormatting.AsChar() );
	}
	if( nodeFormatting.Size() > 0 )
	{
		output += TString<AnsiChar>::Printf("\tnode [%s];\r\n", nodeFormatting.AsChar() );
	}
	if( edgeFormatting.Size() > 0 )
	{
		output += TString<AnsiChar>::Printf("\tedge [%s];\r\n", edgeFormatting.AsChar() );
	}
	m_fileWriter->Serialize( (void*)output.AsChar(), output.GetLength() );
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::EndGraph()
{
	static AnsiChar c_GraphCloser[] = "\r\n}";
	m_fileWriter->Serialize( &c_GraphCloser, sizeof( c_GraphCloser ) / sizeof( *c_GraphCloser ) - 1 );
}

//////////////////////////////////////////////////////////////////////////
// In order to keep interfaces consistent, everything starts with a list of target indices
void CResourceDatabaseGraphGenerator::GetEntireDatabaseTargetList( TDynArray< CResourceBuildDatabase::ResourceIndex >& targetList )
{
	// Not the fastest thing in the world, but it will do.
	Red::System::Uint32 resourceCount = m_buildDb.GetResourceCount();
	targetList.Resize( resourceCount );
	for( Red::System::Uint32 i=0; i<resourceCount; ++i )
	{
		targetList[i] = i;
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::WriteResourceDependencies( const TSortedSet< CResourceBuildDatabase::ResourceIndex >& targetList )
{
	TString<AnsiChar> outputLine;
	TSortedSet< CResourceBuildDatabase::ResourceIndex > childDependencies;
	for( auto it = targetList.Begin(); it != targetList.End(); ++it )
	{
		Red::System::Uint32 srcIndex = *it;
		childDependencies.ClearFast();
		m_buildDb.GetImmediateDependencies( srcIndex, childDependencies );
		for( auto childIt = childDependencies.Begin(); childIt != childDependencies.End(); ++childIt )
		{
			// Only accept children that are in the target list
			if( targetList.Exist( *childIt ) )
			{
				outputLine.ClearFast();
				outputLine = TString<AnsiChar>::Printf( "\t\"%u\" -> \"%u\"\r\n", srcIndex, *childIt );
				m_fileWriter->Serialize( (void*)outputLine.AsChar(), outputLine.Size() - 1 );
			}
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::WriteResourceHeaders( const TSortedSet< CResourceBuildDatabase::ResourceIndex >& targetList )
{
	TString<AnsiChar> outputLine;
	for( auto it = targetList.Begin(); it != targetList.End(); ++it )
	{
		Red::System::Uint32 targetIndex = *it;
		outputLine = TString<AnsiChar>::Printf( "\t\"%u\"", targetIndex );

		TString<AnsiChar> targetLabel;
		m_graphFormatter->GenerateLabel( m_buildDb, targetIndex, targetLabel );
		outputLine += TString<AnsiChar>::Printf(" [label=\"%s\"", targetLabel.AsChar() );

		TString<AnsiChar> formatString;
		m_graphFormatter->GenerateResourceFormattingString( m_buildDb, targetIndex, formatString );
		if( formatString.GetLength() > 0 )
		{
			outputLine += ", " + formatString;
		}
		outputLine += "];\r\n";
		m_fileWriter->Serialize( (void*)outputLine.AsChar(), outputLine.Size() - 1 );
	}
}

//////////////////////////////////////////////////////////////////////////
Red::System::Bool CResourceDatabaseGraphGenerator::OpenFileForWriting( const String& destinationFilename )
{
	RED_ASSERT( m_fileWriter == nullptr, TXT( "File writer already open?!" ) );
	m_fileWriter = GFileManager->CreateFileWriter( destinationFilename, FOF_AbsolutePath | FOF_Buffered );
	return m_fileWriter != nullptr;
}

//////////////////////////////////////////////////////////////////////////
void CResourceDatabaseGraphGenerator::FinishWriting()
{
	delete m_fileWriter;
}

}	// namespace ResourceManagement {
} } // namespace Red {	namespace Core {
