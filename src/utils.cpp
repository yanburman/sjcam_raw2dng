/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include "utils.h"

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode)&S_IFMT) == S_IFDIR)
#endif
#else
#include <dirent.h>
#include <unistd.h>
#endif

const static std::string jpeg_suffix(".JPG");

const std::string raw_suffix(".RAW");

int list_dir(const std::string &dir, std::list<std::string> &files)
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

      if (has_suffix(fname, raw_suffix) || has_suffix(fname, jpeg_suffix))
        files.push_back(fname);
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

      if (has_suffix(fname, raw_suffix) || has_suffix(fname, jpeg_suffix))
        files.push_back(fname);
    }
  }
  closedir(dp);

  files.sort();

  return 0;
}
#endif


