// #define RAYTRACING
#ifndef RAYTRACING
#    include "WorldPartition.hpp"
#else
#    include "Raytracing.hpp"
#endif

int main()
{
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u{800, 600}), "World Partition - Octree", sf::Style::Close);
    window.setFramerateLimit(60);

#ifndef RAYTRACING
    WorldPartition worldPartition;

    size_t nbObjects = 100000;
    glm::vec3 pos = {0, 0, 0};
    glm::vec3 size = {800, 50, 600};
    glm::vec3 maxArea = pos + size;
    std::vector<SpatialObject> objects;

    objects.reserve(nbObjects);

    for (size_t i = 0; i < nbObjects; ++i)
    {
        SpatialObject obj;
        obj.position = {randfloat(pos.x, maxArea.x), randfloat(pos.y, maxArea.y), randfloat(pos.z, maxArea.z)};
        obj.velocity = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.size = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
        obj.colour = glm::vec4(rand() % 255, rand() % 255, rand() % 255, 255);

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
    createInfo.width = 256;
    createInfo.height = 256;
    createInfo.ray_per_pixel = 16;
    Raytracing raytracing(createInfo);
#endif

    sf::RectangleShape player_rect({10, 10});
    player_rect.setFillColor(sf::Color::Red);
    float player_height = 0.0f;

    sf::Clock clock;
    while (window.isOpen())
    {
        // SFML 3: pollEvent returns std::optional<sf::Event>
        while (auto event = window.pollEvent())
        {
            // Close when window closed or Escape pressed as an event
            if (event->is<sf::Event::Closed>() ||
                (event->is<sf::Event::KeyPressed>() && event->getIf<sf::Event::KeyPressed>()->code == sf::Keyboard::Key::Escape))
            {
                window.close();
            }
        }

        float deltaTime = clock.restart().asSeconds();

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right))
            player_rect.move(sf::Vector2f{500.0f * deltaTime, 0.0f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left))
            player_rect.move(sf::Vector2f{-500.0f * deltaTime, 0.0f});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up))
            player_rect.move(sf::Vector2f{0.0f, -500.0f * deltaTime});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down))
            player_rect.move(sf::Vector2f{0.0f, 500.0f * deltaTime});
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Z))
            player_height += 50 * deltaTime;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S))
            player_height -= 50 * deltaTime;

        player_height = std::clamp(player_height, 0.0f, size.y);

        window.clear();

#ifndef RAYTRACING
        pos = glm::vec3({player_rect.getPosition().x, player_height, player_rect.getPosition().y});
        worldPartition.update(pos);
        worldPartition.draw(window, pos);
#else
        const sf::Vector2f &pos = player_rect.getPosition();
        raytracing.update(window, glm::vec3(pos.x, player_height, pos.y));
#endif

        window.draw(player_rect);
        window.display();
    }

    return 0;
}

// Build : g++ -std=c++20 -o optimizing main.cpp -I/usr/include -lsfml-graphics -lsfml-window -lsfml-system
// Run : ./optimizing
