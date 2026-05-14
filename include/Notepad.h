#pragma once

/*
 * Notepad.h
 * ---------
 * The Notepad class manages a 2-D doubly-linked list of character nodes.
 *
 * Responsibilities:
 *   - Document structure  (insert, delete, newline, merge)
 *   - Operation-based undo / redo
 *   - Partial and full console rendering  (highlight-aware)
 *   - File I/O
 *   - Cursor navigation
 *
 * Implementation lives in Notepad.cpp.
 */

#include "Node.h"
#include <Windows.h>

class Notepad {
public:
    Node* head;
    Node* cursor;
    int   mainTextHeight;
    int   mainTextWidth;

    // ── Construction / destruction ──────────────
    Notepad();
    Notepad(int rows, int cols);
    ~Notepad();

    // ── Query helpers ────────────────────────────
    bool canMoveRight() const;
    bool canMoveLeft()  const;
    bool canMoveUp()    const;
    bool canMoveDown()  const;

    // ── Cursor movement ──────────────────────────
    void moveCursorRight();
    void moveCursorLeft();
    void moveCursorUp();
    void moveCursorDown();

    // ── Position queries ─────────────────────────
    void  getCursorRowCol(int& row, int& col) const;
    COORD getCursorPos()                      const;
    bool  navigateTo(int rowIdx, int colIdx);

    // ── Editing ──────────────────────────────────
    bool insertChar(char ch, UndoRecord* rec = nullptr);
    void insertWord(const char* word);
    void newLine(UndoRecord* rec = nullptr);
    void deleteChar(UndoRecord* rec = nullptr);

    // ── Undo / Redo ──────────────────────────────
    int undoOperation(const UndoRecord& rec);
    int redoOperation(const UndoRecord& rec);

    // ── Rendering ────────────────────────────────
    void display()                const;
    void displayRow(int rowIdx)   const;
    void displayFrom(int startRow) const;
    void clearArea(int sx, int sy, int w, int h) const;

    // ── File I/O ─────────────────────────────────
    bool saveToFile(const char* filename)   const;
    bool loadFromFile(const char* filename);
    void promptSaveOnExit(const char* filename);

    // ── Word extraction ──────────────────────────
    void getWordBeforeCursor(char* out, int maxLen) const;

private:
    // Vertical pointer repairs
    void relinkColumnVertical(Node* n);
    void repairNodeColumnVertical(Node* anchor);
    void repairRowVerticalLinks(Node* rowH);

    // Linked-list helpers
    static void  spliceRight(Node* anchor, Node* newNode);
    static Node* rowHead(Node* n);
    static int   colIndex(Node* n);
    static int   rowIndex(Node* h);
    static int   lineLength(Node* sentinelH);

    void freeAll();
};
