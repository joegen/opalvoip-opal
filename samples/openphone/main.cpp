#include <qapplication.h>
#include <ptlib.h>
#include "mainwindow.h"
#include "version.h"


class OpenPhoneApp : public PProcess
{
  PCLASSINFO(OpenPhoneApp, PProcess);
  OpenPhoneApp()
    : PProcess("Equivalence", "OpenPhone",
               MAJOR_VERSION, MINOR_VERSION, BUILD_TYPE, BUILD_NUMBER)
    { }
  void Main()
    { } // Dummy function
};


int main( int argc, char ** argv )
{
    OpenPhoneApp a1;
    QApplication a( argc, argv );
    MainWindow *w = new MainWindow;
    w->show();
    a.connect( &a, SIGNAL( lastWindowClosed() ), &a, SLOT( quit() ) );
    return a.exec();
}
