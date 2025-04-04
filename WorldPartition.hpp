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
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
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

[[nodiscard]] float randfloat(const float min, const float max) noexcept
{
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
};

class Partition {
public:
    Partition(const glm::vec3 &pos, const glm::vec3 &size)
        : _pos(pos), _size(size), _octree(BoundaryBox(pos, size), MAX_CAPACITY, MAX_DEPTH)
    {
    }
    ~Partition() = default;

    void insert(const SpatialObject &obj) { _objects.emplace_back(obj); }

    void load_data()
    {
        if (_objects.empty() || (_loaded && _objects.size() == _octree.size()))
            return;
        _loaded = true;

        for (const auto &obj : _objects)
            _octree.insert(obj, BoundaryBox(obj.position, obj.size));

        std::cout << "Cellule " << _pos.x << " " << _pos.y << " chargée." << std::endl;
    }

    void unload_data()
    {
        if (!_loaded)
            return;

        _loaded = false;
        _octree.clear();
        std::cout << "Cellule " << _pos.x << " " << _pos.y << " déchargée." << std::endl;
    }

    void draw(sf::RenderWindow &window, const glm::vec3 &player_pos)
    {
        if (!_loaded || _objects.empty())
            return;

        glm::vec3 size{50, 50, std::numeric_limits<float>::max()};
        BoundaryBox boundaryBox(size * -0.5f + player_pos, size);

        DEBUG_LINE(auto start = std::chrono::high_resolution_clock::now());
        for (const auto &obj : _octree.search(boundaryBox))
        {
            sf::RectangleShape rect;
            rect.setPosition({obj->item.position.x, obj->item.position.y});
            rect.setSize({obj->item.size.x, obj->item.size.y});
            rect.setFillColor(sf::Color(obj->item.colour.r, obj->item.colour.g, obj->item.colour.b, obj->item.colour.a));
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
    glm::vec3 _pos;
    glm::vec3 _size;
    std::vector<SpatialObject> _objects;
    DynamicOctreeContainer<SpatialObject> _octree;
    DEBUG_LINE(size_t _objCount = 0);
    bool _loaded = false;
};

class WorldPartition {
public:
    WorldPartition() : _threadPool(std::thread::hardware_concurrency()) {}

    void insert(const std::vector<SpatialObject> &_objects)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto &obj : _objects)
        {
            sf::Vector2i grid = {static_cast<int>(obj.position.x / _size.x), static_cast<int>(obj.position.y / _size.y)};

            if (_cells.find(grid) == _cells.end())
                _cells[grid] = std::make_shared<Partition>(glm::vec3(grid.x * _size.x, grid.y * _size.y, 0), _size);

            _cells[grid]->insert(obj);
        }
    }

    void load_partition(sf::Vector2i grid)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_cells.find(grid) == _cells.end())
            _cells[grid] = std::make_shared<Partition>(glm::vec3(grid.x * _size.x, grid.y * _size.y, 0), _size);

        _threadPool.enqueue(&Partition::load_data, _cells[grid]);
    }

    void unload_partition(const std::pair<sf::Vector2i, std::shared_ptr<Partition>> &cell)
    {
        cell.second->unload_data();
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

        std::lock_guard<std::mutex> lock(_mutex);
        for (const auto &cell : _cells)
        {
            if (abs(cell.first.x - player_grid.x) > 1 || abs(cell.first.y - player_grid.y) > 1)
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
    glm::vec3 _size = {255, 255, std::numeric_limits<float>::max()};
    std::unordered_map<sf::Vector2i, std::shared_ptr<Partition>> _cells;
    std::mutex _mutex;
    ThreadPool _threadPool;
};
