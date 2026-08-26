#pragma once

/**
 * Qt IPixelSink adapters — include only from Qt targets (TabletCanvas, ToolCanvas).
 */

#include "rendering/rendering.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPen>

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
        m_painter.fillRect(m_image->rect(), Qt::white);
    }

    void clearRect(double x, double y, double w, double h) override
    {
        m_painter.fillRect(QRectF(x, y, w, h), Qt::white);
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
        QPainterPath path;
        path.moveTo(poly.pts[0].first, poly.pts[0].second);
        for (size_t i = 1; i < poly.pts.size(); ++i)
            path.lineTo(poly.pts[i].first, poly.pts[i].second);
        m_painter.drawPath(path);
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
        QPainterPath path;
        path.moveTo(poly.pts[0].first, poly.pts[0].second);
        for (size_t i = 1; i < poly.pts.size(); ++i)
            path.lineTo(poly.pts[i].first, poly.pts[i].second);
        m_painter->drawPath(path);
    }

    void end() override {}

private:
    QPainter *m_painter = nullptr;
};

} // namespace render
} // namespace epaper
