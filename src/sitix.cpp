/* Sitix, by Tyler Clarke

    NOTE: This comment covers basic Sitix. It doesn't delve into Evals or Sitix Markdown or the nuances of ghost-directives. Also, most of the bits on
    if statements are outdated. For up-to-date information, see the Usage page at https://swaous.asuscomm.com/sitix.

    An efficient templating engine written in C++, meant to reduce the pain of web development.
    Sitix templating is optional; files that desire Sitix templating should include [!] as the first three bytes. Otherwise, sitix will simply copy them.
    Files may also use [?] as the first three bytes; this informs the Sitix engine that the file should not be rendered (most templates will use this).
    Sitix works with a simple scope structure. Variables are set with [=name content] OR [=name-]content[/]. Variables are accessed with [^name].
    Variables can contain other variable accesses! This allows functions of a sort. For instance,
    
    [=link-]
        <a href="[^url]">[^linktext]</a>
    [/]
    [=url https://example.com][=linktext Example Dot Com][^link]

    Variables can be nested:

    [=config-]
        [=title Hello, World]
        [=baseurl https://helloworld.com/]
    [/]

    [^config.title]

    Variables can be reassigned:

    [=config-]
        [=title Hello, ]
        [=baseurl https://helloworld.com/]
    [/]
    [^config.title][=config.title World][^config.title]
    
    Which would output "Hello, World".

    So the rule is that variable operations occur on usage, rather than on assignment. This is obviously useful for templating.

    [#filename] includes a file. It doesn't just copy/paste; that file will be processed separately, but the scope at the inclusion point
    will be treated as the root scope for the include.
    
    [^filename] is a magic variable attached to every variable referencing the name of the file in which it was *created*. This is useful, you'll see why.

    Sitix doesn't have much of a type system. It does, however, have a way to do array-like operations: numerically assigned properties. These are done with
    [=+-][/] (or [=+ content]). [loop]s work on these, ignoring other nested variables. Loops specify a numerically indexed variable to operate on, and
    the name of the iterator variable. For instance,

    [=array-]
        [=+ Hello]
        [=+ , ]
        [=+ World]
    [/]
    [l array i]
        [^i]
    [/]

    would output "Hello, World".

    Arrays can be directly indexed with regular access syntax, like [^array.0], [^array.1], etc. Like in Python, [^array.-1] is the last element,
    [^array.-2] is the second to last, etc. This is rarely desirable. The major useful trait of arrays is that they are project-aware! Child
    directories are treated as variables on the global scope, numerically indexing in every file they contain (and name-indexing their subdirectories),
    so you can use this for things like generating blog links:

    [l posts post]
        <h1>[^post.title]</h1>
        <p>
            [^post.blurb]
        </p>
        <a href="[^post.filename]">Go To [^post.title]!</a>
    [/]

    The parent directory is referenced with the "magic" [^parentdir].

    Conditional branching is pretty important. At the moment the only valid conditional branch is equality, since other operations are ill-formed; equality checking
    just checks if raw scope (the list of text nodes and operations) is the same:

    [=variable Data]
    [=variable2 Data]
    [i equal variable v2]
        Hello, World
    [/]

    Or 

    [=variable Data]
    [=variable2 OtherData]
    [i nequal variable v2]
        Hello, World
    [/]

    Else is also supported -
    
    [=variable Data]
    [=variable2 OtherData]
    [i equal variable v2]
        They're the same.
    [e]
        They're different.
    [/]

    Configuration can be done at command-line with -c and checked with [i config]. A very real usage is like so:

    [=name Test Site]
    [i config production]
        [=name Production Site]
    [/]

    Where sitix -c production would cause that if gate to resolve.

    Escaping is done with backslashes. Because of the nature of the escape-processing mechanism, you can sanely escape whole tags with just one backslash, like

    \[=test Test]\[^test]

    would yield "[=test Test][^test]" rather than "Test".
    Backslashes themselves can be escaped with backslashes.

    NOTE: Escaping inside inline-expressions IS UNDEFINED! It might work, it might crash the program, it might do nothing. If you need to escape text,
    just waste like five bytes on a full expanded-expression.

    The [@] operator manages fileflags. These are specific to the current file and are boolean; [@on <flag>] turns a flag on and [@off <flag>] turns a flag off.
    These are file-wide and do NOT respect render-time constraints; they are applied immediately at parse-time. They are file-wide ONLY; if you want the same
    flags in two files, you must specify them in both files, even if one is including the other. The only currently available flag is minify, enable it with
    [@on minify] to reduce unnecessary whitespace in your Sitix files.
*/

// TODO: memory management. Set up sane destructors and USE THEM!
// At the moment the program leaks a LOT of memory. like, a LOT.

#include <defs.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <vector>
#include <stdlib.h>
#include <ftw.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <fileflags.h>
#include <mapview.hpp>
#include <sitixwriter.hpp>
#include <util.hpp>
#include <evals.hpp>

#include <types/Object.hpp>
#include <types/PlainText.hpp>
#include <types/TextBlob.hpp>
#include <types/DebuggerStatement.hpp>
#include <types/Copier.hpp>
#include <types/ForLoop.hpp>
#include <types/IfStatement.hpp>
#include <types/RedirectorStatement.hpp>
#include <types/Dereference.hpp>

const char* outputDir = "output"; // global variables are gross but some of the c standard library higher-order functions I'm using don't support argument passing
const char* siteDir = "";
std::vector<Object*> config;


int fillObject(MapView& map, Object* container, FileFlags* fileflags) { // designed to recurse
    // if length == 0, just run until we hit a closing tag (and then consume the closing tag, for nesting)
    // returns the amount it consumed
    bool escape = false;
    while (map.len() > 0) {
        if (map[0] == '\\' && !escape) {
            escape = true;
            map ++;
        }
        if (map[0] == '[' && !escape) {
            map ++;
            MapView tagData = map.consume(']');
            map ++;
            char tagOp = tagData[0];
            tagData ++;
            tagData.trim(); // trim whitespace from the start
            // note: whitespace *after* the tag data is considered part of the content, but not whitespace *before*.
            if (tagOp == '=') {
                Object* obj = new Object; // we just created an object with [=]
                bool isExt = tagData[-1] == '-';
                if (isExt) {
                    tagData.popFront();
                }
                MapView objName = tagData.consume(' ');
                if (objName.len() == 1 && objName[0] == '+') { // it's enumerated, an array.
                    obj -> namingScheme = Object::NamingScheme::Enumerated;
                    obj -> number = container -> highestEnumerated;
                    container -> highestEnumerated ++;
                }
                else {
                    obj -> namingScheme = Object::NamingScheme::Named;
                    obj -> name = objName.toString();
                }
                if (isExt) {
                    map ++;
                    fillObject(map, obj, fileflags);
                }
                else {
                    EvalsBlob* text = new EvalsBlob(tagData + 1);
                    text -> fileflags = *fileflags;
                    obj -> addChild(text);
                }
                obj -> fileflags = *fileflags;
                container -> addChild(obj);
            }
            else if (tagOp == 'f') {
                container -> addChild(new ForLoop(tagData, map, fileflags));
            }
            else if (tagOp == 'i') {
                map ++;
                container -> addChild(new IfStatement(map, tagData, fileflags));
            }
            else if (tagOp == 'e') { // same behavior as /, but it also signals to whoever's calling that it terminated-on-else
                map ++;
                return FILLOBJ_EXIT_ELSE;
            }
            else if (tagOp == 'v') {
                container -> addChild(new EvalsBlob(tagData));
            }
            else if (tagOp == 'd') {
                container -> addChild(new DebuggerStatement);
            }
            else if (tagOp == '>') {
                container -> addChild(new RedirectorStatement(map, tagData, fileflags));
            }
            else if (tagOp == '^') {
                Dereference* d = new Dereference;
                d -> name = tagData.toString();
                d -> fileflags = *fileflags;
                container -> addChild(d);
            }
            else if (tagOp == '/') { // if we hit a closing tag while length == 0, it's because there's a closing tag no subroutine consumed.
                // if it wasn't already consumed, it's either INVALID or OURS! Assume the latter and break.
                map ++;
                return FILLOBJ_EXIT_END;
            }
            else if (tagOp == '~') {
                Copier* c = new Copier; // doesn't actually copy, just ghosts
                c -> target = tagData.consume(' ').toString();
                tagData ++;
                c -> object = tagData.toString();
                container -> addChild(c);
            }
            else if (tagOp == '#') { // update: include will be kept because of the auto-escaping feature, which is nice.
                //printf(WARNING "The functionality of [#] has been reviewed and it may be deprecated in the near future.\n\tPlease see the Noteboard (https://swaous.asuscomm.com/sitix/pages/noteboard.html) for March 10th, 2024 for more information.\n");
                Dereference* d = new Dereference;
                d -> name = escapeString(tagData.toString(), '.');
                d -> fileflags = *fileflags;
                container -> addChild(d);
            }
            else if (tagOp == '@') {
                MapView tagRequest = tagData.consume(' ');
                tagData ++;
                MapView tagTarget = tagData;
                if (tagRequest.cmp("on")) {
                    if (tagTarget.cmp("minify")) {
                        fileflags -> minify = true;
                    }
                    else if (tagTarget.cmp("markdown")) {
                        fileflags -> markdown = true;
                    }
                }
                else if (tagRequest.cmp("off")) {
                    if (tagTarget.cmp("minify")) {
                        fileflags -> minify = false;
                    }
                    else if (tagTarget.cmp("markdown")) {
                        fileflags -> markdown = false;
                    }
                }
            }
            else {
                printf(WARNING "Unrecognized tag operation %c! Parsing will continue, but the result may be malformed.\n", tagOp);
            }
        }
        else if (map[0] == ']' && !escape) {
            printf(INFO "Unmatched closing bracket detected! This is probably not important; there are several minor interpreter bugs that can cause this without actually breaking anything.\n");
        }
        else {
            PlainText* text = new PlainText(map.consume('[', escape));
            text -> fileflags = *fileflags;
            container -> addChild(text);
        }
        escape = false;
    }
    map ++; // consume whatever byte we closed on (may eventually be a BUG!)
    return FILLOBJ_EXIT_EOF;
}


Object* string2object(MapView& string, FileFlags* flags) {
    Object* ret = new Object;
    ret -> fileflags = *flags;
    if (string.cmp("[!]")) { // it's a valid Sitix file
        string += 3;
        fillObject(string, ret, flags);
    }
    else if (string.cmp("[?]")) {
        string += 3;
        fillObject(string, ret, flags);
        ret -> isTemplate = true;
    }
    else {
        flags -> sitix = false;
        PlainText* text = new PlainText(string);
        text -> fileflags = *flags;
        ret -> addChild(text);
    }
    return ret;
}


void renderFile(const char* in, const char* out) {
    FileFlags fileflags;
    printf(INFO "Rendering %s to %s.\n", in, out);
    MapView map(in);
    if (map.isValid()) {
        Object* file = string2object(map, &fileflags);
        file -> namingScheme = Object::NamingScheme::Named;
        file -> name = transmuted((std::string)siteDir, (std::string)"", (std::string)in);
        file -> isFile = true;
        Object* fNameObj = new Object;
        fNameObj -> virile = false;
        fNameObj -> namingScheme = Object::NamingScheme::Named;
        fNameObj -> name = "filename";
        TextBlob* fNameContent = new TextBlob;
        fNameContent -> fileflags = fileflags;
        fNameContent -> data = transmuted((std::string)siteDir, (std::string)"", (std::string)in);
        fNameObj -> addChild(fNameContent);
        fNameObj -> fileflags = fileflags;
        file -> addChild(fNameObj);
        if (file -> isTemplate) {
            printf(INFO "%s is marked [?], will not be rendered.\n", in);
            printf("\tIf this file should be rendered, replace [?] with [!] in the header.\n");
        }
        else {
            int output = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0666); // rw for anyone, it's not a protected file (this may lead to security vulnerabilities in the future)
            // TODO: inherit permissions from the parent directory by some intelligent strategy so Sitix doesn't accidentally give attackers access to secure information
            // contained in a statically generated website.
            if (output == -1) {
                printf(ERROR "Couldn't open output file %s (for %s). This file will not be rendered.\n", out, in);
                return;
            }
            FileWriteOutput fOut(output);
            SitixWriter stream(fOut);
            file -> render(&stream, file, true);
            close(output);
        }
        file -> pushedOut();
    }
}


int iterRemove(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    if (remove(fpath)) {
        printf(WARNING "Encountered a non-fatal error while removing %s\n", fpath);
    }
    return 0;
}

void rmrf(const char* path) {
    nftw(path, iterRemove, 64, FTW_DEPTH);
}


int renderTree(const char* fpath, const struct stat* sb, int typeflag, struct FTW* ftwbuf) {
    char* thing = transmuted(siteDir, outputDir, fpath);
    if (typeflag == FTW_D) {
        mkdir(thing, 0);
        chmod(thing, 0755);
    }
    else {
        renderFile(fpath, thing);
    }
    free(thing);
    return FTW_CONTINUE;
}

int main(int argc, char** argv) {
    printf("\033[1mSitix v2.0 by Tyler Clarke\033[0m\n");
    outputDir = "output";
    siteDir = "";
    bool hasSpecificSitedir = false;
    bool wasConf = false;
    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "-o") == 0) {
            i ++;
            outputDir = argv[i];
            wasConf = false;
        }
        else if (strcmp(argv[i], "-c") == 0) {
            i ++;
            Object* obj = new Object;
            obj -> name = argv[i];
            config.push_back(obj);
            wasConf = true;
        }
        else if (!hasSpecificSitedir) {
            hasSpecificSitedir = true;
            siteDir = argv[i];
            wasConf = false;
        }
        else if (wasConf) {
            TextBlob* text = new TextBlob;
            text -> data = argv[i];
            config[config.size() - 1] -> addChild(text);
            wasConf = false;
        }
        else {
            wasConf = false;
            printf(ERROR "Unexpected argument %s\n", argv[i]);
        }
    }
    printf(INFO "Cleaning output directory\n");
    rmrf(outputDir);
    mkdir(outputDir, 0);
    chmod(outputDir, 0755);
    printf(INFO "Output directory clean.\n");
    printf(INFO "Rendering project '%s' to '%s'.\n", siteDir, outputDir);
    nftw(siteDir, renderTree, 64, 0);
    printf("\033[1;33mComplete!\033[0m\n");
    return 0;
}