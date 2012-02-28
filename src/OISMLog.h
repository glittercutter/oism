#pragma once

#include <iostream>

#define LOG(_LINE) std::cout << "oism -- LOG: " << _LINE << std::endl;
#define WLOG(_LINE) std::cout << "oism -- WARNING: " << _LINE << std::endl;
#define ELOG(_LINE) std::cout << "oism -- ERROR: " << _LINE << " -- " __FILE__ << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
