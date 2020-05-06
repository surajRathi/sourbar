#ifndef BAR_LOG_H
#define BAR_LOG_H

#endif //BAR_LOG_H

#ifndef NDEBUG
#include <iostream>
#define LOG(x) std::cout << x << std::endl
#else
#define LOG(x)
#endif
