// https://zhuanlan.zhihu.com/p/112219748

#include <regex>
#include <iostream>

int main() {
    std::regex e("[0-9a-zA-Z]*@g");
    bool m1 = std::regex_search("i love ysyfrank@gmail.com", e);
    bool m2 = std::regex_search("i love ysyfrank@163.com", e);
    // output yes
    std::cout << (m1 ? "yes" : "no") << std::endl;
    // output no
    std::cout << (m2 ? "yes" : "no") << std::endl;
    return 0;
}