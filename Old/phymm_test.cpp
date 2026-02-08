#include <iostream>
#include <cassert>
#include <cmath>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;


#include <SFML/Graphics.hpp>
#include <sfml-3d/3d_engine.hpp>
#include <sfml-3d/math4.hpp>
#include <sfml-3d/3d_camera.hpp>
#include "3d_mm.hpp"



std::string randomString(std::size_t length) {
    static const std::string characters =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";

    static std::random_device rd;  // Seed
    static std::mt19937 generator(rd());  // Mersenne Twister engine
    static std::uniform_int_distribution<> distribution(0, characters.size() - 1);

    std::string result;
    result.reserve(length);

    for (std::size_t i = 0; i < length; ++i) {
        result += characters[distribution(generator)];
    }

    return result;
}

Physical_MM randomPhysical_MM() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    // 1. Determine how many nodes and connections we want
    std::uniform_int_distribution<> nodeCountDist(5, 15);
    std::uniform_int_distribution<> connCountDist(3, 10);
    int numNodes = nodeCountDist(gen);
    int numConns = connCountDist(gen);

    MM randomMM;
    std::vector<std::string> titles;

    // 2. Generate Unique Nodes
    while (titles.size() < (size_t)numNodes) {
        std::string title = randomString(8); 
        // Ensure it's a valid filename and unique
        if (isValidFilename(title) && randomMM.nodes.find(title) == randomMM.nodes.end()) {
            randomMM.nodes[title] = "Body content for " + title + ": " + randomString(20);
            titles.push_back(title);
        }
    }

    // 3. Generate Random Connections
    std::uniform_int_distribution<> indexDist(0, titles.size() - 1);
    int attempts = 0;
    while (randomMM.connections.size() < (size_t)numConns && attempts < 100) {
        std::string a = titles[indexDist(gen)];
        std::string b = titles[indexDist(gen)];

        // Validation: No self-connections, ensure unique pair
        if (a != b) {
            bool exists = false;
            for (const auto& conn : randomMM.connections) {
                if ((conn.first == a && conn.second == b) || (conn.first == b && conn.second == a)) {
                    exists = true;
                    break;
                }
            }
            if (!exists) {
                randomMM.connections.push_back({a, b});
            }
        }
        attempts++;
    }

    // 4. Create the Physical_MM
    // This calls Physical_MM(MM mm), which assigns random positions automatically
    Physical_MM pmm(randomMM);

    // Optional: Jiggle the positions slightly to ensure they aren't just 
    // the default result of a specific seed, ensuring the save/load 
    // actually captures state.
    for (auto& [title, node] : pmm.nodes) {
        node.position.x += 1.0f; 
    }
    pmm.update3DObjects();

    return pmm;
}


int main() {

    Physical_MM myMM = randomPhysical_MM();

    myMM.save("Physical_MM");

    Physical_MM copyMM("Physical_MM");

    assert(myMM == copyMM);

    return 0;
}