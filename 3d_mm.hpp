#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
#include <cassert>
#include <random>
#include <unordered_map>


#include <sfml-3d/math4.hpp>
#include <TGUI/TGUI.hpp>
#include <TGUI/Backend/SFML-Graphics.hpp>

#include "mm.hpp"



const sf::Font FONT("JetBrainsMonoNerdFont-Medium.ttf");
const std::string BODY_GUI_PATH = "forms/body_editor.txt";
const vec4 LABEL_OFFSET(0, 2, 0);

const sf::Color HIGHLIGHT_COLOR = sf::Color::Blue;
const sf::Color LINE_LABEL_COLOR = sf::Color(128, 128, 128);
const sf::Color NEW_CONNECTION_COLOR = sf::Color::Green;

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
    std::unordered_map<std::string, std::unique_ptr<Node>> nodes;

    std::vector<std::unique_ptr<Line3D>> lines;
    Object3D_Collection collection;

    bool physics_paused = true;

    


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
            node_pair.second->sphere.position = node_pair.second->position;
            node_pair.second->label.position = node_pair.second->position + LABEL_OFFSET;
        }
        for (size_t i = 0; i < lines.size(); i++) {
            lines[i]->a = nodes[mm.connections[i].first]->position;
            lines[i]->b = nodes[mm.connections[i].second]->position;
        }
    }

    // = = = GUI STUFF = = =
    enum UserState {
        DEFAULT,
        WRITING,
        CONNECTING,
        DESTROYING,
        PRUNING,
        CREATING,
        SAVING
    };

    UserState user_state = UserState::DEFAULT;
    int hover_id = -1;
    int selected_id = -1;
    int selected_id2 = -1;
    
    tgui::Gui gui;

    tgui::ChildWindow::Ptr bodyEditorWindow;
    tgui::EditBox::Ptr bodyEditorEditBox;
    tgui::TextArea::Ptr bodyEditorTextArea;
    tgui::Button::Ptr addConnectionButton;
    tgui::Button::Ptr removeNodeButton;

    tgui::ChildWindow::Ptr deletionWindow;
    tgui::Label::Ptr deleteLabel;
    tgui::Button::Ptr confirmDeletionButton;
    tgui::Button::Ptr cancelDeleteButton;

    tgui::ChildWindow::Ptr deletionWindow_connection;
    tgui::Label::Ptr deleteLabel_connection;
    tgui::Button::Ptr confirmDeletionButton_connection;
    tgui::Button::Ptr cancelDeleteButton_connection;

    tgui::Button::Ptr addNodeButton;


    
    void exit_gui() {
        deletionWindow_connection->close();
        deletionWindow->close();    
        bodyEditorWindow->close();
        user_state = UserState::DEFAULT;
        selected_id = selected_id2 = -1;
    }


    Camera& camera;

    


    static sf::Vector2f rand_2d_pos(sf::RenderWindow& window) {
        unsigned int width = window.getSize().x;
        unsigned int height = window.getSize().y;

        const float padding = 0.5;

        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<float> x_dist(width*padding, width*(1-padding));
        static std::uniform_real_distribution<float> y_dist(height*padding, height*(1-padding));

        return sf::Vector2f(x_dist(gen), y_dist(gen));
    }


    bool isUserTyping() {
        auto focusedWidget = gui.getFocusedLeaf();
        if (!focusedWidget)
            return false;

        return focusedWidget->getWidgetType() == "EditBox" ||
            focusedWidget->getWidgetType() == "TextArea";
    }


    Physical_MM(MM mm_, Camera& camera) : mm(mm_), camera(camera), gui(camera.window) {
        gui.loadWidgetsFromFile(BODY_GUI_PATH);

        bodyEditorWindow = gui.get<tgui::ChildWindow>("GuiWindow");
        bodyEditorWindow->setCloseBehavior(tgui::ChildWindow::CloseBehavior::Hide);
        bodyEditorWindow->onClose([&]() {
            selected_id = -1; 
            camera.allowMouseLocking = true;
            user_state = UserState::DEFAULT;
        });

        bodyEditorEditBox = gui.get<tgui::EditBox>("TitleBox");
        bodyEditorEditBox->onTextChange([&](const tgui::String& text) {
            changeNodeTitle(id_to_title[selected_id], text.toStdString());
        });
        
        bodyEditorTextArea = gui.get<tgui::TextArea>("BodyBox");
        bodyEditorTextArea->onTextChange([&](const tgui::String& text) {
            changeNodeBody(id_to_title[selected_id], text.toStdString());
        });
        
        addConnectionButton = gui.get<tgui::Button>("AddConnectionButton");
        addConnectionButton->onPress([&]() {
            float temp = selected_id;
            bodyEditorWindow->close();
            selected_id = temp;
            user_state = UserState::CONNECTING;
        });

        removeNodeButton = gui.get<tgui::Button>("DeleteCellButton");
        removeNodeButton->onPress([&]() {
            user_state = UserState::DESTROYING;
            deletionWindow->setVisible(true);
        });

        deletionWindow = gui.get<tgui::ChildWindow>("DeleteWindow");
        deletionWindow->setCloseBehavior(tgui::ChildWindow::CloseBehavior::Hide);
        deletionWindow->onClose([&]() {
            user_state = UserState::WRITING;
        });
        tgui::Label::Ptr deleteLabel = gui.get<tgui::Label>("DeleteLabel");

        tgui::Button::Ptr confirmDeletionButton = gui.get<tgui::Button>("ConfirmDeleteButton");
        confirmDeletionButton->onPress([&]() {
            removeNode(id_to_title[selected_id]);
            exit_gui();
        });

        tgui::Button::Ptr cancelDeleteButton = gui.get<tgui::Button>("CancelDeleteButton");
        cancelDeleteButton->onPress([&]() {
            deletionWindow->close();
        });

        deletionWindow_connection = gui.get<tgui::ChildWindow>("DeleteConnectionWindow");
        deletionWindow_connection->setCloseBehavior(tgui::ChildWindow::CloseBehavior::Hide);
        deletionWindow_connection->onClose([&]() {
            user_state = UserState::DEFAULT;
        });
        tgui::Label::Ptr deleteLabel_connection = gui.get<tgui::Label>("DeleteConnectionLabel");

        tgui::Button::Ptr confirmDeletionButton_connection = gui.get<tgui::Button>("DeleteConnectionButton");
        confirmDeletionButton_connection->onPress([&]() {
            std::cout << mm.connections.size() << std::endl;
            std::cout << selected_id-nodes.size() << std::endl;
            mm.print();
            auto& connection = mm.connections[selected_id-nodes.size()];
            //std::cout << connection.first << " " << connection.second << std::endl;
            removeConnection(connection.first, connection.second);
            exit_gui();
        });

        tgui::Button::Ptr cancelDeleteButton_connection = gui.get<tgui::Button>("CancelDeleteConnectionButton");
        cancelDeleteButton_connection->onPress([&]() {
            deletionWindow_connection->close();
        });

        tgui::Button::Ptr addNodeButton = gui.get<tgui::Button>("AddNodeButton");
        addNodeButton->onPress([&]() {
            mat4 cf = camera.cf;
            cf = cf *  mat4::translation(0,0,20);
            vec4 position = cf.get_position();
            addNode(position);

        });




        //End of UI shenanigans

        size_t id = 0;
        for (auto& node : mm.nodes) {
            vec4 position = rand_position();
            vec4 velocity(0, 0, 0);
            Sphere3D sphere(position, 1.0f);

            nodes[node.first] = std::make_unique<Node>(position, velocity, sphere, node.first);
            id_to_title.push_back(node.first);

            collection.c.push_back({id, &nodes[node.first]->sphere});

            id++;
        }

        lines.reserve(mm.connections.size());

        for (auto& connection : mm.connections) {
            lines.push_back(std::make_unique<Line3D>(nodes[connection.first]->position,
                    nodes[connection.second]->position, 1.0f));

            collection.c.push_back({id, lines.back().get()});
            id++;
        }

        for (auto& node : mm.nodes) {
            collection.c.push_back({id, &nodes[node.first]->label});
            id++;
        }

        //mm.print();
    }



    // = = = ADDITION/REMOVAL/EDITING PROTOCOLS FOR NODES = = =

    void validityCheck() {
        assert(!mm.any_self_connections());
        assert(!mm.any_duplicate_connections());
        assert(mm.are_all_titles_valid());
        assert(mm.are_all_connection_references_valid());
        assert(are_sizes_matching());
    }

    void addNode(vec4 position) {
        //Find valid title 'New Node' or 'New Node 1', etc.
        std::string new_title = "New Node";
        int title_iteration = 1;
        while (mm.nodes.contains(new_title)) new_title = "New Node " + std::to_string(title_iteration++);
        
        //Adding to non-physical MM
        mm.nodes[new_title] = "New Body";

        //Adding to nodes
        Sphere3D sphere(position, 1.0f);
        nodes[new_title] = std::make_unique<Node>(position, vec4(), sphere, new_title);

        //Adding to collections; incrementing ids; adding to id-name store;
        int new_id = nodes.size() -1;
        id_to_title.push_back(new_title);

        for (auto& pair : collection.c) {
            if (pair.first >= new_id) {
                pair.first++;
            }
        }

        collection.c.push_back({new_id, &nodes[new_title]->sphere});

        collection.c.push_back({new_id + nodes.size() + mm.connections.size(), &nodes[new_title]->label});
        

        validityCheck();
    }

    void removeNode(std::string title) {
        assert(mm.nodes.contains(title) && nodes.contains(title));

        //Getting ID
        auto it = std::find(id_to_title.begin(), id_to_title.end(), title);
        assert(it != id_to_title.end());
        int id = std::distance(id_to_title.begin(), it);

        

        //Removing
        id_to_title.erase(it);
        mm.nodes.erase(title);
        nodes.erase(title);

        //Removing from collection
        int label_id = id + nodes.size() + mm.connections.size() + 1;
        std::erase_if(collection.c, [id, label_id](const auto& p) {
            return p.first == id || p.first == label_id;  // Remove both!
        });


        std::erase_if(collection.c, [id](const auto& p) {
            return p.first == id;
        });

        //Decrementing collections
        for (auto& pair : collection.c) {
            if (pair.first > label_id) {
                pair.first-=2;
            } else if (pair.first > id) {
                pair.first--; 
            }
        }


        //Removing all connections that once attached to this node but now must suffer the fate of death
        size_t i = 0;
        while (i < mm.connections.size()) {
            const auto& conn = mm.connections[i];
            
            if (conn.first == title || conn.second == title) {
                // removeConnection handles mm.connections, lines, and collection.c
                // It uses .erase(), so we DO NOT increment 'i'
                removeConnection(conn.first, conn.second, false);
            } else {
                // Only move to the next index if we didn't delete anything
                i++;
            }
        }

        validityCheck();
    }

    void changeNodeTitle(std::string oldTitle, std::string newTitle) {
        if (oldTitle == newTitle) return;
        if (!isValidFilename(newTitle)) return;
        assert(mm.nodes.contains(oldTitle) && nodes.contains(oldTitle));
        assert(!(mm.nodes.contains(newTitle) || nodes.contains(newTitle)));

        mm.nodes[newTitle] = std::move(mm.nodes[oldTitle]);
        mm.nodes.erase(oldTitle);

        auto it = std::find(id_to_title.begin(), id_to_title.end(), oldTitle);
        assert(it != id_to_title.end());
        int id = std::distance(id_to_title.begin(), it);
        
        id_to_title[id] = newTitle;
        nodes[newTitle] = std::move(nodes[oldTitle]);
        nodes.erase(oldTitle);

        nodes[newTitle]->label = Label3D(nodes[newTitle]->position + LABEL_OFFSET, newTitle, FONT);
        for (auto& pair : collection.c) {
            // If this ID corresponds to the label of the node we just renamed
            if (pair.first == id + nodes.size() + mm.connections.size()) {
                pair.second = &nodes[newTitle]->label;
            }
        }

        //Updating connections who previously referred to the old title
        for (auto& connection : mm.connections) {
            if (connection.first == oldTitle) connection.first = newTitle;
            if (connection.second == oldTitle) connection.second = newTitle;
        }

        validityCheck();
    }

    void changeNodeBody(std::string title, std::string body) {
        assert(mm.nodes.contains(title) && nodes.contains(title));

        mm.nodes[title] = body;

        validityCheck();
    }


    // = = = ADDITION/REMOVAL/EDITING PROTOCOLS FOR CONNECTIONS = = =

    void addConnection(std::string first, std::string second) {
        auto it = std::find_if(mm.connections.begin(), mm.connections.end(),
            [&](const auto& p) {
                return (p.first == first && p.second == second) ||
                    (p.first == second && p.second == first);
        });

        if (it != mm.connections.end()) return;


        int id = nodes.size() + mm.connections.size();

        mm.connections.push_back(std::make_pair(first, second));

        std::cout << "pushed connection" << std::endl;
        //Adding line
        lines.push_back(std::make_unique<Line3D>(nodes[first]->position, nodes[second]->position, 1.0f));
        
        //Incrementing ids:
        for (auto& pair : collection.c) {
            if (pair.first >= id) {
                pair.first++;
            }
        }

        std::cout << "pushed line and incremented stuff" << std::endl;

        //Adding to collections
        collection.c.push_back({id, lines.back().get()});


        std::cout << "pushed collection" << std::endl;
        
        validityCheck();
    }

    void removeConnection(std::string first, std::string second, bool checkValidity = true) {
        auto it = std::find_if(mm.connections.begin(), mm.connections.end(),
            [&](const auto& p) {
                return (p.first == first && p.second == second) ||
                    (p.first == second && p.second == first);
        }); 
        assert(it != mm.connections.end());
        int index = std::distance(mm.connections.begin(), it);        
        
        int id = index + nodes.size();

        //Erasing from collection
        std::erase_if(collection.c, [id](const auto& p) {
            return p.first == id;
        });

        lines.erase(lines.begin() + index);
        mm.connections.erase(it);

        //Decrementing ids in collection
        for (auto& pair : collection.c) {
             if (pair.first > id) {
                pair.first --;
            }
        }
    

        if (checkValidity) validityCheck();
    }




    // = = = REGULAR UPDATES = = =

    void render(sf::RenderWindow& window, Camera& camera) {
        collection.depthSort(camera);

        hover_id = -1;
        int hover_id_connection = -1;

        // If our mouse hovers both a node and a connection, we want to prefer
        // the node
        for (auto it = collection.c.rbegin(); it != collection.c.rend(); ++it) {
            auto shape = it->second->computeShape(window, camera);
            if (!shape) continue;

            sf::Vector2f mousePos = sf::Vector2f(sf::Mouse::getPosition(window));
            if (shape->computeCollisionWithPoint(mousePos)) {
                if (it->first < nodes.size() ||
                    it->first >= nodes.size() + mm.connections.size()) {  // NODE ALERT
                    hover_id = it->first;
                    break;
                } else {  // Meh... a connection. Only add this if we have not
                          // found a connection before. Even then, continue the
                          // search for a node if possible
                    if (hover_id_connection == -1 && hover_id == -1) {
                        hover_id_connection = it->first;
                    }
                }
            }
        }

        if (hover_id == -1) {
            hover_id = hover_id_connection;
        } else if (hover_id >= nodes.size() + mm.connections.size()) {
            hover_id -= nodes.size() + mm.connections.size();
        }

        for (auto& pair : collection.c) {
            if ( pair.first == hover_id ||
                pair.first - nodes.size() - mm.connections.size() == hover_id && hover_id != -1) {
                pair.second->draw(window, camera, HIGHLIGHT_COLOR);
            } else {
                pair.second->draw(window, camera, sf::Color::White);
            }
            //Needed in Windows to prevent font drawing from corrupting everything else
            //More efficient way to do this if you draw all the fonts, run this once, and draw everything else
            window.resetGLStates();
        }

        if (user_state == UserState::WRITING) {
            //we selected a node 
            tgui::Vector2f ui_pos = bodyEditorWindow->getPosition();
            std::string title = id_to_title[selected_id];
            Node& node = *(nodes[title]);
            draw3DLineTo2DPoint(window, node.position, ui_pos, camera, 3.0, LINE_LABEL_COLOR);
                
        } else if (user_state == UserState::CONNECTING) {
            //we are in the process of forming a new connection
            std::string title = id_to_title[selected_id];
            Node& node = *(nodes[title]);

            sf::Vector2f mouse = sf::Vector2f(sf::Mouse::getPosition(window));

            draw3DLineTo2DPoint(window, node.position, mouse, camera, 3.0, NEW_CONNECTION_COLOR);

        } else if (user_state == UserState::PRUNING) { // we selected a connection
            tgui::Vector2f ui_pos = deletionWindow_connection->getPosition();

            int connection_index = selected_id - nodes.size();
            vec4 midPos = (lines[connection_index]->a + lines[connection_index]->b) * 0.5f;
            draw3DLineTo2DPoint(window, midPos, ui_pos, camera, 3.0, LINE_LABEL_COLOR);
        }

        gui.draw();
    }



    

    bool handleEvent(sf::RenderWindow& window, const std::optional<sf::Event>& event) {
        bool typing = isUserTyping();
        if (const auto* mouseButtonPressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            if (mouseButtonPressed->button == sf::Mouse::Button::Left) {  
                if (hover_id != -1) {
                    if (hover_id < nodes.size()) { //we selected a true node
                        if (user_state == UserState::CONNECTING) {
                            std::cout << "Connecting" << std::endl;
                            if (hover_id != selected_id) {
                                std::cout << selected_id << " " << hover_id << std::endl;
                                addConnection(id_to_title[selected_id], id_to_title[hover_id]);
                            }
                            
                            user_state = UserState::DEFAULT;
                        } else {
                            deletionWindow_connection->close();

                            selected_id = hover_id;
                            camera.allowMouseLocking = false;
                            camera.mouseLocked = false;
                            sf::Vector2f ui_pos = rand_2d_pos(window);
                            bodyEditorWindow->setVisible(true);
                            bodyEditorWindow->setPosition(ui_pos.x, ui_pos.y);
                            bodyEditorEditBox->setText(id_to_title[selected_id]);
                            bodyEditorTextArea->setText(mm.nodes[id_to_title[selected_id]]);
                            user_state = UserState::WRITING;
                        }
                    } else { // we selected a chud connection
                        bodyEditorWindow->close();
                        
                        selected_id = hover_id;
                        camera.allowMouseLocking = false;
                        camera.mouseLocked = false;
                        sf::Vector2f ui_pos = rand_2d_pos(window);
                        deletionWindow_connection->setVisible(true);
                        bodyEditorWindow->setPosition(ui_pos.x, ui_pos.y);
                        user_state = UserState::PRUNING;
                    }
                }
            }
        } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            if (!typing && keyPressed->scancode == sf::Keyboard::Scan::Space) {
                physics_paused = !physics_paused;
            } else if (keyPressed->scancode == sf::Keyboard::Scan::Escape) {
                    exit_gui();
            }
        }



        gui.handleEvent(*event);

        return typing;
    }

    void physics_step() {
        if (physics_paused) return;
        /*
        Forces:
        - Attractive connection force (hooke's law)
        - Repelling node force (columb's law)
        - Dampening friction force
        - Bounding attractive force
        */
        const float evth_const = 0.1;
        const float hooke_K = 0.01 * evth_const;
        const float columb_K = 250 * evth_const;
        const float dampening_constant = std::pow(0.9, evth_const);
        const float bounding_constant = 0.000001 * evth_const;

        //Update velocities
        for (auto& connection : mm.connections) {
            vec4& pos1 = nodes[connection.first]->position;
            vec4& pos2 = nodes[connection.second]->position;
            nodes[connection.first]->velocity += (pos2 - pos1)*hooke_K;
            nodes[connection.second]->velocity += (pos1 - pos2)*hooke_K;
        }

        for (auto& node_pair_i: nodes) {
            //Columb's force + bounding attractive force
            for (auto& node_pair_j: nodes) {
                if (&node_pair_i==&node_pair_j) continue;
                
                float x_dist = node_pair_i.second->position.x-node_pair_j.second->position.x;
                float y_dist = node_pair_i.second->position.y-node_pair_j.second->position.y;
                float z_dist = node_pair_i.second->position.z-node_pair_j.second->position.z;

                float r_cubed = pow(pow(x_dist, 2) + pow(y_dist, 2) + pow(z_dist, 2), 1.5);

                vec4 columb_factor(columb_K*x_dist/r_cubed, columb_K*y_dist/r_cubed, columb_K*z_dist/r_cubed);
                vec4 bounding_factor(-bounding_constant * x_dist, -bounding_constant * y_dist, -bounding_constant * z_dist);
                node_pair_i.second->velocity += columb_factor + bounding_factor;
            }
            
            //Dampening force
            node_pair_i.second->velocity *= dampening_constant;

        }

        //Update positions
        for (auto& node_pair : nodes) {
            node_pair.second->position += node_pair.second->velocity;
        }

        //Update objects
        update3DObjects();

    }

    //FILE IO


    bool are_sizes_matching() const {
        return mm.nodes.size() == nodes.size() &&
               mm.connections.size() == lines.size();
    }

    //*
    Physical_MM(std::string path, Camera& camera)
    : Physical_MM(
          [&path]() {
              assert(fs::exists(path) && fs::is_directory(path));
              return MM(path + "/Mental-Model");
          }(),
          camera
      ) {
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
                nodes[title]->position = vec4(coords[0], coords[1], coords[2]);
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
                float coords[3] = {static_cast<float>(node->position.x),
                                   static_cast<float>(node->position.y),
                                   static_cast<float>(node->position.z)};
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

    bool operator==(const Physical_MM& b) const {
        if (!(mm == b.mm) || nodes.size() != b.nodes.size()) return false;
        for (auto const& [title, node_ptr] : nodes) {
            auto it = b.nodes.find(title);
            if (it == b.nodes.end() || !(*node_ptr == *it->second)) return false;
        }
        return true;
    }
};