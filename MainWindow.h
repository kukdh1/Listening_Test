#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QtWidgets/QMainWindow>
#include <QtWidgets/qfiledialog.h>
#include <QtCore/qtimer.h>
#include "ui_MainWindow.h"
#include "ui_Progress.h"
#include "Model.h"
#include "Audio.h"

#define SAFE_DELETE(object)   { if (object) { delete object; object = NULL; } }

#define STRING_UI_FILE_NOT_SELECTED   "Stopped"
#define STRING_UI_READ_SONG           "Decoding..."
#define STRING_UI_DOWNSAMPLING        "Downsampling..."
#define STRING_UI_DOWNQUANTIZATION    "Downquantization..."
#define STRING_UI_PLAYING_FIRST       "Playing First..."
#define STRING_UI_PLAYING_SECOND      "Playing Second..."

class MainWindow : public QMainWindow
{
  Q_OBJECT

  public:
    MainWindow(QWidget *parent = NULL);
    ~MainWindow();

  private:
    Ui::Listening_TestClass ui;
    QTimer timer;

    SongModel songModel;
    ResultModel resultModel;
    AudioSystem audio;

    SongSession *session;
};

class ProgressDialog : public QDialog, public Ui_Progress_Dialog {
  Q_OBJECT

  public:
    ProgressDialog(QWidget *parent = NULL);
};

#endif // MAINWINDOW_H
