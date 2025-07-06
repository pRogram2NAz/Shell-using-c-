#include <iostream>
#include <optional>
using namespace std;

optional<int> findEven(int x) {
    if (x % 2 == 0)
        return x;
    else
        return {};  // No value
}

int main() {
    optional<int> result = findEven(7);

    if (result.has_value()) {
        cout << "Even number: " << result.value() << endl;
    } else {
        cout << "Not even!" << endl;
    }
}
