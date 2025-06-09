#pragma once
#include <SFML/Graphics.hpp>

class Cue {
public:
    Cue();

    void update(sf::Vector2f start, sf::Vector2f end, float power, bool visible);
    void draw(sf::RenderWindow& window) const;

private:
    sf::RectangleShape shape_;
    float length_;
    float power_;
    bool visible_;
    sf::Vector2f direction_;
};
