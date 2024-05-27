// the fileflags struct
#pragma once
struct FileFlags {
    bool minify = false;
    bool markdown = false;
    bool sitix = true; // what does this do? perhaps we'll never know
    // I'm too scared to remove it and too lazy to figure it out
    // #cruft
    bool dynamo = false; // Sitix Dynamo
    // it's a Watchdog extension that allows you to make pages that get re-rendered in the same Lua context rather than cold-rendered
    // (it still cold-renders sitix, so watchdog is not useful without Lua mode)
};
