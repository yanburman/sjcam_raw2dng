/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#include "FileFinder.h"
#include "utils.h"
#include "helpers.h"

#include <list>
#include <stdio.h>
#include <stdlib.h>

static const std::string empty_string;

void FileFinder::cleanup_work_items(void)
{
  std::vector<RawWorkItem *>::const_iterator it;

  for (it = m_WorkItems.begin(); it != m_WorkItems.end(); ++it)
    delete *it;

  m_WorkItems.clear();
}

void FileFinder::add_work_item(const std::string &szRawFile, const std::string &szMetadataFile)
{
  RawWorkItem *wi = new RawWorkItem(szRawFile, szMetadataFile);
  m_WorkItems.push_back(wi);
}

int FileFinder::find_files(std::string &dir)
{
  std::list<std::string> files;
  int ret = list_dir(dir, files);
  if (ret)
    return ret;

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

  return 0;
}

int FileFinder::find_file(std::string &fname)
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
    return 0;
  }

  std::string suffix = file_name.substr(suffix_idx + 1, file_name.length());
  int pic_num = atoi(suffix.c_str());
  if (pic_num < 1) {
    add_work_item(fname, res);
    return 0;
  }

  std::list<std::string> files;
  int ret = list_dir(dir_name, files);
  if (ret)
    return ret;

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

  return 0;
}
