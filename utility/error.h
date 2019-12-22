#ifndef ERROR_H
#define ERROR_H

#include <cerrno>
#include <string>

std::string errno_error_description(int error_number = errno);

#endif //ERROR_H