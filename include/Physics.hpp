#pragma once
#include <vector>
#include "Ball.hpp"
#include "Table.hpp"
#include "Pocket.hpp"

class PhysicsEngine {
public:
    PhysicsEngine(std::vector<Ball>& balls, const Table& table, const std::vector<Pocket>& pockets);

    void update(float dt);

private:
    void resolveCollisions();
    void resolveBallBall(Ball& a, Ball& b);

    std::vector<Ball>& balls_;
    const Table& table_;
    const std::vector<Pocket>& pockets_;
};
