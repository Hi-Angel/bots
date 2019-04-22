use std::io::{self, BufRead};
use std::collections::HashMap;

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

struct Point {
    x: u32,
    y: u32,
}

struct PlayerState {
    n_quests: u32,
    pos: Point,
    tile: FreePaths,
}

enum ItemLocation {
    AtMine,
    AtOpponents,
    At(Point)
}

struct ItemState {
    pos: ItemLocation,
    owner_id: u32, // id of a player who can collect the item
}

struct Quest {
    item: String,
    owner_id: u32
}

impl ItemState {
    fn new(maybe_x: i32, maybe_y: i32, owner_id: u32) -> ItemState {
        ItemState{ pos: match maybe_x { -2 => ItemLocation::AtOpponents,
                                         -1 => ItemLocation::AtMine,
                                         _ => ItemLocation::At (Point { x: maybe_x as u32,
                                                                        y: maybe_y as u32 })},
                   owner_id: owner_id}
    }
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

fn read_player_state() -> PlayerState {
    let line = read_line();
    let mut player_str = line.split_whitespace();
    PlayerState { n_quests: parse_input!(player_str.next().unwrap(), u32),
                  pos: Point{ x: parse_input!(player_str.next().unwrap(), u32),
                              y: parse_input!(player_str.next().unwrap(), u32) },
                  tile:  FreePaths::new(player_str.next().unwrap()) }
}

fn read_items_state(n_items: u32) -> HashMap<String,ItemState> {
    let mut items = HashMap::<String,ItemState>::new();
    for _ in 0..n_items {
        let line = read_line();
        let mut item_str = line.split_whitespace();
        let item_name = item_str.next().unwrap();
        let maybe_x = parse_input!(item_str.next().unwrap(), i32);
        let maybe_y = parse_input!(item_str.next().unwrap(), i32);
        let owner_id = parse_input!(item_str.next().unwrap(), u32);
        items.insert(item_name.to_string(), ItemState::new(maybe_x, maybe_y, owner_id));
    }
    items
}

fn read_quests(n_quests: u32) -> Vec<Quest> {
    let mut quests = Vec::<Quest>::new();
    for _ in 0..n_quests {
        let input_line = read_line();
        let mut quest_str = input_line.split_whitespace();
        quests.push(Quest{ item: quest_str.next().unwrap().to_string(),
                           owner_id: parse_input!(quest_str.next().unwrap(), u32)});
    }
    quests
}

fn main() {
    // game loop
    loop {
        let _turn_type = parse_input!(read_line(), i32);
        let _game_map = read_matrix();
        let _me =  read_player_state();
        let _opp = read_player_state();
        // todo: put players to a vector to get their ids
        let num_items = parse_input!(read_line(), u32);
        let _items: HashMap<String,ItemState> = read_items_state(num_items);
        let num_quests = parse_input!(read_line(), u32);
        let _quests = read_quests(num_quests);

        println!("PUSH 3 RIGHT"); // PUSH <id> <direction> | MOVE <direction> | PASS
    }
}

// Write an action using println!("message...");
// To debug: eprintln!("Debug message...");
