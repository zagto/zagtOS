#include <time/Time.hpp>
#include <iostream>

void setTimer(uint64_t /*value*/) noexcept {
    cout << "setTimer" << endl;
    Panic();
}
