#pragma once

#ifndef _MODEL_H_
#define _MODEL_H_

#include <QtCore/qabstractitemmodel.h>
#include <QtCore/qfile.h>
#include <vector>
#include <xlsxwriter.h>

#ifdef WIN32
#ifdef _DEBUG
#pragma comment(lib, "libxlsxwriterd.lib")
#else
#pragma comment(lib, "libxlsxwriter.lib")
#endif
#pragma comment(lib, "Zlib.lib")
#endif

#define STRING_TEST_SAMPLINGRATE      "Sampling Rate"
#define STRING_TEST_BITDEPTH          "Bit Depth"
#define STRING_TEST_FIRST             "First"
#define STRING_TEST_SECOND            "Second"

#define STRING_LIST_FILENAME          "File Name"
#define STRING_LIST_SAMPLINGRATE      "Sampling Rate (Hz)"
#define STRING_LIST_BITDEPTH          "Bit Depth (bits)"
#define STRING_LIST_TESTTYPE          "Test type"
#define STRING_LIST_TESTFACTOR        "Test factor"
#define STRING_LIST_ANSWER            "Answer"
#define STRING_LIST_RESPONSE          "Response"
#define STRING_LIST_MEMO              "Memo"

#define COLUMN_COUNT_SONG             3
#define COLUMN_COUNT_RESULT           6

class Song {
  private:
    QString filepath;

    QString filename;
    uint32_t samplingrate;
    uint8_t bitdepth;

  public:
    Song(QString &, uint32_t, uint8_t);
    Song();

    QString getData(int) const;
    void setData(int, QString &);
    QString getPath();
};

class Result {
  public:
    enum TEST_TYPE {
      TEST_SAMPLINGRATE,
      TEST_BITDEPTH
    };

  private:
    QString filename;
    TEST_TYPE type;
    bool bFirstSoundIsBetter;
    bool bUserSelectFirstSound;
    uint32_t uiFactorHQ;
    uint32_t uiFactorLQ;
    QString memo;
    QString factor;

  public:
    Result(QString &, TEST_TYPE, bool, bool, uint32_t, uint32_t, QString &);
    Result();

    QString getData(int) const;
    void setData(int, QString &);
};

class SongModel : public QAbstractTableModel {
  private:
    std::vector<Song> vSongs;

  public:
    SongModel(QObject *parent = NULL);

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &, int) const override;
    QVariant headerData(int, Qt::Orientation, int) const override;

    void appendSong(Song &);
    void removeSong(int);
    Song getItem(int);
};

class ResultModel : public QAbstractTableModel {
  private:
    std::vector<Result> vResults;

  public:
    ResultModel(QObject *parent = NULL);

    int rowCount(const QModelIndex &) const override;
    int columnCount(const QModelIndex &) const override;
    QVariant data(const QModelIndex &, int) const override;
    QVariant headerData(int, Qt::Orientation, int) const override;
    bool setData(const QModelIndex &, const QVariant &, int role = Qt::EditRole) override;
    Qt::ItemFlags flags(const QModelIndex &) const override;

    void appendResult(Result &);
    void resetList();
    bool saveList(QString &);
};

#endif
