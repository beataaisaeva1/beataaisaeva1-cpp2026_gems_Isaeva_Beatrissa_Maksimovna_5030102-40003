#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <cmath>

std::random_device dev;
std::mt19937 rng(dev());

const int ROWS = 8;
const int COLS = 8;
const int CELL_SIZE = 70;
const int COLORS = 6;

std::vector<std::vector<int>> field(ROWS, std::vector<int>(COLS));

sf::Color colors[COLORS] = {sf::Color::Red, sf::Color::Blue, sf::Color::Green, sf::Color::Yellow, sf::Color::Magenta, sf::Color::Cyan};

int randomColor() {
    std::uniform_int_distribution<int> dist(0, COLORS - 1);
    return dist(rng);
}

bool inside(int r, int c) {
    return r >= 0 && r < ROWS && c >= 0 && c < COLS;
}

bool areNeighbors(int r1, int c1, int r2, int c2) {
    return abs(r1 - r2) + abs(c1 - c2) == 1;
}

void fillField() {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            field[i][j] = randomColor();
        }
    }
}

std::vector<std::pair<int, int>> findMatches() {
    std::vector<std::vector<bool>> visited(ROWS, std::vector<bool>(COLS, false));
    std::vector<std::pair<int, int>> result;

    int dr[4] = { -1, 1, 0, 0 };
    int dc[4] = { 0, 0, -1, 1 };

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (visited[i][j]) {
                continue;
            }
            int color = field[i][j];

            std::vector<std::pair<int, int>> group;
            std::queue<std::pair<int, int>> q;

            q.push({ i, j });

            visited[i][j] = true;

            while (!q.empty()) {
                std::pair<int, int> current = q.front();
                q.pop();
                int r = current.first;
                int c = current.second;
                group.push_back({ r, c });

                for (int k = 0; k < 4; k++) {
                    int nr = r + dr[k];
                    int nc = c + dc[k];
                    if (inside(nr, nc) && !visited[nr][nc] && field[nr][nc] == color) {
                        visited[nr][nc] = true;
                        q.push({ nr, nc });
                    }
                }
            }
            if (group.size() >= 3) {
                for (auto cell : group) {
                    result.push_back(cell);
                }
            }
        }
    }
    return result;
}

std::vector<std::pair<int, int>> cellsInRadius(int r, int c, int radius) {
    std::vector<std::pair<int, int>> cells;

    for (int i = r - radius; i <= r + radius; i++) {
        for (int j = c - radius; j <= c + radius; j++) {
            if (inside(i, j) && !(i == r && j == c)) {
                cells.push_back({ i, j });
            }
        }
    }
    return cells;
}

void bonusRepaint(int r, int c, int originalColor) {
    std::vector<std::pair<int, int>> cells = cellsInRadius(r, c, 3);
    std::vector<std::pair<int, int>> variants;
    for (auto cell : cells) {
        if (!areNeighbors(r, c, cell.first, cell.second)) {
            variants.push_back(cell);
        }
    }

    random_shuffle(variants.begin(), variants.end());

    field[r][c] = originalColor;

    for (int i = 0; i < 2 && i < variants.size(); i++) {
        field[variants[i].first][variants[i].second] = originalColor;
    }
    std::cout << "Bonus: repaint\n";
}

void bonusBomb(int r, int c) {
    std::vector<std::pair<int, int>> cells;

    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            if (!(i == r && j == c)) {
                cells.push_back({ i, j });
            }
        }
    }

    random_shuffle(cells.begin(), cells.end());

    field[r][c] = -1;

    for (int i = 0; i < 4 && i < cells.size(); i++) {
        field[cells[i].first][cells[i].second] = -1;
    }
    std::cout << "Bonus: bomb\n";
}

void tryCreateBonus(int r, int c, int originalColor) {
    std::uniform_int_distribution<int> distChance(0, 99);
    int chance = distChance(rng);

    if (chance >= 20) {
        return;
    }

    std::vector<std::pair<int, int>> cells = cellsInRadius(r, c, 3);

    if (cells.empty()) {
        return;
    }

    std::uniform_int_distribution<int> distCell(0, cells.size() - 1);
    std::pair<int, int> target = cells[distCell(rng)];

    std::uniform_int_distribution<int> distBonus(0, 1);
    int bonusType = distBonus(rng);

    if (bonusType == 0) {
        bonusRepaint(target.first, target.second, originalColor);
    }
    else {
        bonusBomb(target.first, target.second);
    }
}

void destroyCells(std::vector<std::pair<int, int>> matches) {
    if (matches.empty()) {
        return;
    }

    std::uniform_int_distribution<int> distIndex(0, matches.size() - 1);
    int bonusIndex = distIndex(rng);

    for (int i = 0; i < matches.size(); i++) {
        int r = matches[i].first;
        int c = matches[i].second;

        if (i == bonusIndex) {
            int oldColor = field[r][c];
            tryCreateBonus(r, c, oldColor);
        }
        field[r][c] = -1;
    }
}

void dropCells() {
    for (int c = 0; c < COLS; c++) {
        int writeRow = ROWS - 1;
        for (int r = ROWS - 1; r >= 0; r--) {
            if (field[r][c] != -1) {
                field[writeRow][c] = field[r][c];
                writeRow--;
            }
        }
        while (writeRow >= 0) {
            field[writeRow][c] = randomColor();
            writeRow--;
        }
    }
}

void processMatches() {
    std::vector<std::pair<int, int>> matches = findMatches();

    while (!matches.empty()) {
        destroyCells(matches);
        dropCells();

        matches = findMatches();
    }
}

void drawField(sf::RenderWindow& window, int selectedRow, int selectedCol) {
    for (int i = 0; i < ROWS; i++) {
        for (int j = 0; j < COLS; j++) {
            sf::RectangleShape cell;
            cell.setSize(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
            cell.setPosition(j * CELL_SIZE + 2, i * CELL_SIZE + 2);
            cell.setFillColor(colors[field[i][j]]);
            cell.setOutlineThickness(2);

            if (i == selectedRow && j == selectedCol) {
                cell.setOutlineColor(sf::Color::White);
            }
            else {
                cell.setOutlineColor(sf::Color::Black);
            }
            window.draw(cell);
        }
    }
}

int main() {
    fillField();

    sf::RenderWindow window(sf::VideoMode(COLS * CELL_SIZE, ROWS * CELL_SIZE), "GEMS");

    int selectedRow = -1;
    int selectedCol = -1;

    while (window.isOpen()) {
        sf::Event event;

        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window.close();
            }

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    int col = event.mouseButton.x / CELL_SIZE;
                    int row = event.mouseButton.y / CELL_SIZE;

                    if (!inside(row, col)) {
                        continue;
                    }
                    if (selectedRow == -1) {
                        selectedRow = row;
                        selectedCol = col;
                    }
                    else {
                        if (areNeighbors(selectedRow, selectedCol, row, col)) {
                            std::swap(field[selectedRow][selectedCol], field[row][col]);
                            std::vector<std::pair<int, int>> matches = findMatches();
                            if (matches.empty()) {
                                std::swap(field[selectedRow][selectedCol], field[row][col]);
                            }
                            else {
                                processMatches();
                            }
                        }
                        selectedRow = -1;
                        selectedCol = -1;
                    }
                }
            }
        }
        window.clear(sf::Color(40, 40, 40));
        drawField(window, selectedRow, selectedCol);
        window.display();
    }
    return 0;
}
