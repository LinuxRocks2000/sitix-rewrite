// SitixWriter is a class that outputs rendered data to a file. It follows fileflags and does post-processing when enabled.
#include <sitixwriter.hpp>
#include <unistd.h>
#include <util.hpp>


FileWriteOutput::FileWriteOutput(int fd) {
    file = fd;
}

void FileWriteOutput::write(const char* data, size_t length) {
    ::write(file, data, length);
}

FileWriteOutput::~FileWriteOutput() {
    if (!move) { // allow this file descriptor to be moved into another FileWriteOutput without being closed.
        ::close(file);
    }
}

FileWriteOutput::FileWriteOutput(FileWriteOutput& f) {
    file = f.file;
    f.move = true;
}

void StringWriteOutput::write(const char* data, size_t length) {
    content += std::string(data, length);
}


SitixWriter::SitixWriter(WriteOutput& out) : output(out){}

void SitixWriter::minifyWrite(const char* data, size_t length) { // minify is not very smart, it just turns all instances of multiple whitespace into a single whitespace.
    flags.minify = false; // turn OFF minify to stop redirections to this function
    // we're using write rather than ::write because we want to be able to nest them (add a markdown processor to the mix)
    // eventually we should probably come up with a more object-oriented way to handle this, but for now I think it's probably fine
    size_t i = 0;
    if (length == 0) {
        return;
    }
    while (i < length) {
        if (isWhitespace(data[i])) {
            while (isWhitespace(data[i]) && i < length) {
                i ++;
            }
            if (minifyState) {
                minifyState = false;
                write(" ", 1);
            }
        }
        else {
            size_t chunkStart = i;
            while (!isWhitespace(data[i]) && i < length) {
                i ++;
            }
            write(data + chunkStart, i - chunkStart);
            minifyState = true;
        }
    }
    flags.minify = true; // re-enable minify writes
}

void SitixWriter::markdownWrite(const char* data, size_t length) { // this is not a full Markdown implementation. It's best described as Sitix Markdown.
    // ** means bold, * means italic, __ means underline (<u> tag), ~~ means strikethrough, ` means code. <br> tags are inserted at newlines with at
    // least one trailing spaces, and new paragraphs are inserted at empty lines. Unlike most markdown renderers, we actually allow you to embed markdown in HTML.
    flags.markdown = false; // disable markdown to avoid recursion
    if (!markdownState.paragraph) {
        markdownState.paragraph = true;
        write("<p>", 3);
    }
    size_t i = 0;
    size_t chunk = 0;
    while (i < length) {
        size_t to = i;
        if (data[i] == '*') {
            if (i < length - 1 && data[i + 1] == '*') {
                if (i > chunk) {
                    write(data + chunk, to - chunk);
                    chunk = i;
                }
                if (markdownState.bold) {
                    write("</b>", 4);
                }
                else {
                    write("<b>", 3);
                }
                markdownState.bold = !markdownState.bold;
                i += 2;
            }
            else {
                if (i > chunk) {
                    write(data + chunk, to - chunk);
                    chunk = i;
                }
                if (markdownState.italic) {
                    write("</i>", 4);
                }
                else {
                    write("<i>", 3);
                }
                markdownState.italic = !markdownState.italic;
                i ++;
            }
        }
        else if (data[i] == '_' && i < length - 1 && data[i + 1] == '_') {
            if (i > chunk) {
                write(data + chunk, to - chunk);
                chunk = i;
            }
            if (markdownState.underline) {
                write("</u>", 4);
            }
            else {
                write("<u>", 3);
            }
            markdownState.underline = !markdownState.underline;
            i += 2;
        }
        else if (data[i] == '#' && i > 0 && data[i - 1] == 10) { // if # is the first character after a newline, we're doing some kind of header
            markdownState.htype = 0;
            while (data[i] == '#') {
                markdownState.htype ++;
                i ++;
            }
            write("<h" + std::to_string(markdownState.htype) + ">");
        }
        else if (data[i] == '~' && i < length - 1 && data[i + 1] == '~') {
            if (i > chunk) {
                write(data + chunk, to - chunk);
                chunk = i;
            }
            if (markdownState.strikethrough) {
                write("</s>", 4);
            }
            else {
                write("<s>", 3);
            }
            markdownState.strikethrough = !markdownState.strikethrough;
            i += 2;
        }
        else if (data[i] == '`') {
            if (i > chunk) {
                write(data + chunk, to - chunk);
                chunk = i;
            }
            if (markdownState.code) {
                write("</code>", 7);
            }
            else {
                write("<code>", 6);
            }
            markdownState.code = !markdownState.code;
            i ++;
        }
        else if (data[i] == '\n') {
            if (i > chunk) {
                write(data + chunk, to - chunk);
                chunk = i;
            }
            if (markdownState.htype > 0) {
                write("</h" + std::to_string(markdownState.htype) + ">");
                markdownState.htype = 0;
            }
            if (i > 0 && data[i - 1] == ' ') {
                write("<br>", 4);
            }
            else if (i > 0 && data[i - 1] == '\n') {
                write("</p><p>", 7);
            }
            i ++;
        }
        else if (data[i] == '/') { // "/" instead of "["/"]", because "["/"]" is already taken by Sitix itself.
            i ++;
            size_t textStart = i;
            while (data[i] != '/' && i < length) {
                i ++;
            }
            size_t textLen = i - textStart;
            if (data[i + 1] == '(') {
                if (i > chunk) {
                    write(data + chunk, to - chunk);
                }
                i += 2;
                size_t hrefStart = i;
                while (data[i] != ')') {
                    i ++;
                }
                write("<a href=\"");
                write(data + hrefStart, i - hrefStart);
                write("\">");
                write(data + textStart, textLen);
                write("</a>");
                i ++;
                chunk = i;
            }
            else { // "malformed" link, the user probably doesn't want it rendered
                write("/", 1);
                write(data + textStart, textLen);
            }
        }
        else {
            i ++;
            continue;
        }
        if (i > chunk) {
            write(data + chunk, to - chunk);
            chunk = i;
        }
    }
    write("</p>", 4);
    flags.markdown = true; // enable markdown to permit further md writes
}

void SitixWriter::setFlags(FileFlags fl) {
    flags = fl;
    if (!flags.minify) {
        minifyState = true;
    }
}

void SitixWriter::write(const char* data, size_t length) { // TODO: postprocess based on `flags`.
    if (flags.markdown) { // markdown takes precedence because markdown often relies on whitespace
        markdownWrite(data, length);
    }
    else if (flags.minify) {
        minifyWrite(data, length);
    }
    else if (flags.sitix) { // strips out backslashes
        size_t i = 0;
        size_t chunk = 0;
        while (i < length) {
            chunk = i;
            while (data[i] != '\\' && i < length) { i ++; } // i is now on a backslash
            output.write(data + chunk, i - chunk); // write everything up till the backslash
            if (data[i] == '\\') {
                i ++;
                output.write(data + i, 1);
                i ++;
            }
        }
    }
    else {
        output.write(data, length);
    }
}

void SitixWriter::write(std::string data) {
    write(data.c_str(), data.size());
}

void SitixWriter::write(MapView data) {
    write(data.cbuf(), data.len());
}