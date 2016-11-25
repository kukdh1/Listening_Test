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
  ui.resultTableView->setColumnWidth(1, 100);
  ui.resultTableView->setColumnWidth(2, 170);
  ui.resultTableView->setColumnWidth(3, 80);
  ui.resultTableView->setColumnWidth(4, 80);
  ui.resultTableView->setColumnWidth(5, 200);

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
  connect(ui.testConfirmButton, &QPushButton::clicked, [&]() {
    if (session) {
      ProgressDialog progress;
      
      if (session->setTestInfo(ui.hqAudioCombo->currentText().toStdString(), ui.lqAudioCombo->currentText().toStdString())) {
        progress.show();

        session->readSound();

        progress.close();

        ui.playButton_1->setEnabled(true);
        ui.playButton_2->setEnabled(true);
        ui.selectSongButton_1->setEnabled(true);
        ui.selectSongButton_2->setEnabled(true);
        ui.testConfirmButton->setEnabled(false);
        ui.testTypeCombo->setEnabled(false);
        ui.hqAudioCombo->setEnabled(false);
        ui.lqAudioCombo->setEnabled(false);
      }
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
      bool answer;
      bool testtype;
      uint32_t factorH, factorL;
      
      session->getTestResult(answer);
      session->getTestInfo(testtype, factorH, factorL);
      
      QString filename = songModel.getItem(ui.fileTableView->selectionModel()->selectedRows().at(0).row()).getData(0);
      QString empty;
      Result item(filename, testtype ? Result::TEST_SAMPLINGRATE : Result::TEST_BITDEPTH, answer, true, factorH, factorL, empty);

      resultModel.appendResult(item);

      session->stopPlaying();
      
      ui.timeSlider->setEnabled(false);
      ui.testConfirmButton->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.stopButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.stopButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
      ui.testTypeCombo->setEnabled(false);
      ui.hqAudioCombo->setEnabled(false);
      ui.lqAudioCombo->setEnabled(false);
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
      bool answer;
      bool testtype;
      uint32_t factorH, factorL;
      
      session->getTestResult(answer);
      session->getTestInfo(testtype, factorH, factorL);
      
      QString filename = songModel.getItem(ui.fileTableView->selectionModel()->selectedRows().at(0).row()).getData(0);
      QString empty;
      Result item(filename, testtype ? Result::TEST_SAMPLINGRATE : Result::TEST_BITDEPTH, answer, false, factorH, factorL, empty);
      resultModel.appendResult(item);

      session->stopPlaying();

      ui.timeSlider->setEnabled(false);
      ui.testConfirmButton->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.stopButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.stopButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
      ui.testTypeCombo->setEnabled(false);
      ui.hqAudioCombo->setEnabled(false);
      ui.lqAudioCombo->setEnabled(false);
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
    Q_UNUSED(deselected);
    
    if (selected.count() == 0) {
      ui.deleteFileButton->setEnabled(false);
      ui.testConfirmButton->setEnabled(false);
      ui.playButton_1->setEnabled(false);
      ui.stopButton_1->setEnabled(false);
      ui.selectSongButton_1->setEnabled(false);
      ui.playButton_2->setEnabled(false);
      ui.stopButton_2->setEnabled(false);
      ui.selectSongButton_2->setEnabled(false);
      ui.testTypeCombo->setEnabled(false);
      ui.hqAudioCombo->setEnabled(false);
      ui.lqAudioCombo->setEnabled(false);
    }
    else {
      ui.deleteFileButton->setEnabled(true);
      ui.testConfirmButton->setEnabled(false);

      SAFE_DELETE(session);

      int rowidx = selected.at(0).indexes().at(0).row();

      session = new SongSession(&audio);
      session->openSound(songModel.getItem(rowidx).getPath().toStdString().c_str());

      ui.testTypeCombo->clear();
      ui.testTypeCombo->setEnabled(true);
      
      std::vector<std::string> data;
      
      session->getTestTypes(data);
      for (auto value : data) {
        ui.testTypeCombo->addItem(QString::fromStdString(value));
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

  std::function<void(const QString &)> fctComboHandler = [&](const QString &index) {
    Q_UNUSED(index);

    if (ui.testTypeCombo->currentIndex() > 0 && ui.hqAudioCombo->currentIndex() > 0 && ui.lqAudioCombo->currentIndex() > 0) {
      ui.testConfirmButton->setEnabled(true);
    }
    else {
      ui.testConfirmButton->setEnabled(false);
    }
  };
  
  connect(ui.testTypeCombo, &QComboBox::currentTextChanged, [&](const QString &index) {
    if (session) {
      if (session->setTestType(index.toStdString())) {
        std::vector<std::string> data;

        ui.hqAudioCombo->clear();
        ui.lqAudioCombo->clear();
        ui.hqAudioCombo->setEnabled(true);
        ui.lqAudioCombo->setEnabled(true);

        session->getHQFactors(data);
        for (auto value : data) {
          ui.hqAudioCombo->addItem(QString::fromStdString(value));
        }

        session->getLQFactors(data);
        for (auto value : data) {
          ui.lqAudioCombo->addItem(QString::fromStdString(value));
        }

        ui.hqAudioCombo->setEnabled(true);
        ui.lqAudioCombo->setEnabled(true);
      }
      else {
        ui.hqAudioCombo->setEnabled(false);
        ui.lqAudioCombo->setEnabled(false);
      }
    }
  });
  connect(ui.hqAudioCombo, &QComboBox::currentTextChanged, fctComboHandler);
  connect(ui.lqAudioCombo, &QComboBox::currentTextChanged, fctComboHandler);
  
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
  ui.testConfirmButton->setEnabled(false);
  ui.testTypeCombo->setEnabled(false);
  ui.hqAudioCombo->setEnabled(false);
  ui.lqAudioCombo->setEnabled(false);

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
