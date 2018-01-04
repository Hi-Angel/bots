#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <cmath>
#include <stdlib.h>

using namespace std;
using uint = unsigned;

const unsigned short chkPointRadius = 600;

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

float degToRad(int degree) { return degree*(M_PI/180); }

float oppositeLen(float aLen, float bLen, int abAngle) {
    float cLenSquared = (aLen * aLen) + (bLen * bLen)
        - 2*aLen*bLen*cosf(degToRad(abAngle));
    return sqrtf(cLenSquared);
}

// bisection does not produce optimal speed. Consider the len interval (10…7…10),
// where we strive for lowest possible len. Which half shall bisection choose, left
// or right? To get the optimum you'd need to use optimization techniques instead,
// but I don't know them offhand, and for now I consider bisection good enough. I may
// possibly change my mind though.
int bisectSpeed(int carDist, int carAngle, int speed) {
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


int distance(const Point& a, const Point& b) {
    // distance is length of hypotenuse in right triangle between the points
    int adj = a.x - b.x,
        opp = a.y - b.y;
    return sqrtf(adj*adj + opp*opp);
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
    vector<pair<Point,int>> chks;
    bool collected = false, hasBoost = true, firstRun = true;
    Point prevRecordedPoint = {0, 0}, prevPos;
    int prevDistance;
    while (1) {
        Point currPos, chkPoint;
        int nextCheckpointDist; // distance to the next checkpoint
        int nextCheckpointAngle; // angle between your pod orientation and the direction of the next checkpoint
        cin >> currPos.x >> currPos.y >> chkPoint.x >> chkPoint.y >> nextCheckpointDist >> nextCheckpointAngle;
        int opponentX;
        int opponentY;
        cin >> opponentX >> opponentY;

        if (firstRun) { // rationale: as if we always existed in that point
            prevPos = currPos;
            prevDistance = nextCheckpointDist;
            firstRun = false;
        }

        int speedI = bisectSpeed(nextCheckpointDist+chkPointRadius,
                                 nextCheckpointAngle,
                                 distance(prevPos, currPos) // poor man's speed, it doesn't count inertia
                          );
        string tmp = " " + to_string(speedI);
        const char* speed = tmp.c_str();

        if (!collected) {
            if (prevRecordedPoint != chkPoint) {
                for (uint i = 0; i < chks.size(); ++i)
                    if (chks[i].first == chkPoint) {
                        prevRecordedPoint = chkPoint;
                        collected = true;
                        break;
                    }
                if (!collected) {
                    chks.push_back({chkPoint, nextCheckpointDist});
                    prevRecordedPoint = chkPoint;
                }
            }
        } else if (hasBoost && (nextCheckpointAngle >= -1
                                && nextCheckpointAngle <= 1
                                && speedI == 100)) {
            assert(chks.size() > 1);
            pair<Point,int> farthest = chks[0];
            for (uint i = 1; i < chks.size(); ++i)
                if (farthest.second < chks[i].second)
                    farthest = chks[i];
            if (circlesIntersect(farthest.first, chkPoint, chkPointRadius)) {
                speed = " BOOST";
                hasBoost = false;
            }
        }

        const Point dst = farEdgeOfChk(currPos, chkPoint, nextCheckpointDist);
        cout << dst.x << " "
             << dst.y << speed << endl;

        prevDistance = nextCheckpointDist;
        prevPos = currPos;
    }
}

// todo: 1. distance seems to be 400 per 100; also inertia looks undocumented.
// 2. move whole state into a struct in order to pretty print whole info for debugging.
