/**
 * Copyright © 2015 CD Projekt Red. All Rights Reserved.
 */

#ifndef _RED_MEMORY_PROXY_TYPE_ID_HPP_
#define _RED_MEMORY_PROXY_TYPE_ID_HPP_

namespace red
{
namespace memory
{
	template<typename T, typename = void>
	struct HasType : std::false_type 
	{};

	template<typename T>
	struct HasType< T > : std::true_type 
	{
		typedef T Type;
	};

	template< typename ProxyType, typename = std::true_type >
	struct ProxyHasTypeId : std::false_type {}; // enum TypeId is not part of the allocator!

	template< typename ProxyType >
	struct ProxyHasTypeId
	<
		ProxyType,
		std::integral_constant
		<
			bool,
			std::is_enum< typename HasType< typename ProxyType::TypeIdEnum >::Type >::value
		>
	> : std::true_type {};// enum TypeId is part of the proxy.
}
}

#endif
