#include "finger_intent_applier.hpp"

#include "../tabletcanvasitem.h"
#include "selection_manip_controller.hpp"

namespace epaper {
namespace tools {

void FingerIntentApplier::apply(const epaper::fingergesture::FingerResult &r,
                                const QPointF &panel)
{
    using epaper::fingergesture::FingerIntent;
    using epaper::fingergesture::has;
    if (!m_doc || !m_tool || !m_selManip)
        return;

    if (has(r.intent, FingerIntent::ArmSelFreeform))
        m_doc->setExclusiveTool(QStringLiteral("sel_freeform"));
    if (has(r.intent, FingerIntent::BeginHandleResize))
        m_selManip->tryBeginHandleAtPanel(panel, epaper::handtouch::kFingerHandleHitDu);
    if (has(r.intent, FingerIntent::BeginSelectMove))
        m_selManip->beginSelectionGesture(panel);
    if (has(r.intent, FingerIntent::UpdateSelection))
        m_selManip->updateSelectionGesture(panel);
    if (has(r.intent, FingerIntent::ApplyCameraRegion) && r.hasRegion)
        m_doc->applyCamera(r.region, false);
    if (has(r.intent, FingerIntent::PublishViewportLive) && m_doc->surface())
        m_doc->surface()->maybePublishLocalViewport(false);
    if (has(r.intent, FingerIntent::PublishViewportSettle) && m_doc->surface())
        m_doc->surface()->maybePublishLocalViewport(true);
    if (has(r.intent, FingerIntent::ScheduleRasterizeLive) && m_doc->surface())
        m_doc->surface()->scheduleDocumentRasterize(false);
    if (has(r.intent, FingerIntent::ScheduleRasterizeSettle) && m_doc->surface())
        m_doc->surface()->scheduleDocumentRasterize(true);
    if (has(r.intent, FingerIntent::ClearSelection))
        m_selManip->clearSelection();
    if (has(r.intent, FingerIntent::RefreshChrome))
        m_tool->requestChromeRefresh();
    if (has(r.intent, FingerIntent::EndSelectionGesture))
        m_selManip->endSelectionGesture();
    if (has(r.intent, FingerIntent::AbortManip) && m_abortManip)
        m_abortManip();
}

} // namespace tools
} // namespace epaper
