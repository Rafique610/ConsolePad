/*
 * Main.cpp
 * --------
 * ConsolePad — a console text editor with trie-powered search and prediction.
 *
 * Key Bindings:
 *   Ctrl+S   Save file
 *   Ctrl+Z   Undo  (up to 50 operations)
 *   Ctrl+Y   Redo  (up to 50 operations)
 *   Ctrl+F   Search  (highlights all matches, shows line numbers)
 *   Arrows   Move cursor
 *   Backspace Delete character to the left / merge lines
 *   Enter    New line
 *   Escape   Exit  (prompts to save)
 *   @        Word-completion trigger  (bigram suggestions from prior context)
 *   *        Next-word prediction trigger  (same bigram model)
 *   1–9      Accept suggestion from the panel
 *   Printable Insert character
 *
 * Layout:
 *   ~80% W × ~80% H  Main text area
 *   Right ~20%        Search panel
 *   Bottom ~20%       Suggestions panel
 */

#define _CRT_SECURE_NO_WARNINGS

#include "../include/Notepad.h"
#include "../include/SearchTrie.h"
#include "../include/BigramPredictor.h"
#include "../include/UI.h"
#include <cstring>
#include <iostream>
#include <conio.h>
using std::cout;
using std::cin;

// ─────────────────────────────────────────────
//  Start-up menu
// ─────────────────────────────────────────────
static void displayMenu() {
    system("cls");
    cout << "===== ConsolePad =====\n";
    cout << "1. New file\n";
    cout << "2. Open existing file\n";
    cout << "Enter choice: ";
}

// ─────────────────────────────────────────────
//  main
// ─────────────────────────────────────────────
int main() {
    system("cls");

    // ── Geometry ────────────────────────────────
    LayoutGeometry geo{};
    getConsoleSize(geo.cols, geo.rows);
    geo.textH   = static_cast<int>(geo.rows * 0.80);
    geo.textW   = static_cast<int>(geo.cols * 0.80);
    geo.searchX  = geo.textW;
    geo.suggestY = geo.textH;

    // ── Start-up choice ──────────────────────────
    displayMenu();
    int choice = 0;
    cin >> choice;

    static char filename[MAX_QUERY_LEN] = { '\0' };

    Notepad        notepad(geo.textH, geo.textW);
    SearchTrie     searchTrie;
    BigramPredictor bigramPredictor;

    if (choice == 2) {
        system("cls");
        cout << "Enter filename: ";
        cin >> filename;
        notepad.loadFromFile(filename);
        searchTrie.rebuild(notepad.head);
        bigramPredictor.rebuild(notepad.head);
    }

    system("cls");
    drawLayout(geo);
    notepad.display();

    COORD pos = notepad.getCursorPos();
    gotoxy(pos.X, pos.Y);

    // ── Suggestion state ─────────────────────────
    char suggestions[MAX_SUGGESTIONS][MAX_WORD_LEN];
    int  suggCount              = 0;
    int  suggMode               = 0;   // 0 = word completion, 1 = next-word
    bool hasSugg                = false;
    bool suggNeedsLeadingSpace  = false;

    // ── Undo / redo stacks ───────────────────────
    UndoStack undoStack, redoStack;

    // ── Lazy trie rebuild ────────────────────────
    int  editsSinceRebuild = 0;
    bool inWord            = false;

    auto rebuildTries = [&]() {
        searchTrie.rebuild(notepad.head);
        bigramPredictor.rebuild(notepad.head);
        editsSinceRebuild = 0;
    };

    // ── Input loop ───────────────────────────────
    HANDLE rhnd   = GetStdHandle(STD_INPUT_HANDLE);
    bool   running = true;

    while (running) {
        DWORD events = 0;
        GetNumberOfConsoleInputEvents(rhnd, &events);
        if (events == 0) continue;

        INPUT_RECORD buf[256];
        DWORD        readCount = 0;
        ReadConsoleInput(rhnd, buf, min(static_cast<DWORD>(256), events), &readCount);

        for (DWORD i = 0; i < readCount; ++i) {
            if (buf[i].EventType != KEY_EVENT) continue;
            KEY_EVENT_RECORD& ke = buf[i].Event.KeyEvent;
            if (!ke.bKeyDown) continue;

            bool ctrl = (ke.dwControlKeyState &
                         (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) != 0;

            // ── Ctrl combos ──────────────────────
            if (ctrl) {
                switch (ke.wVirtualKeyCode) {

                case 'S': {   // Save
                    if (filename[0] == '\0') {
                        const char* def = "untitled.txt";
                        for (int k = 0; def[k]; k++) filename[k] = def[k];
                        filename[12] = '\0';
                    }
                    notepad.saveToFile(filename);
                    setStatus("Saved.", geo);
                    break;
                }

                case 'Z': {   // Undo
                    UndoRecord rec;
                    if (undoStack.pop(rec)) {
                        redoStack.push(rec);
                        int row = notepad.undoOperation(rec);
                        if (row < 0 || rec.op == OP_NEWLINE || rec.op == OP_MERGE)
                            notepad.displayFrom((row >= 0) ? row : 0);
                        else
                            notepad.displayRow(row);
                        inWord = false;
                        rebuildTries();
                    }
                    break;
                }

                case 'Y': {   // Redo
                    UndoRecord rec;
                    if (redoStack.pop(rec)) {
                        undoStack.push(rec);
                        int row = notepad.redoOperation(rec);
                        if (row < 0 || rec.op == OP_NEWLINE || rec.op == OP_MERGE)
                            notepad.displayFrom((row >= 0) ? row : 0);
                        else
                            notepad.displayRow(row);
                        inWord = false;
                        rebuildTries();
                    }
                    break;
                }

                case 'F': {   // Search
                    clearSearchPanel(geo);
                    gotoxy(geo.searchX + 1, 2);
                    cout << "Search: ";
                    gotoxy(geo.searchX + 1, 3);

                    char query[MAX_QUERY_LEN] = {};
                    int  qi = 0;
                    SetConsoleMode(rhnd, ENABLE_PROCESSED_INPUT);
                    char qc;
                    while ((qc = static_cast<char>(_getch())) != '\r' &&
                           qi < MAX_QUERY_LEN - 1) {
                        if (qc == 27) { qi = 0; break; }
                        if (qc == '\b') {
                            if (qi > 0) { qi--; cout << "\b \b"; }
                        } else {
                            query[qi++] = qc;
                            cout << qc;
                        }
                    }
                    query[qi] = '\0';
                    SetConsoleMode(rhnd, ENABLE_PROCESSED_INPUT |
                                         ENABLE_MOUSE_INPUT |
                                         ENABLE_WINDOW_INPUT);

                    if (qi > 0) {
                        clearHighlights(notepad.head);
                        MatchList matches;
                        int found = searchTrie.search(query, notepad.head, matches);
                        if (found > 0)
                            applyHighlights(matches, static_cast<int>(strlen(query)),
                                            notepad.head);
                        notepad.display();
                        showSearchResults(matches, query, geo);
                    }
                    break;
                }

                } // end ctrl switch

                pos = notepad.getCursorPos();
                gotoxy(pos.X, pos.Y);
                continue;
            } // end ctrl block

            // ── Suggestion selection (1–9) ────────
            if (hasSugg && ke.wVirtualKeyCode >= '1' && ke.wVirtualKeyCode <= '9') {
                int idx = ke.wVirtualKeyCode - '1';
                if (idx < suggCount) {
                    if (suggNeedsLeadingSpace) {
                        UndoRecord rec;
                        notepad.insertChar(' ', &rec);
                        undoStack.push(rec); redoStack.clear();
                    }
                    notepad.insertWord(suggestions[idx]);
                    UndoRecord rec;
                    notepad.insertChar(' ', &rec);
                    undoStack.push(rec); redoStack.clear();

                    notepad.displayFrom(0);
                    clearSuggestPanel(geo);
                    hasSugg               = false;
                    suggNeedsLeadingSpace = false;
                    rebuildTries();
                }
                pos = notepad.getCursorPos();
                gotoxy(pos.X, pos.Y);
                continue;
            }

            // ── Regular keys ─────────────────────
            switch (ke.wVirtualKeyCode) {

            case VK_ESCAPE:
                system("cls");
                notepad.promptSaveOnExit(filename);
                running = false;
                break;

            case VK_BACK: {
                if (notepad.cursor) {
                    UndoRecord rec;
                    notepad.deleteChar(&rec);
                    undoStack.push(rec);
                    redoStack.clear();
                    if (rec.op == OP_MERGE)
                        notepad.displayFrom(rec.cursorRow - 1);
                    else
                        notepad.displayRow(rec.cursorRow);
                    inWord = false;
                    if (++editsSinceRebuild >= 10) rebuildTries();
                }
                break;
            }

            case VK_RETURN: {
                UndoRecord rec;
                notepad.newLine(&rec);
                undoStack.push(rec);
                redoStack.clear();
                notepad.displayFrom(rec.cursorRow);
                inWord = false;
                rebuildTries();
                break;
            }

            case VK_RIGHT: notepad.moveCursorRight(); break;
            case VK_LEFT:  notepad.moveCursorLeft();  break;
            case VK_UP:    notepad.moveCursorUp();    break;
            case VK_DOWN:  notepad.moveCursorDown();  break;

            default: {
                char ch = ke.uChar.AsciiChar;
                if (ch < 32 || ch > 126) break;

                // ── @ : word-completion trigger ──
                if (ch == '@') {
                    char lastWord[MAX_WORD_LEN];
                    notepad.getWordBeforeCursor(lastWord, MAX_WORD_LEN);
                    suggCount = (lastWord[0] != '\0')
                        ? bigramPredictor.suggest(lastWord, suggestions, MAX_SUGGESTIONS)
                        : 0;
                    hasSugg               = (suggCount > 0);
                    suggMode              = 0;
                    suggNeedsLeadingSpace = notepad.cursor &&
                        isalpha(static_cast<unsigned char>(notepad.cursor->data));
                    showSuggestions(suggestions, suggCount, geo, suggMode);
                    break;
                }

                // ── * : next-word prediction trigger ──
                if (ch == '*') {
                    char lastWord[MAX_WORD_LEN];
                    notepad.getWordBeforeCursor(lastWord, MAX_WORD_LEN);
                    suggCount = (lastWord[0] != '\0')
                        ? bigramPredictor.suggest(lastWord, suggestions, MAX_SUGGESTIONS)
                        : 0;
                    hasSugg               = (suggCount > 0);
                    suggMode              = 1;
                    suggNeedsLeadingSpace = false;
                    showSuggestions(suggestions, suggCount, geo, suggMode);
                    break;
                }

                // ── Normal character ─────────────
                bool isAlpha  = (isalpha(static_cast<unsigned char>(ch)) != 0);
                bool wasInWord = inWord;
                inWord        = isAlpha;

                UndoRecord rec;
                if (notepad.insertChar(ch, &rec)) {
                    undoStack.push(rec);
                    redoStack.clear();
                    notepad.displayRow(rec.cursorRow);
                    editsSinceRebuild++;

                    if (!isAlpha && wasInWord)       rebuildTries();
                    else if (editsSinceRebuild >= 20) rebuildTries();
                }
                break;
            }
            } // end regular-key switch

            pos = notepad.getCursorPos();
            gotoxy(pos.X, pos.Y);
        } // end event loop
    } // end while running

    return 0;
}
