/* Sitix, by Tyler Clarke

    An efficient templating engine written in C++, meant to reduce the pain of web development.
    
    Sitix is sane, flexible, and not opinionated. You can template anything with Sitix - whether it be Markdown, CSS, or a hand-edited GIF from 1998.
    There used to be a docs page here; since maintaining it along with https://swaous.asuscomm.com/sitix/usage was getting annoying, I removed it.
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
#include <fts.h>
#include <sys/types.h>
#include <dirent.h>
#include <string>
#include <fileflags.h>
#include <mapview.hpp>
#include <sitixwriter.hpp>
#include <util.hpp>
#include <evals.hpp>
#include <session.hpp>
#include <sys/inotify.h>

#include <types/Object.hpp>
#include <types/PlainText.hpp>
#include <types/TextBlob.hpp>
#include <types/DebuggerStatement.hpp>
#include <types/Copier.hpp>
#include <types/ForLoop.hpp>
#include <types/IfStatement.hpp>
#include <types/RedirectorStatement.hpp>
#include <types/Dereference.hpp>


int fillObject(MapView& map, Object* container, FileFlags* fileflags, Session* sitix) { // designed to recurse
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
                Object* obj = new Object(sitix); // we just created an object with [=]
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
                    fillObject(map, obj, fileflags, sitix);
                }
                else {
                    EvalsBlob* text = new EvalsBlob(sitix, tagData + 1);
                    text -> fileflags = *fileflags;
                    obj -> addChild(text);
                }
                obj -> fileflags = *fileflags;
                container -> addChild(obj);
            }
            else if (tagOp == 'f') {
                container -> addChild(new ForLoop(sitix, tagData, map, fileflags));
            }
            else if (tagOp == 'i') {
                map ++;
                container -> addChild(new IfStatement(sitix, map, tagData, fileflags));
            }
            else if (tagOp == 'e') { // same behavior as /, but it also signals to whoever's calling that it terminated-on-else
                map ++;
                return FILLOBJ_EXIT_ELSE;
            }
            else if (tagOp == 'v') {
                container -> addChild(new EvalsBlob(sitix, tagData));
            }
            else if (tagOp == 'd') {
                container -> addChild(new DebuggerStatement(sitix));
            }
            else if (tagOp == '>') {
                container -> addChild(new RedirectorStatement(sitix, map, tagData, fileflags));
            }
            else if (tagOp == '^') {
                Dereference* d = new Dereference(sitix);
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
                Copier* c = new Copier(sitix); // doesn't actually copy, just ghosts
                c -> target = tagData.consume(' ').toString();
                tagData ++;
                c -> object = tagData.toString();
                container -> addChild(c);
            }
            else if (tagOp == '#') { // update: include will be kept because of the auto-escaping feature, which is nice.
                //printf(WARNING "The functionality of [#] has been reviewed and it may be deprecated in the near future.\n\tPlease see the Noteboard (https://swaous.asuscomm.com/sitix/pages/noteboard.html) for March 10th, 2024 for more information.\n");
                Dereference* d = new Dereference(sitix);
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
            PlainText* text = new PlainText(sitix, map.consume('[', escape));
            text -> fileflags = *fileflags;
            container -> addChild(text);
        }
        escape = false;
    }
    map ++; // consume whatever byte we closed on (may eventually be a BUG!)
    return FILLOBJ_EXIT_EOF;
}


Object* string2object(MapView& string, FileFlags* flags, Session* sitix) {
    Object* ret = new Object(sitix);
    ret -> fileflags = *flags;
    if (string.cmp("[!]")) { // it's a valid Sitix file
        string += 3;
        fillObject(string, ret, flags, sitix);
    }
    else if (string.cmp("[?]")) {
        string += 3;
        fillObject(string, ret, flags, sitix);
        ret -> isTemplate = true;
    }
    else {
        flags -> sitix = false;
        PlainText* text = new PlainText(sitix, string);
        text -> fileflags = *flags;
        ret -> addChild(text);
    }
    return ret;
}


void renderFile(std::string in, Session* sitix) {
    std::string out = sitix -> toOutput(in);
    FileFlags fileflags;
    printf(INFO "Rendering %s to %s.\n", in.c_str(), out.c_str());
    MapView map = sitix -> open(in);
    if (map.isValid()) {
        Object* file = string2object(map, &fileflags, sitix);
        file -> namingScheme = Object::NamingScheme::Named;
        file -> name = transmuted(sitix -> input.dir, (std::string)"", (std::string)in);
        file -> isFile = true;
        Object* fNameObj = new Object(sitix);
        fNameObj -> virile = false;
        fNameObj -> namingScheme = Object::NamingScheme::Named;
        fNameObj -> name = "filename";
        TextBlob* fNameContent = new TextBlob(sitix);
        fNameContent -> fileflags = fileflags;
        fNameContent -> data = file -> name;
        fNameObj -> addChild(fNameContent);
        fNameObj -> fileflags = fileflags;
        file -> addChild(fNameObj);
        if (file -> isTemplate) {
            printf(INFO "%s is marked [?], will not be rendered.\n", in.c_str());
            printf("\tIf this file should be rendered, replace [?] with [!] in the header.\n");
        }
        else {
            FileWriteOutput fOut = sitix -> create(out);
            SitixWriter stream(fOut);
            file -> render(&stream, file, true);
        }
        delete file;
    }
    else {
        printf(ERROR "Invalid map.\n");
    }
}

struct ConfigEntry {
    std::string name;
    std::string content;
};

int main(int argc, char** argv) {
    printf("\033[1mSitix v2.1 by Tyler Clarke\033[0m\n");
    std::string outputDir = "output";
    std::string siteDir = "";
    std::vector<ConfigEntry> config;
    bool hasSpecificSitedir = false;
    bool wasConf = false;
    bool watchdog = false;
    for (int i = 1; i < argc; i ++) {
        if (strcmp(argv[i], "-o") == 0) {
            i ++;
            outputDir = argv[i];
            wasConf = false;
        }
        else if (strcmp(argv[i], "-c") == 0) {
            i ++;
            config.push_back(ConfigEntry{ 
                argv[i],
                "" });
            wasConf = true;
        }
        else if (strcmp(argv[i], "-w") == 0) {
            watchdog = true;
        }
        else if (!hasSpecificSitedir) {
            hasSpecificSitedir = true;
            siteDir = argv[i];
            wasConf = false;
        }
        else if (wasConf) {
            config[config.size() - 1].content = argv[i];
            wasConf = false;
        }
        else {
            wasConf = false;
            printf(ERROR "Unexpected argument %s\n", argv[i]);
        }
    }
    Session session(siteDir, outputDir);
    for (ConfigEntry& conf : config) {
        Object* obj = new Object(&session);
        obj -> name = conf.name;
        session.config.push_back(obj);
        if (conf.content != "") {
            TextBlob* text = new TextBlob(&session);
            text -> data = conf.content;
            obj -> addChild(text);
        }
    }
    printf(INFO "Cleaning output directory\n");
    if (!session.output.empty()) {
        printf("Abort.\n");
        exit(1);
    }
    printf(INFO "Output directory clean.\n");
    printf(INFO "Rendering project '%s' to '%s'.\n", siteDir.c_str(), outputDir.c_str());

    char* paths[] = { (char*)siteDir.c_str(), NULL }; // for FTS
    FTS* ftsp = fts_open(paths, FTS_PHYSICAL | FTS_NOCHDIR, NULL);
    if (ftsp == NULL) {
        printf(ERROR "Couldn't initiate directory traversal.\n");
        perror("\tfts_open");
    }
    FTSENT* ent;
    while ((ent = fts_read(ftsp)) != NULL) {
        if (ent -> fts_info == FTS_F) {
            renderFile(ent -> fts_path, &session);
            session.watcher.filewatch(ent -> fts_path);
        }
        else if (ent -> fts_info == FTS_D) {
            session.watcher.dirwatch(ent -> fts_path);
        }
    }
    fts_close(ftsp);

    if (watchdog) {
        printf("\033[1;33mInitial build complete!\033[0m\n");
        printf(WATCHDOG "Sitix will now idle (it will not consume CPU) until a change is made, and will then re-render the affected files.\n");
        while (true) {
            session.watcher.waitForModifications(&session, [&](std::string name){
                printf(WATCHDOG "%s was modified.\n", name.c_str());
                renderFile(name, &session);
            }, [&](std::string name){
                printf(WATCHDOG "%s was deleted\n", name.c_str());
                remove(session.output.transmuted(session.input.arcTransmuted(name)).c_str());
            });
        }
    }
    printf("\033[1;33mBuild complete!\033[0m\n");
    return 0;
}