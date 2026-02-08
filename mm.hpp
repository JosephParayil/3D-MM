#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <map>
#include <cassert>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;



//For both Windows and Linux systems
bool isValidFilename(const std::string& name) {
    if (name.empty())
        return false;

    // Cannot be "." or ".."
    if (name == "." || name == "..")
        return false;

    // Windows forbidden characters
    const std::string forbidden = "<>:\"/\\|?*";

    for (unsigned char c : name) {
        // Control characters (0–31)
        if (c < 32)
            return false;

        if (forbidden.find(c) != std::string::npos)
            return false;
    }

    // Cannot end with space or dot (Windows restriction)
    if (name.back() == ' ' || name.back() == '.')
        return false;

    // Windows reserved device names (case-insensitive)
    static const std::unordered_set<std::string> reserved = {
        "CON", "PRN", "AUX", "NUL",
        "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
        "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
    };

    // Extract base name before dot (e.g. "CON.txt" is still invalid)
    std::string base = name;
    auto dotPos = name.find('.');
    if (dotPos != std::string::npos)
        base = name.substr(0, dotPos);

    // Convert to uppercase for comparison
    std::string upperBase = base;
    std::transform(upperBase.begin(), upperBase.end(), upperBase.begin(),
                   [](unsigned char c){ return std::toupper(c); });

    if (reserved.find(upperBase) != reserved.end())
        return false;

    return true;
}


std::string& toLower(std::string& str) {
    std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c){ return std::tolower(c); });

    return str;
}


bool lowercaseComparison(std::string a, std::string b) {
    std::string a_ = a;
    std::string b_ = b;
    toLower(a_); toLower(b_);

    return a_ == b_;
}


const std::string LINK_CODE = "@#$%";

struct MM {
    std::map<std::string, std::string> nodes;
    std::vector<std::pair<std::string, std::string>> connections;

    MM() {}

    MM(const std::string& dir) {
        fs::path dirPath(dir);

        // Validate the directory
        assert(fs::exists(dirPath) && "Directory does not exist.");
        assert(fs::is_directory(dirPath) && "Path is not a directory.");

        // Ensure CONNECTIONS.txt exists
        fs::path connPath = dirPath / "CONNECTIONS.txt";
        assert(fs::exists(connPath) && "CONNECTIONS.txt not found in directory.");
        assert(fs::is_regular_file(connPath) && "CONNECTIONS.txt is not a regular file.");

        // Load all node .txt files (everything except CONNECTIONS.txt)
        for (const auto& entry : fs::directory_iterator(dirPath)) {
            assert(fs::is_regular_file(entry.path()) && "Non-file entry found in directory.");

            std::string filename = entry.path().filename().string();
            assert(filename.size() > 4 && filename.substr(filename.size() - 4) == ".txt"
                && "Non-.txt file found in directory.");

            if (filename == "CONNECTIONS.txt") continue;

            std::string title = filename.substr(0, filename.size() - 4);
            assert(isValidFilename(title) && "Invalid filename used as node title.");
            assert(nodes.find(title) == nodes.end() && "Duplicate node title detected.");

            std::ifstream file(entry.path());
            assert(file.is_open() && "Failed to open node file.");

            std::string body((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
            nodes[title] = body;
        }

        // Load connections
        std::ifstream connFile(connPath);
        assert(connFile.is_open() && "Failed to open CONNECTIONS.txt.");

        std::string line;
        while (std::getline(connFile, line)) {
            if (line.empty()) continue;

            auto tabPos = line.find('\t');
            assert(tabPos != std::string::npos && "Malformed connection line, no tab delimiter found.");
            assert(tabPos > 0 && "Malformed connection line, first title is empty.");
            assert(tabPos < line.size() - 1 && "Malformed connection line, second title is empty.");

            std::string a = line.substr(0, tabPos);
            std::string b = line.substr(tabPos + 1);

            // Ensure no extra tabs snuck in
            assert(b.find('\t') == std::string::npos && "Malformed connection line, more than one tab delimiter.");

            // Both titles must exist as nodes
            assert(nodes.find(a) != nodes.end() && "Connection references non-existent node.");
            assert(nodes.find(b) != nodes.end() && "Connection references non-existent node.");

            connections.push_back({a, b});
        }
    }

    bool are_all_titles_valid() {
        for (auto& node : nodes) {
            if (!isValidFilename(node.first)) {
                return false;
            }
        }
        return true;
    }
    bool any_self_connections() {
        for (auto& connection: connections) {
            if (lowercaseComparison(connection.first, connection.second)) return true;
        }
        return false;
    }
    bool any_duplicate_connections() {
        for (auto it = connections.begin(); it != connections.end(); it++) {
            for (auto jt = it; jt != connections.end(); jt++) {
                if (it == jt) continue;
                if ((it->first == jt->first && it->second == jt->second) || (it->first == jt->second && it->second == jt->first)) {
                    return true;
                }
            }
        }

        return false;
    }      

    

    bool are_all_connection_references_valid() const {
        for (const auto& [a, b] : connections) {
            if (!nodes.contains(a) || !nodes.contains(b))
                return false;
        }
        return true;
    }

    void print() const {
        std::cout << "= = = MM = = =\n";
        std::cout << "Nodes:\n";
        for (const auto& node : nodes) {
            std::cout << node.first << ": " << node.second << std::endl;
        }
        std::cout << "\nConnections:\n";
        for (const auto& [a, b] : connections) {
           std::cout << "a: " << a << " b: " << b << std::endl;
        }
    }

    void save(std::string dir) {
        // 1. Validate state before doing anything
        if (!are_all_titles_valid()) {
            throw std::runtime_error("Cannot save: one or more node titles are not valid filenames.");
        }

        fs::path dirPath(dir);
        fs::path tempDir = dirPath.parent_path() / (dirPath.filename().string() + "_tmp");

        // 2. Write everything into a temporary directory first.
        //    If anything goes wrong here, the original data on disk is untouched.
        try {
            fs::remove_all(tempDir);                // clear any leftover temp dir
            fs::create_directories(tempDir);

            // Write each node as its own .txt file
            for (const auto& [title, body] : nodes) {
                std::ofstream file(tempDir / (title + ".txt"));
                if (!file.is_open()) {
                    throw std::runtime_error("Failed to open file for node: " + title);
                }
                file << body;
            }

            // Write the CONNECTIONS file.
            // Format: one connection per line, "title1\ttitle2" (tab-separated).
            std::ofstream connFile(tempDir / "CONNECTIONS.txt");
            if (!connFile.is_open()) {
                throw std::runtime_error("Failed to open CONNECTIONS.txt");
            }
            for (const auto& [a, b] : connections) {
                connFile << a << "\t" << b << "\n";
            }

        } catch (...) {
            // Temp dir write failed — clean up the temp dir and rethrow.
            // Original directory is completely untouched.
            fs::remove_all(tempDir);
            throw;
        }

        // 3. Temp dir is fully written and verified. Now do the atomic swap:
        //    remove the old directory (if it exists), then rename temp -> final.
        try {
            fs::remove_all(dirPath);
            fs::rename(tempDir, dirPath);
        } catch (...) {
            // Swap failed. Temp dir may or may not still exist.
            // Don't delete it — it holds the only good copy now.
            throw std::runtime_error(
                "Critical: directory swap failed. Your data is in the temp directory: " + tempDir.string()
            );
        }
    }

    bool operator==(const MM& b) const {
        return nodes == b.nodes && connections == b.connections;
    }

};

