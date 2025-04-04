#define RAYTRACING
#ifndef RAYTRACING
#    include "WorldPartition.hpp"
#else
#    include "Raytracing.hpp"
#endif

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "World Partition - Octree", sf::Style::Close);
    window.setFramerateLimit(60);

#ifndef RAYTRACING
    WorldPartition worldPartition;

    size_t nbObjects = 100000;
    sf::Vector3f pos = {0, 0, 0};
    sf::Vector3f size = {800, 600, 50};
    sf::Vector3f maxArea = pos + size;
    std::vector<SpatialObject> objects;

    objects.reserve(nbObjects);

    for (size_t i = 0; i < nbObjects; ++i)
    {
        SpatialObject obj;
        obj.position = {randfloat(pos.x, maxArea.x), randfloat(pos.y, maxArea.y), randfloat(pos.z, maxArea.z)};
        obj.velocity = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.size = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.colour = sf::Color(rand() % 255, rand() % 255, rand() % 255, 255);

        objects.emplace_back(obj);
    }

    worldPartition.insert(objects);
#else
    Raytracing::CreateInfo createInfo;
    createInfo.position = {50, 50, 300};
    createInfo.direction = glm::normalize(glm::vec3{0, -0.042612, -1});
    createInfo.background_color = {0, 0, 0};
    createInfo.fov = 0.5135f; // ~30 degrés
    createInfo.depth = 5u;
    createInfo.width = 800;
    createInfo.height = 600;
    createInfo.ray_per_pixel = 16;
    Raytracing raytracing(createInfo);
#endif

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

        window.clear();

#ifndef RAYTRACING
        worldPartition.update(player_rect);
        worldPartition.draw(window, player_rect.getPosition());
#else
        raytracing.update(window);
#endif

        window.draw(player_rect);
        window.display();
    }

    return 0;
}

// Build : g++ -std=c++20 -o optimizing main.cpp -I/usr/include -lsfml-graphics -lsfml-window -lsfml-system
// Run : ./optimizing
