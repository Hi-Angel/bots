#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <cmath>
#include <array>

using namespace std;
using uint = unsigned;

const unsigned short chkPointRadius = 600,
    carRadius = 400,
    numPlayers = 2;

template<typename T>
struct NumWrapper {
    T val;
    constexpr NumWrapper() {}
    constexpr NumWrapper(T v) : val(v) {}
    constexpr NumWrapper(const NumWrapper& w) : val(w.val) {}
    friend ostream& operator<<(ostream& os, const NumWrapper& w) {
        os << w.val;
        return os;
    }
    friend istream& operator>>(istream& is, NumWrapper& w) {
        is >> w.val;
        return is;
    }
    NumWrapper operator-(const T rhs) const {
        return {val - rhs};
    }
    NumWrapper operator-(const NumWrapper rhs) const {
        return {val - rhs.val};
    }
    NumWrapper operator-() const {
        return {-val};
    }
    NumWrapper operator+(const T rhs) const {
        return {val + rhs};
    }
    NumWrapper operator+(const NumWrapper rhs) const {
        return {val + rhs.val};
    }
    bool operator==(const NumWrapper rhs) const {
        return {val == rhs.val};
    }
    bool operator==(const T rhs) const {
        return {val == rhs};
    }
};
using Degree = NumWrapper<int>;
using Radian = NumWrapper<float>;

struct Point {
    int x, y;
    bool operator==(const Point& p) const {
        return p.x == x && p.y == y;
    }
    bool operator!=(const Point& p) const {
        return !(p == *this);
    }
    Point operator+(const Point& rhs) const {
        return {x + rhs.x, y + rhs.y};
    }
    Point operator-(const Point& rhs) const {
        return {x - rhs.x, y - rhs.y};
    }
    void operator+=(const Point& rhs) {
        x += rhs.x;
        y += rhs.y;
    }
    friend ostream& operator<<(ostream& os, const Point& p) {
        os << "x = " << p.x << ", y = " << p.y;
        return os;
    }
};

template <typename T>
struct Maybe {
    bool Just;
    T val;
};

struct OppPod {
    Point pos, prevPos, oughtPos;
    Degree globAngle,
        moveAngle;
    int speedEstim, chkId;

    friend ostream& operator<<(ostream& os, const OppPod& p) {
        os << "pos{" << p.pos << "}, prevPos{" << p.prevPos << "}";
        return os;
    }
};

struct OwnPod {
    Point pos, prevPos,
        target; //target to drive to
    int chkDist, currAcc,
        prevChkDist,
        chkId,
        speedEstim; // it's a substract of prev and curr distance, don't trust too much
    Degree globAngle, // 0..360° with Y flipped, i.e. 90° is facing down
        chkAngle; // NOTE: it's -180..180, negative is left angle
    array<int, numPlayers> speedRelToOpp, // speed relative to opponent by index
        prevOppDist,
        oppDist;
    string speed;
    bool attacking, hasBoost = true;

    friend ostream& operator<<(ostream& os, const OwnPod& p) {
        os << "pos{" << p.pos << "}, prevPos{" << p.prevPos << "}, target{" << p.target << "}" << endl;
        os << "chkDist = " << p.chkDist << ", currAcc = " << p.currAcc << ", prevChkDist = " << p.prevChkDist << endl
           << ", chkAngle = " << p.chkAngle.val << ", chkId = " << p.chkId << ", speedEstim = " << p.speedEstim << endl;
        for (uint i=0; i < p.speedRelToOpp.size(); ++i)
            os << "speedRelToOpp = " << p.speedRelToOpp[i] << endl;
        for (uint i=0; i < p.prevOppDist.size(); ++i)
            os << "prevOppDist = " << p.prevOppDist[i] << endl;
        for (uint i=0; i < p.oppDist.size(); ++i)
            os << "oppDist = " << p.oppDist[i] << endl;
        return os;
    }
};

struct GameState {
    vector<pair<Point,int>> chks;
    bool firstRun = true;
    array<OppPod, numPlayers> opp;
    array<OwnPod, numPlayers> self;

    friend ostream& operator<<(ostream& os, const GameState& s) {
        for (uint i=0; i < s.chks.size(); ++i)
            os << "Check#" << i << ": Point{" << s.chks[i].first << "} dist = " << s.chks[i].second << endl;
        // first run whatever
        for (uint i=0; i < s.opp.size(); ++i)
            os << "Opp#" << i << ": " << s.opp[i] << endl;
        for (uint i=0; i < s.self.size(); ++i)
            os << "Own#" << i << ": " << s.self[i] << endl;
        return os;
    }
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

constexpr Radian degToRad(Degree degree) { return {degree.val*(M_PI/180)}; }
constexpr Radian degToRad(int degree) { return {degree*(M_PI/180)}; }
constexpr Degree radToDeg(Radian rad) { return {rad.val*(180/M_PI)}; }
constexpr Degree radToDeg(float rad) { return {rad*(180/M_PI)}; }

// every big letter is an angle opposite to its small letter counterpart, meaning a
// side. a and b are adjacent sides, order of a and b doesn't matter
Degree angleC(float a, float b, float c) {
    if (!(a && b && c)) // there's no angle, and calculations don't handle it
        return {0};
    float cos_c = (a*a + b*b - c*c) / (float)(2*a*b);
    if (cos_c > 1) // calculations sometimes fail, idk why.
        return {0};
    else
        return radToDeg(acosf(cos_c));
}

float distance(const Point& a, const Point& b) {
    // distance is length of hypotenuse in right triangle between the points
    int adj = a.x - b.x,
        opp = a.y - b.y;
    float ret = sqrtf(adj*adj + opp*opp);
    assert(ret >= 0);
    return ret;
}

// calculates whether the point in question is left or right to line.
// note: if it's on the line, value is undefined, for my purposes that's okay
bool isLeftToLine(const Point& start, const Point& end, const Point& q) {
    return ((q.x - start.x)*(end.y - start.y) - (q.y - start.y) * (end.x - start.x))
        > 0;
}

// moves a point by len, given its *global* angle, using the game coordinate system
// (the one with flipped Y)
Point movePoint(int len, const Radian& globAngle) {
    Point ret = {len * cosf(globAngle.val), len * sinf(globAngle.val)};
    return ret;
}

bool isOppBehind(const OwnPod& self, const OppPod& opp) {
    if (isLeftToLine(self.prevPos, self.pos, opp.pos)) {
        Point left = movePoint(carRadius, degToRad((self.globAngle.val - 90)%360));
        return isLeftToLine(left, self.pos, opp.pos); // quadrants Ⅱ or Ⅲ
    } else {
        Point right = movePoint(carRadius, degToRad((self.globAngle.val + 90)%360));
        return !isLeftToLine(right, self.pos, opp.pos); // quadrants Ⅳ or Ⅰ
    }
}

bool isDotInCircle(const Point& origin, int rad, const Point& dot) {
    int x_diff = dot.x - origin.x,
        y_diff = dot.y - origin.y;
    return x_diff*x_diff + y_diff*y_diff <= rad * rad;
}


constexpr bool isCirclesIntersect(uint rad1, uint rad2, uint distance) {
    return rad1 + rad2 >= distance;
}

template<typename Pod1, typename Pod2>
bool doCarsCollide(Pod1 pod1, Pod2 pod2) {
    if (pod2.speedEstim + pod1.speedEstim <= 400) {
    Point pos1 = pod1.pos + movePoint(pod1.speedEstim, degToRad(pod1.globAngle)),
        pos2 = pod2.pos + movePoint(pod2.speedEstim, degToRad(pod2.globAngle));
        return isCirclesIntersect(carRadius, carRadius, distance(pos1, pos2));
    } else {
        // Every player turn car makes many smaller steps per speed, and there's a
        // possibility cars miss each another. The constant I use is particular.
        const int stepsPerTurn = 10,
            speedEstim1 = pod1.speedEstim / stepsPerTurn,
            speedEstim2 = pod2.speedEstim / stepsPerTurn;
        for (uint i=0; i<stepsPerTurn; ++i) {
            const Point pos1 = pod1.pos + movePoint(speedEstim1*i, degToRad(pod1.globAngle)),
                pos2 = pod2.pos + movePoint(speedEstim2*i, degToRad(pod2.globAngle));
            if (isCirclesIntersect(carRadius, carRadius, distance(pos1, pos2)))
                return true;
        }
    }
    return false;
}

inline bool sameFocusedDirection(const Degree& selfAngle, const Degree& oppAngle) {
    // the angles are 0..360°
    return abs((selfAngle - oppAngle).val) <= 29;
}

const Maybe<OppPod> canHitOpponent(const GameState& s, const OwnPod& self) {
    for (uint i=0; i < s.opp.size(); ++i) {
        const OppPod& opp = s.opp[i];
        if (isDotInCircle(s.chks[self.chkId].first, chkPointRadius, opp.pos)
            && self.oppDist[i] <= carRadius*4)
            return {true, opp};
        if (doCarsCollide(opp, self)) {
            if (!sameFocusedDirection(self.globAngle, opp.globAngle))
                return {true, opp};
            else if (isOppBehind(self, opp))
                return {true, opp};
        }
    }
    return {false, {}};
}

const Maybe<OppPod> canShieldOpponent(const GameState& s, const OwnPod& self,
                                      const int mbopp) {
    if (mbopp != -1) {
        const OppPod& opp = s.opp[mbopp];
        if (doCarsCollide(opp, self) && self.speedRelToOpp[mbopp] >= 140) {
            if (!sameFocusedDirection(self.globAngle, opp.globAngle))
                return {true, opp};
            else if (isOppBehind(self, opp))
                return {true, opp};
        }
    } else {
        for (uint i=0; i < s.opp.size(); ++i) {
            const OppPod& opp = s.opp[i];

            if (doCarsCollide(opp, self) && self.speedRelToOpp[i] >= 140) {
                if (!sameFocusedDirection(self.globAngle, opp.globAngle)) {
                    return {true, opp};
                } else if (isOppBehind(self, opp)) {
                    return {true, opp};
                }
            }
        }
    }
    return {false, {}};
}

float oppositeLen(float aLen, float bLen, const Radian& abAngle) {
    float cLenSquared = (aLen * aLen) + (bLen * bLen)
        - 2*aLen*bLen*cosf(abAngle.val);
    return sqrtf(cLenSquared);
}

// returns the checkpoint angle between prev and curr positions. Negative if
// counterclockwise, positive otherwise.
Degree inertiaAngle(const Point& chkPoint, const OwnPod& self, const Point& target) {
    Degree inertia = angleC(distance(target, self.pos), // s.speed,
                            distance(target, self.prevPos),
                            distance(self.prevPos, self.pos));
    return (isLeftToLine(chkPoint, self.prevPos, self.pos))? -inertia : inertia;
}

// bisection does not produce optimal speed. Consider the len interval (10…7…10),
// where we strive for lowest possible len. Which half shall bisection choose, left
// or right? To get the optimum you'd need to use optimization techniques instead,
// but I don't know them offhand, and for now I consider bisection good enough. I may
// possibly change my mind though.
int bisectAccel(const int carDist, const Degree& carAngle, int speed, bool chk) {
    assert(carAngle.val <= 360);
    // function strives to minimize length to chk by picking appropriate accel
    int bottom = 0,
        // it's hard to describe situation, but in immediate closeness to checkpoint
        // a car might go to an orbit. It's long to solve correctly, nor I see
        // benefits of doing so.
        // added later: the workaround for corner case makes more harm in general, I
        // simplified it.
        top = (chk && carDist <= chkPointRadius+100 && speed >= 100)? 10 : 100;
    float bottomLen = oppositeLen(carDist, bottom+speed, degToRad(carAngle)),
        topLen = oppositeLen(carDist, top+speed, degToRad(carAngle));
    for (;;) {
        int nextAdjLen = bottom + (top - bottom) / 2;
        float oppLen = oppositeLen(carDist, nextAdjLen+speed, degToRad(carAngle));
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
Point shiftByRad(const Point& origin, const Point& edge, Radian shiftRad) {
    Point p = edge - origin;
    p.y *= -1; // flip coordinate system
    if (shiftRad.val >= 0) {
        p = { cosf(shiftRad.val) * p.x + sinf(shiftRad.val) * p.y,
              sinf(-shiftRad.val) * p.x + cosf(-shiftRad.val) * p.y };
    } else {
        shiftRad.val = abs(shiftRad.val);
        p = { cosf(shiftRad.val) * p.x - sinf(shiftRad.val) * p.y,
              sinf(shiftRad.val) * p.x + cosf(shiftRad.val) * p.y };
    }
    p.y *= -1; // restore coordinate system
    p += origin;
    return p;
}
void shieldOpp(OwnPod& self, const OppPod& opp) {
    self.target = opp.pos;
    self.speed = " SHIELD";
    self.currAcc = 0;
    self.attacking = true;
}

void targetOpp(OwnPod& self, const OppPod& opp) {
    self.target = opp.oughtPos;
    self.speed = " 100";
    self.currAcc = 100;
    self.attacking = true;
}

bool intersectsCircle(const Point& circle, int radius,
                      const Degree& moveAngle, const Point& movee) {
    int dist = distance(circle, movee);
    const Point p = movee + movePoint(dist, degToRad(moveAngle));
    int projectionDist = distance(circle, p);
    return projectionDist <= radius;
}

Degree chkAngle(const Point& chk, const OwnPod& self) {
    // todo: this doesn't seem to return more than 90°. Also nowatimes the purpose is
    // dubious — I could as well use movement direction for the bisectSpeed()
    Point faceEdge   = self.pos + movePoint(carRadius, degToRad(self.globAngle));
    float hyp        = distance(faceEdge, chk);
    return isLeftToLine(self.pos, chk, faceEdge)
        ? -angleC(self.chkDist, carRadius, hyp)
        : angleC(self.chkDist, carRadius, hyp);
}

void targetChk(const GameState& s, OwnPod& self, int chkId) {
    int dist;
    Degree angle;
    if (chkId == -1) {
        chkId = self.chkId;
        dist = self.chkDist;
        angle = self.chkAngle;
    } else {
        dist = distance(self.pos, s.chks[chkId].first);
        angle = chkAngle(s.chks[chkId].first, self);
    }
    Degree inertia = inertiaAngle(s.chks[chkId].first, self,
                                  s.chks[chkId].first);
    self.target = shiftByRad(self.prevPos, s.chks[chkId].first, degToRad(inertia));
    self.currAcc = bisectAccel(dist,
                               angle,
                               self.speedEstim, // poor man's speed, it doesn't count inertia
                               true
                               );
    self.speed = " " + to_string(self.currAcc);
    self.attacking = false;
}

Degree vectorAngle(const Point& vec) {
    if (vec.x == 0) // special cases
        return (vec.y > 0)? Degree(90)
            : (vec.y == 0)? Degree(0)
            : Degree(270);
    else if (vec.y == 0) // special cases
        return (vec.x >= 0)? Degree(0)
            : Degree(180);
    Degree ret = radToDeg(atanf((float)vec.y/vec.x));
    if (vec.x < 0 && vec.y < 0) // quadrant Ⅲ
        ret.val = 180 + ret.val;
    else if (vec.x < 0) // quadrant Ⅱ
        ret.val = 180 + ret.val; // it actually substracts
    else if (vec.y < 0) // quadrant Ⅳ
        ret.val = 270 + (90 + ret.val); // it actually substracts
    return ret;
}

int findDefendee() {
    return 1; // todo
}

void defend(const GameState s, OwnPod& self, int defendee) {
    for (uint i=0; i < s.opp.size(); ++i) {
        const OppPod& opp = s.opp[i];
        // if (intersectsCircle(s.chks[defendee].first, chkPointRadius,
        //                      opp.moveAngle, opp.pos)) {
        Maybe<OppPod> maybe = canShieldOpponent(s, self, i);
        if (opp.chkId == defendee) {
            if (maybe.Just)
                shieldOpp(self,opp);
            else
                targetOpp(self, opp);
            return;
        }
    }
    targetChk(s, self, defendee);
}

#ifndef TESTS

int main() {
    GameState s;
    unsigned short rounds = 0,
        laps, checkAmount,
        defender = 2; // disabled
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
            cin >> self.pos.x >> self.pos.y >> unused >> unused
                >> self.globAngle >> self.chkId;
            const Point& chk = s.chks[self.chkId].first;
            self.chkDist  = distance(self.pos, chk);
            self.chkAngle = chkAngle(chk, self);
        }
        for (OppPod& opp : s.opp)
            cin >> opp.pos.x >> opp.pos.y >> unused >> unused
                >> opp.globAngle >> opp.chkId;


        if (s.firstRun) { // first round initialization
            // rationale: as if we always existed in that point
            s.firstRun      = false;
            for (OppPod& opp : s.opp)
                opp.prevPos = opp.pos;
            for (OwnPod& self : s.self) {
                self.prevPos = self.pos;
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
                self.speedRelToOpp[i] = self.prevOppDist[i] - self.oppDist[i];
                self.speedEstim = distance(self.prevPos, self.pos);
            }
        }
        for (OppPod& opp : s.opp) {
            opp.speedEstim  = distance(opp.prevPos, opp.pos);
            opp.moveAngle = vectorAngle(opp.pos - opp.prevPos);
            opp.oughtPos = opp.pos + movePoint(opp.speedEstim, degToRad(opp.moveAngle));
        }

        // the action
        for (uint i = 0; i < s.self.size(); ++i) {
            OwnPod& self = s.self[i];
            if (i == defender) {
                int defendee = findDefendee();
                defend(s, self, defendee);
                continue;
            }
            Maybe<OppPod> opp = canShieldOpponent(s, self, -1);
            if (opp.Just && rounds >= 5) {
                shieldOpp(self, opp.val);
                continue;
            }

            opp = canHitOpponent(s, self);
            if (opp.Just && rounds >= 5)
                targetOpp(self, opp.val);
            else
                targetChk(s, self, -1);
        }

        for (OwnPod& self : s.self) {
            if (self.hasBoost && (self.chkAngle.val >= -1
                                  && self.chkAngle.val <= 1
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

#endif
