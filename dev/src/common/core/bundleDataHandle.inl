//////////////////////////////////////////////////////////////////////////
CBundleDataHandle::CBundleDataHandle()
	: m_buffer( nullptr )
	, m_localOffset( 0 )
{
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle::CBundleDataHandle( CBundleDataBuffer* dataBuffer, Uint32 localOffset )
	: m_buffer( dataBuffer )
	, m_localOffset( localOffset )
{
	if( m_buffer )
	{
		m_buffer->AddRef();
	}
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle::CBundleDataHandle( const CBundleDataHandle& other )
	: m_buffer( other.m_buffer )
	, m_localOffset( other.m_localOffset )
{
	if( m_buffer )
	{
		m_buffer->AddRef();
	}
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle::CBundleDataHandle( CBundleDataHandle&& other )
	: m_buffer( std::move( other.m_buffer ) )
	, m_localOffset( std::move( other.m_localOffset ) )
{
	other.m_buffer = nullptr;
	other.m_localOffset = 0;
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle::~CBundleDataHandle()
{
	if( m_buffer )
	{
		m_buffer->Release();
		// we don't do the delete, that's up to the cache
	}
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle& CBundleDataHandle::operator=( const CBundleDataHandle& other )
{
	if( m_buffer )
	{
		m_buffer->Release();
	}
	m_buffer = other.m_buffer;
	m_localOffset = other.m_localOffset;
	if( m_buffer )
	{
		m_buffer->AddRef();
	}
	return *this;
}

//////////////////////////////////////////////////////////////////////////
CBundleDataHandle& CBundleDataHandle::operator=( CBundleDataHandle&& other )
{
	if( m_buffer != nullptr && m_buffer != other.m_buffer)
	{
		m_buffer->Release();
	}
	m_buffer = std::move( other.m_buffer );
	m_localOffset = std::move( other.m_localOffset );
	other.m_buffer = nullptr;
	other.m_localOffset = 0;

	return *this;
}

//////////////////////////////////////////////////////////////////////////
void* CBundleDataHandle::GetRaw() const
{
	if( m_buffer == nullptr )
	{
		return nullptr;
	}
	MemUint bufferAddress = reinterpret_cast< MemUint >( m_buffer->GetDataBuffer() );
	return reinterpret_cast< void* >( bufferAddress + m_localOffset );
}

//////////////////////////////////////////////////////////////////////////
CBundleDataBuffer* CBundleDataHandle::GetBufferInternal() const
{
	return m_buffer;
}