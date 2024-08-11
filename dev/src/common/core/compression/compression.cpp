/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"

#include "compression.h"

namespace Red { namespace Core {

namespace Compressor {

Base::Base()
{

}

Base::~Base()
{

}

} // namespace Compressor {

namespace Decompressor {

Base::Base()
{

}

Base::~Base()
{

}

EStatus Base::Initialize( const void*, void*, Uint32, Uint32 )
{
	return Status_Success;
}

} // namespace Decompressor {

} } // namespace Red { namespace Core {
