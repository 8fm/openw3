/**
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "sha256.h"

#ifdef RED_USE_CRYPTO

// For std::type_info as referenced in cryptopp.
// Unforuntately needs RTTI atm.
#ifdef RED_COMPILER_MSC
#include <typeinfo>
namespace std { typedef ::type_info type_info; }
#endif

#include "../../../external/cryptopp562/cryptlib.h"
#include "../../../external/cryptopp562/sha.h"

static_assert( CryptoPP::SHA256::DIGESTSIZE == sizeof(Ssha256), "SHA256 size mismatch?" );

namespace CryptoHelpers
{
	static void UpdateFileSha256( IFile& file, CryptoPP::SHA256& outHash )
	{
		const Uint32 BUFSZ = 4096;
		Uint8 buf[BUFSZ];
		Uint64 toRead = file.GetOffset() <= file.GetSize() ? file.GetSize() - file.GetOffset() : 0;
		while ( toRead > 0 )
		{
			const Uint64 chunkSize = toRead > BUFSZ ? BUFSZ : toRead;
			toRead -= chunkSize;
			file.Serialize( buf, chunkSize );
			outHash.Update( reinterpret_cast<const byte*>(buf), chunkSize );
 		}
	}
}

void CalculateSha256( IFile& file, Ssha256& outHash )
{
	Red::System::MemoryZero( outHash.m_value, sizeof(outHash.m_value) );
	CryptoPP::SHA256 hash;
	CryptoHelpers::UpdateFileSha256( file, hash );
	hash.Final(outHash.m_value);
}

void CalculateSha256( IFile& file, void* extraData, size_t extraDataSize, Ssha256& outHash )
{
	Red::System::MemoryZero( outHash.m_value, sizeof(outHash.m_value) );
	CryptoPP::SHA256 hash;
	CryptoHelpers::UpdateFileSha256( file, hash );
	hash.Update( reinterpret_cast<const byte*>(extraData), extraDataSize );
	hash.Final(outHash.m_value);
}

void CalculateSha256( void* buffer, size_t size, Ssha256& outHash )
{
	Red::System::MemoryZero( outHash.m_value, sizeof(outHash.m_value) );
	CryptoPP::SHA256 hash;
	hash.CalculateDigest( outHash.m_value, reinterpret_cast<const byte*>(buffer), size );
}

#endif // RED_USE_CRYPTO
