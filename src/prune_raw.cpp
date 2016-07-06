/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>

#include "FileFinder.h"

static FileFinder g_Files;

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

  default:
    fprintf(stderr, "Only directories are supported\n");
    ret = -1;
  }

  return ret;
}

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:  %s [options] dir\n"
          "\n"
          "Valid options:\n"
          "\t-h, --help          Help\n"
          "\t-i, --input <DIR>   Directory containing .dng files if they are not\n"
          "\t                    in the same folder with .RAW and .JPG\n",
          prog);
}

int main(int argc, char *argv[])
{
  if (argc == 1) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  std::string szInputFolder;
  int index;

  for (index = 1; index < argc && argv[index][0] == '-'; index++) {
    std::string option(&argv[index][1]);

    if (option == "h" || option == "-help") {
      usage(argv[0]);
      return EXIT_SUCCESS;
    } else if (option == "i" || option == "-input") {
      if (index + 1 < argc) {
        szInputFolder = argv[++index];
        struct stat sb;
        int ret = stat(szInputFolder.c_str(), &sb);
        if (ret) {
          perror("stat");
          return EXIT_FAILURE;
        }

        if (!S_ISDIR(sb.st_mode)) {
          fprintf(stderr, "Input directory not a directory\n");
          return EXIT_FAILURE;
        }

      } else {
        fprintf(stderr, "Error: Missing directory name\n");
        return EXIT_FAILURE;
      }
    } else {
      fprintf(stderr, "Error: Unknown option \"-%s\"\n", option.c_str());
      usage(argv[0]);
      return EXIT_FAILURE;
    }
  }

  if (index == argc) {
    fprintf(stderr, "Error: No directory specified\n");
    return EXIT_FAILURE;
  }

  if (index + 1 != argc) {
    fprintf(stderr, "Error: More than one directory specified\n");
    return EXIT_FAILURE;
  }

  int rc = handle_arg(argv[index++]);
  if (rc)
    return EXIT_FAILURE;

  const std::vector<RawWorkItem *> o_WorkItems = g_Files.get_work_items();
  if (o_WorkItems.size() == 0) {
    printf("No raw files found\n");
    return EXIT_SUCCESS;
  }

  std::vector<RawWorkItem *>::const_iterator it;

  for (it = o_WorkItems.begin(); it != o_WorkItems.end(); ++it)
    printf("Found: %s, %s\n", (*it)->m_szRawFile.c_str(), (*it)->m_szMetadataFile.c_str());

  printf("Prune complete\n");

  return EXIT_SUCCESS;
}
