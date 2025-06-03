#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <ctime>
#include <algorithm>

// Функции для цветов плиток и текста 
sf::Color tileColor(int value) {
    switch (value) {
    case 2:    return sf::Color(238, 228, 218);
    case 4:    return sf::Color(237, 224, 200);
    case 8:    return sf::Color(242, 177, 121);
    case 16:   return sf::Color(245, 149, 99);
    case 32:   return sf::Color(246, 124, 95);
    case 64:   return sf::Color(246, 94, 59);
    case 128:  return sf::Color(237, 207, 114);
    case 256:  return sf::Color(237, 204, 97);
    case 512:  return sf::Color(237, 200, 80);
    case 1024: return sf::Color(237, 197, 63);
    case 2048: return sf::Color(237, 194, 46);
    default:   return sf::Color(60, 58, 50);
    }
}

sf::Color textColor(int value) {
    if (value <= 4)
        return sf::Color(119, 110, 101);
    else
        return sf::Color::White;
}

const int SIZE = 4;
const float TILE_SIZE = 100.f;
const float TILE_MARGIN = 10.f;
const float ANIM_DURATION = 0.15f; // 150 мс

sf::Vector2f gridToPixel(int x, int y) {
    return sf::Vector2f(
        TILE_MARGIN + x * (TILE_SIZE + TILE_MARGIN),
        TILE_MARGIN + y * (TILE_SIZE + TILE_MARGIN)
    );
}

struct Tile {
    int value = 0;
    int x = 0, y = 0;
    bool merged = false;
    sf::Vector2f pixelPos;
    sf::Vector2f startPos;
    sf::Vector2f targetPos;
    float animTime = 0.f;
    float animDuration = ANIM_DURATION;
    bool moving = false;
};

struct Board {
    std::vector<Tile> tiles;

    Board() {
        spawnTile();
        spawnTile();
    }

    void spawnTile() {
        std::vector<std::pair<int, int>> empty;
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x)
                if (!getTile(x, y))
                    empty.emplace_back(x, y);

        if (empty.empty()) return;

        size_t rand_index = rand() % empty.size();
        int x = empty[rand_index].first;
        int y = empty[rand_index].second;

        Tile t;
        t.value = (rand() % 10 == 0) ? 4 : 2;
        t.x = x;
        t.y = y;
        t.pixelPos = gridToPixel(x, y);
        tiles.push_back(t);
    }

    Tile* getTile(int x, int y) {
        for (auto& t : tiles)
            if (t.x == x && t.y == y) return &t;
        return nullptr;
    }

    void resetMerged() {
        for (auto& t : tiles) t.merged = false;
    }

    bool move(int dx, int dy) {
        bool moved = false;
        resetMerged();
        auto order = tiles;
        std::sort(order.begin(), order.end(), [dx, dy](const Tile& a, const Tile& b) {
            if (dx == 1)   return a.x > b.x;
            if (dx == -1)  return a.x < b.x;
            if (dy == 1)   return a.y > b.y;
            if (dy == -1)  return a.y < b.y;
            return false;
            });

        for (auto& t : order) {
            Tile* tile = getTile(t.x, t.y);
            if (!tile) continue;
            int nx = tile->x, ny = tile->y;
            while (true) {
                int tx = nx + dx, ty = ny + dy;
                if (tx < 0 || tx >= SIZE || ty < 0 || ty >= SIZE) break;
                Tile* next = getTile(tx, ty);
                if (!next) {
                    nx = tx; ny = ty;
                }
                else if (next->value == tile->value && !next->merged && !tile->merged) {
                    next->value *= 2;
                    next->merged = true;
                    tile->merged = true;
                    tile->x = nx; tile->y = ny;
                    startTileMove(*tile, gridToPixel(tx, ty));
                    tiles.erase(std::remove_if(tiles.begin(), tiles.end(), [&](const Tile& t2) {
                        return &t2 == tile;
                        }), tiles.end());
                    moved = true;
                    break;
                }
                else break;
            }
            if (tile && (tile->x != nx || tile->y != ny)) {
                tile->x = nx; tile->y = ny;
                startTileMove(*tile, gridToPixel(nx, ny));
                moved = true;
            }
        }
        if (moved) spawnTile();
        return moved;
    }

    void startTileMove(Tile& t, sf::Vector2f newPixelPos) {
        t.startPos = t.pixelPos;
        t.targetPos = newPixelPos;
        t.animTime = 0.f;
        t.animDuration = ANIM_DURATION;
        t.moving = true;
    }

    void animateTiles(float dt) {
        for (auto& t : tiles) {
            if (t.moving) {
                t.animTime += dt;
                float alpha = std::min(1.f, t.animTime / t.animDuration);
                alpha = alpha * alpha * (3 - 2 * alpha);
                t.pixelPos = t.startPos + (t.targetPos - t.startPos) * alpha;
                if (alpha >= 1.f) {
                    t.pixelPos = t.targetPos;
                    t.moving = false;
                }
            }
        }
    }

    // Проверка возможности хода
    bool canMove() const {
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x) {
                const Tile* t = nullptr;
                for (const auto& tile : tiles)
                    if (tile.x == x && tile.y == y)
                        t = &tile;
                if (!t) return true; // Есть пустая клетка
                // Проверка соседей
                for (int d = 0; d < 4; ++d) {
                    int nx = x + (d == 0) - (d == 1);
                    int ny = y + (d == 2) - (d == 3);
                    if (nx >= 0 && nx < SIZE && ny >= 0 && ny < SIZE) {
                        for (const auto& t2 : tiles)
                            if (t2.x == nx && t2.y == ny && t2.value == t->value)
                                return true;
                    }
                }
            }
        return false;
    }
};

int main() {
    srand(static_cast<unsigned>(time(nullptr)));
    sf::RenderWindow window(sf::VideoMode(
        SIZE * (TILE_SIZE + TILE_MARGIN) + TILE_MARGIN,
        SIZE * (TILE_SIZE + TILE_MARGIN) + TILE_MARGIN
    ), "2048");

    Board board;
    sf::Font font;
    font.loadFromFile("arial.ttf");

    sf::Clock clock;
    bool animating = false;

    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        bool gameOver = !board.canMove();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (!animating && event.type == sf::Event::KeyPressed) {
                if (gameOver && event.key.code == sf::Keyboard::R) {
                    board = Board();
                    animating = false;
                }
                else if (!gameOver) {
                    switch (event.key.code) {
                    case sf::Keyboard::Left:
                    case sf::Keyboard::A:
                        animating = board.move(-1, 0); break;
                    case sf::Keyboard::Right:
                    case sf::Keyboard::D:
                        animating = board.move(1, 0); break;
                    case sf::Keyboard::Up:
                    case sf::Keyboard::W:
                        animating = board.move(0, -1); break;
                    case sf::Keyboard::Down:
                    case sf::Keyboard::S:
                        animating = board.move(0, 1); break;
                    case sf::Keyboard::R:
                        board = Board();
                        animating = false;
                        break;
                    default:
                        break;
                    }
                }
            }
        }

        if (animating) {
            board.animateTiles(dt);
            animating = false;
            for (auto& t : board.tiles)
                if (t.moving) animating = true;
        }

        window.clear(sf::Color(250, 248, 239));
        // Рисуем сетку
        for (int y = 0; y < SIZE; ++y)
            for (int x = 0; x < SIZE; ++x) {
                sf::RectangleShape rect(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                rect.setPosition(gridToPixel(x, y));
                rect.setFillColor(sf::Color(205, 193, 180));
                window.draw(rect);
            }
        // Рисуем плитки с цветами
        for (auto& t : board.tiles) {
            sf::RectangleShape rect(sf::Vector2f(TILE_SIZE, TILE_SIZE));
            rect.setPosition(t.pixelPos);
            rect.setFillColor(tileColor(t.value));
            window.draw(rect);

            sf::Text text;
            text.setFont(font);
            text.setString(std::to_string(t.value));
            text.setCharacterSize(40);
            text.setFillColor(textColor(t.value));
            sf::FloatRect bounds = text.getLocalBounds();
            // Центрируем текст по центру плитки
            text.setOrigin(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
            text.setPosition(t.pixelPos.x + TILE_SIZE / 2, t.pixelPos.y + TILE_SIZE / 2);
            window.draw(text);
        }

        // Game Over
        if (gameOver) {
            // Затемняющий полупрозрачный прямоугольник на всё окно
            sf::RectangleShape darkOverlay(sf::Vector2f(window.getSize().x, window.getSize().y));
            darkOverlay.setFillColor(sf::Color(0, 0, 0, 150)); // чёрный с прозрачностью
            window.draw(darkOverlay);

            // Надпись Game Over
            sf::Text text;
            text.setFont(font);
            text.setString("Game Over\nPress R to restart");
            text.setCharacterSize(32);
            text.setFillColor(sf::Color::Red);
            sf::FloatRect bounds = text.getLocalBounds();
            text.setOrigin(bounds.left + bounds.width / 2, bounds.top + bounds.height / 2);
            text.setPosition(window.getSize().x / 2, window.getSize().y / 2);
            window.draw(text);
        }

        window.display();
    }
    return 0;
}
