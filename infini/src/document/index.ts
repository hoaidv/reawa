/**
 * @implements [SRS-IN-04] vector document public API
 */

export * from "./types";
export { VectorDocument } from "./VectorDocument";
export { flattenDrawables, drawableBoundsUnion } from "./flatten";
export { resolveAnchor, smartGroupWorldAabb, smartLocalToWorld } from "./anchors";
export { drawablesToPrimitives } from "./toPrimitives";
export {
  serializeInfiniSvg,
  parseInfiniSvg,
  loadInfiniSvgReplacing,
} from "./svgProfile";
export type { SvgLoadResult, SvgLoadOk, SvgLoadErr } from "./svgProfile";
