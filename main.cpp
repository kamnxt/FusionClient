#include "mainwindow.h"
#include <QApplication>

#ifdef _WIN32
    #define OS Windows
#elif __unix__ // all unices not caught above, might not be set
    #define OS Unix
#elif __linux__
    #define OS Linux
#elif __APPLE__
    #include "TargetConditionals.h"
    #if TARGET_OS_MAC //We only want to detect macs, not ios
        #define OS Mac
    #endif
#endif

/** THIS IS FUSION **/

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    if(w.updateInProgress)
        return 0;


    QTranslator qtTranslator;
    qtTranslator.load("FusionClient_lang");
    a.installTranslator(&qtTranslator);


    w.setWindowFlags(Qt::Window |Qt::FramelessWindowHint|Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    w.show();

    return a.exec();
}
