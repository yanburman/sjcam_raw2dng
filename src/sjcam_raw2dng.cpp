/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include <stdio.h>
#include <string>

#include "DNGConverter.h"

#include <dng_globals.h>
#include <dng_string.h>

static void usage(const char *prog)
{
  fprintf(stderr,
          "Usage:  %s [options] file1 file2 ...\n"
          "\n"
          "Valid options:\n"
          "\t-v            Verbose mode\n"
          "\t-h            Help\n"
          "\t-no-lens      Do not apply lens corrections\n"
          "\t-tiff         Write TIFF image to \"<file>.tiff\"\n",
          prog);
}

static dng_error_code handle_arg(DNGConverter &converter, const char *arg)
{
  dng_error_code rc;

  std::string fname(arg);
  rc = converter.ConvertToDNG(fname);

  return rc;
}

int main(int argc, char *argv[])
{
  Config conf;

  if (argc == 1) {
    usage(argv[0]);
    return EXIT_FAILURE;
  }

  int index;

  gVerbose = false;
  for (index = 1; index < argc && argv[index][0] == '-'; index++) {
    dng_string option;

    option.Set(&argv[index][1]);

    if (option.Matches("v", true)) {
      gVerbose = true;
    } else if (option.Matches("h", true)) {
      usage(argv[0]);
      return EXIT_SUCCESS;
    } else if (option.Matches("tiff", true)) {
      conf.m_bTiff = true;
    } else if (option.Matches("no-lens", true)) {
      conf.m_bLensCorrections = false;
    } else {
      fprintf(stderr, "Error: Unknown option \"-%s\"\n", option.Get());
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
