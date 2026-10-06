#pragma once
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>
#include "geometry.hpp"
#include <variant>

// ==== Basic ========================================================================
// Basic entities & value types
// =====================================================================================

// "NodeId" and ceremonies for "unordered_map"

struct NodeId { uint64_t hi = 0, lo = 0; };

inline bool operator==(NodeId a, NodeId b)
{
    return a.hi == b.hi && a.lo == b.lo;
}

namespace std {
    template <>
    struct hash<NodeId> {
        size_t operator()(NodeId id) const noexcept
        {
            return hash<uint64_t>{}(id.hi) ^ (hash<uint64_t>{}(id.lo) << 1);
        }
    };
}

struct Handle { uint32_t slot = 0, gen = 0; };

inline bool operator ==(Handle a, Handle b) {
    return a.slot == b.slot && a.gen == b.gen;
}

inline constexpr Handle kNoParent{0xffffffffu, 0};

enum class NodeType : uint8_t {
    Document,
    Frame,
    Group,
    InkBox,
    Ink,
    Primitive,
    Connector
};

enum class ManipMode : uint8_t {
    All,
    Boundary
};

enum class ShapeKind : uint8_t {
    Segment, Triangle, Square, Rectangle, Circle, Ellipse, Polygon
};

enum class ConnectorStyle: uint8_t { 
    Curve, 
    Ink
};


struct Color { uint8_t r = 0, g = 0, b = 0, a = 255; };
struct Stroke { Color color{}; double width = 0; };          // width in world units


// ==== Node & Structure ===============================================================
// The node itself
// =====================================================================================

struct Children;

struct NodeStorage {
    std::atomic<NodeId> id;
    std::atomic<NodeType> type;
    std::atomic<Handle> parent{kNoParent};
    
    // transform
    std::atomic<double> sx{1}, sy{1}, rotation{0}, tx{0}, ty{0};

    // local bounds
    std::atomic<double> minX{0}, minY{0}, maxX{0}, maxY{0};

    // seqlock: odd while a write is open
    std::atomic<uint32_t> version{0};
    std::atomic<const Children*> children{nullptr};
    std::atomic<const void*> payload{nullptr};
};

struct PlacementPlain {};
struct PlacementBoundary {};
struct PlacementContent {};
struct PlacementFollow { double offset = 0; };
using Placement = std::variant<PlacementPlain, PlacementBoundary,
                               PlacementContent, PlacementFollow>;

struct ChildLink {
    Handle node{};

    // strictly increasing; append uses last + 2^32
    uint64_t orderKey = 0;
    Placement placement{};

};

struct Children {
    std::vector<ChildLink> links;
    // step 4 replaces a flat list with the R-tree; same pointer, same publish
};

// ==== Payloads =======================================================================
// One heap object per live node that has a payload
// NodeType decide how to read the payload
// =====================================================================================

// #### InkPayload

struct InkSample {
    double x = 0, y = 0;
    std::optional<double> pressure, tilt, time;              // missing channels stay absent
};


struct SampleBuffer {
    std::vector<InkSample> samples;
};

struct InkPayload {
    std::atomic<const SampleBuffer*> samples{nullptr};
    std::atomic<Stroke> stroke;

    ~InkPayload() { delete samples.load(std::memory_order_relaxed); }
};


// #### InkBoxPayload

struct PointBuffer { 
    std::vector<Pt> points;
};

struct InkBoxPayload {

    std::atomic<const PointBuffer*> boundaryPolygon{nullptr};
    std::atomic<ManipMode> manipMode{ManipMode::Boundary};

    ~InkBoxPayload() { delete boundaryPolygon.load(std::memory_order_relaxed); }
};

// #### Payload
struct GroupPayload {
    // child origin, local space, treated as top-left corner of the group
    std::atomic<double> originX{0}, originY{0};
};

// #### PrimitivePayload

struct Segment   { Pt a, b; };
struct Triangle  { Pt a, b, c; };
struct Square    { Pt center; double side = 0; };
struct Rectangle { Pt origin; double width = 0, height = 0; };
struct Circle    { Pt center; double radius = 0; };
struct Ellipse   { Pt center; double rx = 0, ry = 0; };
struct Polygon   { std::vector<Pt> points; };           // closed

using ShapeData = std::variant<Segment, Triangle, Square, Rectangle,
                               Circle, Ellipse, Polygon>;

struct Fill { bool present = false; Color color{}; };

struct PrimitivePayload {
    std::atomic<Stroke> stroke;
    std::atomic<Fill> fill;
    std::atomic<ShapeKind> shape{ShapeKind::Rectangle};
    std::atomic<const ShapeData*> data{nullptr};

    ~PrimitivePayload() { delete data.load(std::memory_order_relaxed); }
};

// #### Connector Payload



struct NodeRef {
    NodeId id{};
    Handle cached{};
};

/** 
 * Face's local coordinate system, with 2 axis pointing north (face up) & east (left -> right).
 * - This can be used as a unit-vector, showing the departure/arrival direction of the connector.
 * - This can be used as normal point in face's local coordinate system, representing inks that need
 *   to be transformed as the node is moved, rotated, resized.
 */

struct FacePt { double n = 0, e = 0; };

/**
 * Connector ink has a rest spine (smooth line) and free ink, computed relatively to the rest spine.
 * The offset from the rest spine is represented by this.
 */
struct PathPt { double s = 0, d = 0; };


/** 
 * Used when 1 end of the connector is deleted, we need to keep its last position in the world.
 */
struct Pose { WorldPt at{}; double fx = 1, fy = 0; };


struct SideAnchor {
    Side side = Side::Top; 
    double t = 0;
};

struct CenterAnchor {

};

using Anchor = std::variant<SideAnchor, CenterAnchor>;

/**
 * Document fact, as user draws markers on connector's ends.
 * Because of this, its coordinate is face's coordinate system.
 */
struct EndMarker {

    std::vector<std::vector<FacePt>> strokes;
};

struct ConnectorEnd {
    NodeRef node {}; // Start/End node
    Anchor anchor{}; // Side or Center
    FacePt departure{}; // Departure direction, in face's coordinate system
    EndMarker marker{}; // Document fact
    Pose lastPose{};
};

/**
 * Connector ink has a rest spine (smooth line); and free ink computed relatively to the rest spine.
 * The offset from the rest spine is the rest "body".
 * Final connector strokes are derived from these.
 */

struct RestPath { std::vector<WorldPt> spine; std::vector<PathPt> body; };

struct EdgeLabel { NodeRef node{}; double t = 0, offset = 0; };

struct DerivedPath {

    // Compute the rendering strokes from rest spine & body

    std::vector<WorldPt> spine, body;

    // Convert marker's strokes from document fact to world's coordinate and cache it here.

    std::vector<std::vector<WorldPt>> markers;
};

struct ConnectorPayload {
    ConnectorEnd source {}, target {};

    std::atomic<ConnectorStyle> style{ConnectorStyle::Ink};
    std::atomic<const RestPath*> rest{nullptr};
    std::atomic<Stroke> stroke;
    std::atomic<const std::vector<EdgeLabel>*> labels{nullptr};

    // computed, to be rendered 
    std::atomic<const DerivedPath*> route{nullptr};

    ~ConnectorPayload()
    {
        delete rest.load(std::memory_order_relaxed);
        delete labels.load(std::memory_order_relaxed);
        delete route.load(std::memory_order_relaxed);
    }
};

inline const void* createPayload(NodeType type)
{
    switch (type) {
    case NodeType::Ink:
        return new InkPayload{};
    case NodeType::InkBox:
        return new InkBoxPayload{};
    case NodeType::Group:
        return new GroupPayload{};
    case NodeType::Primitive:
        return new PrimitivePayload{};
    case NodeType::Connector:
        return new ConnectorPayload{};
    case NodeType::Document:
    case NodeType::Frame:
        return nullptr;
    }
    return nullptr;
}

inline void destroyPayload(NodeType type, const void* p)
{
    if (!p)
        return;
    switch (type) {
    case NodeType::Ink:
        delete static_cast<const InkPayload*>(p);
        break;
    case NodeType::InkBox:
        delete static_cast<const InkBoxPayload*>(p);
        break;
    case NodeType::Group:
        delete static_cast<const GroupPayload*>(p);
        break;
    case NodeType::Primitive:
        delete static_cast<const PrimitivePayload*>(p);
        break;
    case NodeType::Connector:
        delete static_cast<const ConnectorPayload*>(p);
        break;
    case NodeType::Document:
    case NodeType::Frame:
        break;
    }
}