#include <iostream>
#include <vector>
#include <array>
#include <cstdio>
#include <cassert>
#include <queue>
using namespace std;

struct Move;
struct Player;
struct State;

struct Position {
  char x;
  char y;
};

ostream& operator << (ostream& out, const Position& pos) {
  out << pos.y << " " << pos.x;
  return out;
}

struct Direction {
  char dx;
  char dy;
};

Position operator+ (Position pos, Direction dir) {
  Position result = pos;
  result.x += dir.dx;
  result.y += dir.dy;
  return result;
}

Position& operator+= (Position& pos, Direction dir) {
  pos.x += dir.dx;
  pos.y += dir.dy;
  return pos;
}

struct Move {
  Position start;
  vector<Position> moves;
};

struct Board {
  constexpr static int kSize = 8;
  array<array<char, kSize>, kSize> board;

  void read(istream& in) {
    for(int y = 0; y < kSize; y++) {
      for(int x = 0; x < kSize; x++) {
        in >> board[y][x];
      }
    }
  }

  char& at(int x, int y) { return board[y][x]; }
  char& at(Position p) { return board[p.y][p.x]; }
  bool is_within(Position p) {
    return p.x >= 0 && p.x < kSize && p.y >= 0 && p.y < kSize;
  }

};


ostream& operator << (ostream& out, const Move& move) {
  out << move.start << endl;
  out << move.moves.size();
  for(const auto& pos : move.moves) out << pos << endl;
  return out;
}

struct Player {
  char id;

  void read(istream& in) {
    in >> id;
  }

  int hash() { return id - '1'; }
  char opposite() { return id == '1'? '2':'1'; }


};

bool operator== (Player player, char c) { return player.id == c; }
bool operator== (char c, Player player) { return player.id == c; }

struct Rules {
  constexpr static array<array<Direction, 2>, 2> kDirections = {{
    {Direction{1,1}, Direction{-1,1}},
      {Direction{1,-1},Direction{-1,-1}}
  }};

  bool try_extend_jump(Board& board, Move& move, Player player, Direction dir) {
    Position last_jump = move.moves.back();
    Position next_pos = last_jump + dir;
    if(!board.is_within(next_pos)) return false;
    if(board.at(next_pos) != player.opposite()) return false;
    Position next_jump = next_pos + dir;
    if(!board.is_within(next_jump)) return false;
    if(board.at(next_pos) != '0') return false;
    move.moves.push_back(next_jump);
    return true;
  }

  vector<Move> possible_extensions_of_a_jump(Board& board, Move jump) {
    vector<Move> possible_extensions;
    queue<Move> temporal_moves;
    temporal_moves.push(jump);
    Player player = {board.at(jump.start)};
    while(!temporal_moves.empty()) {
      bool is_extendable = false;
      for(const auto& dir : kDirections[player.hash()]) {
        auto current_jump = temporal_moves.front();
        bool extended = try_extend_jump(board, current_jump, player, dir);
        if(extended) {
          temporal_moves.push(current_jump);
        }
        is_extendable |= extended;
      }
      if(!is_extendable) {
        possible_extensions.push_back(temporal_moves.front());
      }
      temporal_moves.pop();
    }
  }

  vector<Move> possible_moves(Board& board, Position pos) {
    vector<Move> possible_moves;
    Player player = {board.at(pos)};
    if(player.id != '1' && player.id != '2') return {};
    for(const auto& dir : kDirections[player.hash()]) {
      Position next_pos = pos + dir;
      if(!board.is_within(next_pos) || board.at(next_pos) == player) continue;
      if(board.at(next_pos) == player.opposite()) {
        Position jump = next_pos + dir;
        if(!board.is_within(jump)) continue;
        if(board.at(jump) != '0') continue;
        auto possible_jumps = possible_extensions_of_a_jump(board, {pos, {jump}});
        for(auto& current_jump : possible_jumps) {
          possible_moves.push_back({pos, std::move(current_jump.moves)});
        }
      } else {
        possible_moves.push_back({pos, {next_pos}});
      }
    }
    return possible_moves;
  }
};

struct State {
  Board board;
  Player player;

  void read(istream& in) {
    board.read(in);
    player.read(in);
  }

  bool is_valid_first_move(const Move& move) {
    if(board.at(move.start) != player.id) return false;
    assert(move.moves.size() == 1);
    for(const auto& pos : move.moves) {
    }
    return true;
  }

  Move best_move() {
  }
};

int main() {
  State game_state;
  game_state.read(cin);

  cout << game_state.player.id;

  return 0;
}
