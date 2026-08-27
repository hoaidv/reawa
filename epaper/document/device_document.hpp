#pragma once
/**
 * Working document: tree + DocEdit apply. Undo lives on UndoStack.
 * @implements [SRS-EP-07] DeviceDocument tree, DocEdit apply, inverse undo
 * @implements [SRS-EP-09] device-local tree; lastOpId; inverse entry shape
 * @implements [SRS-EP-13] F20 skip-whole; F21 absence-partial; empty/pure no-op
 * @implements [SRS-EP-08] counterpart / compound / set_ink_samples publish
 */

#include "doc_model.hpp"
#include "operations/undo_stack.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace epaper {
namespace document {

class DocEdit;

/**
 * Working document. Session memory only.
 * @implements [SRS-EP-07] in-memory DeviceDocument
 */
class DeviceDocument {
public:
    std::vector<DocNode> rootChildren;
    std::string status = "open";

    ApplyResult apply(DocEdit &edit);
    ApplyResult commitEdit(DocEdit &edit);
    ApplyResult applyJson(const JsonValue &j);
    ApplyResult commitJson(const JsonValue &j);

    /**
     * One gesture, N edits, one undo entry, one doc_change (compound if N>1).
     * @implements [SRS-EP-08] compound publish of a multi-inverse gesture
     */
    ApplyResult commitGesture(const std::string &opId, std::vector<std::unique_ptr<DocEdit>> parts)
    {
        return commitParts(opId, std::move(parts));
    }

    /**
     * Path A stroke-erase: set_ink_samples; remove_node only if emptied.
     * Never restore_snapshot.
     * @implements [SRS-EP-08] Path A set_ink_samples / compound queue
     */
    ApplyResult commitPathAErase(const std::string &opId, const std::string &inkId,
                                 const std::vector<std::pair<double, double>> &worldHits,
                                 double nibRadius = 8.0);

    /**
     * @implements [SRS-EP-07] undo applies counterpart inverses
     */
    UndoResult undo()
    {
        if (m_gestureInFlight) {
            m_historyLatch = HistoryLatch::Undo;
            return {false, true, false};
        }
        return undoNow();
    }

    /**
     * @implements [SRS-EP-07] redo applies the forward counterparts
     */
    UndoResult redo()
    {
        if (m_gestureInFlight) {
            m_historyLatch = HistoryLatch::Redo;
            return {false, true, false};
        }
        return redoNow();
    }

    /** @implements [SRS-EP-07] mid-gesture latch — gesture in flight */
    void beginGesture()
    {
        m_gestureInFlight = true;
        m_intermediateFrames = 0;
    }

    /**
     * End an uncommitted gesture. Runs a latched undo once the gesture is gone.
     * @implements [SRS-EP-07] mid-gesture undo deferred until not in flight
     */
    void abortGesture()
    {
        m_gestureInFlight = false;
        m_intermediateFrames = 0;
        runLatchedHistory();
    }

    /**
     * Local-paint manipulation frame — does not push the ring or mutate the tree.
     * @implements [SRS-EP-07] one completed gesture is one ring entry
     */
    void previewManipulationFrame() { ++m_intermediateFrames; }

    /** Live paint only — not a ring entry. Restore origin before commitEdit. */
    bool applyLiveSmartGeometry(const std::string &id, const SmartTransform &t, const SmartBounds &b)
    {
        DocNode *n = findMut(id);
        if (!n)
            return false;
        n->transform = t;
        n->smartBounds = b;
        return true;
    }

    int intermediateFrameCount() const { return m_intermediateFrames; }
    bool gestureInFlight() const { return m_gestureInFlight; }
    bool undoLatched() const { return m_historyLatch == HistoryLatch::Undo; }
    bool redoLatched() const { return m_historyLatch == HistoryLatch::Redo; }
    std::size_t undoDepth() const { return m_history.undoDepth(); }
    std::size_t redoDepth() const { return m_history.redoDepth(); }

    const UndoRingEntry *oldestEntry() const { return m_history.oldestUndo(); }
    const UndoRingEntry *newestEntry() const { return m_history.newestUndo(); }

    /** Inverse ring has no whole-tree snapshot. Always 0. */
    static bool entryHasSnapshot(const UndoRingEntry &) { return false; }

    int restoreSnapshotQueued() const
    {
        int n = 0;
        for (const auto &ch : m_publishQueue)
            n += countRestoreSnapshot(ch.op);
        return n;
    }

    const std::vector<DocChange> &publishQueue() const { return m_publishQueue; }
    void clearPublishQueue() { m_publishQueue.clear(); }
    int lastSeq() const { return m_lastSeq; }

    /**
     * Skip / no-op / empty history never surfaces error UI (F20, F21, F1).
     * @implements [SRS-EP-13] 0 error UI on skip and no-op
     */
    int errorUiShown() const { return 0; }

    /**
     * Viewport / tool are not document state and must not push.
     * Selection lives on ToolCanvas, not the document tree.
     * @implements [SRS-EP-07] viewport tool and selection do not push undo
     */
    void applyViewportPan(double dx, double dy)
    {
        m_panX += dx;
        m_panY += dy;
    }
    void applyToolSwitch(std::string tool) { m_tool = std::move(tool); }
    void applySelectionChange(std::string selectedId) { m_selectionId = std::move(selectedId); }
    const std::string &selectionId() const { return m_selectionId; }
    double viewportPanX() const { return m_panX; }
    double viewportPanY() const { return m_panY; }
    const std::string &uiTool() const { return m_tool; }

    /**
     * Copy clones into the session slot. 0 ring entries.
     * @implements [SRS-EP-07] copy does not push undo
     */
    void copyToClipboard(const std::vector<std::string> &ids)
    {
        m_clipboard.clear();
        for (const auto &id : ids) {
            const DocNode *n = find(id);
            if (n)
                m_clipboard.push_back(*n);
        }
    }
    const std::vector<DocNode> &clipboardSlot() const { return m_clipboard; }

    /**
     * Last-live-pose cache when an endpoint is missing. Does not bump lastOpId.
     * @implements [SRS-EP-09] last-live-pose does not count as a change
     */
    bool writeLastLivePose(const std::string &connectorId, double x, double y)
    {
        DocNode *n = findMut(connectorId);
        if (!n || n->kind != NodeKind::Connector)
            return false;
        n->fromPose.x = x;
        n->fromPose.y = y;
        n->fromPose.valid = true;
        return true;
    }

    /**
     * Accepted load empties undo and redo. Handshake/TCP is STORY-EP-020.
     * @implements [SRS-EP-07] accepted doc_load clears the ring
     */
    void onAcceptedDocLoad()
    {
        m_history.clear();
        m_historyLatch = HistoryLatch::None;
        m_gestureInFlight = false;
        m_intermediateFrames = 0;
        m_lastSeq = 0;
        m_publishQueue.clear();
    }

    void onAcceptedDocLoad(const JsonValue &document)
    {
        rootChildren.clear();
        m_applied.clear();
        status = document.getString("status", "open");
        const JsonValue *kids = document.get("rootChildren");
        if (kids && kids->isArray()) {
            for (const auto &c : kids->asArray())
                rootChildren.push_back(nodeFromJson(c));
        }
        onAcceptedDocLoad();
    }

    const DocNode *find(const std::string &id) const { return findMut(id); }
    DocNode *mutableFind(const std::string &id) { return findMut(id); }

    struct NodePlace {
        std::string parentId;
        int index = 0;
    };

    bool findPlace(const std::string &id, NodePlace *out) const
    {
        return findPlaceIn(rootChildren, id, "", out);
    }

    bool detachAny(const std::string &id, DocNode *out)
    {
        return detachAny(rootChildren, id, out);
    }
    bool removeNodeId(const std::string &id) { return removeId(rootChildren, id); }
    void requireUnique(const std::string &id) { assertUniqueId(id); }

    int inkCount() const
    {
        int n = 0;
        walk(rootChildren, [&](const DocNode &node) {
            if (node.kind == NodeKind::Ink)
                ++n;
        });
        return n;
    }

    int nodeCount() const
    {
        int n = 0;
        walk(rootChildren, [&](const DocNode &) { ++n; });
        return n;
    }

    std::vector<std::string> allIds() const
    {
        std::vector<std::string> ids;
        walk(rootChildren, [&](const DocNode &node) { ids.push_back(node.id); });
        return ids;
    }

    JsonValue toJSON() const
    {
        JsonValue::Object o;
        o.emplace_back("version", JsonValue::number(1));
        o.emplace_back("status", JsonValue::string(status));
        JsonValue::Array kids;
        for (const auto &c : rootChildren)
            kids.push_back(nodeToJson(c));
        o.emplace_back("rootChildren", JsonValue::array(std::move(kids)));
        return JsonValue::object(std::move(o));
    }

    std::string snapshotString() const { return stringify(toJSON()); }

    bool hasOp(const std::string &opId) const { return m_applied.count(opId) > 0; }

    /** Paint-order walk (sibling order). */
    template <typename Fn>
    void forEachPaintNode(Fn &&fn) const
    {
        walkPaint(rootChildren, fn);
    }

private:
    std::set<std::string> m_applied;
    enum class HistoryLatch { None, Undo, Redo };
    UndoStack m_history;
    std::vector<DocChange> m_publishQueue;
    int m_lastSeq = 0;
    int m_historySerial = 0;
    bool m_gestureInFlight = false;
    HistoryLatch m_historyLatch = HistoryLatch::None;
    int m_intermediateFrames = 0;
    double m_panX = 0;
    double m_panY = 0;
    std::string m_tool = "pen";
    std::string m_selectionId;
    std::vector<DocNode> m_clipboard;

    void enqueueChange(JsonValue envelope)
    {
        DocChange ch;
        ch.seq = ++m_lastSeq;
        ch.baseSeq = ch.seq - 1;
        ch.opId = envelope.getString("opId");
        ch.op = std::move(envelope);
        ch.committedAtMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now().time_since_epoch())
                               .count();
        m_publishQueue.push_back(std::move(ch));
    }

    std::string nextHistoryOpId(bool isUndo)
    {
        ++m_historySerial;
        return std::string(isUndo ? "undo:" : "redo:") + std::to_string(m_historySerial);
    }

    void stampLastOpId(const std::vector<UndoTarget> &targets, const std::string &opId)
    {
        for (const auto &t : targets) {
            if (DocNode *n = findMut(t.nodeId))
                n->lastOpId = opId;
        }
    }

    std::vector<UndoTarget> captureTargets(const std::vector<std::string> &ids) const;
    UndoResult applyHistoryEntry(UndoRingEntry e, bool isUndo);

    static int countRestoreSnapshot(const JsonValue &op)
    {
        const std::string type = op.getString("type");
        if (kindEq(type.c_str(), edit_kind::kRestoreSnapshot))
            return 1;
        int n = 0;
        if (kindEq(type.c_str(), edit_kind::kCompound)) {
            const JsonValue *payload = op.get("payload");
            const JsonValue *ops = payload ? payload->get("ops") : nullptr;
            if (ops && ops->isArray()) {
                for (const auto &j : ops->asArray()) {
                    if (kindEq(j.getString("type").c_str(), edit_kind::kRestoreSnapshot))
                        ++n;
                }
            }
        }
        return n;
    }

    ApplyResult commitParts(const std::string &opId, std::vector<std::unique_ptr<DocEdit>> parts);

    static bool findPlaceIn(const std::vector<DocNode> &nodes, const std::string &id,
                            const std::string &parentId, NodePlace *out)
    {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id == id) {
                if (out) {
                    out->parentId = parentId;
                    out->index = static_cast<int>(i);
                }
                return true;
            }
            if (findPlaceIn(nodes[i].children, id, nodes[i].id, out))
                return true;
        }
        return false;
    }

    static bool detachAny(std::vector<DocNode> &nodes, const std::string &id, DocNode *out)
    {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id == id) {
                if (out)
                    *out = std::move(nodes[i]);
                nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
            if (detachAny(nodes[i].children, id, out))
                return true;
        }
        return false;
    }

public:
    void insertAt(const std::string &parentId, int index, DocNode node)
    {
        std::vector<DocNode> *sibs = &rootChildren;
        if (!parentId.empty()) {
            DocNode *p = findMut(parentId);
            if (!p)
                throw std::runtime_error(std::string("missing_parent:") + parentId);
            sibs = &p->children;
        }
        if (index < 0)
            index = 0;
        if (index > static_cast<int>(sibs->size()))
            index = static_cast<int>(sibs->size());
        sibs->insert(sibs->begin() + index, std::move(node));
    }

    void runLatchedHistory()
    {
        const HistoryLatch latched = m_historyLatch;
        m_historyLatch = HistoryLatch::None;
        if (latched == HistoryLatch::Undo)
            undoNow();
        else if (latched == HistoryLatch::Redo)
            redoNow();
    }

    /**
     * @implements [SRS-EP-07] pop newest, apply inverses, enqueue counterpart
     */
    UndoResult undoNow()
    {
        if (m_history.undoEmpty())
            return {false, false, true};
        return applyHistoryEntry(m_history.takeUndo(), true);
    }

    UndoResult redoNow()
    {
        if (m_history.redoEmpty())
            return {false, false, true};
        return applyHistoryEntry(m_history.takeRedo(), false);
    }

    template <typename Fn>
    static void walk(const std::vector<DocNode> &nodes, Fn &&fn)
    {
        for (const auto &n : nodes) {
            fn(n);
            if (!n.children.empty())
                walk(n.children, fn);
        }
    }

    template <typename Fn>
    static void walkPaint(const std::vector<DocNode> &nodes, Fn &&fn)
    {
        for (const auto &n : nodes) {
            fn(n);
            if (n.kind == NodeKind::Frame || n.kind == NodeKind::Group
                || n.kind == NodeKind::SmartGroup)
                walkPaint(n.children, fn);
        }
    }

    DocNode *findMut(const std::string &id) const
    {
        return findIn(const_cast<std::vector<DocNode> &>(rootChildren), id);
    }

    static DocNode *findIn(std::vector<DocNode> &nodes, const std::string &id)
    {
        for (auto &n : nodes) {
            if (n.id == id)
                return &n;
            if (auto *c = findIn(n.children, id))
                return c;
        }
        return nullptr;
    }

    void assertUniqueId(const std::string &id)
    {
        if (findMut(id))
            throw std::runtime_error(std::string("duplicate_id:") + id);
    }

    static bool isContainer(const DocNode *n)
    {
        return n && (n->kind == NodeKind::Frame || n->kind == NodeKind::Group);
    }

    void insertUnder(const std::optional<std::string> &parentId, DocNode node)
    {
        if (!parentId || parentId->empty()) {
            rootChildren.push_back(std::move(node));
            return;
        }
        DocNode *parent = findMut(*parentId);
        if (!isContainer(parent))
            throw std::runtime_error(std::string("bad_parent:") + *parentId);
        if (node.kind == NodeKind::Frame)
            throw std::runtime_error("frame_not_under_container");
        parent->children.push_back(std::move(node));
    }

    std::optional<std::string> parentIdOf(const JsonValue &p) const
    {
        const JsonValue *x = p.get("parentId");
        if (!x || !x->isString())
            return std::nullopt;
        return x->asString();
    }

    std::string requireId(const JsonValue &p) const
    {
        const JsonValue *x = p.get("id");
        if (!x || !x->isString())
            throw std::runtime_error("missing_id");
        return x->asString();
    }

    static ConnectorAnchor anchorFromJson(const JsonValue *a)
    {
        ConnectorAnchor o;
        if (!a || !a->isObject())
            return o;
        o.nodeId = a->getString("nodeId");
        o.kind = a->getString("kind", "edge");
        o.edge = int(a->getNumber("edge", 0));
        o.t = a->getNumber("t", 0);
        if (const JsonValue *loc = a->get("drawnEdgeLocal"); loc && loc->isObject()) {
            o.drawnN = loc->getNumber("n", 1);
            o.drawnE = loc->getNumber("e", 0);
        }
        if (const JsonValue *box = a->get("drawnBoxLocal"); box && box->isObject()) {
            o.drawnBoxX = box->getNumber("x", 1);
            o.drawnBoxY = box->getNumber("y", 0);
        }
        if (const JsonValue *lp = a->get("local"); lp && lp->isObject()) {
            o.localX = lp->getNumber("x", 0);
            o.localY = lp->getNumber("y", 0);
            o.hasLocal = true;
        }
        return o;
    }

    static JsonValue anchorToJson(const ConnectorAnchor &a)
    {
        JsonValue::Object o;
        o.emplace_back("nodeId", JsonValue::string(a.nodeId));
        o.emplace_back("kind", JsonValue::string(a.kind));
        o.emplace_back("edge", JsonValue::number(a.edge));
        o.emplace_back("t", JsonValue::number(a.t));
        JsonValue::Object loc;
        loc.emplace_back("n", JsonValue::number(a.drawnN));
        loc.emplace_back("e", JsonValue::number(a.drawnE));
        o.emplace_back("drawnEdgeLocal", JsonValue::object(std::move(loc)));
        JsonValue::Object box;
        box.emplace_back("x", JsonValue::number(a.drawnBoxX));
        box.emplace_back("y", JsonValue::number(a.drawnBoxY));
        o.emplace_back("drawnBoxLocal", JsonValue::object(std::move(box)));
        if (a.hasLocal) {
            JsonValue::Object lp;
            lp.emplace_back("x", JsonValue::number(a.localX));
            lp.emplace_back("y", JsonValue::number(a.localY));
            o.emplace_back("local", JsonValue::object(std::move(lp)));
        }
        return JsonValue::object(std::move(o));
    }

    static ConnectorEndPose poseFromJson(const JsonValue *p)
    {
        ConnectorEndPose o;
        if (!p || !p->isObject())
            return o;
        o.x = p->getNumber("x", 0);
        o.y = p->getNumber("y", 0);
        o.fx = p->getNumber("fx", 1);
        o.fy = p->getNumber("fy", 0);
        o.valid = true;
        return o;
    }

    static JsonValue poseToJson(const ConnectorEndPose &p)
    {
        JsonValue::Object o;
        o.emplace_back("x", JsonValue::number(p.x));
        o.emplace_back("y", JsonValue::number(p.y));
        o.emplace_back("fx", JsonValue::number(p.fx));
        o.emplace_back("fy", JsonValue::number(p.fy));
        return JsonValue::object(std::move(o));
    }

    static void appendRestSpine(DocNode &n, const JsonValue *sp)
    {
        if (!sp || !sp->isArray())
            return;
        for (const auto &pt : sp->asArray()) {
            if (pt.isObject())
                n.restSpine.push_back({pt.getNumber("x"), pt.getNumber("y")});
        }
    }

    static void appendRestOffsets(DocNode &n, const JsonValue *off)
    {
        if (!off || !off->isArray())
            return;
        for (const auto &pt : off->asArray()) {
            if (pt.isObject())
                n.restOffsets.push_back({pt.getNumber("s"), pt.getNumber("d")});
        }
    }

    /** Infini snapshot uses restSpine; device wire uses restShape. */
    static void fillConnectorRest(DocNode &n, const JsonValue &j)
    {
        if (const JsonValue *rs = j.get("restShape"); rs && rs->isObject()) {
            appendRestSpine(n, rs->get("spine"));
            appendRestOffsets(n, rs->get("offsets"));
        }
        if (n.restSpine.empty())
            appendRestSpine(n, j.get("restSpine"));
        if (n.restOffsets.empty())
            appendRestOffsets(n, j.get("restOffsets"));
    }

    /** @implements [SRS-EP-07] create_connector — device-authored; body + rest + anchors
     *  @implements [SRS-EP-17] commit envelope */
    bool detachInk(const std::string &id, DocNode *out)
    {
        return detachInkFrom(rootChildren, id, out);
    }

    static bool detachInkFrom(std::vector<DocNode> &nodes, const std::string &id, DocNode *out)
    {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].kind == NodeKind::Ink && nodes[i].id == id) {
                if (out)
                    *out = std::move(nodes[i]);
                nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
            if (nodes[i].kind == NodeKind::Frame || nodes[i].kind == NodeKind::Group) {
                if (detachInkFrom(nodes[i].children, id, out))
                    return true;
            }
        }
        return false;
    }

    static bool removeId(std::vector<DocNode> &nodes, const std::string &id)
    {
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (nodes[i].id == id) {
                nodes.erase(nodes.begin() + static_cast<std::ptrdiff_t>(i));
                return true;
            }
            if (removeId(nodes[i].children, id))
                return true;
        }
        return false;
    }

    static NodeKind kindFromString(const std::string &k)
    {
        if (k == "ink")
            return NodeKind::Ink;
        if (k == "text")
            return NodeKind::Text;
        if (k == "primitive")
            return NodeKind::Primitive;
        if (k == "group")
            return NodeKind::Group;
        if (k == "frame")
            return NodeKind::Frame;
        if (k == "connector")
            return NodeKind::Connector;
        return NodeKind::SmartGroup;
    }

    static DocNode nodeFromJson(const JsonValue &j)
    {
        DocNode n;
        n.id = j.getString("id");
        n.kind = kindFromString(j.getString("kind"));
        switch (n.kind) {
        case NodeKind::Ink: {
            if (const JsonValue *samples = j.get("samples"); samples && samples->isArray()) {
                for (const auto &s : samples->asArray())
                    n.samples.push_back(sampleFromJson(s));
            }
            n.style = styleFromJson(j.get("style"));
            if (const JsonValue *role = j.get("role"); role && role->isString())
                n.role = role->asString();
            if (const JsonValue *lo = j.get("layoutOffset"); lo && lo->isObject())
                n.layoutOffset = {lo->getNumber("u"), lo->getNumber("v")};
            break;
        }
        case NodeKind::Text: {
            n.box = aabbFromJson(j.get("box"));
            n.style = styleFromJson(j.get("style"));
            if (const JsonValue *runs = j.get("runs"); runs && runs->isArray()) {
                for (const auto &r : runs->asArray()) {
                    TextRun tr;
                    tr.text = r.getString("text");
                    const JsonValue *b = r.get("bold");
                    tr.bold = b && b->isBool() && b->asBool();
                    n.runs.push_back(tr);
                }
            }
            break;
        }
        case NodeKind::Primitive: {
            n.style = styleFromJson(j.get("style"));
            const JsonValue *geom = j.get("geom");
            if (geom && geom->isObject()) {
                const std::string gk = geom->getString("kind");
                if (gk == "line") {
                    n.geomKind = PrimitiveKind::Line;
                    n.x1 = geom->getNumber("x1");
                    n.y1 = geom->getNumber("y1");
                    n.x2 = geom->getNumber("x2");
                    n.y2 = geom->getNumber("y2");
                } else if (gk == "ellipse") {
                    n.geomKind = PrimitiveKind::Ellipse;
                    n.cx = geom->getNumber("cx");
                    n.cy = geom->getNumber("cy");
                    n.rx = geom->getNumber("rx");
                    n.ry = geom->getNumber("ry");
                } else {
                    n.geomKind = PrimitiveKind::Rect;
                    n.gx = geom->getNumber("x");
                    n.gy = geom->getNumber("y");
                    n.gw = geom->getNumber("w");
                    n.gh = geom->getNumber("h");
                }
            }
            break;
        }
        case NodeKind::Group:
        case NodeKind::Frame: {
            if (n.kind == NodeKind::Frame)
                n.bounds = aabbFromJson(j.get("bounds"));
            if (const JsonValue *ch = j.get("children"); ch && ch->isArray()) {
                for (const auto &c : ch->asArray())
                    n.children.push_back(nodeFromJson(c));
            }
            break;
        }
        case NodeKind::Connector: {
            n.fromAnchor = DeviceDocument::anchorFromJson(j.get("from"));
            n.toAnchor = DeviceDocument::anchorFromJson(j.get("to"));
            n.fromNodeId = n.fromAnchor.nodeId;
            n.toNodeId = n.toAnchor.nodeId;
            n.warpStyle = j.getString("warpStyle", "");
            n.connectorInvalid = false;
            n.fromPose = DeviceDocument::poseFromJson(j.get("fromPose"));
            n.toPose = DeviceDocument::poseFromJson(j.get("toPose"));
            // Infini toJSON uses restSpine; epaper wire uses restShape. Accept both
            // or reconnect doc_load paints nodes but skips connectors (empty warp).
            DeviceDocument::fillConnectorRest(n, j);
            if (const JsonValue *ch = j.get("children"); ch && ch->isArray()) {
                for (const auto &c : ch->asArray())
                    n.children.push_back(nodeFromJson(c));
            }
            break;
        }
        case NodeKind::SmartGroup: {
            if (const JsonValue *b = j.get("bounds"); b && b->isObject()) {
                n.smartBounds.x = b->getNumber("x");
                n.smartBounds.y = b->getNumber("y");
                n.smartBounds.width = b->getNumber("width");
                n.smartBounds.height = b->getNumber("height");
            }
            if (const JsonValue *t = j.get("transform"); t && t->isObject()) {
                n.transform.x = t->getNumber("x");
                n.transform.y = t->getNumber("y");
                n.transform.rotation = t->getNumber("rotation");
                n.transform.scaleX = t->has("scaleX") ? t->getNumber("scaleX") : 1;
                n.transform.scaleY = t->has("scaleY") ? t->getNumber("scaleY") : 1;
            }
            n.inkScaleMode = j.getString("inkScaleMode", "fixedInk");
            if (const JsonValue *ch = j.get("children"); ch && ch->isArray()) {
                for (const auto &c : ch->asArray())
                    n.children.push_back(nodeFromJson(c));
            }
            break;
        }
        }
        return n;
    }

    static JsonValue nodeToJson(const DocNode &n)
    {
        JsonValue::Object o;
        o.emplace_back("id", JsonValue::string(n.id));
        switch (n.kind) {
        case NodeKind::Ink: {
            o.emplace_back("kind", JsonValue::string("ink"));
            JsonValue::Array samples;
            for (const auto &s : n.samples)
                samples.push_back(sampleToJson(s));
            o.emplace_back("samples", JsonValue::array(std::move(samples)));
            o.emplace_back("style", styleToJson(n.style));
            if (n.role)
                o.emplace_back("role", JsonValue::string(*n.role));
            if (n.layoutOffset) {
                JsonValue::Object lo;
                lo.emplace_back("u", JsonValue::number(n.layoutOffset->first));
                lo.emplace_back("v", JsonValue::number(n.layoutOffset->second));
                o.emplace_back("layoutOffset", JsonValue::object(std::move(lo)));
            }
            break;
        }
        case NodeKind::Text: {
            o.emplace_back("kind", JsonValue::string("text"));
            o.emplace_back("box", aabbToJson(n.box));
            JsonValue::Array runs;
            for (const auto &r : n.runs) {
                JsonValue::Object ro;
                ro.emplace_back("text", JsonValue::string(r.text));
                if (r.bold)
                    ro.emplace_back("bold", JsonValue::boolean(true));
                runs.push_back(JsonValue::object(std::move(ro)));
            }
            o.emplace_back("runs", JsonValue::array(std::move(runs)));
            o.emplace_back("style", styleToJson(n.style));
            break;
        }
        case NodeKind::Primitive: {
            o.emplace_back("kind", JsonValue::string("primitive"));
            JsonValue::Object g;
            if (n.geomKind == PrimitiveKind::Line) {
                g.emplace_back("kind", JsonValue::string("line"));
                g.emplace_back("x1", JsonValue::number(n.x1));
                g.emplace_back("y1", JsonValue::number(n.y1));
                g.emplace_back("x2", JsonValue::number(n.x2));
                g.emplace_back("y2", JsonValue::number(n.y2));
            } else if (n.geomKind == PrimitiveKind::Ellipse) {
                g.emplace_back("kind", JsonValue::string("ellipse"));
                g.emplace_back("cx", JsonValue::number(n.cx));
                g.emplace_back("cy", JsonValue::number(n.cy));
                g.emplace_back("rx", JsonValue::number(n.rx));
                g.emplace_back("ry", JsonValue::number(n.ry));
            } else {
                g.emplace_back("kind", JsonValue::string("rect"));
                g.emplace_back("x", JsonValue::number(n.gx));
                g.emplace_back("y", JsonValue::number(n.gy));
                g.emplace_back("w", JsonValue::number(n.gw));
                g.emplace_back("h", JsonValue::number(n.gh));
            }
            o.emplace_back("geom", JsonValue::object(std::move(g)));
            o.emplace_back("style", styleToJson(n.style));
            break;
        }
        case NodeKind::Group: {
            o.emplace_back("kind", JsonValue::string("group"));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        case NodeKind::Frame: {
            o.emplace_back("kind", JsonValue::string("frame"));
            o.emplace_back("bounds", aabbToJson(n.bounds));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        case NodeKind::Connector: {
            o.emplace_back("kind", JsonValue::string("connector"));
            o.emplace_back("from", DeviceDocument::anchorToJson(n.fromAnchor.nodeId.empty()
                                                                    ? ConnectorAnchor{n.fromNodeId}
                                                                    : n.fromAnchor));
            o.emplace_back("to", DeviceDocument::anchorToJson(n.toAnchor.nodeId.empty()
                                                                  ? ConnectorAnchor{n.toNodeId}
                                                                  : n.toAnchor));
            if (!n.warpStyle.empty())
                o.emplace_back("warpStyle", JsonValue::string(n.warpStyle));
            if (!n.restSpine.empty() || !n.restOffsets.empty()) {
                JsonValue::Array spine;
                for (const auto &p : n.restSpine) {
                    JsonValue::Object pt;
                    pt.emplace_back("x", JsonValue::number(p.x));
                    pt.emplace_back("y", JsonValue::number(p.y));
                    spine.push_back(JsonValue::object(std::move(pt)));
                }
                JsonValue::Array off;
                for (const auto &p : n.restOffsets) {
                    JsonValue::Object pt;
                    pt.emplace_back("s", JsonValue::number(p.s));
                    pt.emplace_back("d", JsonValue::number(p.d));
                    off.push_back(JsonValue::object(std::move(pt)));
                }
                JsonValue::Object rs;
                rs.emplace_back("spine", JsonValue::array(std::move(spine)));
                rs.emplace_back("offsets", JsonValue::array(std::move(off)));
                o.emplace_back("restShape", JsonValue::object(std::move(rs)));
            }
            if (n.fromPose.valid)
                o.emplace_back("fromPose", DeviceDocument::poseToJson(n.fromPose));
            if (n.toPose.valid)
                o.emplace_back("toPose", DeviceDocument::poseToJson(n.toPose));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        case NodeKind::SmartGroup: {
            o.emplace_back("kind", JsonValue::string("smart_group"));
            JsonValue::Object b;
            b.emplace_back("x", JsonValue::number(n.smartBounds.x));
            b.emplace_back("y", JsonValue::number(n.smartBounds.y));
            b.emplace_back("width", JsonValue::number(n.smartBounds.width));
            b.emplace_back("height", JsonValue::number(n.smartBounds.height));
            o.emplace_back("bounds", JsonValue::object(std::move(b)));
            JsonValue::Object t;
            t.emplace_back("x", JsonValue::number(n.transform.x));
            t.emplace_back("y", JsonValue::number(n.transform.y));
            t.emplace_back("rotation", JsonValue::number(n.transform.rotation));
            t.emplace_back("scaleX", JsonValue::number(n.transform.scaleX));
            t.emplace_back("scaleY", JsonValue::number(n.transform.scaleY));
            o.emplace_back("transform", JsonValue::object(std::move(t)));
            o.emplace_back("inkScaleMode", JsonValue::string(n.inkScaleMode));
            JsonValue::Array kids;
            for (const auto &c : n.children)
                kids.push_back(nodeToJson(c));
            o.emplace_back("children", JsonValue::array(std::move(kids)));
            break;
        }
        }
        return JsonValue::object(std::move(o));
    }
};

} // namespace document
} // namespace epaper

#include "operations/edit_bodies.hpp"

namespace epaper {
namespace document {

inline ApplyResult DeviceDocument::apply(DocEdit &edit)
{
    try {
        return edit.doApply(*this);
    } catch (const std::exception &e) {
        return {false, e.what()};
    }
}

inline UndoResult DeviceDocument::applyHistoryEntry(UndoRingEntry e, bool isUndo)
{
    bool anySkip = false;
    for (const auto &inv : e.inverses) {
        if (!inv)
            continue;
        if (isCreateKind(inv->kind())) {
            const auto ts = inv->targets();
            if (!ts.empty() && find(ts.front()))
                anySkip = true;
        }
    }
    for (const auto &t : e.targets) {
        const DocNode *n = find(t.nodeId);
        if (!n)
            continue;
        const std::string expected = isUndo ? e.forwardOpId : t.prevLastOpId;
        if (n->lastOpId != expected)
            anySkip = true;
    }
    if (anySkip)
        return {false, false, true, true};

    const std::string histId = nextHistoryOpId(isUndo);
    std::vector<std::unique_ptr<DocEdit>> applied;
    for (const auto &inv : e.inverses) {
        if (!inv)
            continue;
        if (kindEq(inv->kind(), edit_kind::kReparent)) {
            const auto *rp = dynamic_cast<const ReparentEdit *>(inv.get());
            if (rp && !rp->hasBody() && !find(rp->nodeId()))
                continue;
        } else if (kindEq(inv->kind(), edit_kind::kSetSmartTransform)
                   || kindEq(inv->kind(), edit_kind::kSetInkScaleMode)
                   || kindEq(inv->kind(), edit_kind::kRemoveNode)
                   || kindEq(inv->kind(), edit_kind::kSetInkSamples)) {
            const auto ts = inv->targets();
            if (!ts.empty() && !find(ts.front()))
                continue;
        }
        auto run = inv->clone();
        run->setId(histId);
        const ApplyResult r = apply(*run);
        if (r.applied)
            applied.push_back(std::move(run));
    }
    if (applied.empty())
        return {false, false, true, false};

    if (isUndo) {
        for (const auto &t : e.targets) {
            if (DocNode *n = findMut(t.nodeId))
                n->lastOpId = t.prevLastOpId;
        }
    } else {
        stampLastOpId(e.targets, e.forwardOpId);
    }

    JsonValue pub;
    if (applied.size() == 1) {
        pub = applied[0]->serialize();
    } else {
        CompoundEdit compound;
        compound.setId(histId);
        compound.setSource("epaper");
        for (auto &a : applied)
            compound.addPart(std::move(a));
        pub = compound.serialize();
    }
    enqueueChange(std::move(pub));

    UndoRingEntry other;
    other.forwardOpId = e.forwardOpId;
    other.seq = m_lastSeq;
    other.inverses = std::move(e.counterparts);
    other.counterparts = std::move(e.inverses);
    other.targets = std::move(e.targets);
    m_history.pushOther(isUndo, std::move(other));
    return {true, false, false, false};
}

inline ApplyResult DeviceDocument::applyJson(const JsonValue &j)
{
    auto e = DocEdit::fromJson(j);
    if (!e)
        return {false, std::string("unknown_type:") + j.getString("type")};
    if (e->id().empty())
        return {false, "missing_opId"};
    if (m_applied.count(e->id()))
        return {false, "duplicate_opId"};
    const ApplyResult r = apply(*e);
    if (!r.applied)
        return r;
    m_applied.insert(e->id());
    status = "dirty";
    stampLastOpId(captureTargets(e->targets()), e->id());
    return {true, {}};
}

inline ApplyResult DeviceDocument::commitJson(const JsonValue &j)
{
    auto e = DocEdit::fromJson(j);
    if (!e)
        return {false, std::string("unknown_type:") + j.getString("type")};
    return commitEdit(*e);
}

inline std::vector<UndoTarget> DeviceDocument::captureTargets(const std::vector<std::string> &ids) const
{
    std::vector<UndoTarget> ts;
    ts.reserve(ids.size());
    for (const auto &id : ids) {
        bool dup = false;
        for (const auto &t : ts) {
            if (t.nodeId == id) {
                dup = true;
                break;
            }
        }
        if (dup)
            continue;
        const DocNode *n = find(id);
        ts.push_back({id, n ? n->lastOpId : std::string()});
    }
    return ts;
}

inline void flattenInverses(std::unique_ptr<DocEdit> inv, std::vector<std::unique_ptr<DocEdit>> *out)
{
    if (!inv || !out)
        return;
    if (kindEq(inv->kind(), edit_kind::kCompound)) {
        auto *c = dynamic_cast<CompoundEdit *>(inv.get());
        if (!c)
            return;
        for (auto &p : c->takeParts())
            flattenInverses(std::move(p), out);
        return;
    }
    out->push_back(std::move(inv));
}

inline ApplyResult DeviceDocument::commitEdit(DocEdit &edit)
{
    if (kindEq(edit.kind(), edit_kind::kCompound)) {
        auto *c = dynamic_cast<CompoundEdit *>(&edit);
        if (!c)
            return {false, "compound_cast"};
        std::vector<std::unique_ptr<DocEdit>> parts;
        for (const auto &p : c->parts()) {
            if (p)
                parts.push_back(p->clone());
        }
        return commitParts(edit.id(), std::move(parts));
    }
    if (edit.id().empty())
        return {false, "missing_opId"};
    const bool structural = isStructuralKind(edit.kind());
    UndoRingEntry pending;
    if (structural) {
        pending.forwardOpId = edit.id();
        pending.counterparts.push_back(edit.clone());
        flattenInverses(edit.generateUndo(*this), &pending.inverses);
        pending.targets = captureTargets(edit.targets());
        m_history.popOldestIfFull();
        m_history.pushUndo(std::move(pending));
    }
    if (m_applied.count(edit.id())) {
        if (structural)
            m_history.popUndoBack();
        return {false, "duplicate_opId"};
    }
    const ApplyResult r = apply(edit);
    if (!r.applied) {
        if (structural)
            m_history.popUndoBack();
        return r;
    }
    m_applied.insert(edit.id());
    status = "dirty";
    if (structural) {
        stampLastOpId(m_history.undoBack().targets, edit.id());
        enqueueChange(edit.serialize());
        m_history.undoBack().seq = m_lastSeq;
        m_history.clearRedo();
    }
    m_intermediateFrames = 0;
    m_gestureInFlight = false;
    runLatchedHistory();
    return r;
}

inline ApplyResult DeviceDocument::commitParts(const std::string &opId,
                                               std::vector<std::unique_ptr<DocEdit>> parts)
{
    if (parts.empty())
        return {false, "empty_gesture"};
    if (m_applied.count(opId))
        return {false, "duplicate_opId"};
    for (auto &p : parts) {
        if (p && p->id().empty())
            p->setId(opId);
    }

    UndoRingEntry pending;
    pending.forwardOpId = opId;
    std::vector<std::vector<std::unique_ptr<DocEdit>>> invParts;
    invParts.reserve(parts.size());
    for (auto &part : parts) {
        if (!part)
            return {false, "null_part"};
        std::vector<std::unique_ptr<DocEdit>> flat;
        flattenInverses(part->generateUndo(*this), &flat);
        invParts.push_back(std::move(flat));
        auto more = captureTargets(part->targets());
        for (auto &t : more) {
            bool dup = false;
            for (const auto &e : pending.targets) {
                if (e.nodeId == t.nodeId) {
                    dup = true;
                    break;
                }
            }
            if (!dup)
                pending.targets.push_back(std::move(t));
        }
        pending.counterparts.push_back(part->clone());
    }
    for (auto it = invParts.rbegin(); it != invParts.rend(); ++it) {
        for (auto &inv : *it)
            pending.inverses.push_back(std::move(inv));
    }

    const std::vector<DocNode> backup = rootChildren;
    for (auto &part : parts) {
        const ApplyResult r = apply(*part);
        if (!r.applied) {
            rootChildren = backup;
            return r;
        }
    }
    m_applied.insert(opId);
    status = "dirty";

    m_history.popOldestIfFull();
    m_history.pushUndo(std::move(pending));
    stampLastOpId(m_history.undoBack().targets, opId);

    JsonValue pub;
    if (parts.size() == 1) {
        pub = parts[0]->serialize();
    } else {
        CompoundEdit compound;
        compound.setId(opId);
        compound.setSource("epaper");
        for (auto &p : parts)
            compound.addPart(std::move(p));
        pub = compound.serialize();
    }
    enqueueChange(std::move(pub));
    m_history.undoBack().seq = m_lastSeq;
    m_history.clearRedo();
    m_intermediateFrames = 0;
    m_gestureInFlight = false;
    runLatchedHistory();
    return {true, {}};
}

inline ApplyResult DeviceDocument::commitPathAErase(const std::string &opId, const std::string &inkId,
                                                    const std::vector<std::pair<double, double>> &worldHits,
                                                    double nibRadius)
{
    const DocNode *ink = find(inkId);
    if (!ink || ink->kind != NodeKind::Ink)
        return {false, std::string("not_ink:") + inkId};
    std::vector<InkSample> remaining;
    remaining.reserve(ink->samples.size());
    const double r2 = nibRadius * nibRadius;
    for (const auto &s : ink->samples) {
        bool hit = false;
        for (const auto &h : worldHits) {
            const double dx = s.x - h.first;
            const double dy = s.y - h.second;
            if (dx * dx + dy * dy <= r2) {
                hit = true;
                break;
            }
        }
        if (!hit)
            remaining.push_back(s);
    }
    auto setSamples = std::make_unique<SetInkSamplesEdit>(inkId, remaining);
    setSamples->setId(opId);
    if (!remaining.empty())
        return commitEdit(*setSamples);
    CompoundEdit compound;
    compound.setId(opId);
    compound.addPart(std::move(setSamples));
    compound.addPart(makeRemoveEdit(opId, inkId));
    return commitEdit(compound);
}

} // namespace document
} // namespace epaper
