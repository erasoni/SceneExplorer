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

#include "tagitem.h"

QListWidget* TagItem::parent_;
TagItem::TagItem(bool bHasParent,
                 const qint64& tagid,
                 TagItemType itemType,
                 const QString& text,
                 const QString& yomi) :
    QListWidgetItem(bHasParent ? parent_ : nullptr),
    tagid_(tagid),
    itemType_(itemType),
    yomi_(yomi)
{
    Q_ASSERT(parent_);

    if(itemType==TI_ALL)
        setIcon(QIcon(":resource/images/mailbox.png"));
    else if(itemType==TI_NORMAL)
    {
        setIcon(QIcon(":resource/images/tag.png"));
        setFlags(flags() | Qt::ItemIsUserCheckable);
    }
    else if(itemType==TI_NOTAG)
    {
        setIcon(QIcon(":resource/images/notags.png"));
        setFlags(flags() | Qt::ItemIsUserCheckable);
    }
    else
        Q_ASSERT(false);
    setText(text);
}
