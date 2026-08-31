#include "canvas_session.h"

#include "document/hand_touch.hpp"

#include <QSettings>

CanvasSession::CanvasSession(QObject *parent)
    : QObject(parent)
{
    loadPersisted();
}

void CanvasSession::loadPersisted()
{
    QSettings settings;
    const QString last = settings.value(QStringLiteral("epaper/lastUsedEraser")).toString();
    if (epaper::toolchip::isEraserId(last.toStdString()))
        chip.lastUsedEraser = last.toStdString();
    if (settings.contains(QStringLiteral("epaper/eraseBrushHover")))
        chip.eraseBrushHover = settings.value(QStringLiteral("epaper/eraseBrushHover")).toBool();
}

void CanvasSession::persistLastUsed() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("epaper/lastUsedEraser"),
                      QString::fromStdString(chip.lastUsedEraser));
}

QString CanvasSession::exclusiveTool() const
{
    return QString::fromStdString(chip.exclusive);
}

bool CanvasSession::setExclusiveTool(const QString &mode)
{
    if (!chip.setExclusive(mode.toStdString()))
        return false;
    persistLastUsed();
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

bool CanvasSession::togglePenEraser()
{
    if (!chip.togglePenEraser())
        return false;
    persistLastUsed();
    emit exclusiveToolChanged();
    return true;
}

bool CanvasSession::beginTempErase()
{
    if (!chip.beginTempErase())
        return false;
    persistLastUsed();
    emit exclusiveToolChanged();
    return true;
}

bool CanvasSession::endTempErase()
{
    if (!chip.endTempErase())
        return false;
    persistLastUsed();
    emit exclusiveToolChanged();
    return true;
}

bool CanvasSession::beginNibErase()
{
    if (!chip.beginNibErase())
        return false;
    persistLastUsed();
    emit exclusiveToolChanged();
    return true;
}

bool CanvasSession::endNibErase()
{
    if (!chip.endNibErase())
        return false;
    persistLastUsed();
    emit exclusiveToolChanged();
    return true;
}

void CanvasSession::setEraseBrushHover(bool on)
{
    if (chip.eraseBrushHover == on)
        return;
    chip.eraseBrushHover = on;
    QSettings settings;
    settings.setValue(QStringLiteral("epaper/eraseBrushHover"), on);
    emit eraseBrushHoverChanged();
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

void CanvasSession::emitRecogChrome(int kind, const QStringList &ids)
{
    emit recogChrome(kind, ids);
}

void CanvasSession::noteCameraChanged()
{
    emit cameraChanged();
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

void CanvasSession::setLiveManipSuppressIds(std::unordered_set<std::string> ids)
{
    m_liveManipSuppressIds = std::move(ids);
}

void CanvasSession::clearLiveManipSuppressIds()
{
    m_liveManipSuppressIds.clear();
}
