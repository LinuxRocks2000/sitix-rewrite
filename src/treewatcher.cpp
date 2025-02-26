#include <treewatcher.hpp>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h> // TODO: pathconf things
#include <sys/stat.h>
#include <session.hpp>


void WatchedPath::addDep(WatchedPath* dep) {
    bool exists = false;
    for (WatchedPath* file : dependants) {
        if (file == dep) {
            exists = true;
            break;
        }
    } // if it's not already a dependency here
    if (!exists) {
        dependants.push_back(dep);
    }
}

void WatchedPath::rmDep(WatchedPath* dep) {
    for (size_t i = 0; i < dependants.size(); i ++) {
        if (dependants[i] == dep) {
            dependants[i] = dependants[dependants.size() - 1]; // swap-remove
            dependants.pop_back();
        }
    }
}

void WatchedPath::treeModify(std::function<void(std::string)> onModify) {
    for (size_t i = 0; i < dependants.size(); i ++) {
        onModify(dependants[i] -> path);
        dependants[i] -> treeModify(onModify);
    }
}


TreeWatcher::TreeWatcher() {
    inotifier = inotify_init();
}

WatchedPath* TreeWatcher::filewatch(std::string file) {
    for (WatchedPath* f : files) {
        if (f -> path == file) {
            return f;
        }
    } // if it doesn't already exist, create it
    WatchedPath* f = new WatchedPath {
        .path = file,
        .watcher = inotify_add_watch(inotifier, file.c_str(), IN_CLOSE_WRITE)
    };
    files.push_back(f);
    return f;
}

WatchedPath* TreeWatcher::dirwatch(std::string path) { // clone of filewatch with different flags
    // todo: make this not copy/pasted
    for (WatchedPath* d : files) {
        if (d -> path == path) {
            return d;
        }
    } // if it doesn't already exist, create it
    WatchedPath* d = new WatchedPath {
        .path = path,
        .watcher = inotify_add_watch(inotifier, path.c_str(), IN_CREATE | IN_DELETE | IN_MOVED_FROM | IN_MOVED_TO)
    };
    files.push_back(d);
    return d;
}

void TreeWatcher::unwatch(std::string path) {
    for (size_t i = 0; i < files.size(); i ++) {
        if (files[i] -> path == path) {
            WatchedPath* f = files[i];
            files[i] = files[files.size() - 1];
            files.pop_back();
            inotify_rm_watch(inotifier, f -> watcher);
            delete f;
            for (WatchedPath* alive : files) {
                alive -> rmDep(f);
            }
            return;
        }
    }
}

void TreeWatcher::waitForModifications(Session* sitix, std::function<void(std::string)> onModify, std::function<void(std::string)> onDelete) {
    char buffer[sizeof(struct inotify_event) + NAME_MAX + 1]; // the manual entry for inotify guarantees this to be a large enough buffer for a single inotify event
    read(inotifier, buffer, sizeof(buffer));
    struct inotify_event* evt = (struct inotify_event*)buffer;
    std::string absname;
    if (evt -> len > 0) { // it's a filename in a directory
        for (WatchedPath* dir : files) {
            if (dir -> watcher == evt -> wd) {
                dir -> treeModify(onModify); // alert the modification tree for the parent directory of the updated file
                absname = dir -> path + '/' + evt -> name;
            }
        }
    }
    else { // it's a regular file
        for (WatchedPath* file : files) {
            if (file -> watcher == evt -> wd) {
                absname = file -> path;
            }
        }
    }
    if (evt -> mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY | IN_CLOSE_WRITE)) {
        onModify(absname);
        struct stat sb; // TODO: only stat on MOVED_TO and CREATE, so we don't waste all these cycles for modifying
        stat(absname.c_str(), &sb);
        if (S_ISDIR(sb.st_mode)) {
            dirwatch(sitix -> toOutput(absname));
        }
        else if (S_ISREG(sb.st_mode)) {
            filewatch(absname) -> treeModify(onModify);
        }
    }
    else if (evt -> mask & (IN_DELETE | IN_MOVED_FROM)) {
        unwatch(absname);
        onDelete(absname);
    }
    else {
        printf(WARNING "Unrecognized inotify event %d on file %s\n", evt -> mask, absname.c_str());
    }
}

TreeWatcher::~TreeWatcher() {
    for (WatchedPath* file : files) {
        inotify_rm_watch(inotifier, file -> watcher);
        delete file;
    }
    close(inotifier);
}