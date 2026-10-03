#pragma once

/**
 * InkSink — live stroke pipe to Tablet (not document blit in general).
 * @implements [SRS-EP-04]
 */

#include "input/pen_sample.hpp"

#include <QEvent>
#include <QPointF>

namespace epaper {
namespace tools {

struct RawPt {
    qreal x = 0;
    qreal y = 0;
};

class InkSink {
public:
    virtual ~InkSink() = default;
    virtual void ingestPen(QEvent::Type type, const QPointF &panel, RawPt raw,
                           const epaper::input::PenSample &ch) = 0;
    virtual void clearStash() = 0;
    virtual void cancelActiveStroke() = 0;
    virtual bool strokeActive() const = 0;
};

} // namespace tools
} // namespace epaper
