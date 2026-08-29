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
#include <unordered_set>

class CanvasSession : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString exclusiveTool READ exclusiveTool NOTIFY exclusiveToolChanged)
    Q_PROPERTY(bool recogInkBox READ recogInkBox NOTIFY recogChanged)
    Q_PROPERTY(bool recogConnector READ recogConnector NOTIFY recogChanged)
    Q_PROPERTY(QString followDirection READ followDirection NOTIFY followChanged)
    Q_PROPERTY(bool eraseBrushHover READ eraseBrushHover WRITE setEraseBrushHover NOTIFY
                   eraseBrushHoverChanged)

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
    bool togglePenEraser();
    bool beginTempErase();
    bool endTempErase();
    bool beginNibErase();
    bool endNibErase();
    bool eraseBrushHover() const { return chip.eraseBrushHover; }
    void setEraseBrushHover(bool on);

    void applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid);
    void noteDocumentMutated();
    /** Emit cameraChanged when frame was mutated outside applyCamera (e.g. applyFrameIntent). */
    void noteCameraChanged();
    void setFollowDirection(const QString &id);
    void syncFollowDirectionFromSession();

    /** Live-manip subtree ids omitted from Tablet rasterize — set by Tool, read by Tablet. */
    const std::unordered_set<std::string> &liveManipSuppressIds() const
    {
        return m_liveManipSuppressIds;
    }
    void setLiveManipSuppressIds(std::unordered_set<std::string> ids);
    void clearLiveManipSuppressIds();

signals:
    void exclusiveToolChanged();
    void recogChanged();
    void cameraChanged();
    void documentMutated();
    void followChanged();
    void eraseBrushHoverChanged();

private:
    void loadPersisted();
    void persistLastUsed() const;

    QString m_followDirection = QStringLiteral("none");
    std::unordered_set<std::string> m_liveManipSuppressIds;
};
