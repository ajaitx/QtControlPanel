#include "mainwindow.h"
#include "version.h"
#include <QApplication>
#include <QWindow>
#include <cstring>

static void handleVisibleChanged(){
    if (!QGuiApplication::inputMethod()->isVisible())
        return;

    QList<QWindow*> allWindows = QGuiApplication::allWindows();
    foreach(QWindow *window, allWindows) {
        const char* windowClassName = window->metaObject()->className();
        if (qstrcmp(windowClassName, "QtVirtualKeyboard::InputView") == 0) {
            if(QObject *keyboard = window->findChild<QObject *>("keyboard")){
                QRect r = window->geometry();
                r.moveTop(keyboard->property("y").toDouble());
                window->setMask(r);
                return;
            }
        }
    }
}

int main(int argc, char *argv[])
{
    int exitCode = 0;
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));
    QApplication a(argc, argv);
    QObject::connect(QGuiApplication::inputMethod(), &QInputMethod::visibleChanged, &handleVisibleChanged);
    MainWindow w;
    w.show();
    exitCode = a.exec();

    return exitCode;
}
