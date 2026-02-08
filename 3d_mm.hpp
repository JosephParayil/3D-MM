#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
#include <cassert>
#include <random>
#include <sfml-3d/math4.hpp>
#include <unordered_map>

#include "mm.hpp"

const sf::Font FONT("JetBrainsMonoNerdFont-Medium.ttf");
const vec4 LABEL_OFFSET(0, 2, 0);

struct Physical_MM {
    MM mm;

    struct Node {
        vec4 position;
        vec4 velocity;
        Sphere3D sphere;
        Label3D label;

        Node() = default;
        Node(vec4 position, vec4 velocity, Sphere3D sphere, std::string title)
            : position(position),
              velocity(velocity),
              sphere(sphere),
              label(position + LABEL_OFFSET, title, FONT) {}

        bool operator==(const Node& other) const {
            return position == other.position;
        }
    };
    std::unordered_map<std::string, Node> nodes;

    std::vector<Line3D> lines;
    Object3D_Collection collection;

    /*
    // ID system:
        0 - nodes.size()-1: node/sphere with title id_to_title[i]

        nodes.size() - nodes.size() + mm.connections.size()-1:
    mm.connections[nodes.size()-i]

    nodes.size() + mm.connections.size() - nodes.size() * 2 +
    mm.connections.size(): node with title id_to_title[nodes.size() +
    mm.connections.size() - i]
    */
    std::vector<std::string> id_to_title;  // same size as nodes

    static vec4 rand_position() {
        static std::random_device rd;
        static std::mt19937 gen(rd());

        static std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

        return vec4(dist(gen), dist(gen), dist(gen));
    }

    void update3DObjects() {
        for (auto& node_pair : nodes) {
            node_pair.second.sphere.position = node_pair.second.position;
            node_pair.second.label.position =
                node_pair.second.position + LABEL_OFFSET;
        }
        for (size_t i = 0; i < lines.size(); i++) {
            lines[i].a = nodes[mm.connections[i].first].position;
            lines[i].b = nodes[mm.connections[i].second].position;
        }
    }

    Physical_MM() {}

    Physical_MM(MM mm) : mm(mm) {
        size_t id = 0;
        for (auto& node : mm.nodes) {
            vec4 position = rand_position();
            vec4 velocity(0, 0, 0);
            Sphere3D sphere(position, 1.0f);

            Node newNode(position, velocity, sphere, node.first);

            nodes[node.first] = newNode;
            id_to_title.push_back(node.first);

            collection.c.push_back(
                std::make_pair(id, &(nodes[node.first].sphere)));

            id++;
        }

        lines.reserve(mm.connections.size());

        for (auto& connection : mm.connections) {
            Line3D line(nodes[connection.first].position,
                        nodes[connection.second].position, 1.0f);
            lines.push_back(line);

            collection.c.push_back(
                std::make_pair(id, &lines[lines.size() - 1]));
            id++;
        }

        for (auto& node : mm.nodes) {
            collection.c.push_back(
                std::make_pair(id, &(nodes[node.first].label)));
            id++;
        }
    }

    void render(sf::RenderWindow& window, Camera& camera) {
        collection.depthSort(camera);

        int selected_id = -1;
        int selected_id_connection = -1;

        // If our mouse hovers both a node and a connection, we want to prefer
        // the node
        for (auto it = collection.c.rbegin(); it != collection.c.rend(); ++it) {
            auto shape = it->second->computeShape(window, camera);
            if (!shape) continue;

            sf::Vector2f mousePos =
                sf::Vector2f(sf::Mouse::getPosition(window));
            if (shape->computeCollisionWithPoint(mousePos)) {
                if (it->first < nodes.size() ||
                    it->first >=
                        nodes.size() + mm.connections.size()) {  // NODE ALERT
                    selected_id = it->first;
                    break;
                } else {  // Meh... a connection. Only add this if we have not
                          // found a connection before. Even then, continue the
                          // search for a node if possible
                    if (selected_id_connection == -1 && selected_id == -1) {
                        selected_id_connection = it->first;
                    }
                }
            }
        }

        if (selected_id == -1) {
            selected_id = selected_id_connection;
        } else if (selected_id >= nodes.size() + mm.connections.size()) {
            selected_id -= nodes.size() + mm.connections.size();
        }

        for (auto& pair : collection.c) {
            if (pair.first == selected_id ||
                pair.first + nodes.size() + mm.connections.size() ==
                    selected_id) {
                pair.second->draw(window, camera, sf::Color::Red);
            } else {
                pair.second->draw(window, camera, sf::Color::White);
            }
        }
    }

    bool are_sizes_matching() const {
        return mm.nodes.size() == nodes.size() &&
               mm.connections.size() == lines.size();
    }

    //*
    Physical_MM(std::string path)
        : Physical_MM([&]() {
              // This immediate-invoked lambda reconstructs the MM first
              // so we can pass it to the delegating constructor
              assert(fs::exists(path) && fs::is_directory(path));
              return MM(path + "/Mental-Model");
          }()) {
        // At this point, the delegating constructor has already:
        // 1. Populated 'nodes' with random positions
        // 2. Created all 'lines'
        // 3. Populated the 'collection' for rendering

        std::string bin_path = path + "/physics.bin";
        std::ifstream bin(bin_path, std::ios::binary);

        if (!bin.is_open()) {
            // If physics file is missing, we just keep the random positions
            return;
        }

        uint64_t count;
        bin.read(reinterpret_cast<char*>(&count), sizeof(count));

        for (uint64_t i = 0; i < count; ++i) {
            uint64_t len;
            bin.read(reinterpret_cast<char*>(&len), sizeof(len));

            std::string title(len, '\0');
            bin.read(title.data(), len);

            float coords[3];
            bin.read(reinterpret_cast<char*>(coords), sizeof(coords));

            // Update the node position if the title exists in our MM
            if (nodes.count(title)) {
                nodes[title].position = vec4(coords[0], coords[1], coords[2]);
            }
        }

        bin.close();

        // Synchronize the spheres and lines to the loaded positions
        update3DObjects();
    }

    void save(std::string path) {
        if (!are_sizes_matching()) {
            throw std::runtime_error("State invalid; cannot save");
        }

        std::string temp_path = path + ".tmp";
        std::string backup_path = path + ".bak";

        // 1. Clean up any leftover temp directory
        if (fs::exists(temp_path)) {
            fs::remove_all(temp_path);
        }
        fs::create_directories(temp_path);

        try {
            // 2. Save the underlying Mental Model (mm)
            mm.save(temp_path + "/Mental-Model");

            // 3. Save the physics positions to physics.bin
            std::ofstream bin(temp_path + "/physics.bin", std::ios::binary);
            if (!bin.is_open()) {
                throw std::runtime_error(
                    "Failed to open physics.bin for writing.");
            }

            uint64_t count = static_cast<uint64_t>(nodes.size());
            bin.write(reinterpret_cast<const char*>(&count), sizeof(count));

            for (const auto& [title, node] : nodes) {
                // Write Title length and data
                uint64_t len = static_cast<uint64_t>(title.size());
                bin.write(reinterpret_cast<const char*>(&len), sizeof(len));
                bin.write(title.data(), len);

                // Write position (x, y, z) as floats
                // We cast to float explicitly to ensure 4-byte size regardless
                // of vec4 internal precision
                float coords[3] = {static_cast<float>(node.position.x),
                                   static_cast<float>(node.position.y),
                                   static_cast<float>(node.position.z)};
                bin.write(reinterpret_cast<const char*>(coords),
                          sizeof(coords));
            }

            bin.close();

            // 4. Atomic Swap: Backup existing data, move temp to main, delete
            // backup
            if (fs::exists(path)) {
                if (fs::exists(backup_path)) fs::remove_all(backup_path);
                fs::rename(path, backup_path);
            }

            fs::rename(temp_path, path);

            if (fs::exists(backup_path)) {
                fs::remove_all(backup_path);
            }
        } catch (...) {
            // If anything fails during the process, clean up temp and rethrow
            if (fs::exists(temp_path)) fs::remove_all(temp_path);
            throw;
        }
    }

    bool operator==(const Physical_MM& b) {
        return mm == b.mm && nodes == b.nodes;
    }
};
