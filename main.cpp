#include "Player.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Player w;
    w.show();
    return a.exec();
}
