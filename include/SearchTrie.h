#pragma once

/*
 * SearchTrie.h
 * ------------
 * N-Ary Trie (max 26 children per node, one per alphabet letter).
 *
 * Responsibilities:
 *   - Index every word in the 2-D linked-list document.
 *   - Search for a word / multi-word phrase.
 *   - Support word-prefix completion for the '@' trigger.
 *
 * Complexity:
 *   - insert  : O(k)   k = word length
 *   - search  : O(k) to reach terminal node, then iterate occurrences
 *   - prefix  : O(k) to reach prefix node, then DFS-collect words below
 *
 * Implementation lives in SearchTrie.cpp.
 */

#include "Node.h"

// ─────────────────────────────────────────────
//  Occurrence  (a single location in the document)
// ─────────────────────────────────────────────
struct Occurrence {
    Node*       docNode;   // pointer into the 2-D linked list (last char of word)
    int         row;       // 0-based line number
    Occurrence* next;

    Occurrence(Node* n, int r);
};

// ─────────────────────────────────────────────
//  OccurrenceList  (singly-linked, owns memory)
// ─────────────────────────────────────────────
struct OccurrenceList {
    Occurrence* head;
    int         count;

    OccurrenceList();
    ~OccurrenceList();

    void add(Node* docNode, int row);
    void clear();
};

// ─────────────────────────────────────────────
//  TrieNode
// ─────────────────────────────────────────────
struct TrieNode {
    TrieNode*      children[26];
    OccurrenceList occurrences;  // populated only at word-end nodes
    bool           isWordEnd;
    char           letter;

    explicit TrieNode(char c = '\0');
    ~TrieNode();
};

// ─────────────────────────────────────────────
//  WordMatch  (result returned to caller)
// ─────────────────────────────────────────────
struct WordMatch {
    Node*      startNode;
    int        row;
    int        col;
    WordMatch* next;

    WordMatch(Node* n, int r, int c);
};

// ─────────────────────────────────────────────
//  MatchList  (owning list of WordMatch)
// ─────────────────────────────────────────────
struct MatchList {
    WordMatch* head;
    int        count;

    MatchList();
    ~MatchList();

    void add(Node* n, int row, int col);
    void clear();
};

// ─────────────────────────────────────────────
//  SearchTrie
// ─────────────────────────────────────────────
class SearchTrie {
public:
    SearchTrie();
    ~SearchTrie();

    // Re-index the entire document.  Call after edits.
    void rebuild(Node* docHead);

    // Search for a word or space-separated phrase.
    // Appends results to 'out'; returns match count.
    int search(const char* query, Node* docHead, MatchList& out) const;

    // Fill 'results' with words that start with 'prefix'.
    // Returns suggestion count (capped at maxResults).
    int complete(const char* prefix, char results[][MAX_WORD_LEN], int maxResults) const;

    // Fill 'results' with all words in the trie (DFS from root).
    int allWords(char results[][MAX_WORD_LEN], int maxResults) const;

private:
    TrieNode* root;

    void      insertWord(const char* word, Node** wordNodes, int len, int row);
    TrieNode* findNode(const char* word) const;
    void      collectWords(TrieNode* node, char* buf, int depth,
                           char results[][MAX_WORD_LEN], int maxResults, int& found) const;
};
