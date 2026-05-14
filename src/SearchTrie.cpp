/*
 * SearchTrie.cpp
 * --------------
 * Implements SearchTrie, Occurrence, OccurrenceList,
 * TrieNode, WordMatch, and MatchList (declared in SearchTrie.h).
 */
#define _CRT_SECURE_NO_WARNINGS

#include "../include/SearchTrie.h"
#include <cctype>
#include <cstring>

// ─────────────────────────────────────────────
//  Occurrence
// ─────────────────────────────────────────────

Occurrence::Occurrence(Node* n, int r)
    : docNode(n), row(r), next(nullptr) {}

// ─────────────────────────────────────────────
//  OccurrenceList
// ─────────────────────────────────────────────

OccurrenceList::OccurrenceList() : head(nullptr), count(0) {}

OccurrenceList::~OccurrenceList() { clear(); }

void OccurrenceList::add(Node* docNode, int row) {
    Occurrence* occ = new Occurrence(docNode, row);
    occ->next = head;
    head      = occ;
    count++;
}

void OccurrenceList::clear() {
    Occurrence* cur = head;
    while (cur) {
        Occurrence* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head  = nullptr;
    count = 0;
}

// ─────────────────────────────────────────────
//  TrieNode
// ─────────────────────────────────────────────

TrieNode::TrieNode(char c) : isWordEnd(false), letter(c) {
    for (int i = 0; i < 26; i++) children[i] = nullptr;
}

TrieNode::~TrieNode() {
    for (int i = 0; i < 26; i++) {
        delete children[i];
        children[i] = nullptr;
    }
}

// ─────────────────────────────────────────────
//  WordMatch
// ─────────────────────────────────────────────

WordMatch::WordMatch(Node* n, int r, int c)
    : startNode(n), row(r), col(c), next(nullptr) {}

// ─────────────────────────────────────────────
//  MatchList
// ─────────────────────────────────────────────

MatchList::MatchList() : head(nullptr), count(0) {}

MatchList::~MatchList() { clear(); }

void MatchList::add(Node* n, int row, int col) {
    WordMatch* m = new WordMatch(n, row, col);
    m->next = head;
    head    = m;
    count++;
}

void MatchList::clear() {
    WordMatch* cur = head;
    while (cur) {
        WordMatch* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head  = nullptr;
    count = 0;
}

// ─────────────────────────────────────────────
//  SearchTrie
// ─────────────────────────────────────────────

SearchTrie::SearchTrie() : root(new TrieNode('\0')) {}

SearchTrie::~SearchTrie() { delete root; }

// ── Rebuild ───────────────────────────────────
void SearchTrie::rebuild(Node* docHead) {
    delete root;
    root = new TrieNode('\0');
    if (!docHead) return;

    Node* rowPtr = docHead;
    int   row    = 0;

    while (rowPtr) {
        Node* col        = rowPtr->right;
        int   wLen       = 0;
        char  word[MAX_QUERY_LEN];
        Node* wordNodes[MAX_QUERY_LEN];

        while (true) {
            bool isAlpha = col && isalpha(static_cast<unsigned char>(col->data));

            if (isAlpha) {
                if (wLen < MAX_QUERY_LEN - 1) {
                    wordNodes[wLen] = col;
                    word[wLen++]    = static_cast<char>(
                        tolower(static_cast<unsigned char>(col->data)));
                }
            } else {
                if (wLen > 0) {
                    word[wLen] = '\0';
                    insertWord(word, wordNodes, wLen, row);
                    wLen = 0;
                }
            }

            if (!col) break;
            col = col->right;
        }

        rowPtr = rowPtr->down;
        row++;
    }
}

// ── Search ────────────────────────────────────
int SearchTrie::search(const char* query, Node* docHead, MatchList& out) const {
    if (!query || query[0] == '\0') return 0;

    // Lowercase copy
    char lq[MAX_QUERY_LEN * 2];
    int  qi = 0;
    while (query[qi] && qi < static_cast<int>(sizeof(lq)) - 1) {
        lq[qi] = static_cast<char>(tolower(static_cast<unsigned char>(query[qi])));
        qi++;
    }
    lq[qi] = '\0';

    // Split into words
    char words[MAX_PHRASE_WORDS][MAX_QUERY_LEN];
    int  wordCount = 0;
    int  i         = 0;

    while (lq[i] && wordCount < MAX_PHRASE_WORDS) {
        while (lq[i] == ' ') i++;
        if (!lq[i]) break;
        int j = 0;
        while (lq[i] && lq[i] != ' ' && j < MAX_QUERY_LEN - 1)
            words[wordCount][j++] = lq[i++];
        words[wordCount][j] = '\0';
        if (j > 0) wordCount++;
    }
    if (wordCount == 0) return 0;

    TrieNode* node = findNode(words[0]);
    if (!node || !node->isWordEnd) return 0;

    int found = 0;

    for (Occurrence* occ = node->occurrences.head; occ; occ = occ->next) {
        // occ->docNode is the LAST character of words[0].
        // Walk backwards to find the start.
        Node* endOfFirst   = occ->docNode;
        Node* startOfFirst = endOfFirst;
        for (int w = 1; w < static_cast<int>(strlen(words[0])); w++) {
            if (startOfFirst->left && startOfFirst->left->data != '\n')
                startOfFirst = startOfFirst->left;
        }

        bool  match = true;
        Node* cur   = endOfFirst;

        for (int w = 1; w < wordCount && match; w++) {
            Node* nxt = cur->right;
            while (nxt && !isalpha(static_cast<unsigned char>(nxt->data))) {
                if (!nxt->right) { nxt = nullptr; break; }
                nxt = nxt->right;
            }
            if (!nxt) { match = false; break; }

            int   wl = 0;
            Node* wc = nxt;
            bool  ok = true;
            while (words[w][wl] != '\0') {
                if (!wc || tolower(static_cast<unsigned char>(wc->data)) != words[w][wl]) {
                    ok = false; break;
                }
                wc = wc->right;
                wl++;
            }
            if (ok && wc && isalpha(static_cast<unsigned char>(wc->data))) ok = false;
            if (!ok) { match = false; break; }
            cur = (wc && wc->left) ? wc->left : cur;
        }

        if (match) {
            int   col = 0;
            Node* tmp = startOfFirst;
            while (tmp->left && tmp->left->data == '\n') break;
            while (tmp->left && tmp->left->data != '\n') { tmp = tmp->left; col++; }
            out.add(startOfFirst, occ->row, col);
            found++;
        }
    }

    return found;
}

// ── Prefix completion ─────────────────────────
int SearchTrie::complete(const char* prefix,
                         char results[][MAX_WORD_LEN], int maxResults) const {
    if (!prefix || prefix[0] == '\0' || maxResults <= 0) return 0;

    char lp[MAX_WORD_LEN];
    int  pi = 0;
    while (prefix[pi] && pi < MAX_WORD_LEN - 1) {
        lp[pi] = static_cast<char>(tolower(static_cast<unsigned char>(prefix[pi])));
        pi++;
    }
    lp[pi] = '\0';

    TrieNode* node = findNode(lp);
    if (!node) return 0;

    int  found = 0;
    char buf[MAX_QUERY_LEN];
    strncpy(buf, lp, MAX_QUERY_LEN - 1);
    buf[MAX_QUERY_LEN - 1] = '\0';
    collectWords(node, buf, static_cast<int>(strlen(buf)), results, maxResults, found);
    return found;
}

// ── All words ─────────────────────────────────
int SearchTrie::allWords(char results[][MAX_WORD_LEN], int maxResults) const {
    if (maxResults <= 0) return 0;
    int  found = 0;
    char buf[MAX_QUERY_LEN];
    collectWords(root, buf, 0, results, maxResults, found);
    return found;
}

// ── Private: insertWord ───────────────────────
void SearchTrie::insertWord(const char* word, Node** wordNodes, int len, int row) {
    TrieNode* cur = root;
    for (int i = 0; i < len; i++) {
        int idx = word[i] - 'a';
        if (idx < 0 || idx >= 26) return;
        if (!cur->children[idx])
            cur->children[idx] = new TrieNode(word[i]);
        cur = cur->children[idx];
    }
    cur->isWordEnd = true;
    cur->occurrences.add(wordNodes[len - 1], row);
}

// ── Private: findNode ─────────────────────────
TrieNode* SearchTrie::findNode(const char* word) const {
    TrieNode* cur = root;
    for (int i = 0; word[i]; i++) {
        int idx = static_cast<int>(tolower(static_cast<unsigned char>(word[i]))) - 'a';
        if (idx < 0 || idx >= 26 || !cur->children[idx]) return nullptr;
        cur = cur->children[idx];
    }
    return cur;
}

// ── Private: collectWords (DFS) ───────────────
void SearchTrie::collectWords(TrieNode* node, char* buf, int depth,
                               char results[][MAX_WORD_LEN],
                               int maxResults, int& found) const {
    if (found >= maxResults) return;
    if (node->isWordEnd) {
        buf[depth] = '\0';
        strncpy(results[found], buf, MAX_WORD_LEN - 1);
        results[found][MAX_WORD_LEN - 1] = '\0';
        found++;
    }
    for (int i = 0; i < 26 && found < maxResults; i++) {
        if (node->children[i]) {
            buf[depth] = 'a' + i;
            collectWords(node->children[i], buf, depth + 1, results, maxResults, found);
        }
    }
}
