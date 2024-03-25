/* By Tyler Clarke
    Fileman is a class that pulls SitixWriter and MapView all together. It manages the output directory and reads from the input directory.
*/
#include <string>
#include <defs.h>
#include <map>
#include <mapview.hpp>


class FileMan {
    std::string dir;
    std::map<std::string, MapView> maps;
    bool valid = true;

public:
    FileMan(std::string rdir); // construct the Fileman to manage the directory referenced by rdir.

    void empty(); // empty the controlled directory and add the .sitix file (it will provide a warning prompt if .sitix doesn't exist)

    FileWriteOutput create(std::string where); // create a file and all of its parent directories, and return the filewriteoutput
    // that controls it. That filewriteoutput can be handed off to a SitixWriter for minification + markdown or can just be used raw.

    MapView open(std::string thing); // memory map a file into the buffer-like MapView, returning an invalid
    // mapview if it doesn't exist (you MUST always check if mapview.isValid()!)

    // open() will recycle MapViews; it checks if the file is already mapped before mapping it. Because running stat, running open, *and* running mmap in sequence is
    // slow ("heavy-hitter" system calls), the overhead from searching through an std::map of memory maps should still be an improvement.
    // (also it lets me joke about a map of maps)
};