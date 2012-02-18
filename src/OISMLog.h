#pragma once

#include <iostream>

#define lprintln(_LINE) std::cout << "oism -- LOG: " << _LINE << std::endl;
#define wprintln(_LINE) std::cout << "oism -- WARNING: " << _LINE << std::endl;
#define eprintln(_LINE) std::cout << "oism -- ERROR: " << _LINE << " -- " __FILE__ << " " << __FUNCTION__ << " " << __LINE__ << std::endl;
