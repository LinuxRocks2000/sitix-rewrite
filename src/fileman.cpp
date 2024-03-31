// definitions for FileMan

#include <fileman.hpp>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <sitixwriter.hpp>


std::string fconcat(std::string one, std::string two) { // sanely glue two filenames together (useful for things like "output-dir" + "test.html")
    if (one[one.size() - 1] == '/' && two[0] == '/') {
        return one.substr(0, one.size() - 1) + two;
    }
    else if (one[one.size() - 1] == '/' || two[0] == '/') {
        return one + two;
    }
    else {
        return one + '/' + two;
    }
}


FileMan::FileMan(std::string rdir) {
    dir = rdir;
    struct stat sb;
    if (stat(rdir.c_str(), &sb) == 0) {
        if (!S_ISDIR(sb.st_mode)) { // if the "folder" exists but is a regular file
            printf(ERROR "%s already exists and is not a directory. Fatal.", rdir.c_str());
            valid = false;
        }
    }
    else {
        mkdir(rdir.c_str(), 0);
        chmod(rdir.c_str(), 0755);
    }
}

FileMan::PathState FileMan::checkPath(std::string path) {
    struct stat sb;
    if (stat(fconcat(dir, path).c_str(), &sb) == 0) {
        if (S_ISDIR(sb.st_mode)) {
            return FileMan::PathState::Directory;
        }
        else if (S_ISREG(sb.st_mode)) {
            return FileMan::PathState::File;
        }
        else {
            return FileMan::PathState::Other;
        }
    }
    else if (errno == ENOENT) {
        return FileMan::PathState::CNEP;
    }
    else {
        return FileMan::PathState::Error;
    }
}

bool FileMan::empty() { // returns whether the directory was emptied
    struct stat sb;
    std::string dotsitix = fconcat(dir, ".sitix");
    if (stat(dotsitix.c_str(), &sb) == -1) {
        printf(WARNING "The directory %s does not appear to be managed by Sitix. This may be because this is the first build."
        "\n\tRemember that Sitix will delete all the contents of %s before rendering!"
        "\n\tAre you sure you want to \033[1mfully delete\033[0m the contents of %s and render this Sitix project to it? Y/N: ", dir.c_str(), dir.c_str(), dir.c_str());
        std::string line;
        std::cin >> line;
        if (line[0] != 'y' && line[0] != 'Y') {
            return false;
        }
    }
    rmrf(dir.c_str()); // todo: process errors from these guys
    mkdir(dir.c_str(), 0);
    mode_t mask = umask(0);
    chmod(dir.c_str(), 0775);
    umask(mask);
    create(dotsitix);
    return true;
}

FileWriteOutput FileMan::create(std::string name) {
    name = transmuted(name);
    mkdirR(name);
    mode_t mask = umask(0);
    int output = ::open(name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0770); // rw for user and group
    // todo: sane permission inheritance (ACL doohickey?)
    umask(mask);
    if (output == -1) {
        printf(ERROR "Couldn't open output file %s. This file will not be rendered.\n", name.c_str());
    }
    FileWriteOutput fOut(output);
    return fOut;
}

MapView FileMan::open(std::string name) {
    if (!maps.contains(name)) {
        maps.insert({ name, MapView(name) });
    }
    return maps.at(name);
}

std::string FileMan::transmuted(std::string path) {
    return fconcat(dir, path);
}

std::string FileMan::arcTransmuted(std::string path) {
    if (path.size() <= dir.size()) {
        return "";
    }
    std::string ret = path.substr(dir.size() + (dir[dir.size() - 1] == '/' ? 0 : 1), path.size());
    return ret;
}