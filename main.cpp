#include <iostream>
#include <format>
#include <tuple>
#include <vector>
#include <set>
#include <utility>
#include <random>
#include <chrono>
#include <thread>
#include <stop_token>
#include <cmath>
#include <cstddef>

int main()
{
    constexpr std::size_t rows = 80,
                          columns = 120,
                          pointCount = rows * columns / 10,
                          minPoints = pointCount / 64;
    constexpr double epsilon = columns / 17.0;

    std::random_device rd;
    std::uniform_int_distribution<std::size_t> xDist(0, columns - 1),
                                               yDist(0, rows - 1);

    using coordinates = std::pair<std::size_t, std::size_t>;

    auto moveCursor = [](std::size_t row, std::size_t column)
    {
        std::cout << std::format("\033[{};{}H", row + 1, column + 1);
    };

    auto changeColor = [](const std::tuple<int, int, int>& color)
    {
        const auto [red, green, blue] = color;
        std::cout << std::format("\033[38;2;{};{};{}m", red, green, blue);
    };

    std::cout << "\033[?25l";

    std::jthread thr{[&](std::stop_token st)
    {
        while (!st.stop_requested())
        {
            std::cout << "\033[2J";

            std::vector<coordinates> points(pointCount);
            for (auto& [xRef, yRef] : points)
            {
                xRef = xDist(rd);
                yRef = yDist(rd);
            }

            auto findNeighbors = [&points](coordinates point)
            {
                const auto [x1, y1] = point;

                std::vector<coordinates> neighbors;
                for (const auto [x2, y2] : points)
                    if (const double distance = std::sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
                        distance < epsilon)
                        neighbors.emplace_back(x2, y2);

                return neighbors;
            };

            std::vector<std::vector<coordinates>> groups;
            std::set<coordinates> visitedPoints;

            for (const coordinates p1 : points)
            {
                if (visitedPoints.contains(p1))
                    continue;

                std::vector<coordinates> checklist{findNeighbors(p1)};

                if (checklist.size() >= minPoints)
                {
                    std::vector<coordinates> group;
                    group.push_back(p1);
                    visitedPoints.insert(p1);

                    while (!checklist.empty())
                    {
                        const coordinates p2{checklist.back()};
                        checklist.pop_back();

                        if (visitedPoints.contains(p2))
                            continue;

                        const std::vector<coordinates> neighbors{findNeighbors(p2)};

                        if (neighbors.size() >= minPoints)
                            checklist.insert(checklist.end(), neighbors.begin(), neighbors.end());

                        group.push_back(p2);
                        visitedPoints.insert(p2);
                    }

                    groups.emplace_back(std::move(group));
                }
            }

            std::vector<std::tuple<int, int, int>> colors{
                std::tuple{255, 0, 0},
                std::tuple{255, 255, 0},
                std::tuple{0, 255, 0},
                std::tuple{0, 255, 255},
                std::tuple{0, 0, 255},
                std::tuple{153, 51, 255},
                std::tuple{255, 102, 255},
                std::tuple{255, 255, 255}
            };

            for (const auto& group : groups)
            {
                if (!colors.empty())
                {
                    std::uniform_int_distribution<std::size_t> colorIndexDist(0, colors.size() - 1);
                    auto iter = colors.begin() + colorIndexDist(rd);
                    changeColor(*iter);
                    colors.erase(iter);
                }
                else
                {
                    std::uniform_int_distribution<int> rgbDist(0, 255);
                    changeColor(std::tuple{rgbDist(rd), rgbDist(rd), rgbDist(rd)});
                }

                for (const auto [column, row] : group)
                {
                    moveCursor(row, column);
                    std::cout << 'o';
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }};
    std::cin.get();

    std::cout << "\033[?25h";
    std::cout << "\033[0m";
}