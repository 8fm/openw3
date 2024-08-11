/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "../../../common/redSystem/log.h"

#include "slnDefinition.h"
#include "slnBase.h"
#include "slnDummy.h"
#include "slnStandard.h"
#include "slnMod.h"

#include "dir.h"

#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include <fstream>

static const int SOLUTION_VERSION = 1;

typedef rapidxml::xml_base< wchar_t > TBase;
typedef rapidxml::xml_document< wchar_t > TDocument;
typedef rapidxml::xml_node< wchar_t > TNode;
typedef rapidxml::xml_attribute< wchar_t > TAttr;

const wchar_t* SOLUTION_TYPE_STRINGS[ Solution_Max ] = 
{
	L"standard",
	L"mod"
};

ESolutionType StringToSolutionType( wchar_t* buffer )
{
	for( int i = 0; i < Solution_Max; ++i )
	{
		if( Red::System::StringCompare( buffer, SOLUTION_TYPE_STRINGS[ i ] ) == 0 )
		{
			return static_cast< ESolutionType >( i );
		}
	}

	return Solution_Dummy;
}

void ConvertToString( ESolutionType value, wchar_t* buffer, size_t size )
{
	if( value >= 0 )
	{
		Red::System::StringCopy( buffer, SOLUTION_TYPE_STRINGS[ value ], size );
	}
}

void ConvertToString( int value, wchar_t* buffer, size_t size )
{
	Red::System::SNPrintF( buffer, size, L"%i", value );
}

template< typename TValue >
void SetValue( TDocument& document, TBase* base, const TValue value )
{
	const size_t localValueSize = 32;
	wchar_t* localValue = document.allocate_string( nullptr, localValueSize );
	ConvertToString( value, localValue, localValueSize );

	base->value( localValue );
}

template<>
void SetValue( TDocument& document, TBase* base, const wchar_t* value )
{
	wchar_t* localValue = document.allocate_string( value );
	base->value( localValue );
}

TNode* AddNode( TDocument& document, TNode* parent, const wchar_t* name, rapidxml::node_type type = rapidxml::node_element )
{
	const wchar_t* localName = document.allocate_string( name );

	TNode* node = document.allocate_node( type, localName );
	parent->append_node( node );

	return node;
}

TAttr* AddAttribute( TDocument& document, TNode* parent, const wchar_t* name )
{
	const wchar_t* localName = document.allocate_string( name );

	TAttr* attribute = document.allocate_attribute( localName );
	parent->append_attribute( attribute );

	return attribute;
}

SolutionDefinition::SolutionDefinition()
{

}

SolutionDefinition::~SolutionDefinition()
{

}

void SolutionDefinition::Save( SolutionBase* solution, const wchar_t* path )
{
	ESolutionType type = solution->GetType();

	TDocument document;

	TNode* doctype = AddNode( document, &document, L"", rapidxml::node_declaration );
	TAttr* versionAttr = AddAttribute( document, doctype, L"version" );
	SetValue( document, versionAttr, L"1.0" );
	TAttr* encodingAttr = AddAttribute( document, doctype, L"encoding" );
	SetValue( document, encodingAttr, L"UTF-16" );

	TNode* root = AddNode( document, &document, L"redsln" );
	TAttr* rootVersion = AddAttribute( document, root, L"version" );
	SetValue( document, rootVersion, SOLUTION_VERSION );

	TAttr* attr = AddAttribute( document, root, L"type" );
	SetValue( document, attr, type );

	wxFileName solutionPathFixer( path );
	solutionPathFixer.MakeAbsolute();

	for( int i = 0; i < Solution_Max; ++i )
	{
		ESolutionType type = static_cast< ESolutionType >( i );
		SolutionDir* dir = solution->GetRoot( type );

		if( dir )
		{
			TNode* node = AddNode( document, root, L"path" );
			TAttr* attr = AddAttribute( document, node, L"type" );
			SetValue( document, attr, type );

			wstring rootPath = dir->GetAbsolutePath();
			wxFileName rootPathFixer( rootPath.c_str() );

			rootPathFixer.MakeRelativeTo( solutionPathFixer.GetPath( true ) );
			wxString fixedRootPath = rootPathFixer.GetFullPath();
			const wchar_t* relativeRootPath = fixedRootPath.wc_str();

			SetValue( document, node, relativeRootPath );
		}
	}

	if( solution->GetType() == Solution_Mod )
	{
		SolutionMod* mod = static_cast< SolutionMod* >( solution );

		wstring name = mod->GetName();
		wstring install = mod->GetInstall();

		TAttr* nameAttr = AddAttribute( document, root, L"name" );
		SetValue( document, nameAttr, name.c_str() );

		// Save this as an absolute path
		TNode* installNode = AddNode( document, root, L"install" );
		SetValue( document, installNode, install.c_str() );
	}

	wchar_t buffer[ 4096 ];
	wchar_t* end = rapidxml::print( buffer, document );
	*end = L'\0';

	FILE* file;
	errno_t result = _wfopen_s( &file, path, L"w, ccs=UTF-16LE" );

	if( result == 0 )
	{
		fputws( buffer, file );
		fclose( file );
	}
	else
	{
		RED_LOG_ERROR( SolutionDefinition, TXT( "Failed to open solution file '%ls' for writing: %i" ), path, result );
	}
}

SolutionBase* SolutionDefinition::Load( const wchar_t* path )
{
	SolutionBase* retval = nullptr;

	const size_t bufferSize = 4096;
	wchar_t buffer[ bufferSize ];
	Red::System::MemoryZero( buffer, bufferSize * sizeof( wchar_t ) );

	FILE* file;
	errno_t result = _wfopen_s( &file, path, L"r, ccs=UTF-16LE" );

	if( result == 0 )
	{
		wchar_t* bufferPos = buffer;
		size_t bufferRemaining = bufferSize;
		do
		{
			size_t length = Red::System::StringLength( bufferPos );
			bufferPos += length;
			bufferRemaining -= length;
		}
		while( fgetws( bufferPos, bufferRemaining, file ) );

		fclose( file );

		ESolutionType slnType = Solution_Dummy;
		wstring paths[ Solution_Max ];

		TDocument document;
		document.parse< 0 >( buffer );

		TNode* root = document.first_node( L"redsln" );

		if( root )
		{
			wxFileName solutionPath( path );

			TAttr* attribute = root->first_attribute( L"type" );
			slnType = StringToSolutionType( attribute->value() );

			TNode* pathnode = root->first_node( L"path" );

			while( pathnode )
			{
				TAttr* attribute = pathnode->first_attribute( L"type" );
				ESolutionType pathType = StringToSolutionType( attribute->value() );

				if( pathType >= 0 )
				{
					wxFileName pathFixer = pathnode->value();
					pathFixer.MakeAbsolute( solutionPath.GetPath( wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR ) );
					wxString absolutePath = pathFixer.GetFullPath();
					paths[ pathType ] = absolutePath.wc_str();
				}

				pathnode = pathnode->next_sibling( L"path" );
			}

			switch( slnType )
			{
			case Solution_Dummy:
				retval = new SolutionDummy();
				break;

			case Solution_Standard:
				retval = new SolutionStandard( paths[ Solution_Standard ] );
				break;

			case Solution_Mod:

				TAttr* nameAttr = root->first_attribute( L"name" );
				wstring name = (nameAttr)? nameAttr->value() : wxFileName( path ).GetName().wc_str();

				TNode* installNode = root->first_node( L"install" );
				wstring install = (installNode)? installNode->value() : L"";

				retval = new SolutionMod( name, paths[ Solution_Standard ], paths[ Solution_Mod ], install );

				break;
			}
		}
	}

	return retval;
}
