#include "MainWindow.h"

MainWindow::MainWindow(QWidget *parent)
  : QMainWindow(parent),
    songModel(parent),
    resultModel(parent) {
  // Initialization
  ui.setupUi(this);
  session = NULL;

  // Assign model for file list
  ui.fileTableView->setModel(&songModel);
  ui.fileTableView->setColumnWidth(0, 480);
  ui.fileTableView->setColumnWidth(1, 120);

  // Assign model for result list
  ui.resultTableView->setModel(&resultModel);
  ui.resultTableView->setColumnWidth(0, 480);
  ui.resultTableView->setColumnWidth(1, 150);

  // Connect handler
  connect(&timer, &QTimer::timeout, [&]() {
    if (session) {
      if (session->isPlaying()) {
        uint32_t cur, max;
        std::function<std::string(uint32_t)> convert = [](uint32_t ms) {
          char temp[16];
          uint32_t hour;
          uint8_t min;
          uint8_t sec;

          hour = ms / 1000 / 60 / 60;
          min = ms / 1000 / 60 % 60;
          sec = ms / 1000 % 60;

#ifdef WIN32
          sprintf_s(temp, "%d:%02d:%02d", hour, min, sec);
#else
          sprintf(temp, "%d:%02d:%02d", hour, min, sec);
#endif

          return std::string(temp);
        };

        session->getTimeInfo(cur, max);
        ui.timeSlider->setRange(0, max);
        ui.timeSlider->setValue(cur);

        std::string str;

        str.append(convert(cur));
        str.append(" / ");
        str.append(convert(max));

        ui.timeLabel->setText(str.c_str());
      }
    }
  });
  connect(ui.addFilebutton, &QPushButton::clicked, [&]() {
    QStringList pathlist = QFileDialog::getOpenFileNames();
    
    if (pathlist.length() > 0) {
      for (auto path : pathlist) {
        uint32_t samplerate;
        uint8_t bitdepth;
        std::string stdpath = path.toStdString();
        
        if (audio.getInfo(stdpath, samplerate, bitdepth)) {
          Song song(path, samplerate, bitdepth);
          
          songModel.appendSong(song);
        }
      }
    }
  });
  connect(ui.deleteFileButton, &QPushButton::clicked, [&]() {
    QItemSelectionModel *select = ui.fileTableView->selectionModel();
    
    if (select->hasSelection()) {
      QModelIndexList list = select->selectedRows();

      SAFE_DELETE(session);

      songModel.removeSong(list.at(0).row());
    }
  });
  connect(ui.testSamplingRateButton, &QPushButton::clicked, [&]() {
    if (session) {
      ProgressDialog progress;

      progress.show();

      session->readSound();
      session->convertSamplingrate();

      progress.close();

      ui.playButton_1->setEnabled(true);
      ui.playButton_2->setEnabled(true);
      ui.selectSongButton_1->setEnabled(true);
      ui.selectSongButton_2->setEnabled(true);
      ui.testSamplingRateButton->setEnabled(false);
      ui.testBitDepthButton->setEnabled(false);
    }
  });
  connect(ui.testBitDepthButton, &QPushButton::clicked, [&]() {
    if (session) {
      ProgressDialog progress;

      progress.show();

      session->readSound();
      session->convertSamplingrate();
      session->convertBitdepth();

      progress.close();

      ui.playButton_1->setEnabled(true);
      ui.playButton_2->setEnabled(true);
      ui.selectSongButton_1->setEnabled(true);
      ui.selectSongButton_2->setEnabled(true);
      ui.testSamplingRateButton->setEnabled(false);
      ui.testBitDepthButton->setEnabled(false);
    }
  });
  connect(ui.playButton_1, &QPushButton::clicked, [&]() {
    if (session) {
      if (!session->isInited()) {
        if (session->startPlaying(true)) {
          ui.currentFileLabel->setText(STRING_UI_PLAYING_FIRST);

          ui.playButton_2->setEnabled(false);
          ui.stopButton_1->setEnabled(true);
          ui.timeSlider->setEnabled(true);
        }
      }

      session->togglePlaying();
    }
  });
  connect(ui.stopButton_1, &QPushButton::clicked, [&]() {
    if (session) {
      ui.currentFileLabel->setText(STRING_UI_FILE_NOT_SELECTED);

      session->stopPlaying();

      ui.playButton_2->setEnabled(true);
      ui.stopButton_1->setEnabled(false);
      ui.timeSlider->setEnabled(false);
    }
  });
  connect(ui.selectSongButton_1, &QPushButton::clicked, [&]() {
    if (session) {
      bool result = session->isCorrectAnswer(true);
      bool test = session->isSamplingrateTest();
      QString filename = songModel.getItem(ui.fileTableView->selectionModel()->selectedRows().at(0).row()).getData(0);
      Result item(filename, test ? Result::TEST_SAMPLINGRATE : Result::TEST_BITDEPTH, result);

      resultModel.appendResult(item);
      
      ui.timeSlider->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
    }
  });
  connect(ui.playButton_2, &QPushButton::clicked, [&]() {
    if (session) {
      if (!session->isInited()) {
        if (session->startPlaying(false)) {
          ui.currentFileLabel->setText(STRING_UI_PLAYING_SECOND);

          ui.playButton_1->setEnabled(false);
          ui.stopButton_2->setEnabled(true);
          ui.timeSlider->setEnabled(true);
        }
      }

      session->togglePlaying();
    }
  });
  connect(ui.stopButton_2, &QPushButton::clicked, [&]() {
    if (session) {
      ui.currentFileLabel->setText(STRING_UI_FILE_NOT_SELECTED);

      session->stopPlaying();

      ui.playButton_1->setEnabled(true);
      ui.stopButton_2->setEnabled(false);
      ui.timeSlider->setEnabled(false);
    }
  });
  connect(ui.selectSongButton_2, &QPushButton::clicked, [&]() {
    if (session) {
      bool result = session->isCorrectAnswer(false);
      bool test = session->isSamplingrateTest();
      QString filename = songModel.getItem(ui.fileTableView->selectionModel()->selectedRows().at(0).row()).getData(0);
      Result item(filename, test ? Result::TEST_SAMPLINGRATE : Result::TEST_BITDEPTH, result);

      resultModel.appendResult(item);

      ui.timeSlider->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
    }
  });
  connect(ui.saveResultButton, &QPushButton::clicked, [&]() {
    QString filter = "Microsoft Excel (*.xlsx)";
    QString path = QFileDialog::getSaveFileName(NULL, QString(), "result.xlsx", filter, &filter);

    if (path.length() > 0) {
      resultModel.saveList(path);
    }
  });
  connect(ui.resetResultButton, &QPushButton::clicked, [&]() {
    resultModel.resetList();
  });
  connect(ui.fileTableView->selectionModel(), &QItemSelectionModel::selectionChanged, [&](const QItemSelection &selected, const QItemSelection &deselected) {
    if (selected.count() == 0) {
      ui.deleteFileButton->setEnabled(false);
      ui.testSamplingRateButton->setEnabled(false);
      ui.testBitDepthButton->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.stopButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.stopButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
    }
    else {
      ui.deleteFileButton->setEnabled(true);
      ui.testSamplingRateButton->setEnabled(false);
      ui.testBitDepthButton->setEnabled(false);

      SAFE_DELETE(session);

      int rowidx = selected.at(0).indexes().at(0).row();

      session = new SongSession(&audio);
      session->openSound(songModel.getItem(rowidx).getPath().toStdString().c_str());

      if (session->enableSamplingrateTest()) {
        ui.testSamplingRateButton->setEnabled(true);
      }
      if (session->enableBitdepthTest()) {
        ui.testBitDepthButton->setEnabled(true);
      }
    }
  });
  connect(ui.timeSlider, &QSlider::valueChanged, [&](int value) {
    if (session) {
      session->setTime(value);
    }
  });
  connect(ui.sineWaveButton, &QPushButton::clicked, [&]() {
    if (!session) {
      session = new SongSession(&audio);

      session->sineWaveTest();

      SAFE_DELETE(session);
    }
  });

  // Begin timer
  timer.start(1000);

  // Disable buttons
  ui.deleteFileButton->setEnabled(false);
  ui.playButton_1->setEnabled(false);
  ui.stopButton_1->setEnabled(false);
  ui.selectSongButton_1->setEnabled(false);
  ui.playButton_2->setEnabled(false);
  ui.stopButton_2->setEnabled(false);
  ui.selectSongButton_2->setEnabled(false);
  ui.timeSlider->setEnabled(false);
  ui.testSamplingRateButton->setEnabled(false);
  ui.testBitDepthButton->setEnabled(false);

  // Set label
  ui.currentFileLabel->setText(STRING_UI_FILE_NOT_SELECTED);
}

MainWindow::~MainWindow() {
  SAFE_DELETE(session);
}

ProgressDialog::ProgressDialog(QWidget *parent)
  : QDialog(parent) {
  setupUi(this);
}
