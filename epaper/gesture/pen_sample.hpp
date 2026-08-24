#pragma once

#include <QMetaType>
#include <QPointF>
#include <QtGlobal>

namespace epaper::input {

/**
 * Digitizer channels reported on one pen sample. Unset = not reported.
 * Neutral on purpose: the raw input filter and the canvas exchange samples
 * without either including the other's header.
 * @implements [SRS-EP-09] digitizer channels on pen ingest
 */
struct PenSample {
    qreal pressure = 0;
    bool hasTilt = false;
    qreal tiltX = 0;
    qreal tiltY = 0;
    bool hasDistance = false;
    qreal distance = 0;
    bool hasTimestamp = false;
    qreal timestamp = 0;
    bool hasRotation = false;
    qreal rotation = 0;
    bool hasTangential = false;
    qreal tangential = 0;
};

/**
 * Digitizer (landscape) → panel framebuffer (portrait). Verified Round 19
 * (RENDERING.md); Infini orientation changes the sync frame, never this.
 * @implements [SRS-EP-01] panel remap on pen ingest
 */
inline QPointF mapPanel(const QPointF &raw, qreal panelW, qreal panelH)
{
    const qreal w = qMax<qreal>(1.0, panelW);
    const qreal h = qMax<qreal>(1.0, panelH);
    return QPointF(raw.y() * (w / h), h - raw.x() * (h / w));
}

} // namespace epaper::input

Q_DECLARE_METATYPE(epaper::input::PenSample)
