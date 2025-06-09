#pragma once
#include <SFML/Graphics.hpp>
#include "Pocket.hpp"

class Ball {
public:
    Ball(float x, float y, float radius, sf::Color color,
         int number = 0, sf::Font* font = nullptr);

    void setVelocity(sf::Vector2f v);
    void move(sf::Vector2f delta);
    void update(float dt);
    void reflectIfNeeded(const sf::FloatRect& bounds, const std::vector<Pocket>& pockets);
    bool isNearPocket(const std::vector<Pocket>& pockets) const;

    sf::Vector2f getPosition() const;
    sf::Vector2f getVelocity() const;
    float getRadius() const;
    int getNumber() const;
    sf::Color getColor() const;

    void draw(sf::RenderTarget& target) const;

private:
    sf::CircleShape circle_;
    sf::Vector2f velocity_;
    float radius_;
    int number_;
    sf::Font* font_;
    sf::Color color_;
};
