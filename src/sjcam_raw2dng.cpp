/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <list>

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

#include "helpers.h"
#include "DNGConverter.h"

#include <dng_globals.h>
#include <dng_string.h>

#define VERSION_STR "v0.9.0"

struct CameraProfile {
  CameraProfile(uint32 w, uint32 h, uint32 black_level, double r, double g, double b, const char *name)
          : m_ulWidth(w), m_ulHeight(h), m_szCameraModel(name), m_ulBlackLevel(black_level), m_oNeutralWB(3)
  {
    m_ulFileSize = (m_ulWidth * m_ulHeight * 12) / 8;
    m_oNeutralWB[0] = r;
    m_oNeutralWB[1] = g;
    m_oNeutralWB[2] = b;
  }

  uint32 m_ulWidth;
  uint32 m_ulHeight;
  std::string m_szCameraModel;
  uint32 m_ulBlackLevel;
  uint32 m_ulFileSize;
  dng_vector m_oNeutralWB;
};

const static CameraProfile gRawSizes[] = {CameraProfile(4000, 3000, 0, 0.85, 1.2, 0.768769, "SJ5000X"),
                                          CameraProfile(4608, 3456, 200, 0.51, 1, 0.64, "M20")};

static const CameraProfile *get_CameraProfile(uint32 sz)
{
  const CameraProfile *oResult = NULL;

  for (unsigned int i = 0; i < sizeof(gRawSizes) / sizeof(gRawSizes[0]); ++i) {
    if (gRawSizes[i].m_ulFileSize == sz) {
      oResult = &gRawSizes[i];
      break;
    }
  }

  return oResult;
}

static dng_error_code handle_file(DNGConverter &converter,
                                  const std::string &fname,
                                  const std::string &metadata,
                                  uint32 sz)
{
  const CameraProfile *oProfile = get_CameraProfile(sz);
  if (NULL == oProfile) {
    fprintf(stderr, "%s: Unsupported format\n", fname.c_str());
    return dng_error_bad_format;
  }

  Exif exif(oProfile->m_ulWidth,
            oProfile->m_ulHeight,
            oProfile->m_ulBlackLevel,
            oProfile->m_oNeutralWB,
            oProfile->m_szCameraModel);

  if (!metadata.empty()) {
    printf("Found metadata in file: %s\n", metadata.c_str());
    int res = converter.ParseMetadata(metadata, exif);
    if (res)
      return dng_error_unknown;

    return converter.ConvertToDNG(fname, exif);
  }

  return converter.ConvertToDNG(fname, exif);
}

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

const static std::string jpeg_suffix(".JPG");
const static std::string raw_suffix(".RAW");

static int list_dir(const std::string &dir, std::list<std::string> &files)
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

static dng_error_code find_files(DNGConverter &converter, std::string &dir)
{
  std::list<std::string> files;
  int ret = list_dir(dir, files);
  if (ret)
    return dng_error_unknown;

  std::list<std::string>::iterator it, found_it = files.begin();
  bool found = false;
  char buffer[16];
  std::string jpg_suffix;
  std::string res;
  struct stat sb;

  dir += DELIM;

  for (it = files.begin(); it != files.end(); ++it) {
    if (found) {
      if (has_suffix(*it, jpg_suffix)) {
        res = dir + *it;
      }

      std::string fname = dir + *found_it;
      ret = stat(fname.c_str(), &sb);
      if (ret) {
        perror("stat");
        return dng_error_unknown;
      }

      handle_file(converter, fname, res, sb.st_size);
      found = false;
      res.clear();
      continue;
    }

    if (has_suffix(*it, raw_suffix)) {
      size_t suffix_idx = it->find_last_of('_');
      if (suffix_idx == std::string::npos) {
        std::string fname = dir + *it;
        ret = stat(fname.c_str(), &sb);
        if (ret) {
          perror("stat");
          return dng_error_unknown;
        }

        handle_file(converter, fname, res, sb.st_size);
        found = false;
        res.clear();
        continue;
      }

      std::string suffix = it->substr(suffix_idx + 1, it->length());
      int pic_num = atoi(suffix.c_str());
      if (pic_num < 1) {
        std::string fname = dir + *it;
        ret = stat(fname.c_str(), &sb);
        if (ret) {
          perror("stat");
          return dng_error_unknown;
        }

        handle_file(converter, fname, res, sb.st_size);
        found = false;
        res.clear();
        continue;
      }

      snprintf(buffer, sizeof(buffer), "_%03u.JPG", pic_num + 1);
      jpg_suffix = buffer;

      found_it = it;
      found = true;
    }
  }

  if (found) {
    std::string fname = dir + *found_it;
    ret = stat(fname.c_str(), &sb);
    if (ret) {
      perror("stat");
      return dng_error_unknown;
    }
    handle_file(converter, fname, "", sb.st_size);
  }

  return dng_error_none;
}

static dng_error_code find_file(DNGConverter &converter, std::string &fname, uint32 sz)
{
  std::string dir_name;
  std::string file_name;
  std::string res;

  size_t idx = fname.find_last_of(DIR_DELIM);
  if (idx == std::string::npos) {
    dir_name = ".";
    file_name = fname;
  } else {
    dir_name = fname.substr(0, idx);
    file_name = fname.substr(idx + 1, fname.length());
  }

  size_t suffix_idx = file_name.find_last_of('_');
  if (suffix_idx == std::string::npos)
    return handle_file(converter, fname, res, sz);

  std::string suffix = file_name.substr(suffix_idx + 1, file_name.length());
  int pic_num = atoi(suffix.c_str());
  if (pic_num < 1)
    return handle_file(converter, fname, res, sz);

  std::list<std::string> files;
  int ret = list_dir(dir_name, files);
  if (ret)
    return dng_error_unknown;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "_%03u.JPG", pic_num + 1);

  dir_name += DELIM;

  const std::string jpg_suffix(buffer);
  std::list<std::string>::iterator it, found_it = files.begin();
  bool found = false;
  for (it = files.begin(); it != files.end(); ++it) {
    if (found) {
      if (has_suffix(*it, jpg_suffix)) {
        res = dir_name + *it;
      }
      handle_file(converter, fname, res, sz);
      found = false;
      break;
    }

    if (*it == file_name) {
      found_it = it;
      found = true;
    }
  }

  if (found) {
    res = dir_name + *found_it;
    handle_file(converter, res, "", sz);
  }

  return dng_error_none;
}

static dng_error_code handle_arg(DNGConverter &converter, const char *arg)
{
  dng_error_code rc;
  struct stat sb;

  int ret = stat(arg, &sb);
  if (ret) {
    perror("stat");
    return dng_error_unknown;
  }

  std::string str_arg(arg);

  switch (sb.st_mode & S_IFMT) {
  case S_IFDIR:
    rc = find_files(converter, str_arg);
    break;

  case S_IFREG:
    rc = find_file(converter, str_arg, sb.st_size);
    break;

  default:
    fprintf(stderr, "Only files/directories are supported");
    rc = dng_error_unknown;
  }

  return rc;
}

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:  %s [options] file1|dir1 file2|dir2 ...\n"
          "\n"
          "Valid options:\n"
#if qDNGValidate
          "\t-verbose            Verbose mode\n"
#endif
          "\t-h, --help          Help\n"
          "\t-v, --version       Print version info and exit\n"
#if 0
          "\t-l, --no-lens       Do not apply lens corrections\n"
#endif
          "\t-c, --no-color      Do not apply color calibration (for color calibration)\n"
          "\t-o, --output <DIR>  Output dir (must exist)\n"
          "\t-t, --tiff          Write TIFF image to \"<file>.tiff\"\n",
          prog);
}

int main(int argc, char *argv[])
{
  Config conf;

  if (argc == 1) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  int index;

#if qDNGValidate
  gVerbose = false;
#endif

  for (index = 1; index < argc && argv[index][0] == '-'; index++) {
    dng_string option;

    option.Set(&argv[index][1]);

    if (option.Matches("h", true) || option.Matches("-help", true)) {
      usage(argv[0]);
      return EXIT_SUCCESS;
#if qDNGValidate
    } else if (option.Matches("verbose", true)) {
      gVerbose = true;
#endif
    } else if (option.Matches("t", true) || option.Matches("-tiff", true)) {
      conf.m_bTiff = true;
    } else if (option.Matches("v", true) || option.Matches("-version", true)) {
      printf("Version: %s\n", VERSION_STR);
      return EXIT_SUCCESS;
#if 0
    } else if (option.Matches("l", true) || option.Matches("-no-lens", true)) {
      conf.m_bLensCorrections = false;
#endif
    } else if (option.Matches("c", true) || option.Matches("-no-color", true)) {
      conf.m_bNoCalibration = true;
    } else if (option.Matches("o", true) || option.Matches("-output", true)) {
      if (index + 1 < argc) {
        conf.m_szPathPrefixOutput = argv[++index];
        struct stat sb;
        int ret = stat(conf.m_szPathPrefixOutput.c_str(), &sb);
        if (ret) {
          perror("stat");
          return EXIT_FAILURE;
        }

        if (!S_ISDIR(sb.st_mode)) {
          fprintf(stderr, "Output directory not a directory\n");
          return EXIT_FAILURE;
        }

        conf.m_szPathPrefixOutput += DELIM;
      } else {
        fprintf(stderr, "Error: Missing directory name\n");
        return EXIT_FAILURE;
      }
    } else {
      fprintf(stderr, "Error: Unknown option \"-%s\"\n", option.Get());
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (index == argc) {
    fprintf(stderr, "Error: No file specified\n");
    return EXIT_FAILURE;
  }

  DNGConverter converter(conf);
  dng_error_code rc;
  while (index < argc) {
    rc = handle_arg(converter, argv[index++]);
    if (rc != dng_error_none)
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
