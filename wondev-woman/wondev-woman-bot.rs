use std::ops::Add;
use std::io;

// situation rating:
// +1: move up higher
// -n: descend n levels.
// +∞: opp. can't move.
// -∞: my unit can't move.
// Rating of the opponent's situation is the inverse of above. I don't rate "building" because it
// looks self-rated by virtue of improving/worsening the next move.
// To simplify instead of infinity use very big rating. Say, 100 should be ok.
// Obviously, if the best move is negative, the game is screwed.

macro_rules! print_err {
    ($($arg:tt)*) => (
        {
            use std::io::Write;
            writeln!(&mut ::std::io::stderr(), $($arg)*).ok();
        }
    )
}

macro_rules! parse_input {
    ($x:expr, $t:ident) => ($x.trim().parse::<$t>().unwrap())
}

fn x_to_direction(x: i8) -> String {
    match x {
        a if a > 0 => String::from("E"),
        a if a < 0 => String::from("W"),
        _ => String::from("")
    }
}

fn y_to_direction(y: i8) -> String {
    match y {
        a if a > 0 => String::from("N"),
        a if a < 0 => String::from("S"),
        _ => String::from("")
    }
}

fn mv_to_enemy(opp_x: i8, opp_y: i8, me_x: i8, me_y: i8, ) {
    print_err!("dist y = {}, dist x = {}", opp_y - me_y, opp_x - me_x);
    let s = y_to_direction(me_y - opp_y) + x_to_direction(opp_x - me_x).as_str();
    println!("MOVE&BUILD 0 {} S", s);
}

enum Move {
    N, NE, E, SE, S, SW, W, NW
}

struct Point {x: i8, y: i8}
struct Distance {x: i8, y: i8}

struct Unit { loc: Point
            , ally: bool }

static STEPS: [Distance; 8] = [Distance{x:1, y:-1}, Distance{x:1, y:0}, Distance{x:1, y:1}, Distance{x:0, y:1},
                              Distance{x:-1, y:1}, Distance{x:-1, y:0}, Distance{x:-1, y:-1}, Distance{x:0, y:-1}];

impl Add for Point {
    type Output = Point;

    fn add(self, other: Point) -> Point {
        Point{x: self.x + other.x, y: self.y + other.y}
    }
}

impl Add<Distance> for Point {
    type Output = Point;

    fn add(self, other: Distance) -> Point {
        Point{x: self.x + other.x, y: self.y + other.y}
    }
}

// move'n'build :: Int -> Matrix Int -> [(Int, Int)] -> [(Int, Int)] -> String
// move'n'build nply map allies opps =
//     -- minimax: evaluate a move in assumption the opponent gonna make the best one

// bestPly :: Int -> Matrix Int -> (Int, Int) -> (Int, Move)
// bestPly d m start =

// nth build: return best build (negate if the opponent)
// nth move: return best move + best build (negate if the opponent)
// (n-1)th build: return best build + nth best move (negate if the opponent)
// (n-1)th move: return best move + best build (negate if the opponent)
// …
// 1st build: return best build + (1+1) best move
// 1st move: return best move + best build

// bestBuild :: Int -> Matrix Int -> Int -> [Point] -> (Int, (Int, Int))
// bestBuild nply map iunit units = foldr (\acc,p -> acc `best` (tick p)) (0, (0,0)) cells
//     where
//       rate = rateBuild pos iunit
//       tick :: (Int, Int) -> (Int, (Int, Int))
//       tick pos = if nply > 0 and rate > -100 then (mbNeg rate) + (valMove $ bestMove (nply - 1) map (nxtUnit iunit) units)
//                  else mbNeg rate
//       best :: (Int, (Int, Int)) -> (Int, (Int, Int)) -> (Int, (Int, Int))
//       best acc@(v, _) new@(v', _) = if v > v' then acc else new
//       mbNeg :: Int -> Int
//       mbNeg val curr = undefined -- negate when curr. point is an opponent
//       nxtUnit i = undefined

// bestMoveNBuild :: Int -> Matrix Int -> Int -> [Point] -> (Int, (Int, Int), (Int, Int))
// bestMoveNBuild nply map iunit units = foldr (\acc,p -> acc `best` (tick p)) (0, (0,0)) cells
//     where
//       rate pos = rateMove pos iunit
//       tick :: (Int, Int) -> (Int, (Int, Int), (Int, Int))
//       tick pos = if nply > 0 and rate pos > -100 then (mbNeg $ rate pos) + bestBuild nply map iunit units
//                  else mbNeg $ rate pos
//       best :: (Int, a, a) -> (Int, a, a) -> (Int, a, a)
//       best l@(v, _, _) r@(v', _, _) = if v > v' then l else r
//       mbNeg :: Int -> Int
//       mbNeg val curr = undefined -- negate when curr. point is an opponent

type value = isize;
static BADMOVE: value = -100;

fn rateMove(old: Point, new: Point, map: &Vec<Vec<value>>) -> value {
    let oldh = map[old.x as usize][old.y as usize];
    let newh = map[new.x as usize][new.y as usize];
    if newh >= oldh + 2 { BADMOVE } // -∞: can't move
    else if newh < oldh { newh - oldh } // -n: descend n levels.
    else { newh - oldh } // move up 1 or 0 levels
}

fn rateBuild( /* old: Point, new: Point, map: &[&[i8]] */ ) -> value {
    // In general to rate build is only possible by rating the next move, i.e. this func is
    // useless. But it could be needed to skip inaccessible places. Let's make it a noop for now.
    0
}

fn negIfOpp(v: value, iunit: usize, units: &[Unit]) -> value {
    if !units[iunit].ally {
        -v
    } else {
        v
    }
}

// todo: you got stuck in hand-writing a fold to work with static lifetimes. So many shitty details needs to be known
fn bestBuild(nply: u8, map: &Vec<Vec<value>>, iunit: usize, units: &[Unit]) -> (value, &Distance) {
    assert!(nply > 0);
    let mbNeg = |r| negIfOpp(r, iunit, units);
    let tick = |step: &Distance| -> (value, &Distance) {
        let rate = rateBuild();
        if rate > BADMOVE {
            let (nxt_ply_val, _, _) = bestMoveNBuild(nply-1, map, iunit, units);
            (mbNeg(rate) + nxt_ply_val, step)
        } else {
            (rate, step)
        }
    };
    let best = |l: (value, Distance), r: (value, Distance)| -> (value, Distance)
        { if l.0 > r.0 {l} else {r} };
    STEPS.iter().fold((0, Distance{x: 0,y :0}), |acc,p| best(acc, tick(p)))
}

/// nply is taken into account only in bestBuild()
fn bestMoveNBuild(nply: u8, map: &Vec<Vec<value>>, iunit: usize, units: &[Unit]) -> (value, Distance, Distance) {
    assert!(nply > 0);
    let mbNeg = |r| negIfOpp(r, iunit, units);
    let tick = |step: Distance| -> (value, Distance, Distance) {
        let rate = rateMove(units[iunit].loc, units[iunit].loc + step, map);
        if rate > BADMOVE {
            let (rate_build, dist_build) = bestBuild(nply, map, iunit, units);
            (mbNeg(rate) + rate_build, step, dist_build)
        } else {
            (mbNeg(rate), step, Distance{x:0,y:0})
        }
    };
    let best = |l: (value, Distance, Distance), r: (value, Distance, Distance)| -> (value, Distance, Distance)
        { if l.0 > r.0 {l} else {r} };
    STEPS.iter().fold((0, Distance{x:0,y:0},Distance{x:0,y:0}), |acc,&p| best(acc, tick(p)))
}

/**
 * Auto-generated code below aims at helping you parse
 * the standard input according to the problem statement.
 **/
fn main() {
    let mut input_line = String::new();
    io::stdin().read_line(&mut input_line).unwrap();
    let size = parse_input!(input_line, i32);
    let mut input_line = String::new();
    io::stdin().read_line(&mut input_line).unwrap();
    let units_per_player = parse_input!(input_line, i32);

    // game loop
    loop {
        for i in 0..size as usize {
            let mut input_line = String::new();
            io::stdin().read_line(&mut input_line).unwrap();
            let row = input_line.trim().to_string();
        }
        // for i in 0..units_per_player as usize {
            let mut input_line = String::new();
            io::stdin().read_line(&mut input_line).unwrap();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let me_x = parse_input!(inputs[0], i8);
            let me_y = parse_input!(inputs[1], i8);
        // }
        // for i in 0..units_per_player as usize {
            let mut input_line = String::new();
            io::stdin().read_line(&mut input_line).unwrap();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let opp_x = parse_input!(inputs[0], i8);
            let opp_y = parse_input!(inputs[1], i8);
        // }
        let mut input_line = String::new();
        io::stdin().read_line(&mut input_line).unwrap();
        let legal_actions = parse_input!(input_line, i32);
        for i in 0..legal_actions as usize {
            let mut input_line = String::new();
            io::stdin().read_line(&mut input_line).unwrap();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let atype = inputs[0].trim().to_string();
            let index = parse_input!(inputs[1], i32);
            let dir_1 = inputs[2].trim().to_string();
            let dir_2 = inputs[3].trim().to_string();
        }

        // notes: 1. battlefield starts at X=0, Y=0 which is the top-left cell, and uses absolute numbers
        // 2. can't move to an occupied cell

        // Write an action using println!("message...");
        // To debug: print_err!("Debug message...");

        // move into if the target around, otherwise:
        // evaluate possible strategies
        // 1. for i in 2..3, look at straight distance of i-1 for cells of i ≥ 3
        // 2. take the biggest
        mv_to_enemy(opp_x, opp_y, me_x, me_y);
    }
}
