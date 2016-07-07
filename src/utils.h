/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __UTILS_H__
#define __UTILS_H__

#include <string>
#include <list>

static inline bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int list_dir(const std::string &dir, std::list<std::string> &files, const std::list<std::string> &filter);

extern const std::string jpeg_suffix;
extern const std::string raw_suffix;
extern const std::string dng_suffix;

size_t get_num_cpus(void);

void set_thread_prio_low(void);

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif

#endif // __UTILS_H__
