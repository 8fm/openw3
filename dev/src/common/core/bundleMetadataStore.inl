/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

namespace Red
{
namespace Core
{
namespace Bundle
{

void CBundleMetadataStoreFilter::EnableAll()
{
	Red::MemorySet( m_mask.Data(), 0xFF, m_mask.DataSize() );
}

void CBundleMetadataStoreFilter::DisableAll()
{
	Red::MemoryZero( m_mask.Data(), m_mask.DataSize() );
}

Bool CBundleMetadataStoreFilter::IsBundleEnabled( const BundleID id ) const
{
	RED_ASSERT( id > 0 && id < m_maxBundles, TXT("Bundle index %d is out of range"), id );

	const Uint32 wordIndex = id >> SHIFT;
	const Uint32 wordMask = 1 << (id & MASK);
	return 0 != (m_mask[ wordIndex ] & wordMask);
}

void CBundleMetadataStoreFilter::EnableBundle( const BundleID id )
{
	RED_ASSERT( id > 0 && id < m_maxBundles, TXT("Bundle index %d is out of range"), id );

	const Uint32 wordIndex = id >> SHIFT;
	const Uint32 wordMask = 1 << (id & MASK);
	m_mask[ wordIndex ] |= wordMask;
}

void CBundleMetadataStoreFilter::DisableBundle( const BundleID id )
{
	RED_ASSERT( id > 0 && id < m_maxBundles, TXT("Bundle index %d is out of range"), id );

	const Uint32 wordIndex = id >> SHIFT;
	const Uint32 wordMask = 1 << (id & MASK);
	m_mask[ wordIndex ] &= ~wordMask;
}

} // Bundle
} // Core
} // Red