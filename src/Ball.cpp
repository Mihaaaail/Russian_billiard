#include "Ball.hpp"
#include <cmath>

Ball::Ball(float x, float y, float radius, sf::Color color, int number, sf::Font* font)
    : velocity_(0, 0), radius_(radius), number_(number), font_(font), color_(color)
{
    circle_.setRadius(radius);
    circle_.setFillColor(color);
    circle_.setOrigin(radius, radius);
    circle_.setPosition(x, y);
    circle_.setOutlineColor(sf::Color::Black);
    circle_.setOutlineThickness(2.f);
}

void Ball::setVelocity(sf::Vector2f v) { velocity_ = v; }
void Ball::move(sf::Vector2f delta)    { circle_.move(delta); }
float Ball::getRadius() const          { return radius_; }
sf::Vector2f Ball::getPosition() const { return circle_.getPosition(); }
sf::Vector2f Ball::getVelocity() const { return velocity_; }
int Ball::getNumber() const            { return number_; }
sf::Color Ball::getColor() const       { return color_; }

void Ball::update(float dt) {
    circle_.move(velocity_ * dt);

    float mu = 1.3f; 
    float g = 9.81f;
    float speed = std::hypot(velocity_.x, velocity_.y);
    if (speed > 0.f) {
        float frictionForce = mu * g;
        float decel = frictionForce * dt;
        if (speed <= decel)
            velocity_ = {0.f, 0.f};
        else
            velocity_ -= (velocity_ / speed) * decel;
    }
}

bool Ball::isNearPocket(const std::vector<Pocket>& pockets) const {
    sf::Vector2f pos = getPosition();
    for (const auto& p : pockets) {
        float d = std::hypot(pos.x - p.pos.x, pos.y - p.pos.y);
        if (d < p.radius * 1.15f) // 1.15 — небольшой запас
            return true;
    }
    return false;
}

void Ball::reflectIfNeeded(const sf::FloatRect& bounds, const std::vector<Pocket>& pockets) {
    if (isNearPocket(pockets))
        return; // В области лузы не отражаем!
    sf::Vector2f pos = getPosition();

    if ((pos.x - radius_ < bounds.left && velocity_.x < 0) ||
        (pos.x + radius_ > bounds.left + bounds.width && velocity_.x > 0))
        velocity_.x = -velocity_.x * 0.85f;
    if ((pos.y - radius_ < bounds.top && velocity_.y < 0) ||
        (pos.y + radius_ > bounds.top + bounds.height && velocity_.y > 0))
        velocity_.y = -velocity_.y * 0.85f;
}

void Ball::draw(sf::RenderTarget& target) const {
    target.draw(circle_);

    if (font_ && number_ > 0) {
        sf::Text label;
        label.setFont(*font_);
        label.setString(std::to_string(number_));
        label.setCharacterSize(static_cast<unsigned>(radius_ * 1.1f));
        label.setFillColor(sf::Color::Black);
        label.setStyle(sf::Text::Bold);
        auto bounds = label.getLocalBounds();
        label.setOrigin(bounds.width / 2.f, bounds.height / 1.3f);
        label.setPosition(circle_.getPosition());
        target.draw(label);
    }
}
