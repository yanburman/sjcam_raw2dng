/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include "utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#else
#include <dirent.h>
#include <unistd.h>
#include <pthread.h>
#endif

const std::string jpeg_suffix(".JPG");
const std::string raw_suffix(".RAW");
const std::string dng_suffix(".dng");

int list_dir(const std::string &dir, std::list<std::string> &files, const std::list<std::string> &filter)
#if defined(_WIN32) || defined(_WIN64)
{
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  DWORD dwError = 0;

  // Find the first file in the directory.

  const std::string files_to_list = dir + "\\*";

  hFind = FindFirstFile(files_to_list.c_str(), &ffd);

  if (INVALID_HANDLE_VALUE == hFind)
    return GetLastError();

  // List all the files in the directory with some info about them.

  do {
    if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      std::string fname(ffd.cFileName);

      std::list<std::string>::const_iterator it;
      for (it = filter.begin(); it != filter.end(); ++it) {
        if (has_suffix(fname, *it)) {
          files.push_back(fname);
          break;
        }
      }
    }
  } while (FindNextFile(hFind, &ffd) != 0);

  dwError = GetLastError();
  if (dwError == ERROR_NO_MORE_FILES)
    dwError = 0;

  FindClose(hFind);

  files.sort();

  return dwError;
}
#else
{
  DIR *dp;
  struct dirent *dirp;

  if ((dp = opendir(dir.c_str())) == NULL) {
    perror("opendir");
    return errno;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if (dirp->d_type == DT_REG) {
      std::string fname(dirp->d_name);

      std::list<std::string>::const_iterator it;
      for (it = filter.begin(); it != filter.end(); ++it) {
        if (has_suffix(fname, *it)) {
          files.push_back(fname);
          break;
        }
      }
    }
  }
  closedir(dp);

  files.sort();

  return 0;
}
#endif

size_t get_num_cpus(void)
#if defined(_WIN32) || defined(_WIN64)
{
  SYSTEM_INFO siSysInfo;
  GetSystemInfo(&siSysInfo);

  return siSysInfo.dwNumberOfProcessors;
}
#else
{
  return sysconf(_SC_NPROCESSORS_ONLN);
}
#endif

void set_thread_prio_low(void)
#if defined(_WIN32) || defined(_WIN64)
{
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
}
#else
{
  struct sched_param param;
  param.sched_priority = 20;
  pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
}
#endif
