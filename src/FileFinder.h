/* vim: set shiftwidth=2 tabstop=2 softtabstop=2 expandtab: */

#ifndef __FILE_FIDER_H__
#define __FILE_FIDER_H__

#include <string>
#include <vector>

struct RawWorkItem {
  RawWorkItem(const std::string &szRawFile, const std::string &szMetadataFile)
          : m_szRawFile(szRawFile), m_szMetadataFile(szMetadataFile)
  {
  }

  const std::string m_szRawFile;
  const std::string m_szMetadataFile;
};

class FileFinder
{
  public:
  ~FileFinder()
  {
    cleanup_work_items();
  }

  int find_files(std::string &dir);
  int find_file(std::string &fname);

  const std::vector<RawWorkItem *> get_work_items(void)
  {
    return m_WorkItems;
  }

  protected:
  void add_work_item(const std::string &szRawFile, const std::string &szMetadataFile);

  void cleanup_work_items(void);

  std::vector<RawWorkItem *> m_WorkItems;
};

#endif // __FILE_FIDER_H__
