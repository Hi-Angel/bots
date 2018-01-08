#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <cmath>
#include <stdlib.h>

using namespace std;
using uint = unsigned;

const unsigned short chkPointRadius = 600,
    carRadius = 400;

struct Point {
    int x, y;
    bool operator==(const Point& p) const {
        return p.x == x && p.y == y;
    }
    bool operator!=(const Point& p) const {
        return !(p == *this);
    }
    Point operator-(const Point& rhs) const {
        return {x - rhs.x, y - rhs.y};
    }
    void operator+=(const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
    }
    friend ostream& operator<<(ostream& os, const Point& p) {
        os << "x: " << p.x << " y: " << p.y;
        return os;
    }
};

template <typename T>
struct Maybe {
    bool Just;
    T val;
};

struct OppPod {
    Point pos, prevPos;
};

struct OwnPod {
    Point pos, prevPos,
        target; //target to drive to
    int chkDist, currAcc,
        prevChkDist,
        nextCheckpointAngle, // NOTE: it's -180..180, negative is left angle
        chkId,
        speedEstim; // it's a substract of prev and curr distance, don't trust too much
    vector<int> speedRelToOpp, // speed relative to opponent by index
        prevOppDist,
        oppDist;
    string speed;
    bool attacking, hasBoost = true;
};

struct GameState {
    vector<pair<Point,int>> chks;
    bool firstRun = true;
    vector<OppPod> opp;
    vector<OwnPod> self;
};

// template< class InputIt, class T >
// T find_or_default( InputIt first, InputIt last, const T& value, T dflt) {
//     InputIt it = find(first, last, value);
//     return (it == last)? dflt : *last;
// }

// algorithm:
// (carX, carY) <- currPoint
// let hyp = chkDistance
//     opp = currChkX - carX
//     adj = currChkY - carY
//     sin = opp/hyp
//     cos = adj/hyp
//     newHyp = hyp - chkPointRadius
//     newOpp = newHyp * sin
//     newAdj = newHyp * cos
//     edgeChkX = carX + newOpp
//     edgeChkY = carY + newAdj
// return (edgeChkX, edgeChkY)

inline constexpr float degToRad(int degree) { return degree*(M_PI/180); }
inline constexpr float radToDeg(float rad) { return rad*(180/M_PI); }

// every big letter is an angle opposite to its small letter counterpart, meaning a
// side. a and b are adjacent sides, order of a and b doesn't matter
int angleC(float a, float b, float c) {
    if (!(a && b && c)) // there's no angle, and calculations don't handle it
        return 0;
    float cos_c = (a*a + b*b - c*c) / (float)(2*a*b);
    return (cos_c > 1)? 0 : radToDeg(acosf(cos_c));
}

float distance(const Point& a, const Point& b) {
    // distance is length of hypotenuse in right triangle between the points
    int adj = a.x - b.x,
        opp = a.y - b.y;
    return sqrtf(adj*adj + opp*opp);
}

// calculates whether the point in question is left or right to line.
// note: if it's on the line, value is undefined, for my purposes that's okay
bool isLeftToLine(const Point& start, const Point& end, const Point& q) {
    return ((q.x - start.x)*(end.y - start.y) - (q.y - start.y) * (end.x - start.x))
        > 0;
}

bool isDotInCircle(const Point& origin, int rad, const Point& dot) {
    int x_diff = dot.x - origin.x,
        y_diff = dot.y - origin.y;
    return x_diff*x_diff + y_diff*y_diff <= rad * rad;
}

const Maybe<OppPod> canHitOpponent(const GameState& s, const OwnPod& self) {
    for (uint i=0; i < s.opp.size(); ++i) {
        const OppPod& opp = s.opp[i];
        if (isDotInCircle(s.chks[self.chkId].first, chkPointRadius, opp.pos)
            && self.oppDist[i] <= carRadius*4)
            return {true, opp};
        // use separate variables to ease future refactoring for multiple cars
        const Point& chk = s.chks[self.chkId].first;
        const int chkAngle = self.nextCheckpointAngle, // -180..180
            chkDist = self.chkDist;

        const int oppDist = distance(self.pos, opp.pos),
            oppChkDist = distance(opp.pos, chk),
            selfToChkOppAngle = isLeftToLine(self.pos, chk, opp.pos)
            ? -angleC(chkDist, oppDist, oppChkDist)
            : angleC(chkDist, oppDist, oppChkDist),
            oppAngle = selfToChkOppAngle + chkAngle;
        // if (chkAngle > 0) { just a memo
        //     if (oppAngle > 0)
        //         oppAngle = selfToChkOppAngle + chkAngle;
        //     else
        //         oppAngle = selfToChkOppAngle + chkAngle;
        // } else {
        //     if (oppAngle > 0)
        //         oppAngle = selfToChkOppAngle + chkAngle;
        //     else
        //         oppAngle = selfToChkOppAngle + chkAngle;
        // }

        // cerr << "chkDist " << chkDist << " oppDist " << oppDist << " oppChkDist " << oppChkDist << endl;
        // cerr << "oppANgle: " << oppAngle << " chkANgle " << chkAngle << endl;
        if (((abs(oppAngle) >= 50 && abs(oppAngle) <= 130)
                 // now go for an experiment: if an opponent right behind me so close,
                 // he probably drives my direction
                 || (abs(oppAngle) >= 190))
                && oppDist - carRadius*2 <= self.speedRelToOpp[i]+100)
            return {true, opp};
    }
    return {false, {}};
}

const Maybe<OppPod> canShieldOpponent(const GameState& s, const OwnPod& self) {
    for (uint i=0; i < s.opp.size(); ++i) {
        const OppPod& opp = s.opp[i];
        const int oppDist = self.oppDist[i],
            oppAngle = angleC(oppDist, self.chkDist,
                              distance(opp.pos, s.chks[self.chkId].first));
        if (oppDist - carRadius*2 <= self.speedRelToOpp[i]
                && self.speedRelToOpp[i] >= 140 // else too slow, following 3 turns not worth it
                && abs(oppAngle) >= 65 && abs(self.nextCheckpointAngle) >= 9 // don't push opponent forward
                )
            return {true, opp};
    }
    return {false, {}};
}

float oppositeLen(float aLen, float bLen, int abAngle) {
    float cLenSquared = (aLen * aLen) + (bLen * bLen)
        - 2*aLen*bLen*cosf(degToRad(abAngle));
    return sqrtf(cLenSquared);
}

// returns the checkpoint angle between prev and curr positions. Negative if
// counterclockwise, positive otherwise.
int inertiaAngle(const Point& chkPoint, const OwnPod& self, const Point& target) {
    // cerr << " distance(s.oughtPos, s.opp.pos): " << distance(s.opp.pos, s.oughtPos)
    //      << " distance(s.opp.pos, s.currPos): " << distance(s.opp.pos, s.currPos)
    //      << " distance(s.oughtPos, s.currPos): " << distance(s.oughtPos, s.currPos) << endl;
    // int inertia = angleC(distance(s.opp.pos, s.chkPoint), // s.speed,
    //                      distance(s.opp.pos, s.currPos),
    //                      distance(s.chkPoint, s.currPos));
    int inertia = angleC(distance(target, self.pos), // s.speed,
                         distance(target, self.prevPos),
                         distance(self.prevPos, self.pos));
    return (isLeftToLine(chkPoint, self.prevPos, self.pos))? -inertia : inertia;
}

// bisection does not produce optimal speed. Consider the len interval (10…7…10),
// where we strive for lowest possible len. Which half shall bisection choose, left
// or right? To get the optimum you'd need to use optimization techniques instead,
// but I don't know them offhand, and for now I consider bisection good enough. I may
// possibly change my mind though.
int bisectAccel(int carDist, int carAngle, int speed) {
    // terms: "adjacent len" is a "speed" from a trigonometric POV
    int bottom = 0, top = 100;
    float bottomLen = oppositeLen(carDist, bottom+speed, carAngle),
        topLen = oppositeLen(carDist, top+speed, carAngle);
    for (;;) {
        int nextAdjLen = bottom + (top - bottom) / 2;
        float oppLen = oppositeLen(carDist, nextAdjLen+speed, carAngle);
        if (nextAdjLen <= bottom+1 || nextAdjLen >= top-1) { // technically, it can be e.g.either bottom or bottom+1
            if (bottomLen < topLen)
                return (bottomLen < oppLen)? bottom : nextAdjLen;
            else
                return (topLen < oppLen)? top : nextAdjLen;
        }

        if (bottomLen <= topLen) { // at (5…x…6) or (5…x…5) blindly pick left half
            top = nextAdjLen;
            topLen = oppLen;
        } else {
            bottom = nextAdjLen;
            bottomLen = oppLen;
        }
    }
}

bool circlesIntersect(const Point& a, const Point& b, int radius) {
    return abs(a.x - b.x) <= radius && abs(a.y - b.y) <= radius;
}

// shifts the point along the edge of circle
// if shiftByRad >= 0 then clockwise movement else counterclockwise
Point shiftByRad(const Point& origin, const Point& edge, float shiftRad) {
    Point p = edge - origin;
    p.y *= -1; // flip coordinate system
    if (shiftRad >= 0) {
        p = { cosf(shiftRad) * p.x + sinf(shiftRad) * p.y,
              sinf(-shiftRad) * p.x + cosf(-shiftRad) * p.y };
    } else {
        shiftRad = abs(shiftRad);
        p = { cosf(shiftRad) * p.x - sinf(shiftRad) * p.y,
              sinf(shiftRad) * p.x + cosf(shiftRad) * p.y };
    }
    p.y *= -1; // restore coordinate system
    p += origin;
    return p;
}

// Point targetInChk(Point prevPos, Point pos, Point chkpoint, int chkDst) {
//     int farEdge = farEdgeOfChk(pos,chkpoint, chkDst);
//     // todo: if angle between prevPos and pos too big, correct the target
// }

// Point target(Point opponent, Point chk) {
//     // if the opponent directly before us, target them
//     int oppAngle = todo
//     if (abs(oppAngle) >= 4)
//         return chk;
// }

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
int main() {
    GameState s;
    s.self = vector<OwnPod>(2);
    s.opp  = vector<OppPod>(2);
    for (OwnPod& self : s.self) {
        self.speedRelToOpp   = vector<int>(s.opp.size());
        self.prevOppDist = vector<int>(s.opp.size());
        self.oppDist         = vector<int>(s.opp.size());
    }
    unsigned short rounds = 0,
        laps, checkAmount;
    cin >> laps >> checkAmount;
    for (uint i = 0; i < checkAmount; ++i) {
        Point aChk;
        cin >> aChk.x >> aChk.y;
        s.chks.push_back({aChk, (i == 0)? 0 : distance(aChk, s.chks[i-1].first)});
    }
    cerr << "Initialized" << endl;

    while (1) {
        rounds++;
        int unused;
        for (OwnPod& self : s.self) {
            cin >> self.pos.x >> self.pos.y >> unused >> unused >> self.nextCheckpointAngle >> self.chkId;
            self.chkDist = distance(self.pos, s.chks[self.chkId].first);
        }
        for (OppPod& opp : s.opp)
            cin >> opp.pos.x >> opp.pos.y >> unused >> unused >> unused >> unused;

        if (s.firstRun) { // first round initialization
            // rationale: as if we always existed in that point
            s.firstRun      = false;
            for (OppPod& opp : s.opp)
                opp.prevPos = opp.pos;
            for (OwnPod& self : s.self) {
                self.pos = self.pos;
                self.prevChkDist = self.chkDist;
                for (uint i = 0; i < s.opp.size(); ++i) {
                    self.oppDist[i] = distance(s.opp[i].pos, self.pos);
                    self.prevOppDist[i] = self.oppDist[i];
                }
            }
        }

        // every start round initialization
        for (OwnPod& self : s.self) {
            for (uint i=0; i < s.opp.size(); ++i) {
                self.oppDist[i] = distance(s.opp[i].pos, self.pos);
                self.speedRelToOpp[i] = abs(self.prevOppDist[i]) - abs(self.oppDist[i]);
                self.speedRelToOpp[i] = distance(s.opp[i].pos, self.pos);
            }
        }

        for (OwnPod& self : s.self) {
            Maybe<OppPod> opp = canShieldOpponent(s, self);
            if (opp.Just && rounds >= 5) {
                self.target = opp.val.pos;
                self.speed = " SHIELD";
                self.currAcc = 0;
                self.attacking = true;
            } else
                opp = canHitOpponent(s, self);
            if (opp.Just && rounds >= 5) {
                self.target = opp.val.pos;
                self.speed = " 100";
                self.currAcc = 100;
                self.attacking = true;
            } else {
                int inertia = inertiaAngle(s.chks[self.chkId].first, self,
                                           s.chks[self.chkId].first);
                assert (abs(inertia) <= 90);
                self.target = shiftByRad(self.prevPos, s.chks[self.chkId].first, degToRad(inertia));
                self.currAcc = bisectAccel(self.chkDist,
                                           self.nextCheckpointAngle+inertia,
                                           self.speedEstim // poor man's speed, it doesn't count inertia
                                        );
                self.speed = " " + to_string(self.currAcc);
                self.attacking = false;
            }
        }

        for (OwnPod& self : s.self) {
            if (self.hasBoost && (self.nextCheckpointAngle >= -1
                                  && self.nextCheckpointAngle <= 1
                                  && !self.attacking)) {
                pair<Point,int> farthest = s.chks[0];
                for (uint i = 1; i < s.chks.size(); ++i)
                    if (farthest.second < s.chks[i].second)
                        farthest = s.chks[i];
                if (circlesIntersect(farthest.first, s.chks[self.chkId].first, chkPointRadius)) {
                    self.speed = " BOOST";
                    self.hasBoost = false;
                }
            }
        }

        for (OppPod& opp : s.opp)
            opp.prevPos = opp.pos;
        for (OwnPod& self : s.self) {
            cout << self.target.x << " "
                 << self.target.y << self.speed << endl;

            self.prevChkDist = self.chkDist;
            self.prevPos = self.pos;
            for (uint i=0; i < s.opp.size(); ++i)
                self.prevOppDist[i] = self.oppDist[i];
        }
    }
}

// todo: 2. find a way to calculate left and right edges of spheres to β) to target
// an opponent withing the check, and γ) to calculate left/right edges of an opponent
// with regard to its speed to figure if we can hit it.
