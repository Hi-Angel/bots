#define TESTS

#include "main.cpp"

#define DO_TEST(test) do {                      \
        if (!(test)) {                          \
            cout << "failing "#test << endl;    \
            fail = true;                        \
        }                                       \
    } while(0)

static bool fail = false;

void test_vectorAngle() {
    DO_TEST(vectorAngle({1,1}).val == 45);
    DO_TEST(vectorAngle({-1,1}).val == 135);
    DO_TEST(vectorAngle({-1,-1}).val == 225);
    DO_TEST(vectorAngle({1,-1}).val == 315);
    DO_TEST(vectorAngle({0,1}).val == 90);
    DO_TEST(vectorAngle({0,-1}).val == 270);
    DO_TEST(vectorAngle({1,0}).val == 0);
    DO_TEST(vectorAngle({-1,0}).val == 180);
}

void test_canShieldOpponent() {
    // test1, trivial
    GameState s;
    s.chks.push_back({{0,0}, 0});
    int opp_i               = 0;
    s.opp[opp_i].pos        = {1000, 1000};
    s.opp[opp_i].speedEstim = 400;
    s.opp[opp_i].globAngle  = 225;
    OwnPod self;
    self.pos              = {0, 0};
    self.chkDist          = 100000; // arbitrarily big number
    self.speedRelToOpp[0] = 500;
    self.globAngle        = 45;
    self.speedEstim       = 400;
    const OppPod* mb1 = canShieldOpponent(s, self, opp_i);
    DO_TEST(mb1);

    // test2, values depend on test1
    s.opp[opp_i].pos = {11003, 3776};
    const OppPod* mb2 = canShieldOpponent(s, self, opp_i);
    DO_TEST(!mb2);

    // test3
    s.opp[opp_i].pos = {11003, 3776};
    s.opp[opp_i].speedEstim = 284;
    s.opp[opp_i].globAngle = 327;
    self.pos = {11838, 3655};
    self.speedRelToOpp[0] = 447;
    self.globAngle = 133;
    self.speedEstim = 199;
    const OppPod* mb3 = canShieldOpponent(s, self, opp_i);
    DO_TEST(mb3);

    // test4
    // this one is failing, and it seems the reason being that acc. to calculations
    // pods pass each after another very closely, whereas they actually collide. It's
    // hard to say if it's an inaccuracy in calculations, or rather that the game
    // considers path of every turn as whole. In any case I think the right way of
    // dealing with that is using the later technique. Surprisingly, such accidents
    // aren't rare.
    s.opp[opp_i].pos = {6839, 6605};
    s.opp[opp_i].speedEstim = 511;
    s.opp[opp_i].globAngle = 9;
    self.pos = {7560, 5681};
    self.speedRelToOpp[opp_i] = 673;
    self.globAngle = 154;
    self.speedEstim = 409;
    const OppPod* mb4 = canShieldOpponent(s, self, opp_i);
    DO_TEST(mb4);
}

void test_doCarsCollide() {
    struct Pod {
        Point pos;
        int speedEstim;
        Degree globAngle;
    };

    Pod self;
    Pod opp;
    opp.pos = {11003, 3776};
    opp.speedEstim = 284;
    opp.globAngle = 327;
    self.pos = {11838, 3655};
    self.speedEstim = 199;
    self.globAngle = 133;
    DO_TEST(doCarsCollide(opp, self));
}

void test_movePoint() {
    // test right
    Radian rad = 0;
    Point p1 = movePoint(5, rad),
        p2 = Point{5, 0};
    DO_TEST(p1 == p2);

    // test quadrant Ⅰ
    rad = M_PI/4;
    p1 = movePoint(5, rad),
        p2 = Point{3, 3};
    DO_TEST(p1 == p2);

    // test up
    rad = M_PI/2;
    p1 = movePoint(5, rad),
        p2 = Point{0, 5};
    DO_TEST(p1 == p2);

    // test quadrant Ⅱ
    rad = M_PI/4 *3;
    p1 = movePoint(5, rad);
    p2 = Point{-3, 3};
    DO_TEST(p1 == p2);

    // test right
    rad = M_PI;
    p1 = movePoint(5, rad),
        p2 = Point{-5, 0};
    DO_TEST(p1 == p2);

    // test quadrant Ⅲ
    rad = M_PI/4 + M_PI;
    p1 = movePoint(5, rad);
    p2 = Point{-3, -3};
    DO_TEST(p1 == p2);

    // test down
    rad = M_PI*1.5;
    p1 = movePoint(5, rad),
        p2 = Point{0, -5};
    DO_TEST(p1 == p2);

    // test quadrant Ⅳ
    rad = M_PI/4 + M_PI*1.5;
    p1 = movePoint(5, rad);
    p2 = Point{3, -3};
    DO_TEST(p1 == p2);
}

// void test_bisectSpeed() {
//     int carDist = 5989,
//         speed = 539;
//     Degree deg = {24};
//     DO_TEST(bisectAccel(carDist, deg, speed, true) == 0);
// }

void test_chkAngle() {
    OwnPod self;
    Point chk;

    // test1
    self.pos = {3614, 6987};
    self.chkDist = 11008;
    self.globAngle = 143;
    chk = {14600,7687};
    Degree deg1 = chkAngle(chk, self);
    DO_TEST(deg1.val != 0);
}

void test_angleC() {
    float a = 1, b = 1.41, c = 2.24;
    DO_TEST(angleC(a, b, c).val == 136);
    a = 6399, b = 1526, c = 7926;
    DO_TEST(angleC(a, b, c).val == 180);
}

int main() {
    test_vectorAngle();
    test_canShieldOpponent();
    test_doCarsCollide();
    test_chkAngle();
    test_movePoint();
    test_angleC();
    cout << "testing finished" << endl;
}
