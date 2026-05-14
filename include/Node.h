#pragma once

/*
 * Node.h
 * ------
 * Core data types for ConsolePad:
 *   - Node          : one character cell in the 2-D doubly-linked list
 *   - OpType        : enum of edit operations for undo/redo
 *   - UndoRecord    : lightweight per-operation undo entry
 *   - UndoStack     : fixed-capacity linked-list stack (max 50 entries)
 *
 * Console utility free functions declared here are defined in UI.cpp
 * and shared across all translation units.
 */

#include <Windows.h>

// ─────────────────────────────────────────────
//  Constants
// ─────────────────────────────────────────────
static const int MAX_WORD_LEN      = 64;
static const int MAX_QUERY_LEN     = 256;
static const int MAX_SUGGESTIONS   = 9;
static const int MAX_UNDO_DEPTH    = 50;
static const int MAX_PHRASE_WORDS  = 16;

// ─────────────────────────────────────────────
//  Node  (one character cell in the 2-D list)
// ─────────────────────────────────────────────
struct Node {
    char  data;
    bool  highlighted;   // true = render in highlight colour
    Node* up;
    Node* down;
    Node* left;
    Node* right;

    explicit Node(char ch = '\0')
        : data(ch), highlighted(false),
          up(nullptr), down(nullptr),
          left(nullptr), right(nullptr) {}
};

// ─────────────────────────────────────────────
//  Console helpers  (implemented in UI.cpp)
// ─────────────────────────────────────────────
void gotoxy(int x, int y);
void getConsoleSize(int& cols, int& rows);
void drawHorizontalLine(int sx, int sy, int len);
void drawVerticalLine(int sx, int sy, int len);
void setHighlightColor();
void resetTextColor();
void clearHighlights(Node* docHead);

// ─────────────────────────────────────────────
//  Operation-based undo/redo records
// ─────────────────────────────────────────────
enum OpType {
    OP_INSERT,   // a character was inserted
    OP_DELETE,   // a character was deleted
    OP_NEWLINE,  // Enter was pressed  (line split)
    OP_MERGE     // Backspace on sentinel merged two lines
};

/*
 * UndoRecord stores only what changed, not the whole document.
 *
 *  OP_INSERT  : ch = the character inserted.
 *               cursorRow/Col = position BEFORE insertion.
 *
 *  OP_DELETE  : ch = the deleted character.
 *               cursorRow/Col = position AFTER deletion.
 *
 *  OP_NEWLINE : cursorRow/Col = position BEFORE Enter.
 *               ch unused.
 *
 *  OP_MERGE   : cursorRow/Col = merge point (sentinel that was deleted).
 *               mergeCol = end-of-prev-row column.
 *               ch unused.
 */
struct UndoRecord {
    OpType op;
    char   ch;
    int    cursorRow;
    int    cursorCol;
    int    mergeCol;    // only for OP_MERGE
    UndoRecord* next;

    UndoRecord()
        : op(OP_INSERT), ch('\0'),
          cursorRow(0), cursorCol(0), mergeCol(0),
          next(nullptr) {}
};

// ─────────────────────────────────────────────
//  UndoStack  (linked list, fixed capacity)
// ─────────────────────────────────────────────
class UndoStack {
public:
    UndoStack();
    ~UndoStack();

    bool isEmpty() const;
    void push(const UndoRecord& rec);
    bool pop(UndoRecord& out);
    void clear();

private:
    UndoRecord* topNode;
    int         sz;

    void removeBottom();
};
