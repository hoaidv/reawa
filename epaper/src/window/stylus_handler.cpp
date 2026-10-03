#include "stylus_handler.hpp"

StylusHandler::StylusHandler(QObject *parent)
    : QObject(parent)
{
}

void StylusHandler::press(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY)
{
    emit stylusPress(x, y, pressure, tiltX, tiltY);
}

void StylusHandler::move(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY)
{
    emit stylusMove(x, y, pressure, tiltX, tiltY);
}

void StylusHandler::release(qreal x, qreal y, qreal pressure, qreal tiltX, qreal tiltY)
{
    emit stylusRelease(x, y, pressure, tiltX, tiltY);
}
