#pragma once

/**
 * TabletCanvas Surface → InkSink adapter.
 * @implements [SRS-EP-04]
 */

#include "ink_sink.hpp"
#include "../tabletcanvasitem.h"

namespace epaper {
namespace tools {

class TabletInkSink final : public InkSink {
public:
    explicit TabletInkSink(TabletCanvasItem *surface)
        : m_surface(surface)
    {
    }

    void ingestPen(QEvent::Type type, const QPointF &panel, RawPt raw,
                   const epaper::input::PenSample &ch) override
    {
        if (!m_surface)
            return;
        TabletCanvasItem::RawPt r{raw.x, raw.y};
        m_surface->ingestPen(type, panel, r, ch);
    }

    void clearStash() override
    {
        if (m_surface)
            m_surface->clearStash();
    }

    void cancelActiveStroke() override
    {
        if (m_surface)
            m_surface->cancelActiveStroke();
    }

    bool strokeActive() const override
    {
        return m_surface && m_surface->strokeActive();
    }

    TabletCanvasItem::IngestChannels stashedChannels(const QPointF &panel, RawPt *raw) const
    {
        TabletCanvasItem::RawPt r;
        const auto ch = m_surface->stashedChannels(panel, &r);
        if (raw) {
            raw->x = r.x;
            raw->y = r.y;
        }
        return ch;
    }

private:
    TabletCanvasItem *m_surface = nullptr;
};

} // namespace tools
} // namespace epaper
