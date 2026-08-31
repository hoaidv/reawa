#pragma once
/**
 * Session undo / redo rings. Not mixed into DocNode.
 * @implements [SRS-EP-07] undo ring depth 20
 * @implements [SRS-EP-13] F20 skip-whole; F21 absence-partial
 */

#include "doc_edit.hpp"

#include <deque>
#include <memory>
#include <vector>

namespace epaper {
namespace document {

/**
 * Inverse of one committed gesture — not a whole-tree snapshot.
 * @implements [SRS-EP-09] undo ring entry { forwardOpId, seq, inverses, targets }
 */
struct UndoRingEntry {
    std::string forwardOpId;
    int seq = 0;
    std::vector<std::unique_ptr<DocEdit>> inverses;
    std::vector<UndoTarget> targets;
    std::vector<std::unique_ptr<DocEdit>> counterparts;
};

class UndoStack {
public:
    std::size_t undoDepth() const { return m_undo.size(); }
    std::size_t redoDepth() const { return m_redo.size(); }

    const UndoRingEntry *oldestUndo() const
    {
        return m_undo.empty() ? nullptr : &m_undo.front();
    }
    const UndoRingEntry *newestUndo() const
    {
        return m_undo.empty() ? nullptr : &m_undo.back();
    }
    const UndoRingEntry *newestRedo() const
    {
        return m_redo.empty() ? nullptr : &m_redo.back();
    }

    void clear()
    {
        m_undo.clear();
        m_redo.clear();
    }

    void pushUndo(UndoRingEntry e) { pushCapped(m_undo, std::move(e)); }

    void popOldestIfFull()
    {
        if (static_cast<int>(m_undo.size()) == kUndoRingDepth)
            m_undo.pop_front();
    }

    UndoRingEntry takeUndo()
    {
        UndoRingEntry e = std::move(m_undo.back());
        m_undo.pop_back();
        return e;
    }

    UndoRingEntry takeRedo()
    {
        UndoRingEntry e = std::move(m_redo.back());
        m_redo.pop_back();
        return e;
    }

    bool undoEmpty() const { return m_undo.empty(); }
    bool redoEmpty() const { return m_redo.empty(); }

    void clearRedo() { m_redo.clear(); }

    void pushRedo(UndoRingEntry e) { pushCapped(m_redo, std::move(e)); }
    void pushOther(bool isUndo, UndoRingEntry e)
    {
        pushCapped(isUndo ? m_redo : m_undo, std::move(e));
    }

    void popUndoBack() { m_undo.pop_back(); }

    UndoRingEntry &undoBack() { return m_undo.back(); }

private:
    static void pushCapped(std::deque<UndoRingEntry> &ring, UndoRingEntry &&e)
    {
        if (static_cast<int>(ring.size()) == kUndoRingDepth)
            ring.pop_front();
        ring.push_back(std::move(e));
    }

    std::deque<UndoRingEntry> m_undo;
    std::deque<UndoRingEntry> m_redo;
};

} // namespace document
} // namespace epaper
