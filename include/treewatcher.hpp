// TreeWatcher, watcher of trees
// Sitix builds cache relevant information in TreeWatchers. TreeWatchers are, among other things, a way to manage inotifying; they will intelligently set inotify
// watchers on newly-indexed files and destroy the watchers on deleted files. They also build dependency trees. When any file is changed in any way, a callback
// function is invoked to signal the change, in dependencies-first order. 
#pragma once
#include <string>
#include <vector>
#include <functional>
#include <defs.h>


struct WatchedPath { // any file or directory being watched.
    std::string path;
    int watcher; // produced by that inotifywait_add_watch function
    std::vector<WatchedPath*> dependants; // any WatchedPath that depends on this WatchedPath is stored here

    void addDep(WatchedPath* dep);
    
    void rmDep(WatchedPath* dep); // if that file depends on us, remove it

    void treeModify(std::function<void(std::string)> onModify); // run modifications up the dep tree
};


struct TreeWatcher {
    std::vector<WatchedPath*> files; // every watched path in this tree
    int inotifier; // the inotify fd

    TreeWatcher();

    ~TreeWatcher();

    WatchedPath* filewatch(std::string file);

    WatchedPath* dirwatch(std::string path);

    void unwatch(std::string path); // un-watch a file or directory

    void waitForModifications(Session* sitix, std::function<void(std::string)> onModify, std::function<void(std::string)> onDelete);
    // onModify is called upon adding as well.
    // waitForModifications breaks every time events are received, giving the outside program the option to either continue or end.
};