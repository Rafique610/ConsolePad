#pragma once

/*
 * BigramPredictor.h
 * -----------------
 * Bigram-frequency trie for next-word prediction.
 *
 * Concept:
 *   For every consecutive word pair (A, B) observed in the document,
 *   we record that B follows A.  On a '*' or '@' trigger the editor
 *   queries the last typed word and receives the top-N most-frequent
 *   successors as ranked suggestions.
 *
 * Structure:
 *   - Outer N-Ary Trie keyed on the context word A.
 *   - Each terminal node holds a FollowerList of (word-B, frequency) pairs.
 *   - topN() walks the FollowerList and returns results sorted by frequency.
 *
 * Complexity:
 *   - build  : O(n)  — one pass over all consecutive word pairs.
 *   - lookup : O(k + f)  k = context-word length, f = follower-list size.
 *
 * Implementation lives in BigramPredictor.cpp.
 *
 * Note: originally named "ChilliMilliTree" in the course assignment.
 */

#include "Node.h"
#include <cstring>

// ─────────────────────────────────────────────
//  Follower  (a word that follows some context word)
// ─────────────────────────────────────────────
struct Follower {
    char      word[MAX_WORD_LEN];
    int       freq;
    Follower* next;

    explicit Follower(const char* w, int f = 1);
};

// ─────────────────────────────────────────────
//  FollowerList
// ─────────────────────────────────────────────
struct FollowerList {
    Follower* head;

    FollowerList();
    ~FollowerList();

    void add(const char* word);
    int  topN(char results[][MAX_WORD_LEN], int maxN) const;
    void clear();
};

// ─────────────────────────────────────────────
//  BigramTrieNode
// ─────────────────────────────────────────────
struct BigramTrieNode {
    BigramTrieNode* children[26];
    FollowerList    followers;   // non-empty only at isWordEnd nodes
    bool            isWordEnd;

    BigramTrieNode();
    ~BigramTrieNode();
};

// ─────────────────────────────────────────────
//  BigramPredictor
// ─────────────────────────────────────────────
class BigramPredictor {
public:
    BigramPredictor();
    ~BigramPredictor();

    // One pass through the document; builds bigram index from scratch.
    void rebuild(Node* docHead);

    // Return up to maxN predicted next-words for 'contextWord'.
    int suggest(const char* contextWord,
                char results[][MAX_WORD_LEN], int maxN) const;

private:
    BigramTrieNode* root;

    void            insertBigram(const char* contextWord, const char* follower);
    BigramTrieNode* findNode(const char* word) const;
};
