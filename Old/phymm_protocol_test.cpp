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
        node->position.x += 1.0f; 
    }
    pmm.update3DObjects();

    return pmm;
}


void test_node_operations() {
    std::cout << "Testing Node Operations..." << std::endl;
    
    MM base_mm;
    Physical_MM pmm(base_mm);
    vec4 test_pos(10.0f, 20.0f, 30.0f);

    // 1. Add Node
    pmm.addNode(test_pos);
    assert(pmm.nodes.size() == 1);
    assert(pmm.mm.nodes.size() == 1);
    std::string assigned_title = pmm.id_to_title[0];
    assert(pmm.nodes.contains(assigned_title));
    assert(pmm.nodes[assigned_title]->position == test_pos);
    
    // Check collection: Should have 1 sphere + 1 label
    assert(pmm.collection.c.size() == 2);
    pmm.validityCheck();

    // 2. Change Title
    std::string new_title = "RenamedNode";
    pmm.changeNodeTitle(assigned_title, new_title);
    assert(pmm.nodes.contains(new_title));
    assert(!pmm.nodes.contains(assigned_title));
    assert(pmm.id_to_title[0] == new_title);
    assert(pmm.nodes[new_title]->position == test_pos); // Position preserved
    pmm.validityCheck();

    // 3. Remove Node
    pmm.removeNode(new_title);
    assert(pmm.nodes.empty());
    assert(pmm.mm.nodes.empty());
    assert(pmm.id_to_title.empty());
    assert(pmm.collection.c.empty());
    pmm.validityCheck();

    std::cout << "Node Operations Passed!" << std::endl;
}

void test_connection_operations() {
    std::cout << "Testing Connection Operations..." << std::endl;
    
    Physical_MM pmm;
    pmm.addNode(vec4(0, 0, 0)); // Creates "New Node"
    pmm.addNode(vec4(10, 0, 0)); // Creates "New Node 1"
    
    std::string n1 = pmm.id_to_title[0];
    std::string n2 = pmm.id_to_title[1];

    // 1. Add Connection
    pmm.addConnection(n1, n2);
    assert(pmm.mm.connections.size() == 1);
    assert(pmm.lines.size() == 1);
    // Collection: 2 Spheres + 2 Labels + 1 Line = 5
    assert(pmm.collection.c.size() == 5);
    pmm.validityCheck();

    // 2. Test implicit removal (Removing a node should kill the connection)
    pmm.removeNode(n1);
    assert(pmm.mm.connections.empty());
    assert(pmm.lines.empty());
    // Remaining: 1 Sphere + 1 Label = 2
    assert(pmm.collection.c.size() == 2);
    pmm.validityCheck();

    std::cout << "Connection Operations Passed!" << std::endl;
}

void test_persistence() {
    std::cout << "Testing Save/Load Persistence..." << std::endl;
    
    std::string test_dir = "test_data_dir";
    if (fs::exists(test_dir)) fs::remove_all(test_dir);
    fs::create_directory(test_dir);

    Physical_MM original = randomPhysical_MM();
    original.validityCheck();

    // Save
    original.save(test_dir);

    // Load
    Physical_MM loaded(test_dir);
    loaded.validityCheck();

    // Verify equality
    assert(original == loaded);
    
    // Verify specific data point
    for (auto const& [title, node] : original.nodes) {
        assert(loaded.nodes.contains(title));
        assert(loaded.nodes[title]->position == node->position);
    }

    fs::remove_all(test_dir);
    std::cout << "Persistence Passed!" << std::endl;
}

void test_complex_integrity() {
    std::cout << "Testing Complex Integrity..." << std::endl;
    
    Physical_MM pmm = randomPhysical_MM();
    
    // Perform a series of chaotic mutations
    for(int i = 0; i < 10; ++i) {
        pmm.addNode(vec4(i, i, i));
        if (pmm.id_to_title.size() > 2) {
            pmm.addConnection(pmm.id_to_title[0], pmm.id_to_title.back());
        }
    }
    
    pmm.validityCheck();

    // Rename everything
    std::vector<std::string> titles = pmm.id_to_title;
    for(size_t i = 0; i < titles.size(); ++i) {
        pmm.changeNodeTitle(titles[i], "BulkRename_" + std::to_string(i));
    }

    pmm.validityCheck();

    
    // Delete half
    for(int i = 0; i < 5; ++i) {
        pmm.removeNode(pmm.id_to_title[0]);
    }

    pmm.validityCheck();
    std::cout << "Complex Integrity Passed!" << std::endl;
}

int main() {
    try {
        test_node_operations();
        test_connection_operations();
        test_persistence();
        test_complex_integrity();
        
        std::cout << "\n============================" << std::endl;
        std::cout << "ALL TESTS PASSED SUCCESSFULLY" << std::endl;
        std::cout << "============================" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}