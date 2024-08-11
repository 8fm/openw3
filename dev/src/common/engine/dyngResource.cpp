
#include "build.h"
#include "dyngResource.h"
#include "skeleton.h"

IMPLEMENT_ENGINE_CLASS( CDyngResource );

#ifndef NO_RESOURCE_IMPORT

CDyngResource* CDyngResource::Create( const FactoryInfo& data )
{
	CDyngResource* dyng = data.CreateResource();
	if ( dyng )
	{
		dyng->m_name = data.m_name;

		dyng->m_nodeNames = data.m_nodeNamesData;
		dyng->m_nodeParents = data.m_nodeParentsData;
		dyng->m_nodeMasses = data.m_nodeMassesData;
		dyng->m_nodeStifnesses = data.m_nodeStifnessesData;
		dyng->m_nodeDistances = data.m_nodeDistancesData;
		dyng->m_nodeTransforms = data.m_nodeTransformsData;

		dyng->m_linkTypes = data.m_linkTypesData;
		dyng->m_linkLengths = data.m_linkLengthsData;
		dyng->m_linkAs = data.m_linkAsData;
		dyng->m_linkBs = data.m_linkBsData;

		dyng->m_triangleAs = data.m_triangleAsData;
		dyng->m_triangleBs = data.m_triangleBsData;
		dyng->m_triangleCs = data.m_triangleCsData;

		dyng->m_collisionParents = data.m_collisionParentsData;
		dyng->m_collisionRadiuses = data.m_collisionRadiusesData;
		dyng->m_collisionHeights = data.m_collisionHeightsData;
		dyng->m_collisionTransforms = data.m_collisionTransformsData;
	}

	return dyng;
}

#endif

void CDyngResource::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
#ifndef NO_RESOURCE_IMPORT
	RecreateSkeleton();
#endif
}

void CDyngResource::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

#ifndef NO_RESOURCE_IMPORT
	if ( file.IsReader() )
	{
		RecreateSkeleton();
	}
#endif
}

void CDyngResource::CreateSkeleton()
{
#ifndef NO_RESOURCE_IMPORT
	CSkeleton::FactoryInfo data;
	CSkeleton::FactoryInfo::BoneImportInfo binf;
	const Int32 numBones = m_nodeNames.Size();
	Int32 i;
	Int32 j;

	data.m_parent = this;
	data.m_reuse = nullptr;

	for( i=0;i<numBones;++i )
	{
		const Matrix & delta = m_nodeTransforms[i];
		RedVector4 w;
		RedQuaternion q;
		RedMatrix3x3 rot;
		rot.SetRow( 0, RedVector3( delta.GetRow(0).X, delta.GetRow(0).Y, delta.GetRow(0).Z ) );
		rot.SetRow( 1, RedVector3( delta.GetRow(1).X, delta.GetRow(1).Y, delta.GetRow(1).Z ) );
		rot.SetRow( 2, RedVector3( delta.GetRow(2).X, delta.GetRow(2).Y, delta.GetRow(2).Z ) );
		q.ConstructFromMatrix( rot );

		q.Quat.X *= -1.0f;
		q.Quat.Y *= -1.0f;
		q.Quat.Z *= -1.0f;

		w.X = delta.GetRow(3).X;
		w.Y = delta.GetRow(3).Y;
		w.Z = delta.GetRow(3).Z;
		w.W = 1.0f;

		binf.m_parentIndex = -1;
		for( j=0;j<numBones;j++ )
		{
			if( m_nodeParents[i] == m_nodeNames[j] )
			{
				binf.m_parentIndex = (Int16)j;
				break;
			}
		}
		binf.m_lockTranslation = false;
		binf.m_name = UNICODE_TO_ANSI( m_nodeNames[i].AsChar() );
		binf.m_referencePose = RedQsTransform( w, q );
		data.m_bones.PushBack( binf );
	}

	m_dyngSkeleton = CSkeleton::Create( data );
	if ( m_dyngSkeleton )
	{
		m_dyngSkeleton->OnPostLoad();
	}
#else
	RED_HALT( "Failed to create skeleton for DyngComponent" );
#endif
}

void CDyngResource::RecreateSkeleton()
{
#ifndef NO_RESOURCE_IMPORT
	m_dyngSkeleton = nullptr;
	CreateSkeleton();
#else
	RED_HALT( "Failed to recreate skeleton for DyngComponent" );
#endif
}

