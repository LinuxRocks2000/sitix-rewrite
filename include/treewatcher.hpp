// TreeWatcher, watcher of trees
// Sitix builds cache relevant information in TreeWatchers. TreeWatchers are, among other things, a way to manage inotifying; they will intelligently set inotify
// watchers on newly-indexed files and destroy the watchers on deleted files. They also build dependency trees. When any file is changed in any way, a callback
// function is invoked to signal the change, in dependencies-first order. 
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <defs.h>


struct WatchedFile {
    std::string filename;
    int watcher; // produced by that inotifywait_add_watch function
    std::vector<WatchedFile*> dependants; // any WatchedFile that depends on this WatchedFile is stored here

    void addDep(WatchedFile* dep);
    
    void rmDep(WatchedFile* dep); // if that file depends on us, remove it

    void treeModify(std::function<void(std::string)> onModify); // run modifications up the dep tree
};


struct WatchedDir { // contains information on a watched directory (necessary for the IN_CREATE flag)
    std::string path;
    int watcher;
};


struct TreeWatcher {
    std::vector<WatchedFile*> files; // every watched file in this tree
    std::vector<WatchedDir*> directories; // every watched directory in this tree
    int inotifier; // the inotify fd

    TreeWatcher();

    ~TreeWatcher();

    WatchedFile* filewatch(std::string file);

    WatchedDir* dirwatch(std::string path);

    void unwatch(std::string path); // un-watch a file or directory

    void waitForModifications(Session* sitix, std::function<void(std::string)> onModify, std::function<void(std::string)> onDelete);
    // onModify is called upon adding as well.
    // waitForModifications breaks every time events are received, giving the outside program the option to either continue or end.
};