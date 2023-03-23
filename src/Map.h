#ifndef MAP_H
#define MAP_H

#include "Object.h"
#include "CommandProcessor.h"
#include <array>

class Map
{
public:
    virtual b2Vec2 randomPosition() const = 0;
    inline static Map *create();

    static void setMap(Map *m)
    {
        if (map != nullptr)
            delete map;
        map = m;
    }
    
    static const Map &getMap()
    {
        return *map;
    }

private:
    static inline Map *map = nullptr;
};

class OpenMap : public Map
{
public:
    static Map *create() { return new OpenMap(); }
    OpenMap()
    {
        std::size_t n = 60;
        while (n--)
            Rock::create();
    }
    b2Vec2 randomPosition() const override
    {
        return {(Rng::uniform() * world::limits * 2.f - world::limits) / world::scale,
                (Rng::uniform() * world::limits * 2.f - world::limits) / world::scale};
    }
};

class MazeMap : public Map
{
public:
    MazeMap() : rooms(roomsNumber)
    {
        char c = 'a';
        for (auto &row : map)
            for (auto &point : row)
                point = '.';
        for (auto &room : rooms)
        {
            room.x = width * Rng::uniform();
            room.y = height * Rng::uniform();
            room.width = radius * Rng::uniform<3, 4, 5, 4>();
            room.height = radius * Rng::uniform<3, 4, 5, 4>();
            for (int i = room.y - room.height; i <= room.y + room.height; i++)
                for (int j = room.x - room.width; j <= room.x + room.width; j++)
                    if (i >= 0 && i < height && j >= 0 && j < width)
                        map[i][j] = c;

            c++;
        }
        for (int r = 1; r < rooms.size(); r++)
            connect(rooms[r - 1], rooms[r]);

        fill();

        /*for (auto &row : map)
        {
            for (auto &point : row)
                std::cout << point;
            std::cout << std::endl;
        }*/
    }

    b2Vec2 randomPosition() const override
    {
        int i = rooms.size() * Rng::uniform();
        return {transform(rooms[i].x), transform(rooms[i].y)};
    }

private:
    static constexpr int size = 100;
    static constexpr int width = size;
    static constexpr int height = size;
    static constexpr int radius = 5;
    static constexpr int roomsNumber = 5;
    struct Rect
    {
        int x, y, width, height;
    };
    std::array<std::array<char, width>, height> map;
    std::vector<Rect> rooms;
    void connect(const Rect &a, const Rect &b)
    {
        int xMin = std::min(a.x, b.x);
        int yMin = std::min(a.y, b.y);
        int xMax = std::max(a.x, b.x);
        int yMax = std::max(a.y, b.y);

        for (int i = yMin; i <= yMax; i++)
            if (i >= 0 && i < height)
                map[i][xMin] = '+';

        for (int i = xMin; i <= xMax; i++)
            if (i >= 0 && i < width)
                map[yMin][i] = '+';

        for (int i = yMin; i <= yMax; i++)
            if (i >= 0 && i < height)
                map[i][xMax] = '+';

        for (int i = xMin; i <= xMax; i++)
            if (i >= 0 && i < width)
                map[yMax][i] = '+';
    }
    bool free(int x1, int y1, int x2, int y2)
    {
        int xMin = std::min(x1, x2);
        int yMin = std::min(y1, y2);
        int xMax = std::max(x1, x2);
        int yMax = std::max(y1, y2);

        for (int i = yMin; i <= yMax; i++)
            for (int j = xMin; j <= xMax; j++)
                if (i >= 0 && i < height && j >= 0 && j < width)
                {
                    if (map[i][j] != '.')
                        return false;
                }
                else
                    return false;
        return true;
    }
    inline float transform(float x) const
    {
        return x * (Borders::pos - Borders::width) * 2.f / float(size) - (Borders::pos - Borders::width);
    }

    void fill(int x1, int y1, int x2, int y2)
    {
        float xMin = std::min(x1, x2);
        float yMin = std::min(y1, y2);
        float xMax = std::max(x1, x2);
        float yMax = std::max(y1, y2);

        for (int i = yMin; i <= yMax; i++)
            for (int j = xMin; j <= xMax; j++)
                if (i >= 0 && i < height && j >= 0 && j < width)
                    map[i][j] = ' ';

        constexpr float margin = 0.1f;

        if (xMax - xMin >= 0.f && yMax - yMin >= 1.f)
            Rock::create({{transform(xMin) + margin, transform(yMin) + margin},
                          {transform(xMax + 1) - margin, transform(yMin) + margin},
                          {transform(xMax + 1) - margin, transform(yMax + 1) - margin},
                          {transform(xMin) + margin, transform(yMax + 1) - margin}});
    }
    void fill()
    {
        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                if (map[i][j] == '.')
                {
                    int y1 = i;
                    int x1 = j;
                    while (j < width && map[i][j] == '.')
                        j++;
                    j--;
                    int ii = i;
                    while (ii < height && free(x1, y1, j, ii))
                        ii++;
                    ii--;
                    fill(x1, y1, j, ii);
                }
    }
};

Map *Map::create()
{
    if (Rng::uniform() >= 0.5f)
        return new MazeMap();
    else
        return new OpenMap();
}

#endif /* MAP_H */