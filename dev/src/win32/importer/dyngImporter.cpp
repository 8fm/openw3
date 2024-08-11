/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "FACFormat.h"
#include "../../common/engine/dyngResource.h"
#include "../../common/core/importer.h"
#include "re_archive/include/reFileNodes.h"
#include "ReFileHelpers.h"

static const Float SCALE = 0.01f;

#pragma optimize("",off)

class CDyngImporter : public IImporter
{
	DECLARE_ENGINE_CLASS( CDyngImporter, IImporter, 0 );

public:
	CDyngImporter();
	virtual CResource*	DoImport( const ImportOptions& options );
	virtual Bool		PrepareForImport( const String&, ImportOptions& options ) override;
};

BEGIN_CLASS_RTTI( CDyngImporter )
	PARENT_CLASS( IImporter )
END_CLASS_RTTI()

IMPLEMENT_ENGINE_CLASS( CDyngImporter );

CDyngImporter::CDyngImporter()
{
	m_resourceClass = ClassID< CDyngResource >();
	m_formats.PushBack( CFileFormat( TXT("re"), TXT("RE File") ) );
}

Bool CDyngImporter::PrepareForImport( const String& str, ImportOptions& options )
{
	return CReFileHelpers::ShouldImportFile( options );
}

CResource* CDyngImporter::DoImport( const ImportOptions& options )
{
	// create factory info
	CDyngResource::FactoryInfo info;
	info.m_parent = options.m_parentObject;
	info.m_reuse = Cast< CDyngResource >( options.m_existingResource );

	FILE* f = _wfopen( options.m_sourceFilePath.AsChar(),TXT("rb") );
	if ( !f )
	{
		return NULL;
	}

	// Load headers
	Int32 numElements = 0;
	fread( &numElements, sizeof( int ), 1, f );
	if(!numElements)
	{
		WARN_IMPORTER( TXT("No data in file %s!"), options.m_sourceFilePath.AsChar() );
		fclose(f);
		return NULL;
	}
	ReFileArchiveHeader* header = new ReFileArchiveHeader[numElements];
	fread( header, sizeof( ReFileArchiveHeader ) * numElements, 1, f );

	Int32 numDyngs = 0;

	ReFileString currentVersion;
	for(Int32 i=0;i<numElements;i++)
	{
		if ( header[i].mType =='hed2' )
		{
			ReFileHeader2 hdr;
			ReFileBuffer buf;
			fseek( f, header[i].mOffset, SEEK_SET );
			buf.setsize( header[i].mSize );
			fread( buf.getBuffer(), header[i].mSize, 1, f );
			hdr.read(&buf);
			currentVersion = hdr.getReFileVersion();
		}

		if( header[i].mType == 'dyng' )
		{
			numDyngs++;
		}
	}

	if (numDyngs == 0)
	{
		WARN_IMPORTER( TXT("File %s contains no dyngs chunks!"), options.m_sourceFilePath.AsChar() );
		fclose(f);
		return NULL;
	}

	ReFileDyng* dyngsArray = new ReFileDyng[numDyngs];

	Int32 dyngIndex = 0;
	for(Int32 i=0;i<numElements;i++)
	{
		if( header[i].mType == 'dyng' )
		{
			ReFileBuffer buf;
			buf.setCurrentVersion(currentVersion);
			fseek( f, header[i].mOffset, SEEK_SET );
			buf.setsize( header[i].mSize );
			fread( buf.getBuffer(), header[i].mSize, 1, f );
			dyngsArray[dyngIndex].read(&buf);

			// Example
			{
				info.m_name = ANSI_TO_UNICODE( dyngsArray[dyngIndex].getDyngName().getData() );
				const Int32 numNodes = dyngsArray[dyngIndex].getNumNodes();
				const Int32 numLinks = dyngsArray[dyngIndex].getNumLinks();
				const Int32 numTriangles = dyngsArray[dyngIndex].getNumTriangles();
				const Int32 numShapes = dyngsArray[dyngIndex].getNumShapes();

				Int32 i;
				for( i=0;i<numNodes;++i)
				{
					info.m_nodeNamesData.PushBack( ANSI_TO_UNICODE( dyngsArray[dyngIndex].mDyngNodes[i].getDyngName().getData() ) );
					info.m_nodeParentsData.PushBack( ANSI_TO_UNICODE( dyngsArray[dyngIndex].mDyngNodes[i].getParentName().getData() ) );

					info.m_nodeMassesData.PushBack( dyngsArray[dyngIndex].mDyngNodes[i].getMass() );
					info.m_nodeStifnessesData.PushBack( dyngsArray[dyngIndex].mDyngNodes[i].getStiffness() );
					info.m_nodeDistancesData.PushBack( dyngsArray[dyngIndex].mDyngNodes[i].getDistance() * SCALE );

					Float* mat = (Float*)&dyngsArray[dyngIndex].mDyngNodes[i].mTransformMatrix;
					Matrix emat;
					emat.SetRow( 0, Vector( mat[0],			mat[1],				mat[2],				0.0f) );
					emat.SetRow( 1, Vector( mat[3],			mat[4],				mat[5],				0.0f) );
					emat.SetRow( 2, Vector( mat[6],			mat[7],				mat[8],				0.0f) );
					emat.SetRow( 3, Vector( mat[9] * SCALE,	mat[10] * SCALE,	mat[11] * SCALE,	1.0f ) );

					info.m_nodeTransformsData.PushBack( emat );
				}

				for( i=0;i<numLinks;++i)
				{
					info.m_linkTypesData.PushBack( dyngsArray[dyngIndex].mDyngLinks[i].getLinkType() );
					info.m_linkLengthsData.PushBack( dyngsArray[dyngIndex].mDyngLinks[i].getLength() * SCALE );
					info.m_linkAsData.PushBack( dyngsArray[dyngIndex].mDyngLinks[i].getIndexA() );
					info.m_linkBsData.PushBack( dyngsArray[dyngIndex].mDyngLinks[i].getIndexB() );
				}
				
				for( i=0;i<numTriangles;++i)
				{
					info.m_triangleAsData.PushBack( dyngsArray[dyngIndex].mDyngTriangles[i].getIndexA() );
					info.m_triangleBsData.PushBack( dyngsArray[dyngIndex].mDyngTriangles[i].getIndexB() );
					info.m_triangleCsData.PushBack( dyngsArray[dyngIndex].mDyngTriangles[i].getIndexC() );
				}

				for( i=0;i<numShapes;++i)
				{
					info.m_collisionParentsData.PushBack( ANSI_TO_UNICODE( dyngsArray[dyngIndex].mDyngShapes[i].getParentName().getData() ) );
					info.m_collisionRadiusesData.PushBack( dyngsArray[dyngIndex].mDyngShapes[i].getRadius() * SCALE );
					info.m_collisionHeightsData.PushBack( dyngsArray[dyngIndex].mDyngShapes[i].getHeight() * SCALE );
					Float* mat = (Float*)&dyngsArray[dyngIndex].mDyngShapes[i].mTransformMatrix;
					Matrix emat;
					emat.SetRow( 0, Vector(	mat[0],			mat[1],				mat[2],				0.0f ) );
					emat.SetRow( 1, Vector(	mat[3],			mat[4],				mat[5],				0.0f ) );
					emat.SetRow( 2, Vector(	mat[6],			mat[7],				mat[8],				0.0f ) );
					emat.SetRow( 3, Vector(	mat[9] * SCALE,	mat[10] * SCALE,	mat[11] * SCALE,	1.0f ) );
					info.m_collisionTransformsData.PushBack( emat );
				}
			}

			dyngIndex++;
		}
	}

	// Close file
	fclose( f );

	// Create object
	CDyngResource* retVal = CDyngResource::Create( info );

	// Set import file
	retVal->SetImportFile( options.m_sourceFilePath );

	delete [] dyngsArray;

	return retVal;
}

#pragma optimize("",on)
