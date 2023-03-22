#ifndef AD9F0591_1A56_4BC9_86C1_A99036FB4D51
#define AD9F0591_1A56_4BC9_86C1_A99036FB4D51

#include <random>

class Rng
{
public:
    template <int AN, int AD, int BN, int BD>
    static float uniform()
    {
        static std::uniform_real_distribution<float> dist((float)AN / (float)AD, (float)BN / (float)BD);
        return dist(mt);
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

#endif /* AD9F0591_1A56_4BC9_86C1_A99036FB4D51 */
