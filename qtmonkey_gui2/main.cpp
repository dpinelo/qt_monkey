#include "testsuitesdlg.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("Asycom Global Service");
    a.setApplicationName("Qt Monkey Gui v2");
    a.setApplicationVersion("1.0");

    TestSuitesDlg w;
    w.show();

    return a.exec();
}
