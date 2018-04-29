//SceneExplorer
//Exploring video files by viewer thumbnails
//
//Copyright (C) 2018  Ambiesoft
//
//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QObject>
#include <QSettings>
#include <QList>
#include <QListWidget>

#include "documentsql.h"

//struct DE
//{
//    QString dir_;
//    bool chk_;
//    bool sel_;

//    DE(const QString& dir, bool chk, bool sel) :
//        dir_(dir),
//        chk_(chk),
//        sel_(sel){}

//};

class Document : public QObject
{
    Q_OBJECT

    QSettings* s_ = nullptr;

    // bool bAllSel_ = false;
    // bool bAllChecked_ = false;

    // QList<DE> delist_;

    QString lastErr_;

    QString file_;

    DocumentSql* docSql_ = nullptr;
public:
    Document(){}
    ~Document() {
        delete s_;
    }
    bool Load(const QString& file, const bool bExists);

    QString GetLastErr() const {
        return lastErr_;
    }
    void Store(QListWidget* pLW,
               const QModelIndex& index);

    bool IsAllSelected() const {
        return docSql_->isAllSelected();
    }
    bool IsAllChecked() const {
        return docSql_->isAllChecked();
    }

    int dirCount() const;
    QString GetDEText(int index) const {
        return docSql_->getDirText(index);
    }
    bool IsDESelected(int index) const {
        return docSql_->isDirSelected(index);
    }
    bool IsDEChecked(int index) const {
        return docSql_->isDirChecked(index);
    }

    QString GetFileName() const;
    QString GetFullName() const;

    // int scrollPos() const ;
    bool getLastPos(int& row, int& column) const;
};

#endif // DOCUMENT_H
