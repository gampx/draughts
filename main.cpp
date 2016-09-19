#include <iostream>
#include <fstream>
#include <vector>
#include <array>
#include <cstdio>
#include <cassert>
#include <queue>
#include <algorithm>
#include <utility>
#include <chrono>
using namespace std;

struct Move;
struct Player;
struct State;

struct Clock {
  static chrono::system_clock::time_point start;
  static long long kTimeLimitMicroseconds;
  static void init() {
    start = chrono::high_resolution_clock::now();
    kTimeLimitMicroseconds = 990000;
  }
  static long long elapsed_mcs() {
    return chrono::duration_cast<chrono::microseconds>(
        chrono::high_resolution_clock::now() - start).count();
  }
  static bool out_of_time() { return elapsed_mcs() > kTimeLimitMicroseconds; }
};
chrono::system_clock::time_point Clock::start;
long long Clock::kTimeLimitMicroseconds;

template<class T>
struct optional {
  bool exists;
  T value;
  operator bool() const {
    return exists;
  }
};

struct Position {
  char x;
  char y;
  size_t distance(Position other) const {
    return abs(x-other.x) + abs(y-other.y);
  }
  Position& operator= (Position rhs) {
    this->x = rhs.x;
    this->y = rhs.y;
    return *this;
  }
};

ostream& operator << (ostream& out, const Position& pos) {
  out << int(pos.y) << " " << int(pos.x);
  return out;
}

struct Direction {
  char dx;
  char dy;
};

Direction operator-(Position lhs, Position rhs) {
  return {lhs.x-rhs.x, lhs.y-rhs.y};
}

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

  Move(Position s, const vector<Position>& m): start(s), moves(m) {}

  bool operator== (Move rhs) {
    return start.x == rhs.start.x && start.y == rhs.start.y && moves.size() > 0 &&
      rhs.moves.size() > 0 && moves[0].x == rhs.moves[0].x && moves[0].y == rhs.moves[0].y;
  }

  Move& operator= (const Move& rhs) {
    start = rhs.start;
    moves.clear();
    moves.insert(moves.end(), rhs.moves.begin(), rhs.moves.end());
    return *this;
  }
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
  char at(int x, int y) const { return board[y][x]; }
  char at(Position p) const { return board[p.y][p.x]; }
  bool is_within(Position p) const {
    return p.x >= 0 && p.x < kSize && p.y >= 0 && p.y < kSize;
  }

};

ostream& operator << (ostream& out, const Move& move) {
  out << move.start << endl;
  out << move.moves.size() << endl;
  for(const auto& pos : move.moves) out << pos << endl;
  return out;
}

struct Player {
  char id;

  void read(istream& in) {
    in >> id;
  }

  int hash() const { return id - '1'; }
  char opposite() const { return id == '1'? '2':'1'; }


};

bool operator== (Player player, char c) { return player.id == c; }
bool operator== (char c, Player player) { return player.id == c; }

struct Rules {
  static array<array<Direction, 2>, 2> kDirections;

  static void init() {
    kDirections = {{
      {Direction{1,1}, Direction{-1,1}},
        {Direction{1,-1},Direction{-1,-1}}
    }};
  }

  static bool try_extend_jump(const Board& board, Move& move, Player player, Direction dir) {
    Position last_jump = move.moves.back();
    Position next_pos = last_jump + dir;
    if(!board.is_within(next_pos)) return false;
    if(board.at(next_pos) != player.opposite()) return false;
    Position next_jump = next_pos + dir;
    if(!board.is_within(next_jump)) return false;
    if(board.at(next_jump) != '0') return false;
    move.moves.push_back(next_jump);
    return true;
  }

  static vector<Move> possible_extensions_of_a_jump(const Board& board, Move jump) {
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
    return possible_extensions;
  }

  static vector<Move> possible_moves(const Board& board, Position pos) {
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
        for(const auto& current_jump : possible_jumps) {
          possible_moves.emplace_back(pos, current_jump.moves);
        }
      } else {
        possible_moves.push_back({pos, {next_pos}});
      }
    }
    return possible_moves;
  }

  static vector<Move> possible_moves(const Board& board, Player player) {
    vector<Move> moves;
    for(int x = 0; x < board.kSize; x++) {
      for(int y = 0; y < board.kSize; y++) {
        if(board.at(x,y) == player.id) {
          auto moves_from_pos = possible_moves(board, {x,y});
          moves.insert(moves.end(), moves_from_pos.begin(), moves_from_pos.end());
        }
      }
    }
    return moves;
  }

  static vector<Position> captures(const Board& board, const Move& move) {
    auto cur_pos = move.start;
    vector<Position> captures;
    for(const auto& next_pos: move.moves) {
      if(cur_pos.distance(next_pos) == 4) {
        auto delta_dir = next_pos - cur_pos;
        delta_dir.dx /= 2;
        delta_dir.dy /= 2;
        captures.push_back(cur_pos+delta_dir);
      }
      cur_pos = next_pos;
    }
    return captures;
  }
};

array<array<Direction, 2>, 2> Rules::kDirections;

struct State {
  Board board;
  Player player;

  void read(istream& in) {
    board.read(in);
    player.read(in);
  }

  State apply_move(const Move& move) const {
    State new_state = *this;
    auto captures = Rules::captures(board, move);
    new_state.board.at(move.start) = '0';
    new_state.board.at(move.moves.back()) = player.id;
    for(const auto& pos : captures) {
      new_state.board.at(pos) = '0';
    }
    new_state.player.id = player.opposite();
    return new_state;
  }

  int evaluate() {
    auto capture = has_capture();
    State new_state = *this;
    while(capture) {
      new_state = new_state.apply_move(capture.value);
      capture = new_state.has_capture();
    }
    if(new_state.player.id == player.id) return new_state.difference_in_pieces();
    else return -new_state.difference_in_pieces();
  }

  int difference_in_pieces() {
    int score = 0;
    for(int x = 0; x < board.kSize; x++) {
      for(int y = 0; y < board.kSize; y++) {
        if(board.at(x,y) == player.id) score++;
        if(board.at(x,y) == player.opposite()) score--;
      }
    }
    return score;
  }

  optional<Move> has_capture() {
    auto moves = Rules::possible_moves(board, player);
    for(const auto& move : moves) {
      if(Rules::captures(board, move).size() > 0) return {true, move};
    }
    return {false, {{0,0}, {}}};
  }

  void print(ostream& out) const {
    for(int y = 0; y < board.kSize; y++) {
      for(int x = 0; x < board.kSize; x++) {
        out << board.at(x,y) << " ";
      }
      out << endl;
    }
    out << player.id << endl;
  }
};

struct ScoreState {
  int score;
  State state;
};

bool operator< (const ScoreState& l, const ScoreState& r) { return l.score < r.score; }

int alpha_beta(State state, int depth, int alpha, int beta) {
  const int kInf = 1e9;
  int score = -kInf;
  if(Clock::out_of_time()) return -kInf;
  if(depth == 0) return state.evaluate();
  vector<ScoreState> states;
  auto all_moves = Rules::possible_moves(state.board, state.player);
  for(const auto& move : all_moves) {
    State new_state = state.apply_move(move);
    int new_state_score = new_state.evaluate();
    states.push_back({new_state_score, new_state});
  }
  sort(states.begin(), states.end());
  for(const auto& score_state : states) {
    int current_score = -alpha_beta(score_state.state, depth-1, -beta, -alpha);
    if(current_score > score) {
      score = current_score;
    }
    if(current_score >= beta) {
      return current_score;
    }
    if(current_score > alpha) {
      alpha = current_score;
    }
  }
  return score;
}

struct ScoreMove{
  int score;
  Move move;
};

bool operator< (const ScoreMove& l, const ScoreMove& r) { return l.score < r.score; }

Move find_best_move(const State& state) {
  const int kInf = 1e9;
  Move best_move = {{0,0}, {}};
  Move last_best_move = {{0,0}, {}};
  int best_score;
  auto all_moves = Rules::possible_moves(state.board, state.player);
  vector<ScoreMove> scored_moves;
  for(const auto& move : all_moves) {
    State new_state = state.apply_move(move);
    scored_moves.push_back({new_state.evaluate(), move});
  }
  sort(scored_moves.begin(), scored_moves.end());
  for(int depth = 0; depth < 1000; depth++) {
#ifdef DEBUG
    cout << "Depth: " << depth << endl;
#endif
    best_score = -kInf-1;
    best_move = last_best_move;
    if(best_move.moves.size() != 0) {
      State new_state = state.apply_move(best_move);
      int score = -alpha_beta(new_state, depth, -kInf, kInf);
      if(score > best_score) {
        best_score = score;
      }
    }

    for(const auto& scored_move : scored_moves) {
      auto move = scored_move.move;
      if(depth > 0 && move == last_best_move) continue;
      State new_state = state.apply_move(move);
      int score = -alpha_beta(new_state, depth, -kInf, -best_score);
      if(score > best_score) {
        best_score = score;
        best_move = move;
      }
    }
    if(!Clock::out_of_time()) last_best_move = best_move;
    else break;
#ifdef DEBUG
    cout << "Best move: " << endl <<  last_best_move;
    cout << "Best score: " << best_score << endl;
#endif
  }
  return last_best_move;
}

void test_jumps() {
  Board board;
  ifstream file("board1");
  board.read(file);
  auto jumps = Rules::possible_moves(board, {4,7});
  assert(jumps.size() == 5);
  for(const auto& jump : jumps) {
    cout << jump << endl;
    cout << "Captures: " << endl;
    for(const auto& cap : Rules::captures(board, jump)) {
      cout << cap << endl;
    }
    cout << endl;
  }
}

int main() {
  Clock::init();
  Rules::init();
  State game;
  game.read(cin);
  auto move = find_best_move(game);
  cout << move << endl;
  //test_jumps();


  return 0;
}
