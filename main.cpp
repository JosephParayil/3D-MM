#include <SFML/Graphics.hpp>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
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

    myMM.nodes["Palau Economics"] = "1. The L3C is known as the Largest Palau.";
    myMM.nodes["L3C"] = "The Largest Lexington Crossing";

    myMM.nodes["Palau"] = "The greatest man (and nation)";
    myMM.nodes["Crooms"] = "The greatest high school in sanford";
    myMM.nodes["Swidersbree"] = "This is related to AP Gallagher";
    myMM.nodes["AP Gallagher"] = "The goat";

    myMM.connections.emplace_back("Palau Economics", "L3C");
    myMM.connections.emplace_back("Palau", "Palau Economics");
    myMM.connections.emplace_back("Palau", "Crooms");
    myMM.connections.emplace_back("Swidersbree", "Crooms");
    myMM.connections.emplace_back("Swidersbree", "AP Gallagher");

    std::cout << "non-physical mm created" << std::endl;

    Physical_MM mm_3d(myMM);

    std::cout << "physical mm created" << std::endl;

    while (window.isOpen()) {
        while (const auto& event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            camera.handleEvent(event);
        }
        camera.update();

        // - - DRAWING - -
        window.clear();
        mm_3d.render(window, camera);
        camera.drawCrosshairIfNeeded(window);
        window.display();
    }

    return 0;
}
