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
 * @file Partition.hpp
 * @brief 2D World Partitioning and Thread Pool for Dynamic Octree in 3D Space.
 *
 * @author @MasterLaplace
 * @version 0.0.0
 * @date 2025-04-03
 **************************************************************************/

#pragma once

#include "DynamicOctree.hpp"
#include <SFML/Graphics.hpp>
#include <future>
#include <limits>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>

namespace std {
template <> struct hash<sf::Vector2i> {
    std::size_t operator()(const sf::Vector2i &v) const noexcept
    {
        return std::hash<int>()(v.x) ^ (std::hash<int>()(v.y) << 1);
    }
};
} // namespace std

class ThreadPool {
public:
    ThreadPool(size_t threads) : stop(false)
    {
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back([this] {
                for (;;)
                {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                        if (this->stop && this->tasks.empty())
                            return;
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    }
                    task();
                }
            });
    }

    template <class F, class... Args>
    auto enqueue(F &&f, Args &&...args) -> std::future<typename std::result_of<F(Args...)>::type>
    {
        using return_type = typename std::result_of<F(Args...)>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            if (stop)
                throw std::runtime_error("enqueue on stopped ThreadPool");

            tasks.emplace([task]() { (*task)(); });
        }
        condition.notify_one();
        return res;
    }

    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;
};

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

    void draw(sf::RenderWindow &window, const sf::Vector3f &player_pos) const
    {
        if (!_loaded)
            return;

        sf::Vector3f size{50, 50, std::numeric_limits<float>::max()};
        BoundaryBox boundaryBox(size * -0.5f + player_pos, size);

        for (const auto &obj : _octree.search(boundaryBox))
        {
            sf::RectangleShape rect;
            rect.setPosition({obj->item.vPos.x, obj->item.vPos.y});
            rect.setSize({obj->item.vSize.x, obj->item.vSize.y});
            rect.setFillColor(obj->item.colour);
            window.draw(rect);
        }

        sf::RectangleShape rect;
        rect.setPosition({_pos.x, _pos.y});
        rect.setSize({_size.x, _size.y});
        rect.setFillColor(sf::Color::Transparent);
        rect.setOutlineColor(sf::Color::White);
        rect.setOutlineThickness(1);
        window.draw(rect);
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
    DynamicOctreeContainer<BoundaryBox, SomeObjectWithArea> _octree;
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
