#pragma once
#include <cstdint>

/**
 * Distinct from world point. This can be passed anywhere without worrying about misunderstanding.
 * Why?
 * A using WorldPt = Pt would compile a world point passed where a local point is required. 
 * This distinction make us cautious when passing these around.
 */
struct Pt { double x = 0, y = 0; };

/**
 * Document local space
 */
struct WorldPt { double x = 0, y = 0; };

/**
 * Panel pixels, origin top-left
 */
struct PanelPt { double x = 0, y = 0; };


/**
 * [0, 1] inside the oriented sync frame
 */
struct FrameUv { double u = 0, y = 0; };


struct Aabb { double minX = 0, minY = 0, maxX = 0, maxY = 0; };

/** Axis-aligned bounding box, in the document local space */
struct WorldAabb {  double minX = 0, minY = 0, maxX = 0, maxY = 0; };

/** Transformation on a node */
struct Transform {
    double  
        sx = 1, sy = 1, 
        rotation = 0, 
        tx = 0, ty = 0;
};

struct ResizeMap {
    double 
        sx = 1, sy = 1, 
        tx = 0, ty = 0;
};



enum class Knob : uint8_t { Nw, N, Ne, E, Se, S, Sw, W };

enum class Side : uint8_t { Top, Right, Bottom, Left };