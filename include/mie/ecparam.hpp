#pragma once
/**
	@file
	@brief Elliptic curve parameter
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <mie/ec.hpp>

namespace mie { namespace ecparam {

const struct mie::EcParam secp192k1 = {
	"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	"0",
	"3",
	"0xdb4ff10ec057e9ae26b07d0280b7f4341da5d1b1eae06c7d",
	"0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9d",
	"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d"
};

} } // mie::ecparam

