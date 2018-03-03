#include <iostream>
#include <array>
#include <unordered_map>

struct ArrayHasher {
    std::size_t operator() (const std::array<float, 3> & a) const {
        std::size_t h = 0;
        for (auto e : a) {
            h ^= std::hash<float>{}(e) + 0x9e3779b9 + (h<<6) + (h>>2);
        }
        return h;
    }
};

std::array<float, 3> test = {1, 2, 3};
std::array<float, 3> test0 = {1, 2, 3};
std::array<float, 3> test1 = {1, 1, 1};
std::array<float, 3> test2 = {1, 0, 0};
std::array<float, 3> test3 = {1, 2, 3};

std::unordered_map< std::array<float, 3>, int, ArrayHasher> counter;

int main() {
    counter[test]++;
    counter[test0]++;
    counter[test1]++;
    counter[test2]++;
    counter[test3]++;

    printf("%i\n", counter[test]);
    printf("%i\n", counter[test0]);
    printf("%i\n", counter[test1]);
    printf("%i\n", counter[test2]);
    printf("%i\n", counter[test3]);
    return 0;
}
