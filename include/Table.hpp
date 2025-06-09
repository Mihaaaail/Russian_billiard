#pragma once
#include <SFML/Graphics.hpp>

class Table {
public:
    Table(float x, float y, float w, float h);

    void draw(sf::RenderTarget& target) const;
    sf::FloatRect getBounds() const;

private:
    sf::RectangleShape field_;
    sf::RectangleShape border_;
    // Можно добавить вектор луз если надо
};
