/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __HELPERS_H__
#define __HELPERS_H__

#if defined(_WIN32) || defined(_WIN64)
#define DELIM "\\"
#define DIR_DELIM '\\'
#else
#define DELIM "/"
#define DIR_DELIM '/'
#endif

#endif // __HELPERS_H__
