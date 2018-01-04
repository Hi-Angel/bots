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
    friend ostream& operator<<(ostream& os, const Point& p) {
        os << "x: " << p.x << " y: " << p.y;
        return os;
    }
};

struct GameState {
    vector<pair<Point,int>> chks;
    bool collected = false, hasBoost = true, firstRun = true;
    Point prevRecordedPoint = {0, 0}, prevPos, currPos,
        chkPoint,
        opponent,
        target, //target to drive to
        oughtPos;
    int nextCheckpointDist, // distance to the next checkpoint
        nextCheckpointAngle, // NOTE: it's -180..180, negative is the left
        prevDistance,
        prevAngle = 0,
        currAcc = 0,
        speed; // it's a substract of prev and curr distance, don't trust too much
};

struct MaybePoint {
    bool valid;
    Point p;
};

struct MaybeStats {
    bool validStat, validPoint;
    int prevDist, prevSpeed;
    Point p;
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

// when in doubt: far edge is a better target than close one because car won't slow
// down unless in the radius. Arguably it's causing problems upon movement under
// inertia, but it's an irrlevant problem, and I don't think putting out inertia by
// moving slower (i.e. to close edge) beats cases when it's okay to move faster.
Point farEdgeOfChk(const Point& car, const Point& chkpoint, int chkDistance) {
    int hyp = chkDistance,
        opposite = chkpoint.x - car.x,
        adjacent = chkpoint.y - car.y;
    float sin = opposite / (float)hyp,
        cos = adjacent / (float)hyp;
    assert(hyp >= chkPointRadius);
    int newHyp = hyp + chkPointRadius,
        newOpp = newHyp * sin,
        newAdj = newHyp * cos,
        edgeChkX = car.x + newOpp,
        edgeChkY = car.y + newAdj;
    return {edgeChkX, edgeChkY};
}

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
bool isLeftToLine(Point start, Point end, Point q) {
    return ((q.x - start.x)*(end.y - start.y) - (q.y - start.y) * (end.x - start.x))
        > 0;
}

bool canHitOpponent(const GameState& s) {
    // use separate variables to ease future refactoring for multiple cars
    const Point& opp = s.opponent, chk = s.chkPoint, self = s.currPos;
    const int chkAngle = s.nextCheckpointAngle, // -180..180
        chkDist = s.nextCheckpointDist;

    const int oppDist = distance(self, opp),
        oppChkDist = distance(opp, chk),
        selfToChkOppAngle = isLeftToLine(self, chk, opp)
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
    return (((abs(oppAngle) >= 50 && abs(oppAngle) <= 130)
             // now go for an experiment: if an opponent right behind me so close,
             // he probably drives my direction
             || (abs(oppAngle) >= 190))
            && oppDist - carRadius*2 <= 200);
}

float oppositeLen(float aLen, float bLen, int abAngle) {
    float cLenSquared = (aLen * aLen) + (bLen * bLen)
        - 2*aLen*bLen*cosf(degToRad(abAngle));
    return sqrtf(cLenSquared);
}

// angle between previous pos and the actual pos
int inertiaAngle(GameState s) {
    // cerr << " distance(s.oughtPos, s.prevPos): " << distance(s.prevPos, s.oughtPos)
    //      << " distance(s.prevPos, s.currPos): " << distance(s.prevPos, s.currPos)
    //      << " distance(s.oughtPos, s.currPos): " << distance(s.oughtPos, s.currPos) << endl;
    return angleC(distance(s.prevPos, s.oughtPos), // s.speed,
                  distance(s.prevPos, s.currPos),
                  distance(s.oughtPos, s.currPos));
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

bool circlesIntersect(Point a, Point b, int radius) {
    return abs(a.x - b.x) <= radius && abs(a.y - b.y) <= radius;
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
    unsigned rounds = 0;
    while (1) {
        rounds++;
        cin >> s.currPos.x >> s.currPos.y >> s.chkPoint.x >> s.chkPoint.y >> s.nextCheckpointDist >> s.nextCheckpointAngle;
        cin >> s.opponent.x >> s.opponent.y;

        if (s.firstRun) { // rationale: as if we always existed in that point
            s.firstRun = false;
            s.prevPos = s.currPos;
            s.prevDistance = s.nextCheckpointDist;
            s.prevAngle = 0;
        }
        s.speed = distance(s.prevPos, s.currPos);

        string speed;
        // cerr << "speed " << s.speed << endl;
        if (canHitOpponent(s) && rounds >= 3) {
            s.target = s.opponent;
            // if (s.speed >= 200) {
                speed = " SHIELD";
                s.currAcc = 0;
            // } else {
            //     speed = " 100";
            //     s.currAcc = 100;
            // }
        } else {
            int targetAngle = s.nextCheckpointAngle;
            s.target = farEdgeOfChk(s.currPos, s.chkPoint, s.nextCheckpointDist);
            s.currAcc = bisectAccel(s.nextCheckpointDist+chkPointRadius,
                                    targetAngle,
                                    s.speed // poor man's speed, it doesn't count inertia
                                    );
            speed = " " + to_string(s.currAcc);
        }

        if (!s.collected) {
            if (s.prevRecordedPoint != s.chkPoint) {
                for (uint i = 0; i < s.chks.size(); ++i)
                    if (s.chks[i].first == s.chkPoint) {
                        s.prevRecordedPoint = s.chkPoint;
                        s.collected = true;
                        break;
                    }
                if (!s.collected) {
                    s.chks.push_back({s.chkPoint, s.nextCheckpointDist});
                    s.prevRecordedPoint = s.chkPoint;
                }
            }
        } else if (s.hasBoost && (s.nextCheckpointAngle >= -1
                                  && s.nextCheckpointAngle <= 1
                                  && s.currAcc == 100)) {
            assert(s.chks.size() > 1);
            pair<Point,int> farthest = s.chks[0];
            for (uint i = 1; i < s.chks.size(); ++i)
                if (farthest.second < s.chks[i].second)
                    farthest = s.chks[i];
            if (circlesIntersect(farthest.first, s.chkPoint, chkPointRadius)) {
                speed = " BOOST";
                s.hasBoost = false;
            }
        }

        cout << s.target.x << " "
             << s.target.y << speed << endl;

        s.prevDistance = s.nextCheckpointDist;
        s.prevPos = s.currPos;
        s.prevAngle = s.nextCheckpointAngle;
    }
}

// todo: 1. distance seems to be 400 per 100; also inertia looks undocumented.
// 2. move whole state into a struct in order to pretty print whole info for debugging.
