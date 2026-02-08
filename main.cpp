
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>

#include <SFML/Graphics.hpp>
#include <sfml-3d/3d_camera.hpp>
#include <sfml-3d/3d_engine.hpp>
#include <sfml-3d/math4.hpp>

#include "3d_mm.hpp"

int main() {
    const int HEIGHT = 1400;
    const int WIDTH = 2100;
    sf::RenderWindow window(sf::VideoMode({WIDTH, HEIGHT}), "Mental Modeller");

    std::cout << "window created" << std::endl;

    Camera camera(window, 60.0f, 0.001f, 2.f, 10.f, 0.5f);

    MM myMM;

    Physical_MM* mm_3d = new Physical_MM(myMM, camera);



    bool locked = false;
    while (window.isOpen()) {
        while (const auto& event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scan::P) {
                    std::cout << "\n--- File Management ---\n";
                    std::cout << "Would you want to Save (S) or Load (L)? ";
                    std::string choice;
                    std::cin >> choice;

                    if (choice == "S" || choice == "s") {
                        std::cout << "Enter directory name to save: ";
                        std::string path;
                        std::cin >> path;
                        
                        mm_3d->save(path);
                        std::cout << "System saved to: " << path << std::endl;

                    } else if (choice == "L" || choice == "l") {
                        std::cout << "Enter directory name to load: ";
                        std::string path;
                        std::cin >> path;

                        // Re-construct mm_3d by calling the path-based constructor
                        // This replaces the current object with the loaded state
                        delete mm_3d;
                        mm_3d = new Physical_MM(path, camera);
                        
                        std::cout << "System loaded from: " << path << std::endl;
                    } else {
                        std::cout << "Operation cancelled (invalid input)." << std::endl;
                    }
                }
            }
            
            if (!locked) {
                camera.handleEvent(event);
            }

            locked = mm_3d->handleEvent(window, event);
        }
        if (!locked) {
            camera.update();
        }
        
        mm_3d->physics_step();

        // - - DRAWING - -
        window.clear();
        mm_3d->render(window, camera);
        camera.drawCrosshairIfNeeded(window);
        window.display();
    }

    delete mm_3d;
    return 0;
}