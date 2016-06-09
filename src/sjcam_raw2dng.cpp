/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <list>

#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "DNGConverter.h"

#include <dng_globals.h>
#include <dng_string.h>

static dng_error_code handle_file(DNGConverter &converter, const std::string &fname, const std::string &metadata)
{
  if (!metadata.empty()) {
    printf("Found metadata in file: %s\n", metadata.c_str());
    Exif exif;
    int res = converter.ParseMetadata(metadata, exif);
    if (res)
      return dng_error_unknown;

    return converter.ConvertToDNG(fname, exif);
  }

  return converter.ConvertToDNG(fname);
}

static bool has_suffix(const std::string &str, const std::string &suffix)
{
  return str.size() >= suffix.size() && str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

static int list_dir(const std::string &dir, std::list<std::string> &files)
{
  DIR *dp;
  struct dirent *dirp;

  if ((dp = opendir(dir.c_str())) == NULL) {
    perror("opendir");
    return errno;
  }

  while ((dirp = readdir(dp)) != NULL) {
    if (dirp->d_type == DT_REG && !strstr(dirp->d_name, ".dng") && !strstr(dirp->d_name, ".DNG"))
      files.push_back(std::string(dirp->d_name));
  }
  closedir(dp);

  files.sort();

  return 0;
}

static dng_error_code find_files(DNGConverter &converter, std::string &dir)
{
  std::list<std::string> files;
  int ret = list_dir(dir, files);
  if (ret)
    return dng_error_unknown;

  std::list<std::string>::iterator it, found_it = files.begin();
  bool found = false;
  const std::string raw_suffix = ".RAW";
  char buffer[16];
  std::string jpg_suffix;
  std::string res;

  dir += "/";

  for (it = files.begin(); it != files.end(); ++it) {
    if (found) {
      if (has_suffix(*it, jpg_suffix)) {
        res = dir + *it;
      }
      handle_file(converter, dir + *found_it, res);
      found = false;
      res.clear();
      continue;
    }

    if (has_suffix(*it, raw_suffix)) {
      size_t suffix_idx = it->find_last_of('_');
      if (suffix_idx == std::string::npos) {
        handle_file(converter, dir + *it, res);
        continue;
      }

      std::string suffix = it->substr(suffix_idx + 1, it->length());
      int pic_num = atoi(suffix.c_str());
      if (pic_num < 1) {
        handle_file(converter, dir + *it, res);
        continue;
      }

      snprintf(buffer, sizeof(buffer), "_%03u.JPG", pic_num + 1);
      jpg_suffix = buffer;

      found_it = it;
      found = true;
    }
  }

  return dng_error_none;
}

static dng_error_code find_file(DNGConverter &converter, std::string &fname)
{
  std::string dir_name;
  std::string file_name;
  std::string res;

  size_t idx = fname.find_last_of('/');
  if (idx == std::string::npos) {
    dir_name = ".";
    file_name = fname;
  } else {
    dir_name = fname.substr(0, idx);
    file_name = fname.substr(idx + 1, fname.length());
  }

  size_t suffix_idx = file_name.find_last_of('_');
  if (suffix_idx == std::string::npos)
    return handle_file(converter, fname, res);

  std::string suffix = file_name.substr(suffix_idx + 1, file_name.length());
  int pic_num = atoi(suffix.c_str());
  if (pic_num < 1)
    return handle_file(converter, fname, res);

  std::list<std::string> files;
  int ret = list_dir(dir_name, files);
  if (ret)
    return dng_error_unknown;

  char buffer[16];
  snprintf(buffer, sizeof(buffer), "_%03u.JPG", pic_num + 1);

  const std::string jpg_suffix(buffer);
  std::list<std::string>::iterator it, found_it = files.begin();
  bool found = false;
  for (it = files.begin(); it != files.end(); ++it) {
    if (found) {
      if (has_suffix(*it, jpg_suffix)) {
        res = dir_name + "/" + *it;
      }
      handle_file(converter, fname, res);
      break;
    }

    if (*it == file_name) {
      found_it = it;
      found = true;
    }
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
    rc = find_file(converter, str_arg);
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
          "Usage:  %s [options] file1 file2 ...\n"
          "\n"
          "Valid options:\n"
#if qDNGValidate
          "\t-v            Verbose mode\n"
#endif
          "\t-h            Help\n"
          "\t-no-lens      Do not apply lens corrections\n"
          "\t-no-color     Do not apply color calibration (for color calibration)\n"
          "\t-tiff         Write TIFF image to \"<file>.tiff\"\n",
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

    if (option.Matches("h", true)) {
      usage(argv[0]);
      return EXIT_SUCCESS;
#if qDNGValidate
    } else if (option.Matches("v", true)) {
      gVerbose = true;
#endif
    } else if (option.Matches("tiff", true)) {
      conf.m_bTiff = true;
    } else if (option.Matches("no-lens", true)) {
      conf.m_bLensCorrections = false;
    } else if (option.Matches("no-color", true)) {
      conf.m_bNoCalibration = true;
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
