#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <array>
#include <algorithm>
#include <string>
#include <memory>
#include "Table.hpp"
#include "Ball.hpp"
#include "Physics.hpp"
#include "Cue.hpp"
#include "Pocket.hpp"

struct FallingBall {
    sf::Vector2f pos;
    float radius;
    sf::Color color;
    int number;
    float t;
    sf::Font* font;
};

const sf::Color ivory(232, 229, 203);
const sf::Color cueBallColor = sf::Color(255, 255, 255);

sf::Color gradientColor(sf::Color a, sf::Color b, float t) {
    return sf::Color(
        static_cast<sf::Uint8>(a.r + (b.r - a.r) * t),
        static_cast<sf::Uint8>(a.g + (b.g - a.g) * t),
        static_cast<sf::Uint8>(a.b + (b.b - a.b) * t),
        static_cast<sf::Uint8>(a.a + (b.a - a.a) * t)
    );
}

int main() {
    constexpr int windowWidth = 1024;
    constexpr int windowHeight = 600;

    float tableX = 50, tableY = 50, tableW = 924, tableH = 500;
    float ballRadius = 15.f;
    float pocketRadius = 17.f;
    float borderThickness = 24.f;
    float borderShadow = 8.f;

    sf::RenderWindow window(sf::VideoMode(windowWidth, windowHeight), "Russian Billiard");
    window.setFramerateLimit(60);

    sf::Font font;
    if (!font.loadFromFile("../assets/SEGUISB.TTF")) {
        return 1;
    }

    // Game state
    int player = 1;
    int score1 = 0, score2 = 0;
    int ballsToWin = 8;
    int ballsCount = 15;

    // Ball reset function
    auto reset_balls = [&](std::vector<Ball>& balls) {
        balls.clear();
        // Cue ball (white, number 0)
        balls.emplace_back(tableX + tableW*0.25f, tableY + tableH/2, ballRadius, cueBallColor, 0, &font);
        // Classic 15-ball pyramid (rows tight)
        float x0 = tableX + tableW*0.75f;
        float y0 = tableY + tableH/2;
        int k = 1;
        float dy = ballRadius * 2 * std::sin(3.1415926 / 3.0);
        for (int row = 0; row < 5; ++row) {
            float yStart = y0 - dy * (row / 2.0f);
            for (int col = 0; col <= row; ++col) {
                float x = x0 + row * ballRadius * 2 * std::cos(3.1415926 / 6);
                float y = yStart + col * dy;
                balls.emplace_back(x, y, ballRadius, ivory, k, &font);
                ++k;
                if (k > ballsCount) break;
            }
            if (k > ballsCount) break;
        }
    };

    std::vector<Ball> balls;
    reset_balls(balls);

    std::vector<Pocket> pockets = {
        { {tableX, tableY}, pocketRadius },
        { {tableX + tableW / 2, tableY}, pocketRadius },
        { {tableX + tableW, tableY}, pocketRadius },
        { {tableX, tableY + tableH}, pocketRadius },
        { {tableX + tableW / 2, tableY + tableH}, pocketRadius },
        { {tableX + tableW, tableY + tableH}, pocketRadius }
    };

    Table table(tableX, tableY, tableW, tableH);
    std::unique_ptr<PhysicsEngine> physics = std::make_unique<PhysicsEngine>(balls, table, pockets);
    Cue cue;

    bool dragging = false;
    sf::Vector2f dragStart;
    sf::Vector2f dragEnd;
    int selectedBall = -1;
    bool showCueAnim = false;
    float cueAnimTime = 0.f;
    const float cueAnimDuration = 0.12f;
    float maxPower = 1600.f;
    float minCueLen = 10.f;
    float maxCueLen = 220.f;
    float hitTolerance = 4.f;
    bool ballsMoving = false;
    bool anyScored = false;
    bool cuePocketed = false;
    sf::Vector2f cueBallStartPos = balls[0].getPosition();
    sf::Clock winClock;
    bool gameJustWon = false;
    int winnerPlayer = 0;

    struct AnimatedFalling {
        FallingBall ball;
        float elapsed = 0.f;
        float duration = 0.6f;
    };
    std::vector<AnimatedFalling> fallingBalls;

    sf::Clock clock;

    sf::Texture clothOverlay;
    clothOverlay.create(static_cast<unsigned int>(tableW), static_cast<unsigned int>(tableH));
    sf::Image overlayImg;
    overlayImg.create(static_cast<unsigned int>(tableW), static_cast<unsigned int>(tableH));
    for (unsigned int y = 0; y < static_cast<unsigned int>(tableH); ++y)
        for (unsigned int x = 0; x < static_cast<unsigned int>(tableW); ++x) {
            float distToCenter = std::abs((int)tableW/2 - (int)x) / (tableW/2);
            float edgeShadow = std::min({ y / tableH, (tableH-1-y) / tableH, x / tableW, (tableW-1-x) / tableW });
            float t = 0.25f*distToCenter + 0.6f*edgeShadow;
            overlayImg.setPixel(x, y, gradientColor(sf::Color(5,95,45), sf::Color(25, 65, 30), t));
        }
    clothOverlay.update(overlayImg);

    sf::Text hud;
    hud.setFont(font);
    hud.setCharacterSize(26);
    hud.setFillColor(sf::Color(250, 250, 250));
    hud.setOutlineColor(sf::Color::Black);
    hud.setOutlineThickness(2.f);

    while (window.isOpen()) {
        sf::Event event;
        sf::Vector2i mousePix = sf::Mouse::getPosition(window);
        sf::Vector2f mouse = window.mapPixelToCoords(mousePix);

        int potentialBall = -1;
        if (dragging) {
            sf::Vector2f a = dragStart;
            sf::Vector2f b = mouse;
            sf::Vector2f ab = b - a;
            float abLen2 = ab.x * ab.x + ab.y * ab.y;
            float closestT = 1e9f;
            for (int i = 0; i < balls.size(); ++i) {
                sf::Vector2f c = balls[i].getPosition();
                float r = balls[i].getRadius() + hitTolerance;
                sf::Vector2f ac = c - a;
                float t = (abLen2 > 1e-4f) ? ((ac.x * ab.x + ac.y * ab.y) / abLen2) : 0.f;
                if (t < 0.f || t > 1.f) continue;
                sf::Vector2f proj = a + ab * t;
                float distToLine = std::hypot(proj.x - c.x, proj.y - c.y);
                if (distToLine <= r && t < closestT) {
                    closestT = t;
                    potentialBall = i;
                }
            }
        }

        // Input
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::R) {
                score1 = 0; score2 = 0; player = 1;
                reset_balls(balls);
                fallingBalls.clear();
                physics = std::make_unique<PhysicsEngine>(balls, table, pockets);
                cueBallStartPos = balls[0].getPosition();
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                dragging = true;
                dragStart = mouse;
            }

            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left && dragging) {
                dragging = false;
                dragEnd = mouse;
                selectedBall = -1;

                sf::Vector2f a = dragStart;
                sf::Vector2f b = dragEnd;
                sf::Vector2f ab = b - a;
                float abLen2 = ab.x * ab.x + ab.y * ab.y;
                float closestT = 1e9f;
                for (int i = 0; i < balls.size(); ++i) {
                    sf::Vector2f c = balls[i].getPosition();
                    float r = balls[i].getRadius() + hitTolerance;
                    sf::Vector2f ac = c - a;
                    float t = (abLen2 > 1e-4f) ? ((ac.x * ab.x + ac.y * ab.y) / abLen2) : 0.f;
                    if (t < 0.f || t > 1.f) continue;
                    sf::Vector2f proj = a + ab * t;
                    float distToLine = std::hypot(proj.x - c.x, proj.y - c.y);
                    if (distToLine <= r && t < closestT) {
                        closestT = t;
                        selectedBall = i;
                    }
                }

                if (selectedBall != -1) {
                    sf::Vector2f shotVec = dragEnd - dragStart;
                    float dragLen = std::hypot(shotVec.x, shotVec.y);
                    float power = std::clamp(dragLen * 2.f, 0.f, maxPower);
                    if (power > 20.f) {
                        float len = std::hypot(shotVec.x, shotVec.y);
                        sf::Vector2f dir = (len > 1e-2f) ? shotVec / len : sf::Vector2f{1, 0};
                        balls[selectedBall].setVelocity(dir * power);

                        showCueAnim = true;
                        cueAnimTime = 0.f;
                        ballsMoving = true;
                        anyScored = false;
                        cuePocketed = false;
                    }
                }
            }
        }

        float dt = clock.restart().asSeconds();
        physics->update(dt);

        // Check pocketed balls
        std::vector<int> toErase;
        for (int i = 0; i < static_cast<int>(balls.size()); ++i) {
            for (const auto& pocket : pockets) {
                float dist = std::hypot(balls[i].getPosition().x - pocket.pos.x, balls[i].getPosition().y - pocket.pos.y);
                if (dist < pocket.radius - balls[i].getRadius() * 0.2f) {
                    if (balls[i].getNumber() == 0) { // Cue ball in pocket!
                        cuePocketed = true;
                        balls[i].setVelocity({0, 0});
                        break;
                    } else {
                        anyScored = true;
                        if (player == 1) score1++; else score2++;
                        FallingBall fb{
                            balls[i].getPosition(),
                            balls[i].getRadius(),
                            balls[i].getColor(),
                            balls[i].getNumber(),
                            0.f,
                            &font
                        };
                        fallingBalls.push_back({fb, 0.f, 0.6f});
                        toErase.push_back(i);
                        break;
                    }
                }
            }
        }
        if (!toErase.empty()) {
            std::sort(toErase.rbegin(), toErase.rend());
            for (int idx : toErase)
                balls.erase(balls.begin() + idx);
        }

        for (auto& falling : fallingBalls) {
            falling.elapsed += dt;
            falling.ball.t = std::min(1.f, falling.elapsed / falling.duration);
        }
        fallingBalls.erase(
            std::remove_if(fallingBalls.begin(), fallingBalls.end(),
                [](const AnimatedFalling& f) { return f.ball.t >= 1.f; }),
            fallingBalls.end()
        );

        // Detect all balls stopped
        if (ballsMoving) {
            bool allStopped = true;
            for (const auto& ball : balls)
                if (std::hypot(ball.getVelocity().x, ball.getVelocity().y) > 1.0f) allStopped = false;
            if (allStopped) {
                ballsMoving = false;
                // Special: Cue ball in pocket?
                if (cuePocketed) {
                    // Move cue ball to start
                    for (auto& ball : balls) {
                        if (ball.getNumber() == 0) {
                            ball.setVelocity({0, 0});
                            ball.move(cueBallStartPos - ball.getPosition());
                            break;
                        }
                    }
                    player = (player == 1 ? 2 : 1); // Switch turn
                }
                // If no ball was pocketed, switch turn
                else if (!anyScored) {
                    player = (player == 1 ? 2 : 1);
                }
                // else: if pocketed, player keeps the turn (as in Russian billiards)
            }
        }

        if (showCueAnim) {
            cueAnimTime += dt;
            if (cueAnimTime > cueAnimDuration) {
                showCueAnim = false;
                cueAnimTime = 0.f;
            }
        }

        // ==== DRAW ====
        window.clear(sf::Color(24, 40, 26));

        sf::Sprite cloth(clothOverlay);
        cloth.setPosition(tableX, tableY);
        window.draw(cloth);

        sf::Color borderColor(90, 45, 20);
        sf::Color shadowColor(55, 27, 10, 70);
        std::array<sf::FloatRect, 4> borders = {
            sf::FloatRect(tableX - borderThickness, tableY - borderThickness, tableW + 2*borderThickness, borderThickness),
            sf::FloatRect(tableX - borderThickness, tableY + tableH, tableW + 2*borderThickness, borderThickness),
            sf::FloatRect(tableX - borderThickness, tableY, borderThickness, tableH),
            sf::FloatRect(tableX + tableW, tableY, borderThickness, tableH)
        };
        for (const auto& border : borders) {
            sf::RectangleShape shadow(sf::Vector2f(border.width, border.height));
            shadow.setPosition(border.left, border.top + border.height - borderShadow);
            shadow.setFillColor(shadowColor);
            window.draw(shadow);
        }
        for (const auto& border : borders) {
            sf::RectangleShape side(sf::Vector2f(border.width, border.height));
            side.setPosition(border.left, border.top);
            side.setFillColor(borderColor);
            side.setOutlineColor(sf::Color(110, 65, 25));
            side.setOutlineThickness(2.f);
            window.draw(side);
        }

        for (const auto& pocket : pockets) {
            sf::CircleShape glow(pocket.radius + 5.f);
            glow.setOrigin(pocket.radius + 5.f, pocket.radius + 5.f);
            glow.setPosition(pocket.pos);
            glow.setFillColor(sf::Color(0, 0, 0, 60));
            window.draw(glow);

            sf::CircleShape ring(pocket.radius + 2.f);
            ring.setOrigin(pocket.radius + 2.f, pocket.radius + 2.f);
            ring.setPosition(pocket.pos);
            ring.setFillColor(sf::Color(38, 20, 10, 200));
            window.draw(ring);

            sf::CircleShape dark(pocket.radius);
            dark.setOrigin(pocket.radius, pocket.radius);
            dark.setPosition(pocket.pos);
            dark.setFillColor(sf::Color(10, 10, 10, 255));
            window.draw(dark);
        }

        for (int i = 0; i < balls.size(); ++i) {
            if (i == potentialBall && dragging) {
                sf::CircleShape glow(balls[i].getRadius() + 7.f);
                glow.setOrigin(balls[i].getRadius() + 7.f, balls[i].getRadius() + 7.f);
                glow.setPosition(balls[i].getPosition());
                glow.setFillColor(sf::Color(255, 255, 0, 80));
                window.draw(glow);
            }
        }

        for (int i = 0; i < balls.size(); ++i) {
            sf::CircleShape shadow(balls[i].getRadius());
            shadow.setOrigin(balls[i].getRadius(), balls[i].getRadius());
            shadow.setPosition(balls[i].getPosition().x + 3.f, balls[i].getPosition().y + 5.f);
            shadow.setFillColor(sf::Color(25, 30, 20, 80));
            window.draw(shadow);

            balls[i].draw(window);
        }

        for (const auto& falling : fallingBalls) {
            float tt = falling.ball.t;
            float scale = 1.f - tt;
            float alpha = 255 * (1.f - tt);

            sf::CircleShape circ(falling.ball.radius * scale);
            circ.setOrigin(falling.ball.radius * scale, falling.ball.radius * scale);
            circ.setPosition(falling.ball.pos);
            circ.setFillColor(sf::Color(
                falling.ball.color.r, falling.ball.color.g, falling.ball.color.b, (sf::Uint8)alpha));
            circ.setOutlineColor(sf::Color(0,0,0, (sf::Uint8)alpha));
            circ.setOutlineThickness(2.f * scale);
            window.draw(circ);

            if (falling.ball.number > 0) {
                sf::Text numTxt;
                numTxt.setFont(*falling.ball.font);
                numTxt.setString(std::to_string(falling.ball.number));
                numTxt.setCharacterSize(static_cast<unsigned int>(falling.ball.radius * 1.2f * scale));
                numTxt.setFillColor(sf::Color(0,0,0, (sf::Uint8)alpha));
                sf::FloatRect bounds = numTxt.getLocalBounds();
                numTxt.setOrigin(bounds.width/2.f, bounds.height/2.f);
                numTxt.setPosition(falling.ball.pos);
                window.draw(numTxt);
            }
        }

        if ((dragging || (showCueAnim && selectedBall != -1)) && (dragging || cueAnimTime < cueAnimDuration)) {
            sf::Vector2f start, end;
            float cueLen = 120.f;
            if (dragging) {
                sf::Vector2f cueVec = mouse - dragStart;
                float dragLen = std::hypot(cueVec.x, cueVec.y);
                cueLen = std::clamp(dragLen, minCueLen, maxCueLen);
                float len = std::hypot(cueVec.x, cueVec.y);
                sf::Vector2f dir = (len > 1e-2f) ? cueVec / len : sf::Vector2f{1, 0};
                start = dragStart;
                end = dragStart + dir * cueLen;
            } else {
                float t = cueAnimTime / cueAnimDuration;
                sf::Vector2f cueVec = dragEnd - dragStart;
                float dragLen = std::hypot(cueVec.x, cueVec.y);
                cueLen = std::clamp(dragLen, minCueLen, maxCueLen);
                sf::Vector2f dir = (dragLen > 1e-2f) ? cueVec / dragLen : sf::Vector2f{1, 0};
                start = dragStart;
                end = dragStart + dir * (1.0f - t) * cueLen;
            }
            sf::RectangleShape cueShadow(sf::Vector2f(cueLen, 6.f));
            cueShadow.setOrigin(0.f, 3.f);
            cueShadow.setPosition(start.x + 3.f, start.y + 5.f);
            cueShadow.setRotation(atan2(end.y - start.y, end.x - start.x) * 180.f / 3.1415926f);
            cueShadow.setFillColor(sf::Color(25, 30, 20, 60));
            window.draw(cueShadow);
            sf::RectangleShape cueStick(sf::Vector2f(cueLen, 6.f));
            cueStick.setOrigin(0.f, 3.f);
            cueStick.setPosition(start);
            cueStick.setRotation(atan2(end.y - start.y, end.x - start.x) * 180.f / 3.1415926f);
            sf::Color cueColorStart(210, 170, 80);
            sf::Color cueColorEnd(110, 65, 20);
            sf::VertexArray gradient(sf::TrianglesStrip, 4);
            gradient[0].position = sf::Vector2f(0.f, 0.f);
            gradient[1].position = sf::Vector2f(0.f, 6.f);
            gradient[2].position = sf::Vector2f(cueLen, 0.f);
            gradient[3].position = sf::Vector2f(cueLen, 6.f);
            gradient[0].color = gradient[1].color = cueColorStart;
            gradient[2].color = gradient[3].color = cueColorEnd;
            sf::RenderTexture cueTex;
            cueTex.create(static_cast<unsigned int>(cueLen), 6);
            cueTex.clear(sf::Color::Transparent);
            cueTex.draw(gradient);
            cueTex.display();
            cueStick.setTexture(&cueTex.getTexture());
            window.draw(cueStick);
            sf::RectangleShape tip(sf::Vector2f(11.f, 6.f));
            tip.setOrigin(11.f, 3.f);
            tip.setPosition(end);
            tip.setRotation(atan2(end.y - start.y, end.x - start.x) * 180.f / 3.1415926f);
            tip.setFillColor(sf::Color(62, 38, 20));
            window.draw(tip);
        }

        // ==== HUD ====
        hud.setString(
            "P1: " + std::to_string(score1) + " / " + std::to_string(ballsToWin) +
            "    P2: " + std::to_string(score2) + " / " + std::to_string(ballsToWin) +
            "    Turn: P" + std::to_string(player) +
            "    [R] Restart"
        );
        hud.setPosition(windowWidth/2.f - hud.getLocalBounds().width/2.f, 6);
        window.draw(hud);

        if (score1 >= ballsToWin || score2 >= ballsToWin) {
            if (!gameJustWon) {
                winnerPlayer = (score1 >= ballsToWin) ? 1 : 2;
                winClock.restart();
                gameJustWon = true;
            }
            // Рисуем надпись победителя
            sf::Text winText("WINNER: PLAYER " + std::to_string(winnerPlayer),
                            font, 42);
            winText.setFillColor(sf::Color::Yellow);
            winText.setOutlineColor(sf::Color::Black);
            winText.setOutlineThickness(3.f);
            winText.setPosition(windowWidth/2.f - winText.getLocalBounds().width/2.f, windowHeight/2.f - 32);
            window.draw(winText);

            // Если прошло 1.5 сек — сбрасываем игру
            if (winClock.getElapsedTime().asSeconds() > 1.5f) {
                score1 = 0; score2 = 0; player = 1;
                reset_balls(balls);
                fallingBalls.clear();
                physics = std::make_unique<PhysicsEngine>(balls, table, pockets);
                cueBallStartPos = balls[0].getPosition();
                gameJustWon = false;
                winnerPlayer = 0;
                continue; // пропускаем всё остальное, пока ресет
            }
        } else {
            // Если игра не выиграна — сбрасываем флаг
            gameJustWon = false;
            winnerPlayer = 0;
}

        window.display();
    }
    return 0;
}
