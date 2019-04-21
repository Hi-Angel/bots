use std::io::{self, BufRead};

macro_rules! parse_input {
    ($x:expr, $t:ident) => ($x.trim().parse::<$t>().unwrap())
}

fn read_line() -> String {
    let stdin = io::stdin();
    let line = stdin.lock().lines().next().unwrap().unwrap();
    line
}

struct FreePaths {
    up: bool,
    down: bool,
    left: bool,
    right: bool,
}

impl FreePaths {
    fn new(tile_raw: &str) -> FreePaths {
        let tile = tile_raw.as_bytes();
        FreePaths { up:    tile[0] == b'1',
                    down:  tile[1] == b'1',
                    left:  tile[2] == b'1',
                    right: tile[3] == b'1' }
    }
}

struct PlayerState {
    n_quests: u32,
    x: u32,
    y: u32,
    tile: FreePaths,
    id: u32
}

struct ItemState {
    name: String,
    x: u32,
    y: u32,
    owner_id: u32, // id of a player who can collect the item
}

fn read_matrix() -> Vec<Vec<FreePaths>> {
    let mut ret = Vec::<Vec<FreePaths>>::new();
    for _ in 0..7 {
        let mut row = Vec::<FreePaths>::new();
        for tile_raw in read_line().split_whitespace() {
            assert_eq!(tile_raw.len(), 4);
            row.push(FreePaths::new(tile_raw));
        }
        ret.push(row);
    }
    ret
}

fn read_player_info(id: u32) -> PlayerState {
    let line = read_line();
    let mut player_str = line.split_whitespace();
    PlayerState { n_quests: parse_input!(player_str.next().unwrap(), u32),
                  x:  parse_input!(player_str.next().unwrap(), u32),
                  y:  parse_input!(player_str.next().unwrap(), u32),
                  tile:  FreePaths::new(player_str.next().unwrap()) }
}

fn main() {
    // game loop
    loop {
        let _turn_type = parse_input!(read_line(), i32);
        let _game_map = read_matrix();
        let _me =  read_player_info(0);
        let _opp = read_player_info(1);
        let num_items = parse_input!(read_line(), u32);
        for _ in 0..num_items as usize {
            let input_line = read_line();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let item_name = inputs[0].trim().to_string();
            let item_x = parse_input!(inputs[1], i32);
            let item_y = parse_input!(inputs[2], i32);
            let item_player_id = parse_input!(inputs[3], i32);
        }
        let input_line = read_line();
        let num_quests = parse_input!(input_line, i32); // the total number of revealed quests for both players
        for _ in 0..num_quests as usize {
            let input_line = read_line();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let quest_item_name = inputs[0].trim().to_string();
            let quest_player_id = parse_input!(inputs[1], i32);
        }

        println!("PUSH 3 RIGHT"); // PUSH <id> <direction> | MOVE <direction> | PASS
    }
}

// Write an action using println!("message...");
// To debug: eprintln!("Debug message...");
