#ifndef _APP_CONTEXT
#define _APP_CONTEXT

#include <array>

#include "utils/globals.hpp"

struct ConstantsContext
{
    std::array<float, (size_t)ConstantFloating::COUNT>
		constFloating = {};
	std::array<int, (size_t)ConstantNumeric::COUNT>
		constNumeric = {};
	std::array<bool, (size_t)ConstantBoolean::COUNT>
		constBoolean = {};
};

class AppContext
{

};

#endif // _APP_CONTEXT