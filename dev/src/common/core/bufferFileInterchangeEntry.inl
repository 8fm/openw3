namespace Red
{
	namespace Core
	{
		namespace Bundle
		{
			//////////////////////////////////////////////////////////////////////////
			CBufferFileInterchangeEntry::CBufferFileInterchangeEntry( const CBundleMetadataStore* store, const SMetadataFileInBundleRange& data, const Uint32 dataBlockOffset )
				: m_store( store )
				, m_data( data )
				, m_dataBlockOffset( data.m_offsetInBundle - dataBlockOffset )
			{
			}
			
			//////////////////////////////////////////////////////////////////////////
			CBufferFileInterchangeEntry::CBufferFileInterchangeEntry( const CBufferFileInterchangeEntry& entry )
				: m_store( entry.m_store )
				, m_data( entry.m_data )
				, m_dataBlockOffset( entry.m_dataBlockOffset )
			{
			}

			//////////////////////////////////////////////////////////////////////////
			CBufferFileInterchangeEntry& CBufferFileInterchangeEntry::operator=( const CBufferFileInterchangeEntry& entry )
			{
				RED_UNUSED( entry );
				return *this;
			}

			//////////////////////////////////////////////////////////////////////////
			const String CBufferFileInterchangeEntry::GetPath() const
			{
				return ANSI_TO_UNICODE( GetPathPtr() );
			}

			//////////////////////////////////////////////////////////////////////////
			const AnsiChar* CBufferFileInterchangeEntry::GetPathPtr() const
			{
				return m_store->GetFilePath( m_data.m_fileID );
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBufferFileInterchangeEntry::GetSizeInBundle() const
			{
				return m_data.m_sizeInBundle;
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBufferFileInterchangeEntry::GetSizeInMemory() const
			{
				return m_store->GetFileSize( m_data.m_fileID );
			}
			
			//////////////////////////////////////////////////////////////////////////
			Uint32 CBufferFileInterchangeEntry::GetAbsoluteOffset() const
			{
				return m_data.m_offsetInBundle;
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBufferFileInterchangeEntry::GetRelativeOffset() const
			{
				return m_dataBlockOffset;
			}

			//////////////////////////////////////////////////////////////////////////
			ECompressionType CBufferFileInterchangeEntry::GetCompressionType() const
			{
				return m_store->GetFileCompressionType( m_data.m_fileID );
			}

			//////////////////////////////////////////////////////////////////////////
			CBundleDataBlockInterchangeEntry::CBundleDataBlockInterchangeEntry()
				: m_dataBlockSize( 0 )
				, m_dataBlockOffset( 0 )
				, m_burstDataBlockSize( 0 )
			{
			}

			//////////////////////////////////////////////////////////////////////////
			CBundleDataBlockInterchangeEntry::CBundleDataBlockInterchangeEntry( const SMetadataBundleEntry& entry )
				: m_dataBlockSize( entry.m_dataBlockSize )
				, m_dataBlockOffset( entry.m_dataBlockOffset )
				, m_burstDataBlockSize( entry.m_burstDataBlockSize )
			{
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBundleDataBlockInterchangeEntry::GetDataBlockSize() const
			{
				return m_dataBlockSize;
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBundleDataBlockInterchangeEntry::GetDataBlockOffset() const
			{
				return m_dataBlockOffset;
			}

			//////////////////////////////////////////////////////////////////////////
			Uint32 CBundleDataBlockInterchangeEntry::GetBurstReadSize() const
			{
				return m_burstDataBlockSize;
			}
		}
	}
}