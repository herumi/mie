#pragma once
/**
	@file
	@brief exception
	@author MITSUNARI Shigeo(@herumi)
	@license modified new BSD license
	http://opensource.org/licenses/BSD-3-Clause
*/
#include <sstream>

namespace mie {

struct Exception : std::exception {
	std::string str_;
	virtual ~Exception() throw() {}
	explicit Exception(const std::string& msg1, const std::string& msg2 = "")
		: str_(msg1 + msg2)
	{
	}
	template<class T>
	Exception& operator<<(const T& t)
	{
		std::ostringstream os;
		os << t;
		str_ += os.str();
		return *this;
	}
	const char *what() const throw() { return str_.c_str(); }
};

} // mie

