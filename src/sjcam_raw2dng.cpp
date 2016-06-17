/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <list>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#include "DNGConverter.h"
#include "helpers.h"
#include "utils.h"

#include <dng_globals.h>
#include <dng_string.h>
#include <dng_pthread.h>

#define VERSION_STR "v1.0.0"

struct RawWorkItem {
  RawWorkItem(const std::string &szRawFile, const std::string &szMetadataFile)
          : m_szRawFile(szRawFile), m_szMetadataFile(szMetadataFile)
  {
  }

  const std::string m_szRawFile;
  const std::string m_szMetadataFile;
};

struct ThreadWork {
  DNGConverter *oConverter;
  const std::vector<RawWorkItem *> *oWorks;
  size_t m_ulStart;
  size_t m_ulEnd;
};

static void *thread_worker(void *arg)
{
  ThreadWork *work = (ThreadWork *)arg;

  for (size_t i = work->m_ulStart; i < work->m_ulEnd; ++i)
    work->oConverter->ConvertToDNG((*(work->oWorks))[i]->m_szRawFile, (*(work->oWorks))[i]->m_szMetadataFile);

  return NULL;
}

static std::vector<RawWorkItem *> g_WorkItems;

static void cleanup_work_items(void)
{
  std::vector<RawWorkItem *>::const_iterator it;

  for (it = g_WorkItems.begin(); it != g_WorkItems.end(); ++it)
    delete *it;

  g_WorkItems.clear();
}

static void add_work_item(const std::string &szRawFile, const std::string &szMetadataFile)
{
  RawWorkItem *wi = new RawWorkItem(szRawFile, szMetadataFile);
  g_WorkItems.push_back(wi);
}

static const std::string empty_string;

static dng_error_code find_files(std::string &dir)
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

  dir += DELIM;

  for (it = files.begin(); it != files.end(); ++it) {
    if (found) {
      if (has_suffix(*it, jpg_suffix)) {
        res = dir + *it;
      }

      std::string fname = dir + *found_it;

      add_work_item(fname, res);
      found = false;
      res.clear();
    }

    if (has_suffix(*it, raw_suffix)) {
      size_t suffix_idx = it->find_last_of('_');
      if (suffix_idx == std::string::npos) {
        std::string fname = dir + *it;

        add_work_item(fname, res);
        found = false;
        res.clear();
        continue;
      }

      std::string suffix = it->substr(suffix_idx + 1, it->length());
      int pic_num = atoi(suffix.c_str());
      if (pic_num < 1) {
        std::string fname = dir + *it;

        add_work_item(fname, res);
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
    add_work_item(fname, empty_string);
  }

  return dng_error_none;
}

static dng_error_code find_file(std::string &fname)
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
  if (suffix_idx == std::string::npos) {
    add_work_item(fname, res);
    return dng_error_none;
  }

  std::string suffix = file_name.substr(suffix_idx + 1, file_name.length());
  int pic_num = atoi(suffix.c_str());
  if (pic_num < 1) {
    add_work_item(fname, res);
    return dng_error_none;
  }

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
      add_work_item(fname, res);
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
    add_work_item(fname, empty_string);
  }

  return dng_error_none;
}

static dng_error_code handle_arg(const char *arg)
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
    rc = find_files(str_arg);
    break;

  case S_IFREG:
    rc = find_file(str_arg);
    break;

  default:
    fprintf(stderr, "Only files/directories are supported\n");
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

  dng_error_code rc;
  while (index < argc) {
    rc = handle_arg(argv[index++]);
    if (rc != dng_error_none) {
      cleanup_work_items();
      return EXIT_FAILURE;
    }
  }

  if (g_WorkItems.size() == 0) {
    printf("No raw files found\n");
    return EXIT_SUCCESS;
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  DNGConverter converter(conf);

  size_t n_cpus = std::min(get_num_cpus(), g_WorkItems.size());
  if (n_cpus == 1) {
    ThreadWork work;
    work.oConverter = &converter;
    work.m_ulStart = 0;
    work.m_ulEnd = g_WorkItems.size();
    work.oWorks = &g_WorkItems;
    thread_worker(&work);
  } else {
    ThreadWork *works = new ThreadWork[n_cpus];
    pthread_t *threads = new pthread_t[n_cpus];

    unsigned int i;
    int res;

    size_t ulStart = 0;
    size_t ulTotal = 0;
    const size_t ulQuota = g_WorkItems.size() / n_cpus;
    const size_t ulLeft = g_WorkItems.size() - ulQuota * n_cpus;

    printf("Starting %zu threads to process files\n", n_cpus);

    for (i = 0; i < n_cpus; ++i) {
      works[i].oConverter = &converter;
      works[i].oWorks = &g_WorkItems;
      works[i].m_ulStart = ulStart;
      works[i].m_ulEnd = ulStart + ulQuota;

      ulTotal += ulQuota;

      if (i < ulLeft) {
        ++works[i].m_ulEnd;
        ++ulTotal;
      }

      ulStart = works[i].m_ulEnd;

      res = pthread_create(&threads[i], NULL, thread_worker, &works[i]);
      if (res) {
        fprintf(stderr, "Error: Unable to start thread: %u\n", i);
      }
    }

    if (ulTotal != g_WorkItems.size()) {
      fprintf(stderr,
              "Error: Not all items consumed (total:%zu, consumed: %zu, n_cpus:%zu)\n",
              g_WorkItems.size(),
              ulTotal,
              n_cpus);
    }

    void *result;
    for (i = 0; i < n_cpus; ++i) {
      pthread_join(threads[i], &result);
    }

    delete[] works;
    delete[] threads;
  }

  printf("Conversion complete\n");

  cleanup_work_items();

  return EXIT_SUCCESS;
}
