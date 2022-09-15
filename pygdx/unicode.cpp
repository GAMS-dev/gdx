#include <iostream>
#include <fstream>
#include <string>
#include <array>
#include <cassert>

int main(int argc, char **argv) {
    std::ifstream ifs {"emojis.txt", std::ios::binary};
    assert(ifs.good());
    std::array<char, 256> buf {};
    ifs.get(buf.data(), 256);
    std::cout << std::string(buf.data()) << std::endl;
    return 0;
}