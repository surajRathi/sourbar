#ifndef BAR_LOG_H
#define BAR_LOG_H

#include <iostream>

#define INFO(x) std::cout << x << std::endl
#define ERR(x) std::cout << "ERROR: " << x << std::endl


#ifndef NDEBUG

#define DEB(x) std::cout << x << std::endl

#else
#define DEB(x)
#endif //NDEBUG


#endif //BAR_LOG_H
