// SitixWriter is a class that outputs rendered data to a file. It follows fileflags and does post-processing when enabled.
#pragma once
#include <string>
#include <fileflags.h>
#include <sitixwriter.hpp>
#include <mapview.hpp>


struct WriteOutput {
    virtual void write(const char* data, size_t length) = 0;
};


struct FileWriteOutput : WriteOutput {
    int file;
    bool move = false;

    FileWriteOutput(FileWriteOutput& f);

    FileWriteOutput(int fd);

    ~FileWriteOutput(); // Destructing a FileWriteOutput WILL close the file.

    void write(const char* data, size_t length);
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
