/**
 * Desktop FollowToggle — WindowFrame chrome, not a ToolChip, not WorldLayer.
 * @implements [SRS-IN-27] FollowToggle icon toggle
 * @implements [SRS-IN-26] opt-in epaper_to_infini
 */

import iconOff from "../assets/icons/icon-viewport-follow-infini.svg";
import iconOn from "../assets/icons/icon-viewport-follow-infini-on.svg";
import iconPeer from "../assets/icons/icon-viewport-follow-infini-peer.svg";
import iconOffline from "../assets/icons/icon-viewport-follow-infini-offline.svg";
import type { FollowToggleView } from "../session/viewportFollow";

export interface FollowToggleProps {
  view: FollowToggleView;
  onToggle: () => void;
}

function iconFor(view: FollowToggleView): string {
  if (!view.disabled && view.pressed) return iconOn;
  if (view.ui === "follow.peer_following_you") return iconPeer;
  if (view.ui === "follow.connection_lost") return iconOffline;
  return iconOff;
}

export function FollowToggle({ view, onToggle }: FollowToggleProps) {
  return (
    <div className="c-follow-cluster" data-region="FollowToggle" data-state={view.ui}>
      <button
        type="button"
        className="c-follow-toggle"
        data-control="btn.viewport_follow"
        aria-pressed={view.pressed}
        aria-label={view.ariaLabel}
        aria-description={view.ariaDescription}
        disabled={view.disabled}
        onClick={(e) => {
          e.stopPropagation();
          if (view.disabled) return;
          onToggle();
        }}
        onPointerDown={(e) => e.stopPropagation()}
      >
        <img src={iconFor(view)} alt="" />
      </button>
      <p className="c-follow-caption">{view.caption}</p>
    </div>
  );
}
