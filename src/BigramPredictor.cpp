/*
 * BigramPredictor.cpp
 * -------------------
 * Implements BigramPredictor and its supporting types
 * (declared in BigramPredictor.h).
 *
 * Originally named "ChilliMilliTree" in the course assignment (CS218).
 */
#define _CRT_SECURE_NO_WARNINGS

#include "../include/BigramPredictor.h"
#include <cctype>
#include <cstring>

// ─────────────────────────────────────────────
//  Follower
// ─────────────────────────────────────────────

Follower::Follower(const char* w, int f) : freq(f), next(nullptr) {
    strncpy(word, w, MAX_WORD_LEN - 1);
    word[MAX_WORD_LEN - 1] = '\0';
}

// ─────────────────────────────────────────────
//  FollowerList
// ─────────────────────────────────────────────

FollowerList::FollowerList() : head(nullptr) {}

FollowerList::~FollowerList() { clear(); }

void FollowerList::add(const char* word) {
    for (Follower* f = head; f; f = f->next) {
        if (strcmp(f->word, word) == 0) { f->freq++; return; }
    }
    Follower* f = new Follower(word);
    f->next = head;
    head    = f;
}

int FollowerList::topN(char results[][MAX_WORD_LEN], int maxN) const {
    // Count followers so we can bound the 'used' array safely.
    int total = 0;
    for (Follower* f = head; f; f = f->next) total++;
    if (total == 0) return 0;

    // Simple O(f * maxN) selection — follower lists are tiny in practice.
    static const int MAX_FOLLOWERS = 128;
    bool used[MAX_FOLLOWERS] = {};
    int  found = 0;

    while (found < maxN) {
        Follower* best    = nullptr;
        int       bestIdx = 0;
        int       idx     = 0;
        for (Follower* f = head; f; f = f->next, idx++) {
            if (idx >= MAX_FOLLOWERS) break;
            if (!used[idx] && (!best || f->freq > best->freq)) {
                best    = f;
                bestIdx = idx;
            }
        }
        if (!best) break;
        strncpy(results[found], best->word, MAX_WORD_LEN - 1);
        results[found][MAX_WORD_LEN - 1] = '\0';
        used[bestIdx] = true;
        found++;
    }
    return found;
}

void FollowerList::clear() {
    Follower* cur = head;
    while (cur) {
        Follower* nxt = cur->next;
        delete cur;
        cur = nxt;
    }
    head = nullptr;
}

// ─────────────────────────────────────────────
//  BigramTrieNode
// ─────────────────────────────────────────────

BigramTrieNode::BigramTrieNode() : isWordEnd(false) {
    for (int i = 0; i < 26; i++) children[i] = nullptr;
}

BigramTrieNode::~BigramTrieNode() {
    for (int i = 0; i < 26; i++) {
        delete children[i];
        children[i] = nullptr;
    }
}

// ─────────────────────────────────────────────
//  BigramPredictor
// ─────────────────────────────────────────────

BigramPredictor::BigramPredictor() : root(new BigramTrieNode()) {}

BigramPredictor::~BigramPredictor() { delete root; }

// ── Rebuild ───────────────────────────────────
void BigramPredictor::rebuild(Node* docHead) {
    delete root;
    root = new BigramTrieNode();
    if (!docHead) return;

    char prev[MAX_WORD_LEN] = {};
    char cur[MAX_WORD_LEN]  = {};
    bool hasPrev             = false;

    Node* rowPtr = docHead;
    while (rowPtr) {
        Node* col  = rowPtr->right;
        int   wLen = 0;

        while (true) {
            bool isAlpha = col && isalpha(static_cast<unsigned char>(col->data));

            if (isAlpha) {
                if (wLen < MAX_WORD_LEN - 1)
                    cur[wLen++] = static_cast<char>(
                        tolower(static_cast<unsigned char>(col->data)));
            } else {
                if (wLen > 0) {
                    cur[wLen] = '\0';
                    if (hasPrev) insertBigram(prev, cur);
                    strncpy(prev, cur, MAX_WORD_LEN - 1);
                    prev[MAX_WORD_LEN - 1] = '\0';
                    hasPrev = true;
                    wLen    = 0;
                }
            }

            if (!col) break;
            col = col->right;
        }

        rowPtr = rowPtr->down;
    }
}

// ── Suggest ───────────────────────────────────
int BigramPredictor::suggest(const char* contextWord,
                              char results[][MAX_WORD_LEN], int maxN) const {
    if (!contextWord || contextWord[0] == '\0') return 0;

    char lw[MAX_WORD_LEN];
    int  i = 0;
    while (contextWord[i] && i < MAX_WORD_LEN - 1) {
        lw[i] = static_cast<char>(tolower(static_cast<unsigned char>(contextWord[i])));
        i++;
    }
    lw[i] = '\0';

    BigramTrieNode* node = findNode(lw);
    if (!node || !node->isWordEnd) return 0;
    return node->followers.topN(results, maxN);
}

// ── Private: insertBigram ─────────────────────
void BigramPredictor::insertBigram(const char* contextWord, const char* follower) {
    BigramTrieNode* cur = root;
    for (int i = 0; contextWord[i]; i++) {
        int idx = contextWord[i] - 'a';
        if (idx < 0 || idx >= 26) return;
        if (!cur->children[idx])
            cur->children[idx] = new BigramTrieNode();
        cur = cur->children[idx];
    }
    cur->isWordEnd = true;
    cur->followers.add(follower);
}

// ── Private: findNode ─────────────────────────
BigramTrieNode* BigramPredictor::findNode(const char* word) const {
    BigramTrieNode* cur = root;
    for (int i = 0; word[i]; i++) {
        int idx = static_cast<int>(tolower(static_cast<unsigned char>(word[i]))) - 'a';
        if (idx < 0 || idx >= 26 || !cur->children[idx]) return nullptr;
        cur = cur->children[idx];
    }
    return cur;
}
