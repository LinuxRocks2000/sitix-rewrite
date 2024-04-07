// SitixWriter is a class that outputs rendered data to a file. It follows fileflags and does post-processing when enabled.
#include <sitixwriter.hpp>
#include <unistd.h>
#include <util.hpp>


FileWriteOutput::FileWriteOutput(int fd) {
    file = fd;
}

void FileWriteOutput::write(const char* data, size_t length) {
    while (length > 0) {
        size_t writeSize = BufferSize - bufferPos; // the space remaining
        if (writeSize > length) {
            writeSize = length;
        }
        if (writeSize == 0) {
            flush();
        }
        else {
            for (size_t i = 0; i < writeSize; i ++) {
                buffer[bufferPos + i] = data[i];
            }
            bufferPos += writeSize;
            data += writeSize;
            length -= writeSize;
        }
    }
}

void FileWriteOutput::flush() {
    ::write(file, buffer, bufferPos);
    bufferPos = 0;
}

FileWriteOutput::~FileWriteOutput() {
    if (!move) { // allow this file descriptor to be moved into another FileWriteOutput without being closed.
        flush();
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
    // least one trailing spaces, and new paragraphs are inserted at empty lines.
    // Lists are handled with numerals (ordered list) or * (unordered list) after one or more spaces on each new line.
    // Unlike most markdown renderers, we actually allow you to embed markdown in HTML. There's probably a really good reason why most markdown renderers
    // avoid that, but personally I like being able to seamlessly inject html in my markdown, so I'm going to do it.
    flags.markdown = false;
    for (size_t i = 0; i < length; i ++) {
        char byte = data[i];
        if (!markdownState.paragraph) {
            markdownState.paragraph = true;
            write("<p>");
        }
        if (markdownState.parserStage == MarkdownState::Standard) {
            if (byte == '\n') {
                markdownState.parserStage = MarkdownState::LineStart;
                if (markdownState.lbyte == ' ') {
                    write("<br/>");
                }
            }
            else if (byte == '`') {
                if (markdownState.code) {
                    write("</code>");
                }
                else {
                    write("<code>");
                }
                markdownState.code = !markdownState.code;
            }
            else if (markdownState.code) { // other effects are not rendered inside code blocks (you can still apply them outside if you want)
                write(&byte, 1);
            }
            else if (byte == '{') {
                markdownState.parserStage = MarkdownState::LinkText;
            }
            else if (byte == '~') {
                markdownState.parserStage = MarkdownState::Strike;
            }
            else if (byte == '_') {
                markdownState.parserStage = MarkdownState::Underl;
            }
            else if (byte == '*') {
                markdownState.parserStage = MarkdownState::Star;
            }
            else {
                write(&byte, 1);
            }
        }
        else if (markdownState.parserStage == MarkdownState::LineStart) {
            if (byte == '\n') { // empty line
                if (markdownState.paragraph && markdownState.list.size() == 0) {
                    write("</p>");
                    markdownState.paragraph = false;
                }
                while (markdownState.list.size() > 0) {
                    if (markdownState.list[markdownState.list.size() - 1] == MarkdownState::ListType::Unordered) {
                        write("</li></ul>");
                    }
                    else {
                        write("</li></ol>");
                    }
                    markdownState.list.pop_back();
                }
            }
            else if (byte == ' ') { // spaces on an empty line aren't necessarily a list, but they're required for a list
                                    // (list items must have at least one space before the * or -)
                markdownState.listPos = 1;
                markdownState.parserStage = MarkdownState::ListPosGrab;
            }
            else {
                while (markdownState.list.size() > 0) { // this newline is not anything special, let's unload any list tiers IF they exist
                    if (markdownState.list[markdownState.list.size() - 1] == MarkdownState::ListType::Unordered) {
                        write("</li></ul>");
                    }
                    else {
                        write("</li></ol>");
                    }
                    markdownState.list.pop_back();
                }
                write("\n"); // this new line isn't anything special, let's just write the newline character
                i --; // reprocess the last character
                markdownState.parserStage = MarkdownState::Standard;
            }
        }
        else if (markdownState.parserStage == MarkdownState::ListPosGrab) {
            if (byte == ' ') {
                markdownState.listPos ++;
            }
            else if (byte == '*' || byte == '-') { // it's a list item!
                if (markdownState.listPos > markdownState.list.size()) { // it can only go up one level (TODO: automatically catch bad syntax, like using two spaces before the first list item)
                    if (byte == '*') {
                        markdownState.list.push_back(MarkdownState::ListType::Unordered);
                        write("<ul>");
                    }
                    else {
                        markdownState.list.push_back(MarkdownState::ListType::Ordered);
                        write("<ol>");
                    }
                }
                else if (markdownState.listPos < markdownState.list.size()) {
                    while (markdownState.list.size() > markdownState.listPos) {
                        if (markdownState.list[markdownState.list.size() - 1] == MarkdownState::ListType::Unordered) {
                            write("</li></ul>");
                        }
                        else {
                            write("</li></ol>");
                        }
                        markdownState.list.pop_back();
                    }
                }
                else if (markdownState.listPos > 0 && markdownState.list.size() > 0) { // if there was at least one list item before this, let's close it
                    write("</li>");
                }
                markdownState.parserStage = MarkdownState::Standard;
                write("<li>");
            }
            else {
                // false alarm, no list here!
                // if we have list tiers loaded up, let's unload them now.
                while (markdownState.list.size() > 0) {
                    if (markdownState.list[markdownState.list.size() - 1] == MarkdownState::ListType::Unordered) {
                        write("</li></ul>");
                    }
                    else {
                        write("</li></ol>");
                    }
                    markdownState.list.pop_back();
                }
                write(&byte, 1);
                markdownState.parserStage = MarkdownState::Standard;
                markdownState.listPos = -1;
            }
        }
        else if (markdownState.parserStage == MarkdownState::Star) {
            markdownState.parserStage = MarkdownState::Standard;
            if (byte == '*') {
                if (markdownState.bold) {
                    write("</b>");
                }
                else {
                    write("<b>");
                }
                markdownState.bold = !markdownState.bold;
            }
            else {
                if (markdownState.italic) {
                    write("</i>");
                }
                else {
                    write("<i>");
                }
                markdownState.italic = !markdownState.italic;
                i --; // re-process whatever byte we landed on, this time in Standard mode
            }
        }
        else if (markdownState.parserStage == MarkdownState::Strike) {
            markdownState.parserStage = MarkdownState::Standard;
            if (byte == '~') {
                if (markdownState.strikethrough) {
                    write("</s>");
                }
                else {
                    write("<s>");
                }
                markdownState.strikethrough = !markdownState.strikethrough;
            }
            else {
                write("~"); // it wasn't part of a strikethrough dec, render it!
                i --; // reprocess the current byte, this time in Standard mode
            }
        }
        else if (markdownState.parserStage == MarkdownState::Underl) {
            markdownState.parserStage = MarkdownState::Standard;
            if (byte == '_') {
                if (markdownState.underline) {
                    write("</u>");
                }
                else {
                    write("<u>");
                }
                markdownState.underline = !markdownState.underline;
            }
            else {
                write("_"); // it wasn't part of an underline dec, render it!
                i --; // reprocess the current byte, this time in Standard mode
            }
        }
        else if (markdownState.parserStage == MarkdownState::LinkText) {
            if (byte == '@') {
                markdownState.parserStage = MarkdownState::LinkHref;
            }
            else if (byte == '}') {
                write("<img src=\"");
                write(markdownState.linkTextBuffer);
                write("\"/>");
                markdownState.parserStage = MarkdownState::Standard;
            }
            else {
                markdownState.linkTextBuffer += byte;
            }
        }
        else if (markdownState.parserStage == MarkdownState::LinkHref) {
            if (byte == '}') {
                markdownState.parserStage = MarkdownState::Standard;
                write("<a href=\"");
                write(markdownState.linkHrefBuffer);
                write("\">");
                write(markdownState.linkTextBuffer);
                write("</a>");
                markdownState.linkHrefBuffer.clear();
                markdownState.linkTextBuffer.clear();
            }
            else {
                markdownState.linkHrefBuffer += byte;
            }
        }
        markdownState.lbyte = byte;
    }
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