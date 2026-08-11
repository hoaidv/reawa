/**
 * Infini ink-box ToolStrip — Selection · Ink-box + create CTA.
 * Icons from design system assets (UI-IN-02).
 * @implements [SRS-IN-14] ToolStrip + create refuse affordance
 */

import type { InfiniTool } from "../document/selection";
import iconSelection from "../assets/icons/icon-tool-selection.svg";
import iconInkBox from "../assets/icons/icon-tool-ink-box.svg";

export interface ToolStripProps {
  tool: InfiniTool;
  onToolChange: (tool: InfiniTool) => void;
  onCreateSmartGroup: () => void;
  createDisabled?: boolean;
  refuseReason?: string | null;
}

export function ToolStrip({
  tool,
  onToolChange,
  onCreateSmartGroup,
  createDisabled,
  refuseReason,
}: ToolStripProps) {
  return (
    <div className="c-tool-strip" data-region="ToolStrip" role="toolbar" aria-label="Ink-box tools">
      <button
        type="button"
        className="c-tool-btn"
        id="tool.selection"
        aria-pressed={tool === "selection"}
        aria-label="Selection tool"
        onClick={() => onToolChange("selection")}
        onPointerDown={(e) => e.stopPropagation()}
      >
        <img src={iconSelection} alt="" width={20} height={20} />
        Select
      </button>
      <button
        type="button"
        className="c-tool-btn"
        id="tool.ink_box"
        aria-pressed={tool === "ink_box"}
        aria-label="Ink-box tool"
        onClick={() => onToolChange("ink_box")}
        onPointerDown={(e) => e.stopPropagation()}
      >
        <img src={iconInkBox} alt="" width={20} height={20} />
        Ink-box
      </button>
      <button
        type="button"
        className="c-create-cta"
        id="cta.create_smart_group"
        disabled={createDisabled}
        aria-label="Create Smart Group from selection"
        onClick={(e) => {
          e.stopPropagation();
          onCreateSmartGroup();
        }}
        onPointerDown={(e) => e.stopPropagation()}
      >
        Group
      </button>
      {refuseReason ? (
        <p
          className="c-create-refused"
          id="ind.create_refused_no_surround"
          data-state="tool.selection.create_refused"
          role="status"
        >
          {refuseReason === "no_surround"
            ? "Need a surround stroke"
            : refuseReason === "need_at_least_two"
              ? "Select 2+ strokes"
              : "Create refused"}
        </p>
      ) : null}
    </div>
  );
}
