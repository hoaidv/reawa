#pragma once

/**
 * Qt IPixelSink adapters — include only from Qt targets (TabletCanvas, ToolCanvas).
 */

#include "rendering/rendering.hpp"

#include <QPainter>
#include <QPen>
#include <QPolygonF>

namespace epaper {
namespace render {

class QImagePixelSink : public IPixelSink {
public:
    explicit QImagePixelSink(QImage *image)
        : m_image(image)
    {
    }

    void begin(bool sharp) override
    {
        if (!m_image || m_image->isNull())
            return;
        m_painter.begin(m_image);
        m_painter.setRenderHint(QPainter::Antialiasing, sharp);
    }

    void clearFull() override
    {
        if (!m_image)
            return;
        m_painter.setClipping(false);
        m_painter.fillRect(m_image->rect(), Qt::white);
    }

    void clearRect(double x, double y, double w, double h) override
    {
        // [D03] Clip so overlapping polylines cannot overpaint outside the dirty halo.
        const QRectF r(x, y, w, h);
        m_painter.setClipRect(r);
        m_painter.fillRect(r, Qt::white);
    }

    void drawPolyline(const PanelPolyline &poly) override
    {
        if (poly.pts.size() < 2)
            return;
        QPen pen(Qt::black);
        pen.setWidthF(poly.width);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        m_painter.setPen(pen);
        m_painter.setBrush(Qt::NoBrush);
        QPolygonF pts;
        pts.reserve(int(poly.pts.size()));
        for (const auto &pt : poly.pts)
            pts << QPointF(pt.first, pt.second);
        m_painter.drawPolyline(pts);
    }

    void end() override
    {
        if (m_painter.isActive())
            m_painter.end();
    }

private:
    QImage *m_image = nullptr;
    QPainter m_painter;
};

class QPainterPixelSink : public IPixelSink {
public:
    explicit QPainterPixelSink(QPainter *painter)
        : m_painter(painter)
    {
    }

    void begin(bool sharp) override
    {
        if (!m_painter)
            return;
        m_painter->setRenderHint(QPainter::Antialiasing, sharp);
    }

    void clearFull() override {}
    void clearRect(double, double, double, double) override {}

    void drawPolyline(const PanelPolyline &poly) override
    {
        if (!m_painter || poly.pts.size() < 2)
            return;
        QPen pen(Qt::black);
        pen.setWidthF(poly.width);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        m_painter->setPen(pen);
        m_painter->setBrush(Qt::NoBrush);
        QPolygonF pts;
        pts.reserve(int(poly.pts.size()));
        for (const auto &pt : poly.pts)
            pts << QPointF(pt.first, pt.second);
        m_painter->drawPolyline(pts);
    }

    void end() override {}

private:
    QPainter *m_painter = nullptr;
};

} // namespace render
} // namespace epaper
