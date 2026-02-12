#include <iostream>
#include <vector>
#include "PuzzleLogic.h"

using namespace std;

// Display the board in a nice format
void displayBoard(const vector<int>& board) {
    cout << "\n+---+---+---+\n";
    for (int i = 0; i < 9; i++) {
        if (i % 3 == 0 && i != 0) {
            cout << "|\n+---+---+---+\n";
        }
        cout << "| ";
        if (board[i] == 0) {
            cout << " "; // Empty space
        }
        else {
            cout << board[i];
        }
        cout << " ";
        if (i % 3 == 2) {
            cout << "|";
        }
    }
    cout << "|\n+---+---+---+\n";
}

// Display position numbers for reference
void displayPositions() {
    cout << "\nPosition numbers:\n";
    cout << "+---+---+---+\n";
    for (int i = 0; i < 9; i++) {
        if (i % 3 == 0 && i != 0) {
            cout << "|\n+---+---+---+\n";
        }
        cout << "| " << i << " ";
        if (i % 3 == 2) {
            cout << "|";
        }
    }
    cout << "|\n+---+---+---+\n";
}

int main() {
    cout << "=== Sliding Puzzle Game ===\n";

    // Create new game
    vector<int> board = createShuffledBoard();
    int moves = 0;

    cout << "\nNew puzzle created!";
    displayBoard(board);
    displayPositions();

    cout << "\nGoal: Arrange tiles 1-8 in order with empty space at bottom-right\n";
    cout << "Enter the position number (0-8) of the tile you want to move\n";
    cout << "Enter -1 to quit, -2 to show valid moves, -3 for new game\n";

    while (!isSolved(board)) {
        cout << "\nMoves: " << moves << " | Enter position: ";
        int pos;
        cin >> pos;

        // Check for special commands
        if (pos == -1) {
            cout << "Thanks for playing! Final moves: " << moves << "\n";
            return 0;
        }

        if (pos == -2) {
            vector<int> validMoves = getValidMoves(board);
            cout << "Valid moves: ";
            for (int move : validMoves) {
                cout << move << " ";
            }
            cout << "\n";
            continue;
        }

        if (pos == -3) {
            board = createShuffledBoard();
            moves = 0;
            cout << "\nNew game started!";
            displayBoard(board);
            continue;
        }

        // Validate input
        if (pos < 0 || pos > 8) {
            cout << "Invalid position! Must be 0-8.\n";
            continue;
        }

        // Try to make the move
        if (makeMove(board, pos)) {
            moves++;
            displayBoard(board);

            if (isSolved(board)) {
                cout << "\n*** CONGRATULATIONS! ***\n";
                cout << "You solved the puzzle in " << moves << " moves!\n";
            }
        }
        else {
            cout << "Invalid move! That tile is not adjacent to the empty space.\n";
            vector<int> validMoves = getValidMoves(board);
            cout << "Valid positions to move: ";
            for (int move : validMoves) {
                cout << move << " ";
            }
            cout << "\n";
        }
    }

    // Ask if they want to play again
    cout << "\nPlay again? (y/n): ";
    char choice;
    cin >> choice;

    if (choice == 'y' || choice == 'Y') {
        return main(); // Restart game
    }

    cout << "Thanks for playing!\n";
    return 0;
}