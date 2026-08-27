#include "selection_intent_applier.hpp"

namespace epaper {
namespace tools {

void SelectionIntentApplier::apply(const epaper::selection::SelectionResult &r)
{
    using epaper::selection::SelectionIntent;
    using epaper::selection::has;
    if (!m_tool)
        return;

    if (has(r.intent, SelectionIntent::ResetDrag) && m_resetManip)
        m_resetManip();
    if (has(r.intent, SelectionIntent::StrokeWaveformOn))
        m_tool->setStrokeWaveform(true);
    if (has(r.intent, SelectionIntent::StrokeWaveformOff))
        m_tool->setStrokeWaveform(false);
    if (has(r.intent, SelectionIntent::ChromeChanged)) {
        m_tool->resetTransientChromeFlags();
        m_tool->emitChromeChanged();
    }
    if (has(r.intent, SelectionIntent::SyncToolCanvas))
        m_tool->syncOverlayPresence();
    if (has(r.intent, SelectionIntent::DamageLive) && r.hasDamage) {
        const QRectF live =
            QRectF(QPointF(r.damageA.x, r.damageA.y), QPointF(r.damageB.x, r.damageB.y))
                .normalized()
                .adjusted(-8, -8, 8, 8);
        m_tool->damageChrome(live);
    }
    if (has(r.intent, SelectionIntent::DamageSegment) && r.hasDamage) {
        const QRectF seg =
            QRectF(QPointF(r.damageA.x, r.damageA.y), QPointF(r.damageB.x, r.damageB.y))
                .normalized()
                .adjusted(-8, -8, 8, 8);
        m_tool->damageChromeSegment(seg);
    }
    if (has(r.intent, SelectionIntent::DebugChanged) && !r.debugInfo.empty() && m_setDebug)
        m_setDebug(r.debugInfo);
    if (has(r.intent, SelectionIntent::RefreshChrome))
        m_tool->requestChromeRefresh();
}

} // namespace tools
} // namespace epaper
