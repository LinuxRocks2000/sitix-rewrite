#include <session.hpp>
#include <types/Object.hpp>

Session::Session(std::string inDir, std::string outDir) : input(inDir), output(outDir){}

Object* Session::configLookup(std::string name) {
    for (Object* o : config) {
        if (o -> name == name) {
            return o;
        }
    }
    return NULL;
}

FileMan::PathState Session::checkPath(std::string path) {
    return input.checkPath(path);
}

std::string Session::transmuted(std::string path) {
    return input.transmuted(path);
}

FileWriteOutput Session::create(std::string path) {
    return output.create(path);
}

MapView Session::open(std::string path) {
    return input.open(path);
}

std::string Session::toOutput(std::string path) {
    return output.transmuted(input.arcTransmuted(path));
}