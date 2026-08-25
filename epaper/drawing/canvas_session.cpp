#include "canvas_session.h"

#include "document/hand_touch.hpp"

CanvasSession::CanvasSession(QObject *parent)
    : QObject(parent)
{
}

QString CanvasSession::exclusiveTool() const
{
    return QString::fromStdString(chip.exclusive);
}

bool CanvasSession::setExclusiveTool(const QString &mode)
{
    if (!chip.setExclusive(mode.toStdString()))
        return false;
    emit exclusiveToolChanged();
    return true;
}

bool CanvasSession::flipRecogInkBox()
{
    if (!chip.flipRecogInkBox())
        return false;
    emit recogChanged();
    return true;
}

bool CanvasSession::flipRecogConnector()
{
    if (!chip.flipRecogConnector())
        return false;
    emit recogChanged();
    return true;
}

void CanvasSession::applyCamera(const epaper::handtouch::WorldAabb &region, bool markValid)
{
    const auto intent = frame.applyDrawingRegion(region, markValid);
    Q_UNUSED(intent);
    emit cameraChanged();
}

void CanvasSession::noteDocumentMutated()
{
    emit documentMutated();
}

void CanvasSession::setFollowDirection(const QString &id)
{
    if (m_followDirection == id)
        return;
    m_followDirection = id;
    emit followChanged();
}

void CanvasSession::syncFollowDirectionFromSession()
{
    setFollowDirection(QString::fromLatin1(epaper::handtouch::followId(follow.direction)));
}
