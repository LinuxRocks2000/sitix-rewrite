// Evals is a simple, turing-incomplete evaluated language designed for string processing. It directly integrates with and depends on the Sitix scope model.
// Evals is primarily designed for string manipulation and value comparison.
// It is not fancy; the syntax is redolent of Tyler++.
#pragma once
#include <defs.h>
#include <evals/core.hpp>
#include <evals/types.hpp>
#include <evals/ops.hpp>
#include <node.hpp>


struct EvalsSession {
    Object* parent;
    Object* scope;

    EvalsObject* render(MapView data);
};


struct EvalsBlob : Node {
    MapView data;

    EvalsBlob(Session*, MapView d);

    void render(SitixWriter* out, Object* scope, bool dereference);
};
