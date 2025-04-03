// Tutorial: https://www.youtube.com/watch?v=ASAowY6yJII&ab_channel=javidx9

#include <SFML/Graphics.hpp>
#include <vector>
#include <random>
#include <chrono>
#include <iostream>
#include <array>
#include <memory>
#include <list>
#include <fstream>

struct rect : sf::RectangleShape
{
    rect() = default;

    rect(float x, float y, float w, float h)
    {
        setSize({w, h});
        setPosition(x, y);
        setFillColor(sf::Color::Transparent);
        setOutlineThickness(1);
        setOutlineColor(sf::Color::White);
    }

    rect(const sf::Vector2f &pos, const sf::Vector2f &size)
    {
        setSize(size);
        setPosition(pos);
        setFillColor(sf::Color::Transparent);
        setOutlineThickness(1);
        setOutlineColor(sf::Color::White);
    }

    bool contains(const sf::Vector2f &point) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();

        return !(point.x < pos.x || point.y < pos.y || point.x >= (pos.x + size.x) || point.y >= (pos.y + size.y));
    }

    bool contains(const rect &other) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();
        const auto &otherPos = other.getPosition();
        const auto &otherSize = other.getSize();

        return (otherPos.x >= pos.x) &&
               (otherPos.y >= pos.y) &&
               (otherPos.x + otherSize.x < pos.x + size.x) &&
               (otherPos.y + otherSize.y < pos.y + size.y);
    }

    bool overlaps(const rect &range) const
    {
        const auto &pos = getPosition();
        const auto &size = getSize();
        const auto &rangePos = range.getPosition();
        const auto &rangeSize = range.getSize();

        return (pos.x < rangePos.x + rangeSize.x) &&
               (pos.x + size.x >= rangePos.x) &&
               (pos.y < rangePos.y + rangeSize.y) &&
               (pos.y + size.y >= rangePos.y);
    }
};

struct SomeObjectWithArea
{
    sf::Vector2f vPos;
    sf::Vector2f vVel;
    sf::Vector2f vSize;
    sf::Color colour;
};

struct SearchCache
{
    sf::FloatRect previousView;
    std::list<SomeObjectWithArea> cachedResults;
};

constexpr size_t MAX_DEPTH = 5;

template <typename OBJ_TYPE>
class StaticQuadTree
{
public:
    StaticQuadTree(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
        : _rect(size), _depth(depth)
    {
        resize(size);
    }

    void resize(const rect &rArea)
    {
        clear();
        _rect = rArea;

        sf::Vector2f size = _rect.getSize() / 2.0f;
        sf::Vector2f pos = _rect.getPosition();

        _rChild[0] = rect(pos, size);
        _rChild[1] = rect({pos.x + size.x, pos.y}, size);
        _rChild[2] = rect({pos.x, pos.y + size.y}, size);
        _rChild[3] = rect(pos + size, size);
    }

    void clear()
    {
        _pItems.clear();

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                _pChild[i]->clear();

            _pChild[i].reset();
        }
    }

    size_t size() const
    {
        size_t nCount = _pItems.size();

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                nCount += _pChild[i]->size();
        }

        return nCount;
    }

    void insert(const OBJ_TYPE &item, const rect &itemsize)
    {
        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_depth + 1 < MAX_DEPTH && _rChild[i].contains(itemsize))
            {
                if (!_pChild[i])
                    _pChild[i] = std::make_shared<StaticQuadTree<OBJ_TYPE>>(_rChild[i], _depth + 1);

                _pChild[i]->insert(item, itemsize);
                return;
            }
        }

        _pItems.emplace_back(itemsize, item);
    }

    std::list<OBJ_TYPE> search(const rect &rArea) const
    {
        std::list<OBJ_TYPE> listItems;
        search(rArea, listItems);
        return listItems;
    }

    void search(const rect &rArea, std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
        {
            if (rArea.overlaps(rItem))
                listItems.emplace_back(item);
        }

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (!_pChild[i])
                continue;

            if (rArea.contains(_rChild[i]))
                _pChild[i]->items(listItems);
            else if (rArea.overlaps(_rChild[i]))
                _pChild[i]->search(rArea, listItems);
        }
    }

    void items(std::list<OBJ_TYPE> &listItems) const
    {
        for (const auto &[rItem, item] : _pItems)
            listItems.emplace_back(item);

        for (uint32_t i = 0; i < 4; ++i)
        {
            if (_pChild[i])
                _pChild[i]->items(listItems);
        }
    }

    const rect &area() const
    {
        return _rect;
    }

protected:
    size_t _depth = 0;

    rect _rect;

    std::array<rect, 4u> _rChild{};

    std::array<std::shared_ptr<StaticQuadTree<OBJ_TYPE>>, 4u> _pChild{};

    std::vector<std::pair<rect, OBJ_TYPE>> _pItems;
};

template <typename OBJ_TYPE>
class StaticQuadTreeContainer
{
public:
    using QuadTreeContainer = std::vector<OBJ_TYPE>;

    StaticQuadTreeContainer(const rect &size = {0, 0, 100, 100}, const size_t depth = 0)
        : _root(size) {}

    void resize(const rect &rArea)
    {
        _root.resize(rArea);
    }

    size_t size() const
    {
        return _allItems.size();
    }

    bool empty() const
    {
        return _allItems.empty();
    }

    void clear()
    {
        _root.clear();
        _allItems.clear();
    }

    typename QuadTreeContainer::iterator begin()
    {
        return _allItems.begin();
    }

    typename QuadTreeContainer::iterator end()
    {
        return _allItems.end();
    }

    typename QuadTreeContainer::const_iterator cbegin() const
    {
        return _allItems.cbegin();
    }

    typename QuadTreeContainer::const_iterator cend() const
    {
        return _allItems.cend();
    }

    void insert(const OBJ_TYPE &item, const rect &itemsize)
    {
        if (_allItems.size() == _allItems.capacity())
        {
            // Si une réallocation est nécessaire, effacer et réinitialiser le quadtree
            _root.clear();
            _allItems.reserve(_allItems.size() * 2); // Doubler la capacité pour éviter les réallocations fréquentes
            for (auto it = _allItems.begin(); it != _allItems.end(); ++it)
            {
                _root.insert(it, {it->vPos, it->vSize});
            }
        }

        _allItems.emplace_back(item);
        _root.insert(std::prev(_allItems.end()), itemsize);
    }

    std::list<typename QuadTreeContainer::iterator> search(const rect &rArea) const
    {
        std::list<typename QuadTreeContainer::iterator> listItemsPointers;
        _root.search(rArea, listItemsPointers);
        return listItemsPointers;
    }

protected:
    QuadTreeContainer _allItems;
    StaticQuadTree<typename QuadTreeContainer::iterator> _root;
};

std::vector<SomeObjectWithArea> objects;
StaticQuadTreeContainer<SomeObjectWithArea> quadtree;
float fArea = 100000.0f;
bool bUseQuadTree = true;
SearchCache searchCache;

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "QuadTree", sf::Style::Close);
    window.setFramerateLimit(60);

    sf::View viewRectangles = window.getDefaultView();
    sf::View viewUI = window.getDefaultView();
    const float moveSpeed = 500.0f; // Vitesse de déplacement en pixels par seconde

    auto rand_float = [](const float min, const float max) -> float
    {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(min, max);
        return dis(gen);
    };

    objects.reserve(1000000);
    quadtree.resize({0, 0, fArea, fArea});

    for (size_t i = 0; i < 1000000; ++i)
    {
        SomeObjectWithArea obj = {
            {rand_float(0, fArea), rand_float(0, fArea)},
            {0, 0},
            {rand_float(0.1f, 100.0f), rand_float(0.1f, 100.0f)},
            sf::Color(rand() % 255, rand() % 255, rand() % 255, 255)
        };

        objects.emplace_back(obj);
        quadtree.insert(obj, {obj.vPos, obj.vSize});
    }

    rect rScreen(0, 0, window.getSize().x, window.getSize().y);
    size_t nObjCount = 0;

    sf::Font MyFont;
    if (!MyFont.loadFromFile("/home/laplace/EIP/arial/ARIAL.TTF"))
        return 1;

    sf::Text text("", MyFont, 24);
    text.setFillColor(sf::Color::White);
    text.setPosition(10, 10);

    float fZoom = 1.0f;

    sf::Clock clock;
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();

            if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Tab)
                bUseQuadTree = !bUseQuadTree;

            if (event.type == sf::Event::MouseWheelScrolled)
            {
                if (event.mouseWheelScroll.delta > 0) {
                    fZoom *= 0.9f; // Zoom in
                    viewRectangles.zoom(0.9f);
                }
                else if (event.mouseWheelScroll.delta < 0) {
                    fZoom *= 1.1f; // Zoom out
                    viewRectangles.zoom(1.1f);
                }
            }
        }
        float deltaTime = clock.restart().asSeconds();
        // Déplacement de la vue avec les touches directionnelles
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
            viewRectangles.move(moveSpeed * fZoom * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
            viewRectangles.move(-moveSpeed * fZoom * deltaTime, 0);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
            viewRectangles.move(0, -moveSpeed * fZoom * deltaTime);
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
            viewRectangles.move(0, moveSpeed * fZoom * deltaTime);

        // Mettre à jour rScreen en fonction de la vue actuelle
        sf::Vector2f viewCenter = viewRectangles.getCenter();
        sf::Vector2f viewSize = viewRectangles.getSize();
        rScreen.setPosition(viewCenter - viewSize / 2.0f);
        rScreen.setSize(viewSize);

        window.clear();

        // Dessiner les rectangles avec la vue des rectangles
        window.setView(viewRectangles);

        if (bUseQuadTree)
        {
            auto tpStart = std::chrono::system_clock::now();

            // Vérifier si la vue a changé de manière significative
            sf::FloatRect currentView(viewCenter - viewSize / 2.0f, viewSize);
            if (std::abs(currentView.left - searchCache.previousView.left) > 1.0f ||
                std::abs(currentView.top - searchCache.previousView.top) > 1.0f ||
                std::abs(currentView.width - searchCache.previousView.width) > 1.0f ||
                std::abs(currentView.height - searchCache.previousView.height) > 1.0f)
            {
                // La vue a changé de manière significative, effectuer une nouvelle recherche
                searchCache.cachedResults.clear();
                for (auto it : quadtree.search(rScreen))
                {
                    searchCache.cachedResults.emplace_back(*it);
                }
                searchCache.previousView = currentView;
            }

            for (const auto &obj : searchCache.cachedResults)
            {
                sf::RectangleShape shape(obj.vSize);
                shape.setPosition(obj.vPos);
                shape.setFillColor(obj.colour);
                window.draw(shape);
                ++nObjCount;
            }

            std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
            text.setString("QuadTree: " + std::to_string(nObjCount) + "/" + std::to_string(objects.size()) + " in " + std::to_string(duration.count()) + "s");

            // Log to file if duration exceeds threshold
            if (duration.count() > 0.1f) // Example threshold of 0.1 seconds
            {
                std::ofstream logFile("/home/laplace/EIP/Optimizing/QuadTree.log", std::ios_base::app);
                logFile << "QuadTree: " << nObjCount << " objects displayed in " << duration.count() << " seconds\n";
            }
        }
        else
        {
            auto tpStart = std::chrono::system_clock::now();
            for (const auto &obj : objects)
            {
                if (rScreen.overlaps(rect(obj.vPos, obj.vSize)))
                {
                    sf::RectangleShape shape(obj.vSize);
                    shape.setPosition(obj.vPos);
                    shape.setFillColor(obj.colour);
                    window.draw(shape);
                    ++nObjCount;
                }
            }
            std::chrono::duration<float> duration = std::chrono::system_clock::now() - tpStart;
            text.setString("Linear: " + std::to_string(nObjCount) + "/" + std::to_string(objects.size()) + " in " + std::to_string(duration.count()) + "s");
        }

        // Dessiner l'interface utilisateur avec la vue de l'interface utilisateur
        window.setView(viewUI);
        window.draw(text);

        window.display();
        nObjCount = 0;
    }

    return 0;
}

// Build : g++ -std=c++20 QuadTree.cpp -lsfml-graphics -lsfml-window -lsfml-system -o QuadTree
