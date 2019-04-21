use std::io::{self, BufRead};

macro_rules! parse_input {
    ($x:expr, $t:ident) => ($x.trim().parse::<$t>().unwrap())
}

fn read_line() -> String {
    let stdin = io::stdin();
    let line = stdin.lock().lines().next().unwrap().unwrap();
    line
}

fn main() {
    // game loop
    loop {
        let turn_type = parse_input!(read_line(), i32);
        for _ in 0..7 as usize {
            let inputs = read_line();
            for tile in inputs.split_whitespace() {
            }
        }
        for _ in 0..2 as usize {
            let input_line = read_line();
            let inputs = input_line.split(" ").collect::<Vec<_>>();
            let num_player_cards = parse_input!(inputs[0], i32); // the total number of quests for a player (hidden and revealed)
            let player_x = parse_input!(inputs[1], i32);
            let player_y = parse_input!(inputs[2], i32);
            let player_tile = inputs[3].trim().to_string();
        }
        let input_line = read_line();
        let num_items = parse_input!(input_line, i32); // the total number of items available on board and on player tiles
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
