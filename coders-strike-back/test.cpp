#define TESTS

#include "main.cpp"

#define DO_TEST(test) do {                      \
        if (!(test)) {                          \
            cout << "failing "#test << endl;   \
            fail = true;                        \
        }                                       \
    } while(0)

int main() {
    bool fail = false;
    DO_TEST(vectorAngle({1,1}).val == 45);
    DO_TEST(vectorAngle({-1,1}).val == 135);
    DO_TEST(vectorAngle({-1,-1}).val == 225);
    DO_TEST(vectorAngle({1,-1}).val == 315);
    DO_TEST(vectorAngle({0,1}).val == 90);
    DO_TEST(vectorAngle({0,-1}).val == 270);
    DO_TEST(vectorAngle({1,0}).val == 0);
    DO_TEST(vectorAngle({-1,0}).val == 180);
    cout << "tests" << ((fail)? " failed" : " passed") << endl;
}
