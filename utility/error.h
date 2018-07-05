#ifndef ERROR_H
#define ERROR_H

#include <string>
#include <cerrno>

std::string errno_error_description(int error_number = errno);

#endif //ERROR_H