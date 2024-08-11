#include "build.h"
#ifdef USE_RED_RESOURCEMANAGER
//////////////////////////////////////////////////////////////////////////
CComponentResourceCollection::CComponentResourceCollection( const Red::System::GUID& guid )
	: m_ownerGUID( guid )
	, m_rawResourceCollection( nullptr )
{

}

//////////////////////////////////////////////////////////////////////////
CComponentResourceCollection::~CComponentResourceCollection()
{
	
}

//////////////////////////////////////////////////////////////////////////
void CComponentResourceCollection::Initialize()
{
	m_rawResourceCollection = new Red::Core::ResourceManagement::CRawResourceCollection();
}

//////////////////////////////////////////////////////////////////////////
void CComponentResourceCollection::Destroy()
{
	delete m_rawResourceCollection;
}

//////////////////////////////////////////////////////////////////////////
void CComponentResourceCollection::Serialize( IFile& file )
{
	file << m_ownerGUID;
	if( m_rawResourceCollection )
	{
		m_rawResourceCollection->Serialize( file );
	}
}

//////////////////////////////////////////////////////////////////////////
void CComponentResourceCollection::AddDataBuffer( const DataBuffer& dataBuffer )
{
	m_rawResourceCollection->AddDataBuffer( dataBuffer );
}

//////////////////////////////////////////////////////////////////////////
const DataBuffer& CComponentResourceCollection::GetDataBuffer( const Uint32 index )
{
	return m_rawResourceCollection->GetDataBuffer( index );
}

#endif // USE_RED_RESOURCEMANAGER