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

#include "ui_mainwindow.h"
#include "mainwindow.h"

void MainWindow::OnTableViewScrollChanged(int pos)
{
    QModelIndex indexTop = ui->tableView->indexAt(ui->tableView->rect().topLeft());
    QModelIndex indexBottom = ui->tableView->indexAt(ui->tableView->rect().bottomLeft());
    int rowCountPerScreen = indexBottom.row()-indexTop.row()+1;

    int top = qMax(0, indexTop.row()-rowCountPerScreen);
    int topEnd = pos < 0 ? indexBottom.row() : indexTop.row();
    for(int i=top ; i <= topEnd; ++i)
    {
        QModelIndex mi = proxyModel_->index(i,0);
        proxyModel_->data(mi, Qt::DecorationRole);
        proxyModel_->data(mi, Qt::DisplayRole);
        // ui->tableView->resizeRowToContents(mi.row());
    }


    int rowBottom = indexBottom.row();
    int maxBottom = rowBottom + rowCountPerScreen; // upto next page
    for(int i=rowBottom; i < maxBottom; ++i)
    {
        QModelIndex mi = proxyModel_->index(i,0);
        proxyModel_->data(mi, Qt::DecorationRole);
        proxyModel_->data(mi, Qt::DisplayRole);
        // ui->tableView->resizeRowToContents(mi.row());
    }
}
