// SitixWriter is a class that outputs rendered data to a file. It follows fileflags and does post-processing when enabled.
#pragma once
#include <string>
#include <fileflags.h>
#include <sitixwriter.hpp>
#include <mapview.hpp>
#include <vector>


struct WriteOutput {
    virtual void write(const char* data, size_t length) = 0;
};


struct FileWriteOutput : WriteOutput {
    const static int BufferSize = 4096; // 4kb buffer
    int file;
    bool move = false;
    char buffer[BufferSize]; // buffer to prevent small writes
    size_t bufferPos = 0;

    FileWriteOutput(FileWriteOutput& f);

    FileWriteOutput(int fd);

    ~FileWriteOutput(); // Destructing a FileWriteOutput will flush the buffer and close the file.

    void write(const char* data, size_t length); // load some data into the buffer, and flush the buffer if the data overfills

    void flush();
};


struct StringWriteOutput : WriteOutput {
    std::string content;

    void write(const char* data, size_t length);
};


struct MarkdownState {
    bool italic = false;
    bool bold = false;
    bool underline = false;
    bool strikethrough = false;
    bool code = false;
    bool paragraph = false;

    enum ListType {
        Unordered,
        Ordered
    };
    std::vector<ListType> list;

    char lbyte = 0;
    int listPos = -1;
    int lastListPos = 0;
    enum {
        Standard,   // data just passes through
        Star,       // it's either italics or bold. The next character will decide that.
        Underl,     // the last character was a '_'. If this character is also '_', go into underline. If not, write out _ and the current character.
        Strike,     // the last character was a '`'. If this character is a '`' go into strikethrough.
        LinkText,   // the last character was a '{'. We're going to fill linkTextBuffer until the :, then fill linkHrefBuffer until the }.
        LinkHref,   // Finally, we'll write out the link.
        LineStart,  // the stage becomes LineStart every newline. If the next character is also a newline, a paragraph break is added. Yay!
        ListPosGrab // The line *might* be a list, so we're counting spaces. IF the first non-space character is a * or a -, we have a list entry!
    } parserStage = Standard;

    std::string linkTextBuffer;
    std::string linkHrefBuffer;
};


struct SitixWriter {
    FileFlags flags;
    bool minifyState = false;
    MarkdownState markdownState;
    WriteOutput& output;

    SitixWriter(WriteOutput& out);

    void minifyWrite(const char* data, size_t length);

    void markdownWrite(const char* data, size_t length);

    void setFlags(FileFlags fl);

    void write(const char* data, size_t length);

    void write(std::string data);

    void write(MapView data);
};
