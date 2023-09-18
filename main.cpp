#include "mainwindow.h"
#include "login.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QCoreApplication::setOrganizationName("");
    QCoreApplication::setOrganizationDomain("");
    QCoreApplication::setApplicationName("Diagnostics tools");
    w.setWindowTitle("Diagnostics tools");
    //QFont font  = a.font();
    //font.setPointSize(12);
    //a.setFont(font);
    Login login;
    w.setLoginWidget(&login);
    login.setMainWindow(&w);
    login.show();
    login.autoLogin();
    w.setAppFilepath(a.applicationFilePath());
    //w.show();
    return a.exec();
}
