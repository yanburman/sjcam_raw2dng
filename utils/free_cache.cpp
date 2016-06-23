/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <stdio.h>
#include <windows.h>
#include <list>
#include <string>

static int list_dir(const std::string dir, std::list<std::string> &files)
{
  WIN32_FIND_DATA ffd;
  HANDLE hFind = INVALID_HANDLE_VALUE;
  DWORD dwError = 0;

  // Find the first file in the directory.

  const std::string files_to_list = dir + "\\*.RAW";

  hFind = FindFirstFile(files_to_list.c_str(), &ffd);

  if (INVALID_HANDLE_VALUE == hFind)
    return GetLastError();

  // List all the files in the directory with some info about them.

  do {
    if (!(ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
      std::string fname(ffd.cFileName);
      files.push_back(fname);
    }
  } while (FindNextFile(hFind, &ffd) != 0);

  dwError = GetLastError();
  if (dwError == ERROR_NO_MORE_FILES)
    dwError = 0;

  FindClose(hFind);

  return dwError;
}

int main(int argc, char *argv[])
{
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <dir to clear cache for>\n", argv[0]);
    return 0;
  }

  std::list<std::string> files;
  int ret = list_dir(argv[1], files) if (ret) return 1;

  std::list<std::string>::const_iterator it;
  for (it = files.begin(); it != files.end(); ++it) {
    HANDLE fd = CreateFile(it->c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FFILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING,
                           NULL);
    CloseHandle(fd);
  }

  return 0;
}
