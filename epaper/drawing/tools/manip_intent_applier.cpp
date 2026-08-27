#include "manip_intent_applier.hpp"

namespace epaper {
namespace tools {

void ManipIntentApplier::apply(const epaper::manip::ManipResult &r,
                               epaper::manip::ManipSession &manip, bool restoreOrigin,
                               int *toolIntentSeq)
{
    using epaper::manip::ManipIntent;
    using epaper::manip::has;
    if (!m_doc || !m_tool)
        return;

    if (has(r.intent, ManipIntent::BeginGesture))
        m_doc->beginGesture();
    if (has(r.intent, ManipIntent::ApplyLiveGeometry)) {
        if (restoreOrigin)
            m_doc->applyLiveSmartGeometry(manip.nodeId, manip.originT, manip.originB);
        else
            m_doc->applyLiveSmartGeometry(manip.nodeId, manip.liveT, manip.liveB);
    }
    if (has(r.intent, ManipIntent::RefreshBoundConnectors))
        m_doc->refreshConnectorsBoundTo(manip.nodeId);
    if (has(r.intent, ManipIntent::PreviewFrame))
        m_doc->previewManipulationFrame();
    if (has(r.intent, ManipIntent::SendPreview))
        m_tool->sendManipPreview(manip.resizing());
    if (has(r.intent, ManipIntent::RefreshAllConnectors)
        && !has(r.intent, ManipIntent::CommitTransform))
        m_doc->refreshAllConnectorWarps();
    if (has(r.intent, ManipIntent::AbortGesture))
        m_doc->abortGesture();
    if (has(r.intent, ManipIntent::CommitTransform)) {
        if (toolIntentSeq)
            ++*toolIntentSeq;
        const std::string opId =
            std::string("sst-") + std::to_string(toolIntentSeq ? *toolIntentSeq : 0);
        const epaper::document::SmartBounds liveB = manip.liveB;
        const epaper::document::SmartBounds *bptr = r.resized ? &liveB : nullptr;
        m_doc->commitSetSmartTransform(opId, manip.nodeId, manip.liveT, bptr);
        m_tool->clearOriginPanelRect();
        m_doc->clearLiveManipSuppressIds();
    }
    if (has(r.intent, ManipIntent::RefreshAllConnectors)
        && has(r.intent, ManipIntent::CommitTransform))
        m_doc->refreshAllConnectorWarps();
    if (has(r.intent, ManipIntent::ScheduleRasterize)) {
        if (has(r.intent, ManipIntent::AbortGesture))
            m_doc->clearLiveManipSuppressIds();
        m_doc->noteDocumentMutated();
    }
    if (has(r.intent, ManipIntent::RefreshChrome))
        m_tool->requestChromeRefresh();
    if (has(r.intent, ManipIntent::Redraw))
        m_tool->redrawLiveManip();
    if (has(r.intent, ManipIntent::NotifyHistory))
        m_doc->notifyHistory();
    if (has(r.intent, ManipIntent::FlushWire))
        m_doc->flushWire();
}

} // namespace tools
} // namespace epaper
