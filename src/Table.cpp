#include "Table.hpp"

Table::Table(float x, float y, float w, float h) {
    field_.setPosition(x, y);
    field_.setSize({w, h});
    field_.setFillColor(sf::Color(34, 139, 34)); // тёмно-зелёное сукно

    border_.setPosition(x - 10, y - 10);
    border_.setSize({w + 20, h + 20});
    border_.setFillColor(sf::Color(100, 65, 25)); // цвет борта
}

void Table::draw(sf::RenderTarget& target) const {
    target.draw(border_);
    target.draw(field_);
}

sf::FloatRect Table::getBounds() const {
    return field_.getGlobalBounds();
}
