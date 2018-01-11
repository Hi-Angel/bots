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
    int opp_i               = 0;
    s.opp[opp_i].pos        = {1000, 1000};
    s.opp[opp_i].speedEstim = 400;
    s.opp[opp_i].globAngle  = 225;
    OwnPod self;
    self.pos              = {0, 0};
    self.speedRelToOpp[0] = 500;
    self.globAngle        = 45;
    self.speedEstim       = 400;
    Maybe<OppPod> mb = canShieldOpponent(s, self, 0);
    DO_TEST(mb.Just);

    // test2, values depend on test1
    s.opp[opp_i].pos = {11003, 3776};
    mb = canShieldOpponent(s, self, 0);
    DO_TEST(!mb.Just);

    // test3
    s.opp[opp_i].pos = {11003, 3776};
    s.opp[opp_i].speedEstim = 284;
    s.opp[opp_i].globAngle = 327;
    self.pos = {11838, 3655};
    self.speedRelToOpp[0] = 447;
    self.globAngle = 133;
    self.speedEstim = 199;
    mb = canShieldOpponent(s, self, 0);
    DO_TEST(mb.Just);
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

int main() {
    test_vectorAngle();
    test_canShieldOpponent();
    test_doCarsCollide();
    test_chkAngle();
    cout << "testing finished" << endl;
}
