#include <QApplication>
#include <QResource>

#include "BR_MainWindow.h"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QResource::registerResource("br-res.qrc");
  QApplication::setWindowIcon(QIcon(":/appicon.png"));
  br_ui::BR_MainWindow mainWindow;
  mainWindow.show();
  return QApplication::exec();
}
