#ifndef QTCSVFILECHECKER_H
#define QTCSVFILECHECKER_H

#include <QDebug>
#include <QFileInfo>
#include <QString>

namespace QtCSV
{
// Check if path to csv file is valid
// @input:
// - filePath - string with absolute path to file
// - mustExist - True if file must exist, False if this is not important
// @output:
// - bool - True if file is OK, else False
inline bool CheckFile(const QString &filePath, const bool mustExist = false)
{
  if (filePath.isEmpty())
  {
    qDebug() << __FUNCTION__ << "Error - file path is empty";
    return false;
  }

  QFileInfo fileInfo(filePath);
  if (fileInfo.isAbsolute() && false == fileInfo.isDir())
  {
    if (mustExist && false == fileInfo.exists())
    {
      return false;
    }
    if ("csv" != fileInfo.suffix())
    {
      qDebug() << __FUNCTION__ << "Warning - file suffix is not .csv";
    }

    return true;
  }

  return false;
}
} // namespace QtCSV

#endif // QTCSVFILECHECKER_H
