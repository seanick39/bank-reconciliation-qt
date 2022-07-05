#ifndef BR_ABOUTDIALOG_H
#define BR_ABOUTDIALOG_H

#include <QDialog>

#include <ui_aboutdialog.h>

namespace br_ui {

class AboutDialog : public QDialog, Ui_Dialog {
public:
  explicit AboutDialog(QWidget *parent = nullptr)
      : QDialog(parent), Ui_Dialog() {
    setupUi(this);
    QPixmap image(":/appicon.png");
    lblAboutIcon->setPixmap(image);
  }
};

} // namespace br_ui

#endif // BR_ABOUTDIALOG_H
