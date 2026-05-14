/*
 * UI.cpp
 * ------
 * Console rendering, panel management, and layout chrome for ConsolePad.
 */
#define _CRT_SECURE_NO_WARNINGS

#include "../include/UI.h"
#include <iostream>
#include <cstring>
using std::cout;

// ─────────────────────────────────────────────
//  Console primitives
// ─────────────────────────────────────────────

void gotoxy(int x, int y) {
    COORD c = { static_cast<SHORT>(x), static_cast<SHORT>(y) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), c);
}

void getConsoleSize(int& cols, int& rows) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
    cols = csbi.srWindow.Right  - csbi.srWindow.Left + 1;
    rows = csbi.srWindow.Bottom - csbi.srWindow.Top  + 1;
}

void drawHorizontalLine(int sx, int sy, int len) {
    gotoxy(sx, sy);
    for (int i = 0; i < len; ++i) cout << '-';
}

void drawVerticalLine(int sx, int sy, int len) {
    for (int i = 0; i < len; ++i) {
        gotoxy(sx, sy + i);
        cout << '|';
    }
}

static HANDLE hOut() {
    static HANDLE h = INVALID_HANDLE_VALUE;
    if (h == INVALID_HANDLE_VALUE)
        h = GetStdHandle(STD_OUTPUT_HANDLE);
    return h;
}

void setHighlightColor() {
    SetConsoleTextAttribute(hOut(),
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}

void resetTextColor() {
    SetConsoleTextAttribute(hOut(),
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

void clearHighlights(Node* docHead) {
    Node* row = docHead;
    while (row) {
        Node* col = row;
        while (col) { col->highlighted = false; col = col->right; }
        row = row->down;
    }
}

// ─────────────────────────────────────────────
//  Layout chrome
// ─────────────────────────────────────────────

void drawLayout(const LayoutGeometry& geo) {
    drawHorizontalLine(0, geo.textH, geo.textW);
    drawVerticalLine(geo.textW, 0, geo.rows);

    gotoxy(2, 0);                cout << " ConsolePad ";
    gotoxy(2, geo.textH + 1);   cout << " Suggestions ";
    gotoxy(geo.textW + 2, 0);   cout << " Search ";
    gotoxy(geo.textW + 2, 1);   cout << " Ctrl+F to search ";
}

// ─────────────────────────────────────────────
//  Panel clear helpers
// ─────────────────────────────────────────────

void clearSearchPanel(const LayoutGeometry& geo) {
    int panelW = geo.cols - geo.searchX;
    for (int y = 1; y < geo.textH; y++) {
        gotoxy(geo.searchX + 1, y);
        for (int x = 0; x < panelW - 1; x++) cout << ' ';
    }
}

void clearSuggestPanel(const LayoutGeometry& geo) {
    for (int y = geo.suggestY + 1; y < geo.rows - 1; y++) {
        gotoxy(1, y);
        for (int x = 1; x < geo.textW; x++) cout << ' ';
    }
}

// ─────────────────────────────────────────────
//  Search panel
// ─────────────────────────────────────────────

void showSearchResults(const MatchList& matches, const char* query,
                       const LayoutGeometry& geo) {
    clearSearchPanel(geo);

    int panelW = geo.cols - geo.searchX - 1;
    int y = 2;

    gotoxy(geo.searchX + 1, y++);
    cout << "Query: ";
    for (int qi = 0; query[qi] && qi < panelW - 8; qi++)
        cout << query[qi];

    gotoxy(geo.searchX + 1, y++);
    cout << "Found: " << matches.count;

    y++;  // blank line

    bool shownRows[512] = {};
    for (WordMatch* m = matches.head; m && y < geo.textH - 1; m = m->next) {
        int lineNum = m->row + 1;
        if (lineNum < 512 && !shownRows[lineNum]) {
            shownRows[lineNum] = true;
            gotoxy(geo.searchX + 1, y++);
            cout << "  Line " << lineNum << ", col " << (m->col + 1);
        }
    }
}

// ─────────────────────────────────────────────
//  Suggestion panel
// ─────────────────────────────────────────────

void showSuggestions(char suggestions[][MAX_WORD_LEN], int count,
                     const LayoutGeometry& geo, int mode) {
    clearSuggestPanel(geo);

    gotoxy(2, geo.textH + 2);
    cout << (mode == 0 ? "[Word completion] " : "[Next-word prediction] ");

    if (count == 0) {
        cout << "No suggestions found.";
        return;
    }

    cout << "Press 1-" << count << " to insert:";

    int y = geo.textH + 3;
    for (int i = 0; i < count && y < geo.rows - 1; i++, y++) {
        gotoxy(2, y);
        cout << "  " << (i + 1) << ") " << suggestions[i];
    }
}

// ─────────────────────────────────────────────
//  Status bar
// ─────────────────────────────────────────────

void setStatus(const char* msg, const LayoutGeometry& geo) {
    gotoxy(1, geo.textH + 1);
    int i = 0;
    while (msg[i] && i < geo.textW - 2) cout << msg[i++];
    while (i < geo.textW - 2) { cout << ' '; i++; }
}

// ─────────────────────────────────────────────
//  Highlight application
// ─────────────────────────────────────────────

void applyHighlights(const MatchList& matches, int queryLen, Node* docHead) {
    clearHighlights(docHead);
    for (WordMatch* m = matches.head; m; m = m->next) {
        Node* n = m->startNode;
        for (int i = 0; i < queryLen && n && n->data != '\n'; i++) {
            n->highlighted = true;
            n = n->right;
        }
    }
}
