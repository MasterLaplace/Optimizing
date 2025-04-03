#include "Partition.hpp"

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "World Partition - Octree", sf::Style::Close);
    window.setFramerateLimit(60);

    WorldPartition worldPartition;

    sf::RectangleShape player_rect({10, 10});
    player_rect.setFillColor(sf::Color::Red);

    sf::Clock clock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();
        }

        float deltaTime = clock.restart().asSeconds();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            player_rect.move(500 * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            player_rect.move(-500 * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            player_rect.move(0, -500 * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            player_rect.move(0, 500 * deltaTime);

        worldPartition.update(player_rect);

        window.clear();
        worldPartition.draw(window);
        window.draw(player_rect);
        window.display();
    }

    return 0;
}

// Build : g++ -std=c++20 -o optimizing main.cpp -lsfml-graphics -lsfml-window -lsfml-system
// Run : ./optimizing
