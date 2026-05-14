/*
 * Notepad.cpp
 * -----------
 * Implements the Notepad class (declared in Notepad.h).
 *
 * Sections:
 *   1. Construction / destruction
 *   2. Query helpers
 *   3. Cursor movement
 *   4. Position queries
 *   5. Editing  (insertChar, insertWord, newLine, deleteChar)
 *   6. Undo / Redo
 *   7. Rendering
 *   8. File I/O
 *   9. Word extraction
 *  10. Private helpers (vertical links, linked-list utils)
 */
#define _CRT_SECURE_NO_WARNINGS

#include "../include/Notepad.h"
#include <fstream>
#include <cstring>
#include <cctype>
#include <conio.h>
#include <iostream>
using std::cout;

// ══════════════════════════════════════════════
//  1. Construction / destruction
// ══════════════════════════════════════════════

Notepad::Notepad()
    : head(nullptr), cursor(nullptr),
      mainTextHeight(25), mainTextWidth(80) {}

Notepad::Notepad(int rows, int cols)
    : head(nullptr), cursor(nullptr),
      mainTextHeight(rows), mainTextWidth(cols) {}

Notepad::~Notepad() { freeAll(); }

// ══════════════════════════════════════════════
//  2. Query helpers
// ══════════════════════════════════════════════

bool Notepad::canMoveRight() const {
    return cursor != nullptr && cursor->right != nullptr;
}
bool Notepad::canMoveLeft() const {
    return cursor != nullptr && cursor->left != nullptr;
}
bool Notepad::canMoveUp() const {
    if (!cursor) return false;
    return rowHead(cursor)->up != nullptr;
}
bool Notepad::canMoveDown() const {
    if (!cursor) return false;
    return rowHead(cursor)->down != nullptr;
}

// ══════════════════════════════════════════════
//  3. Cursor movement
// ══════════════════════════════════════════════

void Notepad::moveCursorRight() { if (canMoveRight()) cursor = cursor->right; }
void Notepad::moveCursorLeft()  { if (canMoveLeft())  cursor = cursor->left;  }

void Notepad::moveCursorUp() {
    if (!canMoveUp()) return;
    int   col    = colIndex(cursor);
    Node* target = rowHead(cursor)->up;
    for (int i = 0; i < col && target->right; i++) target = target->right;
    cursor = target;
}

void Notepad::moveCursorDown() {
    if (!canMoveDown()) return;
    int   col    = colIndex(cursor);
    Node* target = rowHead(cursor)->down;
    for (int i = 0; i < col && target->right; i++) target = target->right;
    cursor = target;
}

// ══════════════════════════════════════════════
//  4. Position queries
// ══════════════════════════════════════════════

void Notepad::getCursorRowCol(int& row, int& col) const {
    if (!cursor) { row = 0; col = 0; return; }
    col = colIndex(cursor);
    row = rowIndex(rowHead(cursor));
}

COORD Notepad::getCursorPos() const {
    if (!cursor) return { 1, 1 };
    int col = colIndex(cursor);
    int row = rowIndex(rowHead(cursor));
    return { static_cast<SHORT>(col + 1), static_cast<SHORT>(row + 1) };
}

bool Notepad::navigateTo(int rowIdx, int colIdx) {
    Node* r = head;
    for (int i = 0; i < rowIdx && r; i++) r = r->down;
    if (!r) return false;
    Node* c = r;
    for (int i = 0; i < colIdx && c->right; i++) c = c->right;
    cursor = c;
    return true;
}

// ══════════════════════════════════════════════
//  5. Editing
// ══════════════════════════════════════════════

bool Notepad::insertChar(char ch, UndoRecord* rec) {
    if (head == nullptr) {
        Node* sentinel = new Node('\n');
        Node* newNode  = new Node(ch);
        sentinel->right = newNode;
        newNode->left   = sentinel;
        head   = sentinel;
        cursor = newNode;
        if (rec) {
            rec->op        = OP_INSERT;
            rec->ch        = ch;
            rec->cursorRow = 0;
            rec->cursorCol = 0;
        }
        return true;
    }

    if (lineLength(rowHead(cursor)) >= mainTextWidth - 2) return false;

    if (rec) {
        rec->op        = OP_INSERT;
        rec->ch        = ch;
        rec->cursorRow = rowIndex(rowHead(cursor));
        rec->cursorCol = colIndex(cursor);
    }

    Node* newNode = new Node(ch);
    spliceRight(cursor, newNode);
    relinkColumnVertical(newNode);
    cursor = newNode;
    return true;
}

void Notepad::insertWord(const char* word) {
    for (int i = 0; word[i]; i++) {
        if (!insertChar(word[i], nullptr)) break;
    }
}

void Notepad::newLine(UndoRecord* rec) {
    if (rec) {
        rec->op        = OP_NEWLINE;
        rec->ch        = '\0';
        rec->cursorRow = cursor ? rowIndex(rowHead(cursor)) : 0;
        rec->cursorCol = cursor ? colIndex(cursor)          : 0;
    }

    Node* newRowHead  = new Node('\n');
    Node* splitPoint  = cursor ? cursor->right : nullptr;

    if (cursor)     cursor->right    = nullptr;
    if (splitPoint) splitPoint->left = nullptr;

    newRowHead->right = splitPoint;
    if (splitPoint) splitPoint->left = newRowHead;

    Node* curRowHead = cursor ? rowHead(cursor) : nullptr;
    Node* nextRow    = curRowHead ? curRowHead->down : nullptr;

    if (curRowHead) {
        curRowHead->down  = newRowHead;
        newRowHead->up    = curRowHead;
    } else {
        head = newRowHead;
    }
    newRowHead->down = nextRow;
    if (nextRow) nextRow->up = newRowHead;

    repairRowVerticalLinks(newRowHead);
    cursor = newRowHead;
    if (!head) head = newRowHead;
}

void Notepad::deleteChar(UndoRecord* rec) {
    if (!cursor) return;

    // Backspace on a sentinel  →  merge with previous row
    if (cursor->data == '\n') {
        if (!cursor->up) return;

        Node* prevRowHead = cursor->up;
        Node* prevRowEnd  = prevRowHead;
        while (prevRowEnd->right) prevRowEnd = prevRowEnd->right;

        if (rec) {
            rec->op        = OP_MERGE;
            rec->ch        = '\0';
            rec->cursorRow = rowIndex(cursor);
            rec->cursorCol = 0;
            rec->mergeCol  = colIndex(prevRowEnd);
        }

        Node* curContent  = cursor->right;
        prevRowEnd->right = curContent;
        if (curContent) curContent->left = prevRowEnd;

        Node* belowRow    = cursor->down;
        prevRowHead->down = belowRow;
        if (belowRow) belowRow->up = prevRowHead;

        repairNodeColumnVertical(prevRowEnd);

        delete cursor;
        cursor = prevRowEnd;
        return;
    }

    if (rec) {
        rec->op        = OP_DELETE;
        rec->ch        = cursor->data;
        rec->cursorRow = rowIndex(rowHead(cursor));
        rec->cursorCol = colIndex(cursor->left);
    }

    Node* tmp    = cursor;
    Node* newCur = cursor->left;

    newCur->right = tmp->right;
    if (tmp->right) tmp->right->left = newCur;

    repairNodeColumnVertical(newCur);

    delete tmp;
    cursor = newCur;
}

// ══════════════════════════════════════════════
//  6. Undo / Redo
// ══════════════════════════════════════════════

int Notepad::undoOperation(const UndoRecord& rec) {
    switch (rec.op) {
    case OP_INSERT:
        navigateTo(rec.cursorRow, rec.cursorCol + 1);
        deleteChar(nullptr);
        navigateTo(rec.cursorRow, rec.cursorCol);
        return rec.cursorRow;

    case OP_DELETE:
        navigateTo(rec.cursorRow, rec.cursorCol);
        insertChar(rec.ch, nullptr);
        return rec.cursorRow;

    case OP_NEWLINE:
        navigateTo(rec.cursorRow + 1, 0);
        deleteChar(nullptr);
        navigateTo(rec.cursorRow, rec.cursorCol);
        return rec.cursorRow;

    case OP_MERGE:
        navigateTo(rec.cursorRow - 1, rec.mergeCol);
        newLine(nullptr);
        return rec.cursorRow - 1;
    }
    return -1;
}

int Notepad::redoOperation(const UndoRecord& rec) {
    switch (rec.op) {
    case OP_INSERT:
        navigateTo(rec.cursorRow, rec.cursorCol);
        insertChar(rec.ch, nullptr);
        return rec.cursorRow;

    case OP_DELETE:
        navigateTo(rec.cursorRow, rec.cursorCol + 1);
        deleteChar(nullptr);
        navigateTo(rec.cursorRow, rec.cursorCol);
        return rec.cursorRow;

    case OP_NEWLINE:
        navigateTo(rec.cursorRow, rec.cursorCol);
        newLine(nullptr);
        return rec.cursorRow;

    case OP_MERGE:
        navigateTo(rec.cursorRow, 0);
        deleteChar(nullptr);
        return rec.cursorRow - 1;
    }
    return -1;
}

// ══════════════════════════════════════════════
//  7. Rendering  (highlight-aware)
// ══════════════════════════════════════════════

void Notepad::displayRow(int rowIdx) const {
    Node* r = head;
    for (int i = 0; i < rowIdx && r; i++) r = r->down;

    gotoxy(1, rowIdx + 1);
    if (!r) {
        resetTextColor();
        for (int x = 1; x < mainTextWidth; x++) cout << ' ';
        return;
    }

    Node* col = r->right;
    int   x   = 1;
    while (col && x < mainTextWidth) {
        if (col->highlighted) setHighlightColor();
        else                  resetTextColor();
        cout << col->data;
        col = col->right;
        x++;
    }
    resetTextColor();
    while (x < mainTextWidth) { cout << ' '; x++; }
}

void Notepad::displayFrom(int startRow) const {
    Node* r = head;
    for (int i = 0; i < startRow && r; i++) r = r->down;

    int y = startRow + 1;
    while (y < mainTextHeight) {
        gotoxy(1, y);
        if (r) {
            Node* col = r->right;
            int   x   = 1;
            while (col && x < mainTextWidth) {
                if (col->highlighted) setHighlightColor();
                else                  resetTextColor();
                cout << col->data;
                col = col->right;
                x++;
            }
            resetTextColor();
            while (x < mainTextWidth) { cout << ' '; x++; }
            r = r->down;
        } else {
            resetTextColor();
            for (int x = 1; x < mainTextWidth; x++) cout << ' ';
        }
        y++;
    }
}

void Notepad::display() const {
    Node* row = head;
    int   y   = 1;
    while (row && y < mainTextHeight) {
        gotoxy(1, y);
        Node* col = row->right;
        int   x   = 1;
        while (col && x < mainTextWidth) {
            if (col->highlighted) setHighlightColor();
            else                  resetTextColor();
            cout << col->data;
            col = col->right;
            x++;
        }
        resetTextColor();
        while (x < mainTextWidth) { cout << ' '; x++; }
        row = row->down;
        y++;
    }
    resetTextColor();
    while (y < mainTextHeight) {
        gotoxy(1, y);
        for (int x = 1; x < mainTextWidth; x++) cout << ' ';
        y++;
    }
}

void Notepad::clearArea(int sx, int sy, int w, int h) const {
    for (int i = 0; i < h; ++i) {
        gotoxy(sx, sy + i);
        for (int j = 0; j < w; ++j) cout << ' ';
    }
    gotoxy(sx, sy);
}

// ══════════════════════════════════════════════
//  8. File I/O
// ══════════════════════════════════════════════

bool Notepad::saveToFile(const char* filename) const {
    std::ofstream file(filename);
    if (!file) {
        cout << "Error: cannot open '" << filename << "' for writing.\n";
        return false;
    }
    Node* row = head;
    while (row) {
        Node* col = row;
        while (col) {
            if (col->data != '\n') file << col->data;
            col = col->right;
        }
        file << '\n';
        row = row->down;
    }
    file.close();
    return true;
}

bool Notepad::loadFromFile(const char* filename) {
    freeAll();
    head = cursor = nullptr;

    std::ifstream file(filename);
    if (!file) {
        std::ofstream create(filename);
        if (!create) {
            cout << "Error: cannot create '" << filename << "'.\n";
            return false;
        }
        create.close();
        return true;
    }

    char ch;
    while (file.get(ch)) {
        if (ch == '\n') {
            newLine(nullptr);
        } else if (ch >= 32 && ch <= 126) {
            if (!head) {
                head   = new Node('\n');
                cursor = head;
            }
            insertChar(ch, nullptr);
        }
    }
    file.close();
    return true;
}

void Notepad::promptSaveOnExit(const char* filename) {
    cout << "\nSave before exit? (y/n): ";
    char c;
    std::cin >> c;
    if (c == 'y' || c == 'Y') {
        if (filename[0] == '\0') {
            cout << "Enter filename: ";
            static char buf[MAX_QUERY_LEN];
            int idx = 0;
            char in;
            while ((in = static_cast<char>(_getch())) != '\r' && idx < MAX_QUERY_LEN - 1) {
                buf[idx++] = in;
                cout << in;
            }
            buf[idx] = '\0';
            cout << '\n';
            saveToFile(buf);
        } else {
            saveToFile(filename);
        }
    }
}

// ══════════════════════════════════════════════
//  9. Word extraction
// ══════════════════════════════════════════════

void Notepad::getWordBeforeCursor(char* out, int maxLen) const {
    out[0] = '\0';
    if (!cursor) return;

    Node* n = cursor;
    if (!isalpha(static_cast<unsigned char>(n->data)))
        n = n->left;

    char tmp[MAX_QUERY_LEN];
    int  len = 0;
    while (n && n->data != '\n'
           && isalpha(static_cast<unsigned char>(n->data))
           && len < maxLen - 1) {
        tmp[len++] = static_cast<char>(tolower(static_cast<unsigned char>(n->data)));
        n = n->left;
    }
    for (int i = 0; i < len; i++)
        out[i] = tmp[len - 1 - i];
    out[len] = '\0';
}

// ══════════════════════════════════════════════
//  10. Private helpers
// ══════════════════════════════════════════════

void Notepad::relinkColumnVertical(Node* n) {
    if (!n) return;
    Node* left  = n->left;
    Node* above = (left && left->up)   ? left->up->right   : nullptr;
    Node* below = (left && left->down) ? left->down->right : nullptr;
    n->up   = above;
    n->down = below;
    if (above) above->down = n;
    if (below) below->up   = n;
}

void Notepad::repairNodeColumnVertical(Node* anchor) {
    if (!anchor) return;
    Node* above = anchor->up   ? anchor->up->right   : nullptr;
    Node* below = anchor->down ? anchor->down->right : nullptr;
    Node* cur   = anchor->right;
    while (cur) {
        cur->up   = above;
        cur->down = below;
        if (above) { above->down = cur; above = above->right; }
        if (below) { below->up   = cur; below = below->right; }
        cur = cur->right;
    }
}

void Notepad::repairRowVerticalLinks(Node* rowH) {
    if (!rowH) return;
    Node* above = rowH->up;
    Node* below = rowH->down;
    Node* cur   = rowH;
    while (cur) {
        cur->up   = above;
        cur->down = below;
        if (above) above->down = cur;
        if (below) below->up   = cur;
        cur   = cur->right;
        if (above) above = above->right;
        if (below) below = below->right;
    }
}

void Notepad::spliceRight(Node* anchor, Node* newNode) {
    if (!anchor || !newNode) return;
    newNode->right = anchor->right;
    newNode->left  = anchor;
    if (anchor->right) anchor->right->left = newNode;
    anchor->right  = newNode;
}

Node* Notepad::rowHead(Node* n) {
    if (!n) return nullptr;
    while (n->left) n = n->left;
    return n;
}

int Notepad::colIndex(Node* n) {
    int col = 0;
    while (n && n->left) { n = n->left; col++; }
    return col;
}

int Notepad::rowIndex(Node* h) {
    int row = 0;
    while (h && h->up) { h = h->up; row++; }
    return row;
}

int Notepad::lineLength(Node* sentinelH) {
    int   len = 0;
    Node* n   = sentinelH ? sentinelH->right : nullptr;
    while (n) { len++; n = n->right; }
    return len;
}

void Notepad::freeAll() {
    Node* row = head;
    while (row) {
        Node* nextRow = row->down;
        Node* col     = row;
        while (col) {
            Node* nextCol = col->right;
            delete col;
            col = nextCol;
        }
        row = nextRow;
    }
    head = cursor = nullptr;
}
