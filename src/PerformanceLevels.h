#ifndef PERFORMANCELEVELS_H_
#define PERFORMANCELEVELS_H_

namespace PerformanceLevels
{

enum class Id
{
    High,
    Normal,
    Low
};

class High
{
public:
    static constexpr Id id = Id::High;
    static constexpr int velocityIterations = 10;
    static constexpr int positionIterations = 6;
    static constexpr float minimumBulletVelocity = 0.5f;
};

class Normal
{
public:
    static constexpr Id id = Id::Normal;
    static constexpr int velocityIterations = 8;
    static constexpr int positionIterations = 3;
    static constexpr float minimumBulletVelocity = 0.5f;
};

class Low
{
public:
    static constexpr Id id = Id::Low;
    static constexpr int velocityIterations = 6;
    static constexpr int positionIterations = 2;
    static constexpr float minimumBulletVelocity = 0.7f;
};

template <class T>
void set(Id &id, int &velocityIterations, int &positionIterations, float &minimumBulletVelocity)
{
    id = T::id;
    velocityIterations = T::velocityIterations;
    positionIterations = T::positionIterations;
    minimumBulletVelocity = T::minimumBulletVelocity;
}

}; // namespace PerformanceLevels

#endif /* !PERFORMANCELEVELS_H_ */
