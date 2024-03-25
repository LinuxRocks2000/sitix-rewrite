// definitions for FileMan

#include <fileman.hpp>
#include <sys/stat.h>


FileMan::FileMan(std::string rdir) {
    dir = rdir;
    struct stat sb;
    if (stat(rdir.c_str(), &sb) == 0 && !S_ISDIR(sb.st_mode)) { // if the "folder" exists but is a regular file
        printf(ERROR "%s already exists and is not a directory. Fatal.", rdir.c_str());
        valid = false;
    }
}

void FileMan::empty() {
    
}