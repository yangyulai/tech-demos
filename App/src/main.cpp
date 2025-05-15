#include <Common.hpp>
#include <iostream>
int main() {
    //lua_State *L = luaL_newstate();
    //luaL_openlibs(L);
    // ...
    //lua_close(L);
    std::cout << "Hello, World!\n";
    std::cout << MyLib::greet("World") << "\n";
    return 0;
}
