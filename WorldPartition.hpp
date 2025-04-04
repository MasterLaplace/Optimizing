/**************************************************************************
 * Optimizing v0.0.0
 *
 * Optimizing is a C/CPP software package, part of the Laplace-Project.
 * It is designed to provide a set of tools and utilities for optimizing
 * various aspects of software development, including performance,
 * memory usage, and code organization.
 *
 * This file is part of the Optimizing project that is under Anti-NN License.
 * https://github.com/MasterLaplace/Anti-NN_LICENSE
 * Copyright © 2025 by @MasterLaplace, All rights reserved.
 *
 * Optimizing is a free software: you can redistribute it and/or modify
 * it under the terms of the Anti-NN License as published by MasterLaplace.
 * See the Anti-NN License for more details.
 *
 * @file WorldPartition.hpp
 * @brief 2D World Partitioning and Thread Pool for Dynamic Octree in 3D Space.
 *
 * @author @MasterLaplace
 * @version 0.0.0
 * @date 2025-04-03
 **************************************************************************/

#pragma once

#include "DynamicOctree.hpp"
#include "ThreadPool.hpp"
#include <limits>
#include <unordered_map>

#ifdef DEBUG
#    define DEBUG_LINE(_) _
#else
#    define DEBUG_LINE(_)
#endif

namespace std {
template <> struct hash<sf::Vector2i> {
    std::size_t operator()(const sf::Vector2i &v) const noexcept
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
} // namespace std

class Partition {
public:
    Partition(const sf::Vector3f &pos, const sf::Vector3f &size)
        : _pos(pos), _size(size), _octree(BoundaryBox(pos, size), MAX_CAPACITY, MAX_DEPTH)
    {
    }

    void load_data()
    {
        _loaded = true;

        size_t nbObjects = 1000;
        sf::Vector3f maxArea = _pos + _size;

        _objects.reserve(nbObjects);

        for (size_t i = 0; i < nbObjects; ++i)
        {
            SomeObjectWithArea obj;
            obj.vPos = {randfloat(_pos.x, maxArea.x), randfloat(_pos.y, maxArea.y), randfloat(_pos.z, maxArea.z)};
            obj.vVel = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.vSize = {randfloat(0, 10), randfloat(0, 10), randfloat(0, 10)};
            obj.colour = sf::Color(rand() % 255, rand() % 255, rand() % 255, 255);

            _objects.emplace_back(obj);
            _octree.insert(obj, BoundaryBox(obj.vPos, obj.vSize));
        }

        std::cout << "Cellule " << _pos.x << " " << _pos.y << " chargée." << std::endl;
    }

    void draw(sf::RenderWindow &window, const sf::Vector3f &player_pos)
    {
        if (!_loaded)
            return;

        sf::Vector3f size{50, 50, std::numeric_limits<float>::max()};
        BoundaryBox boundaryBox(size * -0.5f + player_pos, size);

        DEBUG_LINE(auto start = std::chrono::high_resolution_clock::now());
        for (const auto &obj : _octree.search(boundaryBox))
        {
            sf::RectangleShape rect;
            rect.setPosition({obj->item.vPos.x, obj->item.vPos.y});
            rect.setSize({obj->item.vSize.x, obj->item.vSize.y});
            rect.setFillColor(obj->item.colour);
            window.draw(rect);
            DEBUG_LINE(++_objCount);
        }

        sf::RectangleShape rect;
        rect.setPosition({_pos.x, _pos.y});
        rect.setSize({_size.x, _size.y});
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1);
        window.draw(rect);

#ifdef DEBUG
        std::chrono::duration<float> duration = std::chrono::high_resolution_clock::now() - start;

        // Log to file if duration exceeds threshold of 0.1 seconds
        if (duration.count() > 0.1f)
        {
            std::ofstream logFile("DebugDynamicOctree.log", std::ios_base::app);
            logFile << "OctTree: " << _objCount << " objects displayed in " << duration.count() << " seconds\n";
        }

        _octree.draw(window, boundaryBox);

        _objCount = 0;
#endif
    }

private:
    [[nodiscard]] static float randfloat(const float min, const float max)
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    };

private:
    sf::Vector3f _pos;
    sf::Vector3f _size;
    std::vector<SomeObjectWithArea> _objects;
    DynamicOctreeContainer<SomeObjectWithArea> _octree;
    DEBUG_LINE(size_t _objCount = 0);
    bool _loaded = false;
};

class WorldPartition {
public:
    WorldPartition() : _threadPool(std::thread::hardware_concurrency()) {}

    void load_partition(sf::Vector2i grid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cells.find(grid) == _cells.end())
        {
            _cells[grid] = std::make_shared<Partition>(sf::Vector3f(grid.x * _size.x, grid.y * _size.y, 0), _size);
            _threadPool.enqueue(&Partition::load_data, _cells[grid]);
            std::cout << "Lancement du chargement de la cellule " << grid.x << ", " << grid.y << std::endl;
        }
    }

    void unload_partition(sf::Vector2i grid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cells.find(grid) != _cells.end())
        {
            _cells.erase(grid);
            std::cout << "Déchargement de la cellule " << grid.x << ", " << grid.y << std::endl;
        }
    }

    void update(sf::RectangleShape player_rect)
    {
        sf::Vector2i player_grid = {static_cast<int>(player_rect.getPosition().x / _size.x),
                                    static_cast<int>(player_rect.getPosition().y / _size.y)};

        for (int x = player_grid.x - 1; x <= player_grid.x + 1; ++x)
        {
            for (int y = player_grid.y - 1; y <= player_grid.y + 1; ++y)
            {
                load_partition({x, y});
            }
        }

        std::vector<sf::Vector2i> cells_to_unload;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            for (const auto &cell : _cells)
            {
                if (abs(cell.first.x - player_grid.x) > 1 || abs(cell.first.y - player_grid.y) > 1)
                {
                    cells_to_unload.push_back(cell.first);
                }
            }
        }

        for (const auto &cell : cells_to_unload)
        {
            unload_partition(cell);
        }
    }

    void draw(sf::RenderWindow &window, const sf::Vector2f &player_pos)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto &cell : _cells)
        {
            cell.second->draw(window, {player_pos.x, player_pos.y, 0});
        }
    }

private:
    sf::Vector3f _size = {255, 255, std::numeric_limits<float>::max()};
    std::unordered_map<sf::Vector2i, std::shared_ptr<Partition>> _cells;
    std::mutex _mutex;
    ThreadPool _threadPool;
};
