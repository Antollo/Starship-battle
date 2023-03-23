#ifndef RNG_H
#define RNG_H

#include <random>
#include "Vec2f.h"

class Rng
{
public:
    template <int AN, int AD, int BN, int BD>
    static float uniform()
    {
        static std::uniform_real_distribution<float> dist((float)AN / (float)AD, (float)BN / (float)BD);
        return dist(mt);
    }

    template <int A, int B>
    static float uniform()
    {
        static std::uniform_real_distribution<float> dist((float)A, (float)B);
        return dist(mt);
    }

    static float uniform()
    {
        static std::uniform_real_distribution<float> dist(0.f, 1.f);
        return dist(mt);
    }

    static Vec2f uniform2d()
    {
        static std::uniform_real_distribution<float> dist(0.f, 1.f);
        return {dist(mt), dist(mt)};
    }

    template <int MN, int MD, int DN, int DD>
    static float normal()
    {
        static std::normal_distribution<float> dist((float)MN / (float)MD, (float)DN / (float)DD);
        return dist(mt);
    }

    class Normal
    {
        public:
            float operator()()
            {
                return dist(mt);
            }
            void setParams(float mean, float stddev)
            {
                dist.param(std::normal_distribution<float>::param_type(mean, stddev));
            }
            private:
            std::normal_distribution<float> dist;
    };

private:
    static inline std::random_device rd;
    static inline std::mt19937 mt{rd()};
};

#endif /* RNG_H */
