/*
 * Node.cpp
 * --------
 * Implements UndoStack (declared in Node.h).
 */
#define _CRT_SECURE_NO_WARNINGS

#include "../include/Node.h"

// ─────────────────────────────────────────────
//  UndoStack
// ─────────────────────────────────────────────

UndoStack::UndoStack() : topNode(nullptr), sz(0) {}

UndoStack::~UndoStack() { clear(); }

bool UndoStack::isEmpty() const { return sz == 0; }

void UndoStack::push(const UndoRecord& rec) {
    if (sz == MAX_UNDO_DEPTH) removeBottom();
    UndoRecord* n = new UndoRecord(rec);
    n->next = topNode;
    topNode = n;
    sz++;
}

bool UndoStack::pop(UndoRecord& out) {
    if (isEmpty()) return false;
    out = *topNode;
    UndoRecord* tmp = topNode;
    topNode = topNode->next;
    delete tmp;
    sz--;
    return true;
}

void UndoStack::clear() {
    while (topNode) {
        UndoRecord* tmp = topNode;
        topNode = topNode->next;
        delete tmp;
    }
    sz = 0;
}

void UndoStack::removeBottom() {
    if (!topNode) return;
    if (!topNode->next) {
        delete topNode;
        topNode = nullptr;
        sz = 0;
        return;
    }
    UndoRecord* prev = topNode;
    while (prev->next && prev->next->next) prev = prev->next;
    delete prev->next;
    prev->next = nullptr;
    sz--;
}
