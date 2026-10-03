// @implements [STORY-EP-081] empty reMarkable 2 shell
#include <QGuiApplication>

int main(int argc, char *argv[])
{
#ifdef __arm__
    qputenv("QMLSCENE_DEVICE", "epaper");
    qputenv("QT_QPA_PLATFORM", "epaper:enable_fonts");
    qputenv("QT_QUICK_BACKEND", "epaper");
#endif

    QGuiApplication app(argc, argv);
    return app.exec();
}
