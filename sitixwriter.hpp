// SitixWriter is a class that outputs rendered data to a file. It follows fileflags and does post-processing when enabled.
#pragma once
#include <string>
#include "fileflags.h"


bool isWhitespace(char thing) {
    return thing == ' ' || thing == '\t' || thing == '\n' || thing == '\r';
}


struct WriteOutput {
    virtual void write(const char* data, size_t length) = 0;
};


struct FileWriteOutput : WriteOutput {
    int file;

    FileWriteOutput(int fd) {
        file = fd;
    }

    void write(const char* data, size_t length) {
        ::write(file, data, length);
    }
};


struct StringWriteOutput : WriteOutput {
    std::string content;

    void write(const char* data, size_t length) {
        content += std::string(data, length);
    }
};


struct SitixWriter {
    FileFlags flags;
    bool minifyState = false;
    WriteOutput& output;

    SitixWriter(WriteOutput& out) : output(out){}

    void minifyWrite(const char* data, size_t length) { // minify is not very smart, it just turns all instances of multiple whitespace into a single whitespace.
        flags.minify = false; // turn OFF minify to stop redirections to this function
        // we're using write rather than ::write because we want to be able to nest them (add a markdown processor to the mix)
        // eventually we should probably come up with a more object-oriented way to handle this, but for now I think it's probably fine
        size_t i = 0;
        size_t chunkStart = 0;
        if (length == 0) {
            return;
        }
        while (i < length) {
            if (isWhitespace(data[i])) {
                if (minifyState) {
                    i ++;
                    minifyState = false;
                }
                if (i > chunkStart) {
                    write(data + chunkStart, i - chunkStart);
                }
                while (isWhitespace(data[i]) && i < length) {
                    i ++;
                }
                chunkStart = i;
            }
            else {
                minifyState = true;
            }
            i ++;
        }
        if (chunkStart < i && i <= length) {
            write(data + chunkStart, i - chunkStart);
        }
    }

    void setFlags(FileFlags fl) {
        flags = fl;
        if (!flags.minify) {
            minifyState = true;
        }
    }

    void write(const char* data, size_t length) { // TODO: postprocess based on `flags`.
        if (flags.minify) {
            minifyWrite(data, length);
        }
        else {
            output.write(data, length);
        }
    }

    void write(std::string data) {
        write(data.c_str(), data.size());
    }

    void write(MapView data) {
        write(data.cbuf(), data.len());
    }
};