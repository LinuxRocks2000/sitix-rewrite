/* By Tyler Clarke
    Fileman is a class that pulls SitixWriter and MapView all together. It manages the output directory and reads from the input directory.
*/
#include <string>
#include <defs.h>
#include <map>
#include <mapview.hpp>
#include <util.hpp>


class FileMan {
    std::map<std::string, MapView> maps;

public:
    enum PathState {
        CNEP,      // Ce n'existe pas
        Directory, // it's a directory
        File,      // it's a file
        Other,     // it's something else (symlink?)
        Error      // an error occurred when stat'ing it
    };

    PathState checkPath(std::string path);

    std::string transmuted(std::string path); // like old transmuted but less awful

    std::string arcTransmuted(std::string path); // strip off this directory from a path (returning something relative to this directory), if possible

    void uncache(std::string path); // remove a path from the mmap cache

    bool valid = true; // set to false by the FileMan if there's an error

    std::string dir;

    FileMan(std::string rdir); // construct the FileMan to manage the directory referenced by rdir.

    bool empty(bool); // empty the controlled directory and add the .sitix file (it will provide a warning prompt if .sitix doesn't exist)
    // returns whether or not it the directory was emptied.

    FileWriteOutput create(std::string where); // create a file and all of its parent directories, and return the filewriteoutput
    // that controls it. That filewriteoutput can be handed off to a SitixWriter for minification + markdown or can just be used raw.

    MapView open(std::string thing); // memory map a file into the buffer-like MapView, returning an invalid
    // mapview if it doesn't exist (you MUST always check if mapview.isValid()!)

    // open() will recycle MapViews; it checks if the file is already mapped before mapping it. Because running stat, running open, *and* running mmap in sequence is
    // slow ("heavy-hitter" system calls), the overhead from searching through an std::map of memory maps should still be an improvement.
    // (also it lets me joke about a map of maps)
};