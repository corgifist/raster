#include "common/zip.h"
#include "zip/zip.h"

namespace Raster {
    void ZIP::Extract(std::string t_zipFile, std::string t_outputDirectory) {
        zip_extract(t_zipFile.c_str(), t_outputDirectory.c_str(), nullptr, nullptr);
    }

    static void zip_walk(struct zip_t *zip, const char *path) {
        DIR *dir;
        struct dirent *entry;
        char fullpath[4096];
        struct stat s;
    
        memset(fullpath, 0, 4096);
        dir = opendir(path);
        assert(dir);
    
        while ((entry = readdir(dir))) {
          // skip "." and ".."
          if (!strcmp(entry->d_name, ".\0") || !strcmp(entry->d_name, "..\0"))
            continue;
    
          snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
          stat(fullpath, &s);
          if (S_ISDIR(s.st_mode))
            zip_walk(zip, fullpath);
          else {
            zip_entry_open(zip, fullpath);
            zip_entry_fwrite(zip, fullpath);
            zip_entry_close(zip);
          }
        }
    
        closedir(dir);
    }

    void ZIP::Pack(std::string t_zipFile, std::string t_inputDirectory) {
        auto zip = zip_open(t_zipFile.c_str(), 0, 'w');
        zip_walk(zip, t_inputDirectory.c_str());
        zip_close(zip);
    }
};