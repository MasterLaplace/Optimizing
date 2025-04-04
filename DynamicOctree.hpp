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
 * @file DynamicOctree.hpp
 * @brief Dynamic Octree Implementation for 3D Spatial Partitioning.
 *
 * This file provides a dynamic octree implementation for efficient spatial
 * partitioning in 3D space. It allows for the insertion, searching, and removal
 * of objects within a defined 3D boundary. The octree is designed to handle
 * dynamic objects and can be resized as needed.
 *
 * The implementation is based on the work of javidx9, which can be
 * found in the following tutorial:
 * - https://www.youtube.com/watch?v=ASAowY6yJII&ab_channel=javidx9
 *
 * @author @MasterLaplace
 * @version 0.0.0
 * @date 2025-04-03
 **************************************************************************/

#pragma once

#include "BoundaryBox.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <list>
#include <memory>
#include <random>
#include <vector>

template <typename OBJ_TYPE> struct OctreeItemLocation {
    typename std::list<std::pair<BoundaryBox, OBJ_TYPE>> *container;
    typename std::list<std::pair<BoundaryBox, OBJ_TYPE>>::iterator iterator;
};

constexpr uint8_t MAX_DEPTH = 5;
constexpr uint8_t MAX_CAPACITY = 4;

template <typename OBJ_TYPE> class DynamicOctree {
private:
    enum class INDEX : uint8_t {
        SWD, // South-West-Down (min corner)
        SED, // South-East-Down
        NWD, // North-West-Down
        NED, // North-East-Down
        SWU, // South-West-Up
        SEU, // South-East-Up
        NWU, // North-West-Up
        NEU  // North-East-Up (max corner)
    };

public:
    DynamicOctree(const BoundaryBox &boundary, const uint8_t capacity = MAX_CAPACITY,
                  const uint8_t depth = MAX_DEPTH) noexcept
        : _boundary(boundary), _DEPTH(depth), _CAPACITY(capacity)
    {
        resize(boundary);
    }
    ~DynamicOctree() = default;

    inline void resize(const BoundaryBox &rArea) noexcept
    {
        if (_boundary == rArea)
            return;

        clear();
        _boundary = rArea;

        sf::Vector3f size = _boundary.getSize() * 0.5f;
        sf::Vector3f pos = _boundary.getMin();

        _rNodes[static_cast<size_t>(INDEX::SWD)] = BoundaryBox(pos, size);
        _rNodes[static_cast<size_t>(INDEX::SED)] = BoundaryBox({pos.x + size.x, pos.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NWD)] = BoundaryBox({pos.x, pos.y + size.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NED)] = BoundaryBox({pos.x + size.x, pos.y + size.y, pos.z}, size);
        _rNodes[static_cast<size_t>(INDEX::SWU)] = BoundaryBox({pos.x, pos.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::SEU)] = BoundaryBox({pos.x + size.x, pos.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NWU)] = BoundaryBox({pos.x, pos.y + size.y, pos.z + size.z}, size);
        _rNodes[static_cast<size_t>(INDEX::NEU)] = BoundaryBox(pos + size, size);
    }

    inline void clear() noexcept
    {
        _pItems.clear();

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                _nodes[i]->clear();

            _nodes[i].reset();
        }

        _rNodes.fill(BoundaryBox());
        _boundary = BoundaryBox();
    }

    [[nodiscard, deprecated("Use DynamicOctreeContainer::size() instead.")]] inline size_t size() const noexcept
    {
        size_t size = _pItems.size();

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                size += _nodes[i]->size();
        }

        return size;
    }

    [[nodiscard]] inline OctreeItemLocation<OBJ_TYPE> insert(const OBJ_TYPE &item, const BoundaryBox &itemsize) noexcept
    {
        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_DEPTH == 0 || _pItems.size() < _CAPACITY)
                break;

            else if (!_rNodes[i].contains(itemsize))
                continue;

            if (!_nodes[i])
                _nodes[i] = std::make_unique<DynamicOctree<OBJ_TYPE>>(_rNodes[i], _CAPACITY, _DEPTH - 1);

            return _nodes[i]->insert(item, itemsize);
        }

        _pItems.emplace_back(itemsize, item);
        return {&_pItems, std::prev(_pItems.end())};
    }

    [[nodiscard]] inline std::list<OBJ_TYPE> search(const BoundaryBox &rArea) const noexcept
    {
        std::list<OBJ_TYPE> listItems;
        search(rArea, listItems);
        return listItems;
    }

    inline void search(const BoundaryBox &rArea, std::list<OBJ_TYPE> &listItems) const noexcept
    {
        for (const auto &[rItem, item] : _pItems)
        {
            if (rArea.overlaps(rItem))
                listItems.emplace_back(item);
        }

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (!_nodes[i])
                continue;

            if (rArea.contains(_rNodes[i]))
                _nodes[i]->items(listItems);
            else if (rArea.overlaps(_rNodes[i]))
                _nodes[i]->search(rArea, listItems);
        }
    }

    inline void items(std::list<OBJ_TYPE> &listItems) const noexcept
    {
        for (const auto &[rItem, item] : _pItems)
            listItems.emplace_back(item);

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                _nodes[i]->items(listItems);
        }
    }

    [[nodiscard]] inline const BoundaryBox &boundary() const noexcept { return _boundary; }

    [[nodiscard]] inline bool remove(OBJ_TYPE pItem) noexcept
    {
        auto it = std::find_if(_pItems.begin(), _pItems.end(),
                               [pItem](const std::pair<BoundaryBox, OBJ_TYPE> &p) { return p.second == pItem; });

        if (it != _pItems.end())
        {
            _pItems->erase(it);
            return true;
        }

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i] && _nodes[i]->remove(pItem))
                return true;
        }

        return false;
    }

#ifdef DEBUG
    inline void draw(sf::RenderWindow &window, const BoundaryBox &rArea) const noexcept
    {
        sf::RectangleShape rectangle;
        rectangle.setPosition({rArea.getMin().x, rArea.getMin().y});
        rectangle.setSize({rArea.getMax().x - rArea.getMin().x, rArea.getMax().y - rArea.getMin().y});
        rectangle.setFillColor(sf::Color::Transparent);
        rectangle.setOutlineColor(sf::Color::Green);
        rectangle.setOutlineThickness(1);
        window.draw(rectangle);

        for (uint8_t i = 0; i < 8u; ++i)
        {
            if (_nodes[i])
                _nodes[i]->draw(window, rArea);
        }
    }
#endif

protected:
    const uint8_t _DEPTH = 1;
    const uint8_t _CAPACITY = 4;

    BoundaryBox _boundary{};

    std::array<BoundaryBox, 8u> _rNodes{};

    std::array<std::unique_ptr<DynamicOctree<OBJ_TYPE>>, 8u> _nodes{};

    std::list<std::pair<BoundaryBox, OBJ_TYPE>> _pItems{};
};

template <typename OBJ_TYPE> struct OctreeItem {
    OBJ_TYPE item;
    OctreeItemLocation<typename std::list<OctreeItem<OBJ_TYPE>>::iterator> pItem;
};

template <typename OBJ_TYPE> class DynamicOctreeContainer {
public:
    using OctreeContainer = std::list<OctreeItem<OBJ_TYPE>>;

public:
    DynamicOctreeContainer(const BoundaryBox &size, const uint8_t capacity = MAX_CAPACITY,
                           const uint8_t depth = MAX_DEPTH) noexcept
        : _root(size, capacity, depth)
    {
    }
    ~DynamicOctreeContainer() = default;

    inline void resize(const BoundaryBox &rArea) noexcept { _root.resize(rArea); }

    [[nodiscard]] inline size_t size() const noexcept { return _allItems.size(); }

    [[nodiscard]] inline bool empty() const noexcept { return _allItems.empty(); }

    inline void clear() noexcept
    {
        _root.clear();
        _allItems.clear();
    }

    [[nodiscard]] inline const BoundaryBox &boundary() const noexcept { return _root.boundary(); }

    [[nodiscard]] inline typename OctreeContainer::iterator begin() noexcept { return _allItems.begin(); }

    [[nodiscard]] inline typename OctreeContainer::iterator end() noexcept { return _allItems.end(); }

    [[nodiscard]] inline typename OctreeContainer::const_iterator cbegin() const { return _allItems.cbegin(); }

    [[nodiscard]] inline typename OctreeContainer::const_iterator cend() const { return _allItems.cend(); }

    inline void insert(const OBJ_TYPE &item, const BoundaryBox &itemsize) noexcept
    {
        OctreeItem<OBJ_TYPE> newItem;
        newItem.item = item;
        _allItems.emplace_back(newItem);
        _allItems.back().pItem = _root.insert(std::prev(_allItems.end()), itemsize);
    }

    [[nodiscard]] inline std::list<typename OctreeContainer::iterator> search(const BoundaryBox &rArea) const noexcept
    {
        std::list<typename OctreeContainer::iterator> listItemsPointers;
        _root.search(rArea, listItemsPointers);
        return listItemsPointers;
    }

    inline void remove(typename OctreeContainer::iterator item) noexcept
    {
        item->pItem.container->erase(item->pItem.iterator);
        _allItems.erase(item);
    }

    inline void relocate(typename OctreeContainer::iterator &item, const BoundaryBox &itemsize) noexcept
    {
        item->pItem.container->erase(item->pItem.iterator);
        item->pItem = _root.insert(item, itemsize);
    }

#ifdef DEBUG
    inline void draw(sf::RenderWindow &window, const BoundaryBox &rArea) const noexcept { _root.draw(window, rArea); }
#endif

protected:
    OctreeContainer _allItems;
    DynamicOctree<typename OctreeContainer::iterator> _root;
};
