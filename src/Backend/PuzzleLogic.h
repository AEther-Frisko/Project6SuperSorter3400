#pragma once
#ifndef PUZZLE_LOGIC_H
#define PUZZLE_LOGIC_H

#include <vector>
#include <algorithm>
#include <random>
#include "PuzzleLogic.h"

using namespace std;

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
    swap(board[emptyPos], board[tilePos]);
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

#endif // PUZZLE_LOGIC_H


