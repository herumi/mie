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

const struct mie::EcParam secp160k1 = {
	"secp160k1",
	"0xFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFAC73",
	"0",
	"7",
	"0x3B4C382CE37AA192A4019E763036F4F5DD4D7EBB",
	"0x938CF935318FDCED6BC28286531733C3F03C4FEE",
	"0x100000000000000000001B8FA16DFAB9ACA16B6B3"
};
// p=2^160 + 7
const struct mie::EcParam p160_1 = {
	"p160_1",
	"0x10000000000000000000000000000000000000007",
	"10",
	"1343632762150092499701637438970764818528075565078",
	"1",
	"1236612389951462151661156731535316138439983579284",
	"1461501637330902918203683518218126812711137002561",
};
const struct mie::EcParam secp192k1 = {
	"secp192k1",
	"0xfffffffffffffffffffffffffffffffffffffffeffffee37",
	"0",
	"3",
	"0xdb4ff10ec057e9ae26b07d0280b7f4341da5d1b1eae06c7d",
	"0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9d",
	"0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d"
};

} } // mie::ecparam

