#pragma once

/*
 * UI.h
 * ----
 * All console rendering and panel management for ConsolePad.
 *
 * Responsibilities:
 *   - Console primitive helpers (gotoxy, colour, line drawing)
 *   - Fixed chrome / layout drawing
 *   - Search panel rendering
 *   - Suggestion panel rendering
 *   - Status bar
 *   - Match highlighting
 *
 * Implementation lives in UI.cpp.
 */

#include "Node.h"
#include "SearchTrie.h"
#include <Windows.h>

// ─────────────────────────────────────────────
//  Layout geometry  (set once in main, read by UI functions)
// ─────────────────────────────────────────────
struct LayoutGeometry {
    int cols;
    int rows;
    int textW;
    int textH;
    int searchX;    // X where the search panel starts
    int suggestY;   // Y where the suggestions panel starts
};

// ─────────────────────────────────────────────
//  Console primitives  (also used by Node.h consumers)
// ─────────────────────────────────────────────
void gotoxy(int x, int y);
void getConsoleSize(int& cols, int& rows);
void drawHorizontalLine(int sx, int sy, int len);
void drawVerticalLine(int sx, int sy, int len);
void setHighlightColor();
void resetTextColor();
void clearHighlights(Node* docHead);

// ─────────────────────────────────────────────
//  Layout chrome
// ─────────────────────────────────────────────
void drawLayout(const LayoutGeometry& geo);

// ─────────────────────────────────────────────
//  Panel clear helpers
// ─────────────────────────────────────────────
void clearSearchPanel (const LayoutGeometry& geo);
void clearSuggestPanel(const LayoutGeometry& geo);

// ─────────────────────────────────────────────
//  Search panel
// ─────────────────────────────────────────────
void showSearchResults(const MatchList& matches, const char* query,
                       const LayoutGeometry& geo);

// ─────────────────────────────────────────────
//  Suggestion panel
//   mode: 0 = word completion, 1 = next-word prediction
// ─────────────────────────────────────────────
void showSuggestions(char suggestions[][MAX_WORD_LEN], int count,
                     const LayoutGeometry& geo, int mode);

// ─────────────────────────────────────────────
//  Status bar
// ─────────────────────────────────────────────
void setStatus(const char* msg, const LayoutGeometry& geo);

// ─────────────────────────────────────────────
//  Highlight application
// ─────────────────────────────────────────────
void applyHighlights(const MatchList& matches, int queryLen, Node* docHead);
