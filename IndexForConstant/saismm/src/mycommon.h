#ifndef MYCOMMON_H
#define MYCOMMON_H

#include <iostream>
#include <limits>

#include "namespace.h"


WTL_BEG_NAMESPACE
//alias
using uint8 = uint8_t;
using uint32 = uint32_t;



//constexpr
constexpr uint8 L_TYPE = 0;
constexpr uint8 S_TYPE = 1;
constexpr uint8 B_TYPE = 2;


constexpr uint8 D_LOW = 5; 
constexpr uint8 D_HIGH = 4;

WTL_END_NAMESPACE
#endif
