/**
 * Infini SVG profile — serialize / parse materialised trees.
 * @implements [SRS-IN-09] SVG persistence profile
 */

import { VectorDocument } from "./VectorDocument";
import type {
  Anchor,
  BoundaryParam,
  DocNode,
  FrameNode,
  GroupNode,
  InkNode,
  InkSample,
  PrimitiveNode,
  SmartGroupNode,
  Style,
  TextNode,
  ConnectorNode,
} from "./types";
import { DEFAULT_STYLE, IDENTITY_SMART_TRANSFORM } from "./types";

const DOC_VERSION = "1";

export type SvgLoadOk = { ok: true; doc: VectorDocument; warnings: string[] };
export type SvgLoadErr = { ok: false; error: string; warnings: string[] };
export type SvgLoadResult = SvgLoadOk | SvgLoadErr;

function escAttr(s: string): string {
  return s
    .replace(/&/g, "&amp;")
    .replace(/"/g, "&quot;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;");
}

function styleAttrs(style: Style): string {
  const parts = [
    `stroke="${escAttr(style.stroke)}"`,
    `stroke-width="${style.strokeWidth}"`,
  ];
  if (style.fill) parts.push(`fill="${escAttr(style.fill)}"`);
  else parts.push(`fill="none"`);
  return parts.join(" ");
}

function styleFromAttrs(attrs: Record<string, string>): Style {
  return {
    stroke: attrs.stroke ?? DEFAULT_STYLE.stroke,
    strokeWidth: Number(attrs["stroke-width"] ?? DEFAULT_STYLE.strokeWidth),
    fill: attrs.fill && attrs.fill !== "none" ? attrs.fill : undefined,
  };
}

function pointsAttr(samples: InkSample[]): string {
  return samples.map((s) => `${s.x},${s.y}`).join(" ");
}

function serializeAnchor(prefix: string, a: Anchor): string {
  const parts = [`data-infini-${prefix}="${escAttr(a.nodeId)}"`];
  if (a.port) parts.push(`data-infini-${prefix}-port="${escAttr(a.port)}"`);
  if (a.boundary) {
    parts.push(
      `data-infini-${prefix}-boundary="${escAttr(JSON.stringify(a.boundary))}"`,
    );
  }
  return parts.join(" ");
}

function serializeNode(node: DocNode, indent: string): string {
  const id = `data-infini-id="${escAttr(node.id)}"`;
  switch (node.kind) {
    case "frame": {
      const b = node.bounds;
      const open = `${indent}<g data-infini-kind="frame" ${id} data-infini-bounds="${b.minX},${b.minY},${b.maxX},${b.maxY}">`;
      const kids = node.children.map((c) => serializeNode(c, indent + "  ")).join("\n");
      return `${open}\n${kids}\n${indent}</g>`;
    }
    case "group": {
      const open = `${indent}<g data-infini-kind="group" ${id}>`;
      const kids = node.children.map((c) => serializeNode(c, indent + "  ")).join("\n");
      return `${open}\n${kids}\n${indent}</g>`;
    }
    case "smart_group": {
      const b = node.bounds;
      const t = node.transform;
      const open =
        `${indent}<g data-infini-kind="smart-group" ${id} ` +
        `data-infini-bounds="${b.x},${b.y},${b.width},${b.height}" ` +
        `data-infini-transform="${t.x},${t.y},${t.rotation},${t.scaleX},${t.scaleY}" ` +
        `data-infini-ink-scale-mode="${node.inkScaleMode}">`;
      const kids = node.children.map((c) => serializeNode(c, indent + "  ")).join("\n");
      return `${open}\n${kids}\n${indent}</g>`;
    }
    case "ink": {
      const role = node.role ? ` data-infini-role="${node.role}"` : "";
      const samples = escAttr(JSON.stringify(node.samples));
      return (
        `${indent}<polyline data-infini-kind="ink" ${id}${role} ` +
        `points="${escAttr(pointsAttr(node.samples))}" ` +
        `data-infini-samples="${samples}" ${styleAttrs(node.style)} />`
      );
    }
    case "text": {
      const box = node.box;
      const runs = escAttr(JSON.stringify(node.runs));
      const content = escAttr(node.runs.map((r) => r.text).join(""));
      return (
        `${indent}<text data-infini-kind="text" ${id} ` +
        `x="${box.minX}" y="${box.maxY}" ` +
        `data-infini-box="${box.minX},${box.minY},${box.maxX},${box.maxY}" ` +
        `data-infini-runs="${runs}" ${styleAttrs(node.style)}>${content}</text>`
      );
    }
    case "primitive": {
      const g = node.geom;
      const st = styleAttrs(node.style);
      if (g.kind === "rect") {
        return `${indent}<rect data-infini-kind="primitive" ${id} x="${g.x}" y="${g.y}" width="${g.w}" height="${g.h}" ${st} />`;
      }
      if (g.kind === "ellipse") {
        return `${indent}<ellipse data-infini-kind="primitive" ${id} cx="${g.cx}" cy="${g.cy}" rx="${g.rx}" ry="${g.ry}" ${st} />`;
      }
      return `${indent}<line data-infini-kind="primitive" ${id} x1="${g.x1}" y1="${g.y1}" x2="${g.x2}" y2="${g.y2}" ${st} />`;
    }
    case "connector": {
      const from = serializeAnchor("from", node.from);
      const to = serializeAnchor("to", node.to);
      const d =
        node.path && node.path.length >= 2
          ? `M ${node.path[0].x} ${node.path[0].y} L ${node.path[1].x} ${node.path[1].y}`
          : "M 0 0";
      return `${indent}<path data-infini-kind="connector" ${id} ${from} ${to} d="${escAttr(d)}" fill="none" stroke="#5B6B7C" stroke-width="1.5" />`;
    }
  }
}

/**
 * Serialize a materialised tree to Infini SVG profile v1.
 * @implements [SRS-IN-09] tree → SVG profile
 */
export function serializeInfiniSvg(doc: VectorDocument): string {
  const body = doc.rootChildren
    .map((n) => serializeNode(n, "  "))
    .join("\n");
  return (
    `<?xml version="1.0" encoding="UTF-8"?>\n` +
    `<svg xmlns="http://www.w3.org/2000/svg" data-infini-doc-version="${DOC_VERSION}">\n` +
    `${body}\n` +
    `</svg>\n`
  );
}

// --- minimal XML element tree for our profile ---

interface XmlEl {
  tag: string;
  attrs: Record<string, string>;
  children: XmlEl[];
  text: string;
}

function parseAttrs(s: string): Record<string, string> {
  const attrs: Record<string, string> = {};
  const re = /([:@A-Za-z_][\w:.-]*)\s*=\s*"([^"]*)"/g;
  let m: RegExpExecArray | null;
  while ((m = re.exec(s))) {
    attrs[m[1]] = m[2]
      .replace(/&quot;/g, '"')
      .replace(/&lt;/g, "<")
      .replace(/&gt;/g, ">")
      .replace(/&amp;/g, "&");
  }
  return attrs;
}

function parseXml(xml: string): XmlEl {
  const tokens: Array<
    | { type: "open" | "self"; tag: string; attrs: Record<string, string> }
    | { type: "close"; tag: string }
    | { type: "text"; text: string }
  > = [];

  const re =
    /<!--[\s\S]*?-->|<!\[CDATA\[[\s\S]*?\]\]>|<\/([A-Za-z_][\w:.-]*)\s*>|<([A-Za-z_][\w:.-]*)([^>]*?)\/>|<([A-Za-z_][\w:.-]*)([^>]*?)>|([^<]+)/g;
  let m: RegExpExecArray | null;
  while ((m = re.exec(xml))) {
    if (m[0].startsWith("<!--") || m[0].startsWith("<![CDATA")) continue;
    if (m[1]) {
      tokens.push({ type: "close", tag: m[1] });
    } else if (m[2]) {
      tokens.push({ type: "self", tag: m[2], attrs: parseAttrs(m[3] ?? "") });
    } else if (m[4]) {
      tokens.push({ type: "open", tag: m[4], attrs: parseAttrs(m[5] ?? "") });
    } else if (m[6] && m[6].trim()) {
      tokens.push({ type: "text", text: m[6] });
    }
  }

  const root: XmlEl = { tag: "#root", attrs: {}, children: [], text: "" };
  const stack: XmlEl[] = [root];
  for (const t of tokens) {
    if (t.type === "text") {
      stack[stack.length - 1].text += t.text;
      continue;
    }
    if (t.type === "self") {
      stack[stack.length - 1].children.push({
        tag: t.tag,
        attrs: t.attrs,
        children: [],
        text: "",
      });
      continue;
    }
    if (t.type === "open") {
      const el: XmlEl = { tag: t.tag, attrs: t.attrs, children: [], text: "" };
      stack[stack.length - 1].children.push(el);
      stack.push(el);
      continue;
    }
    // close
    if (stack.length > 1) stack.pop();
  }
  return root;
}

function requireId(attrs: Record<string, string>, warnings: string[]): string {
  const id = attrs["data-infini-id"];
  if (!id) throw new Error("missing data-infini-id on Infini element");
  void warnings;
  return id;
}

function parseBounds4(s: string | undefined, label: string): [number, number, number, number] {
  if (!s) throw new Error(`missing ${label}`);
  const parts = s.split(",").map(Number);
  if (parts.length !== 4 || parts.some((n) => !Number.isFinite(n))) {
    throw new Error(`bad ${label}`);
  }
  return [parts[0], parts[1], parts[2], parts[3]];
}

function parseAnchor(
  attrs: Record<string, string>,
  prefix: "from" | "to",
): Anchor {
  const nodeId = attrs[`data-infini-${prefix}`];
  if (!nodeId) throw new Error(`missing connector ${prefix}`);
  const port = attrs[`data-infini-${prefix}-port`];
  const boundaryRaw = attrs[`data-infini-${prefix}-boundary`];
  const anchor: Anchor = { nodeId };
  if (port) anchor.port = port as Anchor["port"];
  if (boundaryRaw) anchor.boundary = JSON.parse(boundaryRaw) as BoundaryParam;
  if (!anchor.port && !anchor.boundary) {
    throw new Error(`connector ${prefix} needs port or boundary`);
  }
  return anchor;
}

function decodeNode(el: XmlEl, warnings: string[]): DocNode | null {
  const kind = el.attrs["data-infini-kind"];
  if (!kind) {
    // Foreign fluff (no Infini kind) — skip with warning; still walk? skip entire subtree.
    warnings.push(`skip foreign <${el.tag}> without data-infini-kind`);
    return null;
  }

  const id = requireId(el.attrs, warnings);

  switch (kind) {
    case "frame": {
      const [minX, minY, maxX, maxY] = parseBounds4(
        el.attrs["data-infini-bounds"],
        "data-infini-bounds",
      );
      const children: DocNode[] = [];
      for (const c of el.children) {
        const n = decodeNode(c, warnings);
        if (n) children.push(n);
      }
      const node: FrameNode = {
        id,
        kind: "frame",
        bounds: { minX, minY, maxX, maxY },
        children,
      };
      return node;
    }
    case "group": {
      const children: DocNode[] = [];
      for (const c of el.children) {
        const n = decodeNode(c, warnings);
        if (n) children.push(n);
      }
      const node: GroupNode = { id, kind: "group", children };
      return node;
    }
    case "smart-group":
    case "smart_group": {
      const [x, y, width, height] = parseBounds4(
        el.attrs["data-infini-bounds"],
        "data-infini-bounds",
      );
      const tr = el.attrs["data-infini-transform"];
      let transform = { ...IDENTITY_SMART_TRANSFORM };
      if (tr) {
        const p = tr.split(",").map(Number);
        if (p.length !== 5 || p.some((n) => !Number.isFinite(n))) {
          throw new Error("bad data-infini-transform");
        }
        transform = {
          x: p[0],
          y: p[1],
          rotation: p[2],
          scaleX: p[3],
          scaleY: p[4],
        };
      }
      const mode = el.attrs["data-infini-ink-scale-mode"] ?? "withBounds";
      if (mode !== "withBounds" && mode !== "fixedInk") {
        throw new Error("bad inkScaleMode");
      }
      const children: InkNode[] = [];
      for (const c of el.children) {
        const n = decodeNode(c, warnings);
        if (!n) continue;
        if (n.kind !== "ink") throw new Error("smart_group_ink_only");
        children.push(n);
      }
      const node: SmartGroupNode = {
        id,
        kind: "smart_group",
        bounds: { x, y, width, height },
        transform,
        inkScaleMode: mode,
        children,
      };
      return node;
    }
    case "ink": {
      const raw = el.attrs["data-infini-samples"];
      if (!raw) throw new Error("ink missing data-infini-samples");
      const samples = JSON.parse(raw) as InkSample[];
      if (!Array.isArray(samples) || samples.some((s) => typeof s.x !== "number" || typeof s.y !== "number")) {
        throw new Error("ink samples invalid");
      }
      const node: InkNode = {
        id,
        kind: "ink",
        samples,
        style: styleFromAttrs(el.attrs),
        role: el.attrs["data-infini-role"] as InkNode["role"] | undefined,
      };
      return node;
    }
    case "text": {
      const [minX, minY, maxX, maxY] = parseBounds4(
        el.attrs["data-infini-box"],
        "data-infini-box",
      );
      const runsRaw = el.attrs["data-infini-runs"];
      const runs = runsRaw
        ? (JSON.parse(runsRaw) as TextNode["runs"])
        : [{ text: el.text.trim() }];
      const node: TextNode = {
        id,
        kind: "text",
        box: { minX, minY, maxX, maxY },
        runs,
        style: styleFromAttrs(el.attrs),
      };
      return node;
    }
    case "primitive": {
      const style = styleFromAttrs(el.attrs);
      let node: PrimitiveNode;
      if (el.tag === "rect") {
        node = {
          id,
          kind: "primitive",
          geom: {
            kind: "rect",
            x: Number(el.attrs.x),
            y: Number(el.attrs.y),
            w: Number(el.attrs.width),
            h: Number(el.attrs.height),
          },
          style,
        };
      } else if (el.tag === "ellipse") {
        node = {
          id,
          kind: "primitive",
          geom: {
            kind: "ellipse",
            cx: Number(el.attrs.cx),
            cy: Number(el.attrs.cy),
            rx: Number(el.attrs.rx),
            ry: Number(el.attrs.ry),
          },
          style,
        };
      } else if (el.tag === "line") {
        node = {
          id,
          kind: "primitive",
          geom: {
            kind: "line",
            x1: Number(el.attrs.x1),
            y1: Number(el.attrs.y1),
            x2: Number(el.attrs.x2),
            y2: Number(el.attrs.y2),
          },
          style,
        };
      } else {
        throw new Error(`unknown primitive tag <${el.tag}>`);
      }
      return node;
    }
    case "connector": {
      const node: ConnectorNode = {
        id,
        kind: "connector",
        from: parseAnchor(el.attrs, "from"),
        to: parseAnchor(el.attrs, "to"),
      };
      return node;
    }
    default:
      throw new Error(`unknown Infini kind "${kind}"`);
  }
}

/**
 * Parse Infini SVG profile. Fail closed on broken Infini-required structure;
 * skip foreign fluff with warnings.
 * @implements [SRS-IN-09] SVG → tree load
 */
export function parseInfiniSvg(svg: string): SvgLoadResult {
  const warnings: string[] = [];
  try {
    const root = parseXml(svg);
    const svgEl = root.children.find((c) => c.tag === "svg");
    if (!svgEl) {
      return { ok: false, error: "missing <svg> root", warnings };
    }
    const ver = svgEl.attrs["data-infini-doc-version"];
    if (ver !== DOC_VERSION) {
      return {
        ok: false,
        error: `missing or unsupported data-infini-doc-version (want ${DOC_VERSION})`,
        warnings,
      };
    }

    const children: DocNode[] = [];
    for (const c of svgEl.children) {
      const n = decodeNode(c, warnings);
      if (n) children.push(n);
    }

    const doc = new VectorDocument();
    doc.replaceTree(children);
    return { ok: true, doc, warnings };
  } catch (e) {
    const error = e instanceof Error ? e.message : String(e);
    return { ok: false, error, warnings };
  }
}

/**
 * Round-trip helper: save then load into a new document.
 * On failure, returns error and does not mutate `prior`.
 */
export function loadInfiniSvgReplacing(
  prior: VectorDocument,
  svg: string,
): SvgLoadResult {
  const result = parseInfiniSvg(svg);
  if (!result.ok) return result;
  prior.replaceTree(result.doc.rootChildren);
  return { ok: true, doc: prior, warnings: result.warnings };
}
