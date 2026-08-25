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
#include <QString>

#include <string>

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
    void setFollowDirection(const QString &id);
    void syncFollowDirectionFromSession();

signals:
    void exclusiveToolChanged();
    void recogChanged();
    void cameraChanged();
    void documentMutated();
    void followChanged();

private:
    QString m_followDirection = QStringLiteral("none");
};
