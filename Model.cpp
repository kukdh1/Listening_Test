#include "Model.h"

Song::Song() {}

Song::Song(QString &_filepath, uint32_t _samplingrate, uint8_t _bitdepth) {
  setData(0, _filepath);
  samplingrate = _samplingrate;
  bitdepth = _bitdepth;
}

QString Song::getData(int idx) const {
  switch (idx) {
    case 0:
      return filename;
    case 1:
      return QString::number(samplingrate);
    case 2:
      return QString::number(bitdepth);
  }

  return QString();
}

void Song::setData(int idx, QString &str) {
  switch (idx) {
    case 0:
      {
        filepath = str;

        // split
        QStringList list = filepath.split('/', QString::SkipEmptyParts);

        if (list.size() > 0) {
          filename = list.back();
        }
        else {
          __asm { int 3 };
        }
      }

      break;
    case 1:
      samplingrate = (uint32_t)str.toInt();

      break;
    case 2:
      bitdepth = (uint8_t)str.toInt();

      break;
  }
}

QString Song::getPath() {
  return filepath;
}

Result::Result() {}

Result::Result(QString &_filename, TEST_TYPE _type, bool _bFirstSoundIsBetter, bool _bUserSelectFirstSound, uint32_t uiHQ, uint32_t uiLQ, QString &_memo) {
  setData(0, _filename);
  type = _type;
  bFirstSoundIsBetter = _bFirstSoundIsBetter;
  bUserSelectFirstSound = _bUserSelectFirstSound;
  uiFactorHQ = uiHQ;
  uiFactorLQ = uiLQ;
  setData(6, _memo);

  factor.append(QString::number(uiFactorHQ));
  factor.append(" vs ");
  factor.append(QString::number(uiFactorLQ));
}

QString Result::getData(int idx) const {
  switch (idx) {
    case 0:
      return filename;
    case 1:
      return type == TEST_SAMPLINGRATE ? STRING_TEST_SAMPLINGRATE : STRING_TEST_BITDEPTH;
    case 2:
      return factor;
    case 3:
      return bFirstSoundIsBetter ? STRING_TEST_FIRST : STRING_TEST_SECOND;
    case 4:
      return bUserSelectFirstSound ? STRING_TEST_FIRST : STRING_TEST_SECOND;
    case 5:
      return memo;
  }

  return QString();
}

void Result::setData(int idx, QString &str) {
  switch (idx) {
    case 0:
      filename = str;

      break;
    case 1:
      if (str.compare(STRING_TEST_SAMPLINGRATE) == 0) {
        type = TEST_SAMPLINGRATE;
      }
      else if (str.compare(STRING_TEST_BITDEPTH) == 0) {
        type = TEST_BITDEPTH;
      }

      break;
    case 2:
      factor = str;

      break;
    case 3:
      if (str.compare(STRING_TEST_FIRST) == 0) {
        bFirstSoundIsBetter = true;
      }
      else if (str.compare(STRING_TEST_SECOND) == 0) {
        bFirstSoundIsBetter = false;
      }

      break;
    case 4:
      if (str.compare(STRING_TEST_FIRST) == 0) {
        bUserSelectFirstSound = true;
      }
      else if (str.compare(STRING_TEST_SECOND) == 0) {
        bUserSelectFirstSound = false;
      }

      break;
    case 5:
      memo = str;

      break;
  }
}

SongModel::SongModel(QObject *parent)
  : QAbstractTableModel(parent) {}

int SongModel::rowCount(const QModelIndex &) const {
  return vSongs.size();
}

int SongModel::columnCount(const QModelIndex &) const {
  return COLUMN_COUNT_SONG;
}

QVariant SongModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return QVariant();
  }

  return vSongs.at(index.row()).getData(index.column());
}

QVariant SongModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal) {
    switch (section) {
      case 0:
        return STRING_LIST_FILENAME;
      case 1:
        return STRING_LIST_SAMPLINGRATE;
      case 2:
        return STRING_LIST_BITDEPTH;
      default:
        return QVariant();
    }
  }
  else {
    return QString::number(section + 1);
  }
}

void SongModel::appendSong(Song &song) {
  beginInsertRows(QModelIndex{}, vSongs.size(), vSongs.size());
  vSongs.push_back(song);
  endInsertRows();
}

void SongModel::removeSong(int idx) {
  if ((size_t)idx >= vSongs.size()) {
    return;
  }

  beginRemoveRows(QModelIndex{}, idx, idx);
  vSongs.erase(vSongs.begin() + idx);
  endRemoveRows();
}

Song SongModel::getItem(int idx) {
  if ((size_t)idx >= vSongs.size()) {
    return Song();
  }

  return vSongs.at(idx);
}

ResultModel::ResultModel(QObject *parent)
  :QAbstractTableModel(parent) {}

int ResultModel::rowCount(const QModelIndex &) const {
  return vResults.size();
}

int ResultModel::columnCount(const QModelIndex &) const {
  return COLUMN_COUNT_RESULT;
}

QVariant ResultModel::data(const QModelIndex &index, int role) const {
  if (role != Qt::DisplayRole && role != Qt::EditRole) {
    return QVariant();
  }

  return vResults.at(index.row()).getData(index.column());
}

QVariant ResultModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole) {
    return QVariant();
  }

  if (orientation == Qt::Horizontal) {
    switch (section) {
    case 0:
      return STRING_LIST_FILENAME;
    case 1:
      return STRING_LIST_TESTTYPE;
    case 2:
      return STRING_LIST_TESTFACTOR;
    case 3:
      return STRING_LIST_ANSWER;
    case 4:
      return STRING_LIST_RESPONSE;
    case 5:
      return STRING_LIST_MEMO;
    default:
      return QVariant();
    }
  }
  else {
    return QString::number(section + 1);
  }
}

bool ResultModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (role == Qt::EditRole) {
    if (index.column() == 5) {
      QString str = value.toString();
      vResults.at(index.row()).setData(5, str);
    }
  }

  return true;
}

Qt::ItemFlags ResultModel::flags(const QModelIndex &index) const {
  if (index.column() == 5) {
    return QAbstractTableModel::flags(index) | Qt::ItemIsEditable;
  }

  return QAbstractTableModel::flags(index);
}

void ResultModel::appendResult(Result & result) {
  beginInsertRows(QModelIndex{}, vResults.size(), vResults.size());
  vResults.push_back(result);
  endInsertRows();
}

void ResultModel::resetList() {
  beginRemoveRows(QModelIndex{}, 0, vResults.size() - 1);
  vResults.clear();
  endRemoveRows();
}

bool ResultModel::saveList(QString &path) {
  lxw_workbook *wb = workbook_new(path.toStdString().c_str());

  if (wb) {
    lxw_worksheet *ws = workbook_add_worksheet(wb, "Result");

    // Write column names
    worksheet_write_string(ws, 0, 0, STRING_LIST_FILENAME, NULL);
    worksheet_write_string(ws, 0, 1, STRING_LIST_TESTTYPE, NULL);
    worksheet_write_string(ws, 0, 2, STRING_LIST_TESTFACTOR, NULL);
    worksheet_write_string(ws, 0, 3, STRING_LIST_ANSWER, NULL);
    worksheet_write_string(ws, 0, 4, STRING_LIST_RESPONSE, NULL);
    worksheet_write_string(ws, 0, 5, STRING_LIST_MEMO, NULL);

    // Write data
    int rowidx = 1;

    for (auto result : vResults) {
      for (int i = 0; i < COLUMN_COUNT_RESULT; i++)
        worksheet_write_string(ws, rowidx, i, result.getData(i).toStdString().c_str(), NULL);
    
      rowidx++;
    }

    workbook_close(wb);

    return true;
  }

  return false;
}
