/**
 * Connector warp tunables — must match epaper/document/connector_warp_params.hpp.
 * @implements [SRS-IN-09] Morph/Cubic tunables (device SoT)
 * @implements [ADR-0020] production reimplementation; Local G1 is not a style
 */

/** Rest spine sample spacing (world u). Device: kRestResampleWorld. */
export const kRestResampleWorld = 2.0;

/** Rest-spine Gaussian σ (world u). Device: kRestSigmaWorld. */
export const kRestSigmaWorld = 6.0;

/** ≤ this many inflections on S → cubic, else morph. Device: kInflectionCubicMax. */
export const kInflectionCubicMax = 1;

/** Centre-end facing cone half-angle (degrees). Device: kCentreConeDeg. */
export const kCentreConeDeg = 60.0;

/** false = drawn leave in edge frame; true = face normal. Device: kEdgeFacingPerpendicular. */
export const kEdgeFacingPerpendicular = false;

/** Morph mix saturates at this turn (degrees). Device: kMorphSatDeg. */
export const kMorphSatDeg = 90.0;

/** true = Hermite speed L'; false = chord/3. Device: kHandleModeRestSpeed. */
export const kHandleModeRestSpeed = true;

/** Centre vs boundary-ink attach ratio. Device: kCentreVsBoundary. */
export const kCentreVsBoundary = 1.0;

/** Documented bend floor (not a live clamp). Device: kMinInkRadiusWorld. */
export const kMinInkRadiusWorld = 12.0;
