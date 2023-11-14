#include <vector>

// 朴素的 KMP Matcher, 应该用AC自动机的:(
template <typename T>
class Matcher
{
    std::vector<T> pattern;
    std::vector<size_t> f;
    size_t index = 0;

public:
    Matcher() = default;

    template <typename Ty>
    constexpr Matcher(Ty&& pattern) : pattern(std::forward<Ty>(pattern))
    {
        f.resize(this->pattern.size() + 1);
        f[0] = f[1] = 0;
        for (size_t i = 1, j = 0; i < this->pattern.size(); i++)
        {
            while (j && this->pattern[i] != this->pattern[j])
                j = f[j];
            j += (this->pattern[i] == this->pattern[j]);
            f[i + 1] = j;
        }
    }

    void reset() { index = 0; }

    size_t size() const { return this->pattern.size(); }

    bool match(const T& c)
    {
        if (this->pattern.empty())
            return false;
        while (index && c != this->pattern[index])
            index = f[index];
        index += (c == this->pattern[index]);
        return index == this->pattern.size();
    }
};
