#pragma once
#ifndef PUZZLE_LOGIC_H
#define PUZZLE_LOGIC_H

#include <vector>
#include <algorithm>
#include <random>
#include <queue>
#include <unordered_map>
#include <cmath>
#include <stack>
#include "PuzzleLogic.h"


using namespace std;


extern stack<int> moveHistory; // Stack to track move history for undo functionality
extern vector<int> board;
extern int moveCount;


static vector<int> cachedSolution;
static vector<int> cachedBoard;

// Check if puzzle is in solved state
bool isSolved(const vector<int>& board) {
    for (int i = 0; i < 8; i++) {
        if (board[i] != i + 1) return false;
    }
    return board[8] == 0;
}

// Count inversions for solvability check
int countInversions(const vector<int>& board) {
    int inversions = 0;
    for (int i = 0; i < 9; i++) {
        if (board[i] == 0) continue;
        for (int j = i + 1; j < 9; j++) {
            if (board[j] == 0) continue;
            if (board[i] > board[j]) inversions++;
        }
    }
    return inversions;
}

// Check if board configuration is solvable
bool isSolvable(const vector<int>& board) {
    return countInversions(board) % 2 == 0;
}

// Generate a solvable shuffled board
vector<int> createShuffledBoard() {
    vector<int> board = { 1, 2, 3, 4, 5, 6, 7, 8, 0 };
    random_device rd;
    mt19937 gen(rd());
    //moveHistory.empty(); // Clear move history when creating a new board 
	moveHistory = stack<int>(); // Reset the move history stack

    do {
        shuffle(board.begin(), board.end(), gen);
    } while (!isSolvable(board));
    

    return board;
}

// Find position of empty tile (0)
int findEmptyPos(const vector<int>& board) {
    return find(board.begin(), board.end(), 0) - board.begin();
}

// Check if move is valid (adjacent to empty space)
bool isValidMove(const vector<int>& board, int tilePos) {
    if (tilePos < 0 || tilePos > 8) return false;

    int emptyPos = findEmptyPos(board);
    int emptyRow = emptyPos / 3;
    int emptyCol = emptyPos % 3;
    int tileRow = tilePos / 3;
    int tileCol = tilePos % 3;

    int rowDiff = abs(emptyRow - tileRow);
    int colDiff = abs(emptyCol - tileCol);

    return (rowDiff == 1 && colDiff == 0) || (rowDiff == 0 && colDiff == 1);
}

// Make a move (swap tile with empty space)
bool makeMove(vector<int>& board, int tilePos) {
    if (!isValidMove(board, tilePos)) return false;

    int emptyPos = findEmptyPos(board);
	moveHistory.push(emptyPos); // Save current empty position for undo
    swap(board[emptyPos], board[tilePos]);
	
    return true;
}

bool undoMove(vector<int>& board) {
    if (moveHistory.empty()) return false;
    int prevPos = moveHistory.top();
    moveHistory.pop();
    int emptyPos = findEmptyPos(board);
    swap(board[emptyPos], board[prevPos]);
    return true;
}

// Get all valid move positions from current state
vector<int> getValidMoves(const vector<int>& board) {
    vector<int> moves;
    int emptyPos = findEmptyPos(board);
    int row = emptyPos / 3;
    int col = emptyPos % 3;

    // Up
    if (row > 0) moves.push_back((row - 1) * 3 + col);
    // Down
    if (row < 2) moves.push_back((row + 1) * 3 + col);
    // Left
    if (col > 0) moves.push_back(row * 3 + (col - 1));
    // Right
    if (col < 2) moves.push_back(row * 3 + (col + 1));

    return moves;
}

// Manhattan distance heuristic
int manhattan(const vector<int>& board) {
    int dist = 0;
    for (int i = 0; i < board.size(); i++) {
        if (board[i] == 0) continue;
        int val = board[i] - 1;
        int targetRow = val / 3, targetCol = val % 3;
        int curRow = i / 3, curCol = i % 3;
        dist += abs(targetRow - curRow) + abs(targetCol - curCol);
    }
    return dist;
}

vector<int> solvePuzzle(vector<int> start) {
    using State = pair<int, vector<int>>; // cost, board
    priority_queue<State, vector<State>, greater<State>> pq;
    unordered_map<string, int> visited;
    unordered_map<string, pair<string, int>> parent; // state -> {prevState, move}

    auto encode = [](const vector<int>& b) {
        string s;
        for (int x : b) s += to_string(x) + ",";
        return s;
        };

    string startKey = encode(start);
    pq.push({ manhattan(start), start });
    visited[startKey] = 0;
    parent[startKey] = { "", -1 };

    vector<int> goal = { 1,2,3,4,5,6,7,8,0 };

    while (!pq.empty()) {
        auto [cost, board] = pq.top(); pq.pop();
        string key = encode(board);

        if (board == goal) {
            // Reconstruct move sequence
            vector<int> moves;
            string cur = key;
            while (parent[cur].second != -1) {
                moves.push_back(parent[cur].second);
                cur = parent[cur].first;
            }
            reverse(moves.begin(), moves.end());
            return moves;
        }

        int g = visited[key];
        int empty = findEmptyPos(board);
        vector<int> neighbours = getValidMoves(board);

        for (int pos : neighbours) {
            vector<int> next = board;
            swap(next[empty], next[pos]);
            string nextKey = encode(next);
            int ng = g + 1;

            if (!visited.count(nextKey) || visited[nextKey] > ng) {
                visited[nextKey] = ng;
                parent[nextKey] = { key, pos };
                pq.push({ ng + manhattan(next), next });
            }
        }
    }
    return {}; // unsolvable
}

vector<int> getSolution() {
    if (board == cachedBoard && !cachedSolution.empty()) {
        return cachedSolution;
    }
    cachedBoard = board;
    cachedSolution = solvePuzzle(board);
    return cachedSolution;
}

void invalidateCache() {
    cachedBoard.clear();
    cachedSolution.clear();
}



#endif // PUZZLE_LOGIC_H


