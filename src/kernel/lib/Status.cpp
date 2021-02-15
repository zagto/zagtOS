#include <common/common.hpp>
#include <lib/Status.hpp>

void Status::unhandled() {
    if (type == StatusType::OK) {
        cout << "Unchecked Status" << endl;
    } else {
        cout << "Unhandled Error" << endl;
    }
    Panic();
}
