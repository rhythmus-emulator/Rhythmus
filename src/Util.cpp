#include "Util.h"
#include "rutil.h"
#include <stack>

#ifdef WIN32
# include <Windows.h>
# include <sys/types.h>
# include <sys/stat.h>
# define stat _wstat
#else
# include <unistd.h>
#endif

namespace rhythmus
{

/* copied from rparser/rutil */
  
#ifdef WIN32
bool GetDirectoryItems(const std::string& dirpath, std::vector<DirItem>& out)
{
  HANDLE hFind = INVALID_HANDLE_VALUE;
  WIN32_FIND_DATAW ffd;
  std::wstring spec, wpath, mask=L"*";
  std::stack<std::wstring> directories; // currently going-to-search directories
  std::stack<std::wstring> dir_name;    // name for current directories
  std::wstring curr_dir_name;

  rutil::DecodeToWStr(dirpath, wpath, rutil::E_UTF8);
  directories.push(wpath);
  dir_name.push(L"");

  while (!directories.empty()) {
    wpath = directories.top();
    spec = wpath + L"\\" + mask;
    curr_dir_name = dir_name.top();
    directories.pop();
    dir_name.pop();

    hFind = FindFirstFileW(spec.c_str(), &ffd);
    if (hFind == INVALID_HANDLE_VALUE)  {
      return false;
    } 

    do {
      if (wcscmp(ffd.cFileName, L".") != 0 && wcscmp(ffd.cFileName, L"..") != 0)
      {
        int is_file = 0;
        if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
          /// will not recursive currently...
          //directories.push(wpath + L"\\" + ffd.cFileName);
          //dir_name.push(curr_dir_name + ffd.cFileName + L"\\");
        }
        else {
          is_file = 1;
        }
        std::string fn;
        std::wstring wfn = curr_dir_name + ffd.cFileName;
        struct _stat result;
        stat(wfn.c_str(), &result);
        rutil::EncodeFromWStr(wfn, fn, rutil::E_UTF8);
        out.push_back({
          fn, is_file, result.st_mtime
          });
      }
    } while (FindNextFileW(hFind, &ffd) != 0);

    if (GetLastError() != ERROR_NO_MORE_FILES) {
      FindClose(hFind);
      return false;
    }

    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
  }

  return true;
}
#else
bool GetDirectoryItems(const std::string& path, std::vector<DirItem>& out)
{
  DIR *dp;
  struct dirent *dirp;
  std::stack<std::string> directories;
  std::stack<std::string> dir_name;

  directories.push(path);
  dir_name.push("");

  std::string curr_dir_name;
  std::string curr_dir;

  while (directories.size() > 0)
  {
    curr_dir = directories.top();
    curr_dir_name = dir_name.top();
    directories.pop();
    dir_name.pop();
    if (curr_dir_name == "./" || curr_dir_name == "../")
      continue;
    if((dp  = opendir(curr_dir.c_str())) == NULL) {
      printf("Error opening %s dir.\n", curr_dir.c_str());
      return false;
    }
    while ((dirp = readdir(dp)) != NULL) {
      int is_file = 0;
      if (dirp->d_type == DT_DIR)
      {
        //directories.push(curr_dir + "/" + dirp->d_name);
        //dir_name.push(curr_dir_name + dirp->d_name + "/");
      }
      else if (dirp->d_type == DT_REG)
      {
        is_file = 1;
      }
      std::string fn = curr_dir_name + std::string(dirp->d_name);
      struct _stat result;
      stat(fn.c_str(), &result);
      out.push_back({
        fn, is_file, result.st_mtime
        });
    }
    closedir(dp);
  }

  return true;
}
#endif

}