#include "WorldPartition.hpp"

[[nodiscard]] static float randfloat(const float min, const float max) noexcept
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "World Partition - Octree", sf::Style::Close);
    window.setFramerateLimit(60);

    WorldPartition worldPartition;

    size_t nbObjects = 100000;
    sf::Vector3f pos = {0, 0, 0};
    sf::Vector3f size = {800, 600, 50};
    sf::Vector3f maxArea = pos + size;
    std::vector<SomeObjectWithArea> objects;

    objects.reserve(nbObjects);

    for (size_t i = 0; i < nbObjects; ++i)
    {
        SomeObjectWithArea obj;
        obj.vPos = {randfloat(pos.x, maxArea.x), randfloat(pos.y, maxArea.y), randfloat(pos.z, maxArea.z)};
        obj.vVel = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.vSize = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.colour = sf::Color(rand() % 255, rand() % 255, rand() % 255, 255);

        objects.emplace_back(obj);
    }

    worldPartition.insert(objects);

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
        worldPartition.draw(window, player_rect.getPosition());
        window.draw(player_rect);
        window.display();
    }

    return 0;
}

// Build : g++ -std=c++20 -o optimizing main.cpp -I/usr/include -lsfml-graphics -lsfml-window -lsfml-system
// Run : ./optimizing
