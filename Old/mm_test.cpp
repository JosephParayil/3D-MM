#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cassert>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#include "mm.hpp"


// ... (your existing code here) ...

bool operator==(const MM& a, const MM& b) {
    return a.nodes == b.nodes && a.connections == b.connections;
}

int main() {
    // --- Construct and populate an MM ---
    MM original;
    original.nodes["Hello"] = "This is the body of Hello.";
    original.nodes["World"] = "This is the body of World.";
    original.nodes["Foo"]   = "Foo has some\nmultiline\ncontent.";
    original.nodes["Bar"]   = "";  // deliberately empty body

    original.connections.push_back({"Hello", "World"});
    original.connections.push_back({"Foo",   "Bar"});
    original.connections.push_back({"Hello", "Foo"});

    // --- Save ---
    std::string saveDir = "test_mm_save";
    original.save(saveDir);

    std::cout << "Saved MM to: " << saveDir << std::endl;

    // --- Load into a new MM via the directory constructor ---
    MM loaded(saveDir);

    std::cout << "Loaded MM from: " << saveDir << std::endl;

    // --- Compare ---
    // Nodes are a std::map, so ordering is deterministic and == works directly.
    assert(original.nodes == loaded.nodes
           && "FAIL: nodes do not match.");

    // Connections are a vector, so order matters.
    // The directory iterator does not guarantee file order, but connections
    // are all stored in a single file and read back sequentially,
    // so their order is preserved. We assert exact match.
    assert(original.connections == loaded.connections
           && "FAIL: connections do not match.");

    // One combined check for clarity
    assert(original == loaded && "FAIL: original and loaded MM are not equivalent.");

    // --- Clean up ---
    //fs::remove_all(saveDir);

    std::cout << "All checks passed. Original and loaded MM are identical." << std::endl;
    return 0;
}