#pragma once

class HaltonGenerator
{
public:
    float getNextValue()
    {
        float range = max - min;
        return halton(index++) * range + min;
    }

    void reset()
    {
        index = 1;
    }

    void setMin (float newMin) {
        min = newMin;
    }
    
    void setMax (float newMax) {
        max = newMax;
    }

private:
    static float halton(int n)
    {
        float f = 1.0f;
        float r = 0.0f;

        while (n > 0)
        {
            f *= 0.5f;
            r += f * (n % 2);
            n /= 2;
        }

        return r;
    }

    int index = 1;
    float min = 0.0f;
    float max = 1.0f;
};