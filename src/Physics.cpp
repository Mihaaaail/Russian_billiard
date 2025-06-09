#include "Physics.hpp"
#include "Pocket.hpp"
#include <cmath>
PhysicsEngine::PhysicsEngine(std::vector<Ball>& balls, const Table& table, const std::vector<Pocket>& pockets)
    : balls_(balls), table_(table), pockets_(pockets) {}

void PhysicsEngine::update(float dt) {
    for (auto& ball : balls_) {
        ball.update(dt);
        ball.reflectIfNeeded(table_.getBounds(), pockets_);
    }
    resolveCollisions();
}

void PhysicsEngine::resolveCollisions() {
    for (size_t i = 0; i < balls_.size(); ++i) {
        for (size_t j = i + 1; j < balls_.size(); ++j) {
            resolveBallBall(balls_[i], balls_[j]);
        }
    }
}

void PhysicsEngine::resolveBallBall(Ball& a, Ball& b) {
    sf::Vector2f posA = a.getPosition();
    sf::Vector2f posB = b.getPosition();
    sf::Vector2f delta = posB - posA;
    float dist = std::hypot(delta.x, delta.y);
    float r = a.getRadius() + b.getRadius();

    if (dist > 0.f && dist < r) {
        float overlap = r - dist + 0.1f;
        sf::Vector2f correction = delta / dist * (overlap / 2.f);
        a.move(-correction);
        b.move(correction);

        sf::Vector2f norm = delta / dist;
        sf::Vector2f velA = a.getVelocity();
        sf::Vector2f velB = b.getVelocity();

        float vA = velA.x * norm.x + velA.y * norm.y;
        float vB = velB.x * norm.x + velB.y * norm.y;

        if (vA - vB > 0) {
            float m1 = 1.f, m2 = 1.f;
            float p = (2.f * (vA - vB)) / (m1 + m2);
            float restitution = 0.95f;
            velA -= p * m2 * norm; velA *= restitution;
            velB += p * m1 * norm; velB *= restitution;

            a.setVelocity(velA);
            b.setVelocity(velB);
        }
    }
}

