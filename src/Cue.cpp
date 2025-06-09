#include "Cue.hpp"
#include <cmath>

Cue::Cue()
    : length_(150.f), power_(0.f), visible_(true)
{
    shape_.setSize(sf::Vector2f(length_, 6.f));
    shape_.setOrigin(0.f, 3.f); // Левый край — начало кия
    shape_.setFillColor(sf::Color(160, 110, 60));
    shape_.setOutlineColor(sf::Color(80, 60, 30));
    shape_.setOutlineThickness(1.f);
    direction_ = {1.f, 0.f};
}

void Cue::update(sf::Vector2f cueStart, sf::Vector2f cueEnd, float power, bool visible)
{
    sf::Vector2f dir = cueEnd - cueStart;
    float len = std::hypot(dir.x, dir.y);
    direction_ = (len > 1e-2f) ? dir / len : sf::Vector2f{1, 0};
    power_ = power;
    visible_ = visible;

    shape_.setPosition(cueStart);
    float angle = std::atan2(direction_.y, direction_.x) * 180 / 3.1415926f;
    shape_.setRotation(angle);
    float cueLen = std::clamp(power_, 10.f, 200.f);
    shape_.setSize(sf::Vector2f(cueLen, 5.f));
}

void Cue::draw(sf::RenderWindow& window) const
{
    if (visible_)
        window.draw(shape_);
}
