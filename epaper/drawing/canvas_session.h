#pragma once

/**
 * Shared epaper scene state for TabletCanvas (surface) and ToolCanvas (interaction).
 * QObject so both sides connect to coarse NOTIFY signals instead of poking peers.
 */

#include "canvas_frame.hpp"
#include "document/device_document.hpp"
#include "document/viewport_follow.hpp"
#include "primary_toolbar.hpp"

#include <QObject>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QVector>

#include <optional>
#include <string>

/** Rest-pose connector polylines in panel space — Tablet origin hole (stroke, not AABB). */
struct OriginConnStroke {
    QVector<QPointF> panel;
    qreal width = 4;
};

/** Snapshot for TabletCanvas white-hole during live manip — POD shared via CanvasSession. */
struct OriginPunchSnapshot {
    QRectF panelRect;
    QVector<OriginConnStroke> connStrokes;
};

class CanvasSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString exclusiveTool READ exclusiveTool NOTIFY exclusiveToolChanged)
    Q_PROPERTY(bool recogInkBox READ recogInkBox NOTIFY recogChanged)
    Q_PROPERTY(bool recogConnector READ recogConnector NOTIFY recogChanged)
    Q_PROPERTY(QString followDirection READ followDirection NOTIFY followChanged)

public:
    explicit CanvasSession(QObject *parent = nullptr);

    epaper::document::DeviceDocument document;
    epaper::canvasframe::CanvasFrame frame;
    epaper::toolchip::ChipModel chip;
    epaper::follow::FollowSession follow;

    QString exclusiveTool() const;
    bool recogInkBox() const { return chip.recogInkBox; }
    bool recogConnector() const { return chip.recogConnector; }
    QString followDirection() const { return m_followDirection; }

    /** Update exclusive tool; emits exclusiveToolChanged when changed. */
    bool setExclusiveTool(const QString &mode);
    bool flipRecogInkBox();
    bool flipRecogConnector();

    void applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid);
    void noteDocumentMutated();
    /** Emit cameraChanged when frame was mutated outside applyCamera (e.g. applyFrameIntent). */
    void noteCameraChanged();
    void setFollowDirection(const QString &id);
    void syncFollowDirectionFromSession();

    /** Live-manip origin hole for Tablet paint — set by Tool, read by Tablet. */
    const std::optional<OriginPunchSnapshot> &liveManipOrigin() const { return m_liveManipOrigin; }
    void setLiveManipOrigin(OriginPunchSnapshot punch);
    void clearLiveManipOrigin();

signals:
    void exclusiveToolChanged();
    void recogChanged();
    void cameraChanged();
    void documentMutated();
    void followChanged();

private:
    QString m_followDirection = QStringLiteral("none");
    std::optional<OriginPunchSnapshot> m_liveManipOrigin;
};
