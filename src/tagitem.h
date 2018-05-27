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

#ifndef TAGITEM_H
#define TAGITEM_H

#include <QListWidgetItem>

class TagItem : public QListWidgetItem
{
public:
    enum TagItemType
    {
        TI_NORMAL,
        TI_ALL,
    } ;
private:
    qint64 tagid_;
    TagItemType itemType_;

public:
    static QListWidget* parent_;
    TagItem(const qint64& tagid, TagItemType itemType);
    qint64 tagid() const {
        return tagid_;
    }
    bool IsNormalItem() const {
        return itemType_==TI_NORMAL;
    }
    bool IsAllItem() const {
        return itemType_==TI_ALL;
    }

    bool IsChecked() const {
        return checkState()==Qt::Checked;
    }
};

#endif // TAGITEM_H
