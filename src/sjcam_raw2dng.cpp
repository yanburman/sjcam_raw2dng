/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <vector>
#include <algorithm>

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#include "DNGConverter.h"
#include "FileFinder.h"
#include "helpers.h"
#include "utils.h"

#include <dng_globals.h>
#include <dng_string.h>
#include <dng_pthread.h>

#define VERSION_STR "v1.1.1"

static FileFinder g_Files;

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

static int handle_arg(const char *arg)
{
  struct stat sb;

  int ret = stat(arg, &sb);
  if (ret) {
    perror("stat");
    return ret;
  }

  std::string str_arg(arg);

  switch (sb.st_mode & S_IFMT) {
  case S_IFDIR:
    ret = g_Files.find_files(str_arg);
    break;

  case S_IFREG:
    ret = g_Files.find_file(str_arg);
    break;

  default:
    fprintf(stderr, "Only files/directories are supported\n");
    ret = -1;
  }

  return ret;
}

static void usage(const char *prog, Config &conf)
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
          "\t-p, --threads <NUM> Number of threads to run. Default: %d (0 or -1 for number of CPUS in the system)\n"
          "\t-c, --no-color      Do not apply color calibration (for color calibration)\n"
          "\t-m, --thumb         Add JPEG thumbnails (disabled by default to save disk space and conversion time)\n"
          "\t-o, --output <DIR>  Output dir (must exist)\n"
          "\t-t, --tiff          Write TIFF image to \"<file>.tiff\" (false by default)\n"
          "\t-d, --dng           Write DNG image to \"<file>.dng\" (used by default if no output is supplied)\n",
          "\t-r, --rotated       Image was taken in rotated orientation (false by default)\n",
          prog,
          conf.m_iThreads);
}

int main(int argc, char *argv[])
{
  Config conf;

  if (argc == 1) {
    usage(argv[0], conf);
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
      usage(argv[0], conf);
      return EXIT_SUCCESS;
#if qDNGValidate
    } else if (option.Matches("verbose", true)) {
      gVerbose = true;
#endif
    } else if (option.Matches("t", true) || option.Matches("-tiff", true)) {
      conf.m_bTiff = true;
    } else if (option.Matches("d", true) || option.Matches("-dng", true)) {
      conf.m_bDng = true;
    } else if (option.Matches("r", true) || option.Matches("-rotated", true)) {
      conf.m_bFlipped = true;
    } else if (option.Matches("p", true) || option.Matches("-threads", true)) {
      if (index + 1 < argc) {
        ++index;
        if (!isdigit(argv[index][0])) {
          fprintf(stderr, "Error: Missing number of threads\n");
          return EXIT_FAILURE;
        }
        conf.m_iThreads = atoi(argv[index]);
      } else {
        fprintf(stderr, "Error: Missing number of threads\n");
        return EXIT_FAILURE;
      }
    } else if (option.Matches("v", true) || option.Matches("-version", true)) {
      printf("Version: %s\n", VERSION_STR);
      return EXIT_SUCCESS;
#if 0
    } else if (option.Matches("l", true) || option.Matches("-no-lens", true)) {
      conf.m_bLensCorrections = false;
#endif
    } else if (option.Matches("c", true) || option.Matches("-no-color", true)) {
      conf.m_bNoCalibration = true;
    } else if (option.Matches("m", true) || option.Matches("-thumb", true)) {
      conf.m_bGenPreview = true;
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
      usage(argv[0], conf);
      return EXIT_FAILURE;
    }
  }

  if (index == argc) {
    fprintf(stderr, "Error: No file specified\n");
    return EXIT_FAILURE;
  }

  if (!conf.m_bTiff && !conf.m_bDng) {
    /* Most users want to convert to DNG */
    conf.m_bDng = true;
  }

  int rc;
  while (index < argc) {
    rc = handle_arg(argv[index++]);
    if (rc)
      return EXIT_FAILURE;
  }

  const std::vector<RawWorkItem *> o_WorkItems = g_Files.get_work_items();
  if (o_WorkItems.size() == 0) {
    printf("No raw files found\n");
    return EXIT_SUCCESS;
  }

  setvbuf(stdout, NULL, _IONBF, 0);

  DNGConverter converter(conf);

  size_t n_cpus;

  if (conf.m_iThreads == 0 || conf.m_iThreads == -1) {
    n_cpus = get_num_cpus();
  } else {
    n_cpus = (size_t)conf.m_iThreads;
  }

  n_cpus = std::min(n_cpus, o_WorkItems.size());
  if (n_cpus == 1) {
    std::vector<RawWorkItem *>::const_iterator it;

    for (it = o_WorkItems.begin(); it != o_WorkItems.end(); ++it)
      converter.ConvertToDNG((*it)->m_szRawFile, (*it)->m_szMetadataFile);
  } else {
    ThreadWork *works = new ThreadWork[n_cpus];
    pthread_t *threads = new pthread_t[n_cpus];

    unsigned int i;
    int res;

    size_t ulStart = 0;
    size_t ulTotal = 0;
    const size_t ulQuota = o_WorkItems.size() / n_cpus;
    const size_t ulLeft = o_WorkItems.size() - ulQuota * n_cpus;

    printf("Starting %zu threads to process files\n", n_cpus);

    for (i = 0; i < n_cpus; ++i) {
      works[i].oConverter = &converter;
      works[i].oWorks = &o_WorkItems;
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

    if (ulTotal != o_WorkItems.size()) {
      fprintf(stderr,
              "Error: Not all items consumed (total:%zu, consumed: %zu, n_cpus:%zu)\n",
              o_WorkItems.size(),
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

  return EXIT_SUCCESS;
}
