#include <iostream>
#include <vector>
#include <array>
#include <list>
#include <cmath>
#include <memory>
#include <random>
#include <chrono>
#include <SFML/Graphics.hpp>

class Shape : public sf::Drawable
{
public:
    virtual ~Shape() = default;
    virtual void draw(sf::RenderTarget &target, sf::RenderStates states) const = 0;
};

static size_t current_id = 0;

class Point : public Shape
{
public:
    Point()
    {
        _circle.setRadius(2);
        _circle.setFillColor(sf::Color::Red);
        _circle.setPosition(0, 0);
        _id = ++current_id;
    }

    Point(const Point &point) = default;
    Point &operator=(const Point &point) = default;
    Point(Point &point) = default;
    Point &operator=(Point &point) = default;

    Point(float x, float y)
    {
        _circle.setRadius(2);
        _circle.setFillColor(sf::Color::Red);
        _circle.setPosition(x, y);
        _id = ++current_id;
    }

    Point(int min, int max)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);

        _circle.setRadius(2);
        _circle.setFillColor(sf::Color::Red);
        _circle.setPosition(dis(gen), dis(gen));
        _id = ++current_id;
    }

    // ~Point()
    // {
    //     std::cout << "Point with id: " << _id << " has been destroyed" << std::endl;
    // }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override
    {
        target.draw(_circle, states);
    }

    void calculatePosition(int min, int max)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(min, max);

        _newPosition = sf::Vector2f(dis(gen), dis(gen));
    }

    void applyNewPosition()
    {
        _circle.setPosition(_newPosition);
    }

public:
    sf::Vector2f getPosition() const
    {
        return _circle.getPosition();
    }

    sf::Vector2f getNewPosition() const
    {
        return _newPosition;
    }

    size_t getId() const
    {
        return _id;
    }

    void setColor(sf::Color color)
    {
        _circle.setFillColor(color);
    }

private:
    sf::CircleShape _circle;
    sf::Vector2f _newPosition;
    size_t _id;
};

class Rectangle : public Shape
{
public:
    Rectangle(float x, float y, float width, float height)
    {
        _rectangle.setSize(sf::Vector2f(width, height));
        _rectangle.setFillColor(sf::Color::Transparent);
        _rectangle.setOutlineColor(sf::Color::White);
        _rectangle.setOutlineThickness(1);
        _rectangle.setPosition(x, y);
    }

    void setColor(sf::Color color)
    {
        _rectangle.setOutlineColor(color);
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override
    {
        target.draw(_rectangle, states);
    }

    bool contains(Point point) const
    {
        return _rectangle.getGlobalBounds().contains(point.getPosition());
    }

    bool contains(sf::Vector2f point) const
    {
        return _rectangle.getGlobalBounds().contains(point);
    }

    bool contains(Rectangle range) const
    {
        sf::FloatRect bounds = _rectangle.getGlobalBounds();
        sf::FloatRect rangeBounds = range.getGlobalBounds();
        return bounds.left <= rangeBounds.left && bounds.top <= rangeBounds.top &&
               bounds.left + bounds.width >= rangeBounds.left + rangeBounds.width &&
               bounds.top + bounds.height >= rangeBounds.top + rangeBounds.height;
    }

    bool overlaps(Rectangle range) const
    {
        return _rectangle.getGlobalBounds().intersects(range.getGlobalBounds());
    }

    bool overlaps(Point point) const
    {
        return _rectangle.getGlobalBounds().contains(point.getPosition());
    }

    sf::FloatRect getGlobalBounds() const
    {
        return _rectangle.getGlobalBounds();
    }

    float x() const { return _rectangle.getPosition().x; }
    float y() const { return _rectangle.getPosition().y; }
    float width() const { return _rectangle.getSize().x; }
    float height() const { return _rectangle.getSize().y; }

private:
    sf::RectangleShape _rectangle;
};

class SomeShape : public Shape
{
public:
    SomeShape(float x, float y, float radius, sf::Color color)
    {
        vPos = sf::Vector2f(x, y);
        vVel = sf::Vector2f(0, 0);
        vSize = sf::Vector2f(radius, radius);
        colour = color;
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const override
    {
        sf::RectangleShape shape(vSize);
        shape.setPosition(vPos);
        shape.setFillColor(colour);
        target.draw(shape, states);
    }

private:
    sf::Vector2f vPos;
    sf::Vector2f vVel;
    sf::Vector2f vSize;
    sf::Color colour;
};

template <size_t T>
class QuadTree
{
    public:
        enum class INDEX : int
        {
            NE,
            NW,
            SE,
            SW
        };

public:
    QuadTree(Rectangle &boundary, int capacity, int depth) //* COMPLETED
        : _boundary(boundary), _capacity(capacity), _depth(depth)
        {
            _nodes.fill(nullptr);
        }

    void subdivide() //* COMPLETED
    {
        float x = _boundary.x();
        float y = _boundary.y();
        float width = _boundary.width() / 2;
        float height = _boundary.height() / 2;

        Rectangle nw = Rectangle(x, y, width, height);
        _nodes[static_cast<int>(INDEX::NW)] = new QuadTree(nw, _capacity, _depth - 1);

        Rectangle ne = Rectangle(x + width, y, width, height);
        _nodes[static_cast<int>(INDEX::NE)] = new QuadTree(ne, _capacity, _depth - 1);

        Rectangle sw = Rectangle(x, y + height, width, height);
        _nodes[static_cast<int>(INDEX::SW)] = new QuadTree(sw, _capacity, _depth - 1);

        Rectangle se = Rectangle(x + width, y + height, width, height);
        _nodes[static_cast<int>(INDEX::SE)] = new QuadTree(se, _capacity, _depth - 1);
    }

    void insert(std::shared_ptr<Point> point) //* COMPLETED
    {
        if (!_boundary.overlaps(*point))
            return;

        if (_points.size() < _capacity || _depth == 0)
            return _points.emplace_back(point), void();

        else if (_nodes[0] == nullptr)
            subdivide();

        for (auto &node : _nodes)
        {
            node->insert(point);
            continue;
        }
    }

    void query(Rectangle &range, std::vector<std::shared_ptr<Point>> &found) //* COMPLETED
    {
        if (!_boundary.getGlobalBounds().intersects(range.getGlobalBounds()))
            return;

        for (auto &point : _points)
        {
            if (range.contains(*point))
            {
                point->setColor(sf::Color::Green);
                found.emplace_back(point);
            } else {
                point->setColor(sf::Color::Red);
            }
        }

        if (_nodes[0] == nullptr)
            return;

        for (auto &node : _nodes)
        {
            node->query(range, found);
        }
    }

    bool erase(std::shared_ptr<Point> oldPoint)
    {
        if (!_boundary.contains(*oldPoint))
            return false;

        for (auto &point : _points)
        {
            if (oldPoint != point)
                continue;

            if (!_boundary.contains(oldPoint->getNewPosition()))
                return false;

            _points.erase(std::remove(_points.begin(), _points.end(), point), _points.end());

            if (empty())
                clear();
            return true;
        }

        for (auto &node : _nodes)
        {
            if (!node)
                continue;

            if (node->erase(oldPoint))
                return true;
        }
        return false;
    }

    void update(std::shared_ptr<Point> point)
    {
        if (erase(point))
            insert(point);
    }

    void draw(sf::RenderTarget &target, sf::RenderStates states) const //* COMPLETED
    {
        target.draw(_boundary);

        for (auto &point : _points)
        {
            target.draw(*point);
        }

        for (auto &node : _nodes)
        {
            if (!node)
                continue;

            node->draw(target, states);
        }
    }

    void resize(Rectangle &newBoundary) //* COMPLETED
    {
        clear();
        _boundary = newBoundary;
    }

    void clear() //* COMPLETED
    {
        _points.clear();

        for (auto &node : _nodes)
        {
            if (!node)
                continue;

            node->clear();
            delete node;
            node = nullptr;
        }
    }

    size_t size() const //* COMPLETED
    {
        size_t size = _points.size();

        for (auto &node : _nodes)
        {
            if (!node)
                continue;

            size += node->size();
        }
        return size;
    }

    bool empty() const //* COMPLETED
    {
        if (!_points.empty())
            return false;

        for (auto &node : _nodes)
        {
            if (!node)
                continue;

            if (!node->empty())
                return false;
        }
        return true;
    }

private:
    Rectangle _boundary;
    std::list<std::shared_ptr<Point>> _points;
    std::array<QuadTree*, T> _nodes;
    int _capacity;
    int _depth;
};

int main()
{
    sf::RenderWindow window(sf::VideoMode(800, 600), "QuadTree");

    Rectangle rectangle(0, 0, 600, 600);
    QuadTree<4u> quadtree(rectangle, 4, 8);

    Rectangle range(200, 200, 100, 100);
    range.setColor(sf::Color::Green);

    std::vector<std::shared_ptr<Point>> found;

    found.reserve(10);

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed || sf::Keyboard::isKeyPressed(sf::Keyboard::Escape))
                window.close();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::C))
                quadtree.clear();

            if (sf::Keyboard::isKeyPressed(sf::Keyboard::M))
            {
                quadtree.query(range, found);
                for (auto &point : found)
                {
                    point->calculatePosition(0, 600);
                    quadtree.update(point);
                    point->applyNewPosition();
                }
                found.clear();
            }

            if (sf::Mouse::isButtonPressed(sf::Mouse::Left))
            {
                sf::Vector2i mousePos = sf::Mouse::getPosition(window);
                quadtree.insert(std::make_shared<Point>(float(mousePos.x), float(mousePos.y)));
            }
        }

        // Move the range rectangle following the mouse
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        range = Rectangle(float(mousePos.x) - range.width() / 2, float(mousePos.y) - range.height() / 2, range.width(), range.height());

        window.clear();

        quadtree.draw(window, sf::RenderStates::Default);

        quadtree.query(range, found);
        window.draw(range);

        found.clear();

        window.display();
    }
    return 0;
}

// build command: g++ main.cpp -o quadtree -lsfml-graphics -lsfml-window -lsfml-system
