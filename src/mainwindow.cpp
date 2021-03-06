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

#include <QDebug>
#include <QStringList>
#include <QDesktopServices>
#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QObject>
#include <QProcess>
#include <QScrollBar>
#include <QScopedPointer>
#include <QSizePolicy>
#include <QStandardItemModel>
#include <QThread>
#include <QThreadPool>
#include <QTime>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QWidget>
#include <QClipboard>
#include <QToolButton>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QElapsedTimer>
#include <QSortFilterProxyModel>
#include <QFontDialog>
#include <QInputDialog>

#include "../../lsMisc/stdQt/stdQt.h"

#include "taskgetdir.h"
#include "taskffmpeg.h"
// #include "optionfontdialog.h"
#include "tablemodel.h"
#include "taskmodel.h"
#include "tableitemdata.h"
#include "settings.h"
#include "waitcursorq.h"
// #include "taskfilter.h"
#include "errorinfoexception.h"

#include "optiondialog.h"
#include "optionextension.h"
#include "optionexternaltoolsdialog.h"
#include "ffmpeg.h"
#include "globals.h"
#include "helper.h"
#include "osd.h"
#include "blockedbool.h"
#include "extension.h"
#include "sql.h"
#include "tableitemdelegate.h"
#include "docinfodialog.h"
#include "tagitem.h"
#include "taginputdialog.h"
#include "mycontextmenu.h"
#include "tagidsinfo.h"

#include "ui_mainwindow.h"
#include "mainwindow.h"

using namespace Consts;

MainWindow::SortManager::SortManager()
{
	sort_ = SORT_NONE;
    for (size_t i = 0; i < (sizeof(ascending_)/sizeof(ascending_[0])); ++i)
	{
        ascending_[i] = false;
        acs_[i] = nullptr;
		tbs_[i] = nullptr;
	}

	ascending_[SORT_FILENAME] = true;
	ascending_[SORT_SIZE] = true;
}

void MainWindow::CreateLimitManager()
{
    if(settings_.valueBool(KEY_LIMIT_ITEMS, false))
    { // Create
        if(limitManager_)
        { // already exits
            limitManager_->SetNumberOfRows(settings_.valueInt(KEY_LIMIT_NUMBEROFROWS, 1000));
        }
        else
        {
            actionLimitFirst_ = new QAction("<<",ui->mainToolBar);
            connect(actionLimitFirst_, &QAction::triggered,
                    this, &MainWindow::OnLimitFirstTriggered);
            ui->mainToolBar->insertAction(ui->actionplaceHolder_Limit, actionLimitFirst_);

            actionLimitPrev_ = new QAction("<",ui->mainToolBar);
            connect(actionLimitPrev_, &QAction::triggered,
                    this, &MainWindow::OnLimitPrevTriggered);
            ui->mainToolBar->insertAction(ui->actionplaceHolder_Limit, actionLimitPrev_);

            if(!cmbLimit_)
            {
                cmbLimit_ = new QComboBox(ui->mainToolBar);  // intentional leak
                cmbLimit_->setEditable(true);
                cmbLimit_->lineEdit()->setReadOnly(true);
                cmbLimit_->lineEdit()->setAlignment(Qt::AlignCenter);

                connect(cmbLimit_, SIGNAL(currentIndexChanged(int)),
                        this, SLOT(OnCmbLimitCurrentIndexChanged(int)));

                ui->mainToolBar->insertWidget(ui->actionplaceHolder_Limit, cmbLimit_);
            }
            else
            {
                Q_ASSERT(false);
            }

            actionLimitNext_ = new QAction(">",ui->mainToolBar);
            connect(actionLimitNext_, &QAction::triggered,
                    this, &MainWindow::OnLimitNextTriggered);
            ui->mainToolBar->insertAction(ui->actionplaceHolder_Limit, actionLimitNext_);

            actionLimitLast_ = new QAction(">>",ui->mainToolBar);
            connect(actionLimitLast_, &QAction::triggered,
                    this, &MainWindow::OnLimitLastTriggered);


            ui->mainToolBar->insertAction(ui->actionplaceHolder_Limit, actionLimitLast_);

            sepLimit_ = ui->mainToolBar->insertSeparator(ui->actionplaceHolder_Limit);

            int numofrows = settings_.valueInt(KEY_LIMIT_NUMBEROFROWS, 1000);

			Q_ASSERT(!limitManager_);
            limitManager_.reset(new LimitManager(numofrows, cmbLimit_));
        }
    }
    else
    { //Destroy limit manager
        if(limitManager_)
        {
            Q_ASSERT(actionLimitFirst_);
            disconnect(actionLimitFirst_, &QAction::triggered,
                       this, &MainWindow::OnLimitFirstTriggered);
            ui->mainToolBar->removeAction(actionLimitFirst_);
            delete actionLimitFirst_;
            actionLimitFirst_ = nullptr;

            Q_ASSERT(actionLimitPrev_);
            disconnect(actionLimitPrev_, &QAction::triggered,
                       this, &MainWindow::OnLimitPrevTriggered);
            ui->mainToolBar->removeAction(actionLimitPrev_);
            delete actionLimitPrev_;
            actionLimitPrev_ = nullptr;

            Q_ASSERT(cmbLimit_);
            delete cmbLimit_;
            cmbLimit_ = nullptr;

            Q_ASSERT(actionLimitNext_);
            disconnect(actionLimitNext_, &QAction::triggered,
                       this, &MainWindow::OnLimitNextTriggered);
            ui->mainToolBar->removeAction(actionLimitNext_);
            delete actionLimitNext_;
            actionLimitNext_ = nullptr;

            Q_ASSERT(actionLimitLast_);
            disconnect(actionLimitLast_, &QAction::triggered,
                       this, &MainWindow::OnLimitLastTriggered);
            ui->mainToolBar->removeAction(actionLimitLast_);
            delete actionLimitLast_;
            actionLimitLast_ = nullptr;

            ui->mainToolBar->removeAction(sepLimit_);
            delete sepLimit_;
            sepLimit_ = nullptr;

			// This is converted to std::unique_ptr
            limitManager_.reset();
            //delete limitManager_;
            //limitManager_ = nullptr;
        }
    }
    ui->actionplaceHolder_Limit->setVisible(false);
    //QAction* sep = ui->mainToolBar->findChild("sepAfterLimit");

    // ui->mainToolBar->removeAction(ui->actionplaceHolder_Limit);
}
QString MainWindow::GetDefaultDocumentPath()
{
    QString docdir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if(docdir.isEmpty())
    {
        Alert(this, tr("Failed to get document directory."));
        return QString();
    }
    docdir=pathCombine(docdir, APPNAME);
    QDir().mkpath(docdir);
    if(!QDir(docdir).exists())
    {
        Alert(this, tr("Failed to create directory. \"%1\"").arg(docdir));
        return QString();
    }

    return pathCombine(docdir,"default.scexd");
}

bool MainWindow::OpenDocument(const QString& file, const bool bExists)
{
    CloseDocument();

    Document* pNewDoc = new Document;
    if(!pNewDoc->Load(file, bExists))
    {
        Alert(this,
              QString("%0\n%1").arg(pNewDoc->GetLastErr()).arg(file));
        delete pNewDoc;
        return false;
    }

    qDebug() << "Document Opened: " << file;
    recents_.removeDuplicates();
    recents_.removeOne(file);
    recents_.insert(0, file);

    pDoc_ = pNewDoc;
    InitDocument();
    return true;
}
bool MainWindow::CloseDocument()
{
    if(!pDoc_)
        return true;

    BlockedBool dc(&directoryChanging_);
    BlockedBool tc(&tagChanging_);

    StoreDocument();
    delete pDoc_;
    pDoc_=nullptr;

    ui->directoryWidget->clear();
    ui->listTag->clear();

    return true;
}

bool MainWindow::LoadTags()
{
    Q_ASSERT(!tagChanging_);
    BlockedBool blockUpdate(&tagChanging_);

    // Tags
    ui->listTag->clear();
    if(!pDoc_)
        return false;

    TagItem* tagAll = new TagItem(true,
                                  -1,
                                  TagItem::TI_ALL,
                                  tr("All"),
                                  QString());
    tagAll->setSelected(pDoc_->IsTagAllSelected());
    ui->listTag->addItem(tagAll);

    // QList<QPair<qint64, QString> > tags;
    QList<TagItem*> tags;
    if(pDoc_->GetAllTags(tags, true))
    {
        // for(qint64& key : tags.keys())
        // for( const QPair<qint64,QString>& pair : tags)
        foreach(TagItem* ti, tags)
        {
            bool bSel,bCheck;
            if(pDoc_->GetTagSelectedAndChecked(ti->tagid(),bSel,bCheck))
            {
                ti->setSelected(bSel);
                ti->setCheckState(bCheck ? Qt::Checked : Qt::Unchecked);
            }
			
			ui->listTag->addItem(ti);
        }
    }

    TagItem* tagNoTag = new TagItem(true,
                                    -2,
                                    TagItem::TI_NOTAG,
                                    tr("No Tags"),
                                    QString());
    tagNoTag->setSelected(pDoc_->IsTagNotagsSelected());
    ui->listTag->addItem(tagNoTag);

    // select all if nothing is selected
    if(ui->listTag->selectedItems().empty())
    {
        tagAll->setSelected(true);
    }
    return true;
}
void MainWindow::InitDocument()
{
    if(!pDoc_)
        return;

    BlockedBool btD(&directoryChanging_);

    ui->directoryWidget->clear();

//    AddUserEntryDirectory(DirectoryItem::DI_ALL,
//                          -1,
//                          QString(),
//                          pDoc_->IsDirAllSelected(),
//                          pDoc_->IsDirAllChecked());
    DirectoryItem* allitem = new DirectoryItem(-1,
                                               DirectoryItem::DI_ALL_MY,
                                               tr("All"));
    // allitem->setChecked(pDoc_->IsDirAllChecked());
    allitem->setSelected(pDoc_->IsDirAllSelected());
    ui->directoryWidget->addItem(allitem);

//    const int count = pDoc_==nullptr ? 0 : pDoc_->dirCount();
//    for(int i=1 ; i <= count ; ++i)
//    {
//        AddUserEntryDirectory(DirectoryItem::DI_NORMAL,
//                              po
//                              pDoc_->GetDEText(i),
//                              pDoc_->IsDESelected(i),
//                              pDoc_->IsDEChecked(i));
//    }
    QList<DirectoryItem*> dirs;
    pDoc_->GetAllDirs(dirs);
    for(DirectoryItem* di : dirs)
    {
        ui->directoryWidget->addItem(di);
    }
//    AddUserEntryDirectory(DirectoryItem::DI_MISSING,
//                          -2,
//                          QString(), false, false);

    DirectoryItem* missitem = new DirectoryItem(-2,
                                                DirectoryItem::DI_MISSING_MY,
                                                tr("Missing"));
    ui->directoryWidget->addItem(missitem);

    LoadTags();

    itemChangedCommon(true);
}
//void MainWindow::setTableSpan()
//{
//    int newRowFilename = ui->tableView->model()->rowCount()-TableModel::RowCountPerEntry;
//    int newRowInfo = newRowFilename+1;
//    int newRowImage = newRowFilename+2;

//    ui->tableView->setSpan(newRowFilename,0,1,5);
//    ui->tableView->setSpan(newRowInfo,0,1,5);
//    // ui->tableView->resizeRowToContents(newRowFilename);
//    // ui->tableView->resizeRowToContents(newRowInfo);

//    static bool initColumnWidth=false;
//    if(!initColumnWidth)
//    {
//        initColumnWidth=true;
//        for(int i=0 ; i < 5 ; ++i)
//        {
//            ui->tableView->setColumnWidth(i, THUMB_WIDTH);
//        }
//    }

//    ui->tableView->setRowHeight(newRowImage, THUMB_HEIGHT);
//}

QThreadPool* MainWindow::getPoolGetDir()
{
    if(!pPoolGetDir_)
    {
        pPoolGetDir_ = new QThreadPool;
        pPoolGetDir_->setExpiryTimeout(-1);
        Q_ASSERT(optionThreadcountGetDir_ > 0);
        pPoolGetDir_->setMaxThreadCount(optionThreadcountGetDir_);
    }
    return pPoolGetDir_;
}
//QThreadPool* MainWindow::getPoolFilter()
//{
//    if(!pPoolFilter_)
//    {
//        pPoolFilter_ = new QThreadPool;
//        pPoolFilter_->setExpiryTimeout(-1);
//        Q_ASSERT(threadcountFilter_ > 0);
//        pPoolFilter_->setMaxThreadCount(threadcountFilter_);
//    }
//    return pPoolFilter_;
//}





QThreadPool* MainWindow::getPoolFFmpeg()
{
    if(!pPoolFFmpeg_)
    {
        pPoolFFmpeg_ = new QThreadPool;
        pPoolFFmpeg_->setExpiryTimeout(-1);
        Q_ASSERT(optionThreadcountThumbnail_ > 0);
        pPoolFFmpeg_->setMaxThreadCount(optionThreadcountThumbnail_);
    }
    return pPoolFFmpeg_;
}

void MainWindow::clearAllPool(bool bAppendLog)
{
    if(bAppendLog)
        insertLog(TaskKind::App, 0, tr("Clearing all tasks..."));

    BlockedBool btStop(&gStop, true, false);
    BlockedBool btPaused(&gPaused, false, gPaused);

    // Wait All tasks in pools
    for(;;)
    {
        if(pPoolFFmpeg_)
            pPoolFFmpeg_->clear();
        if(pPoolGetDir_)
            pPoolGetDir_->clear();
        if(pPoolFFmpeg_)
            pPoolFFmpeg_->clear();

        if( (pPoolGetDir_ && pPoolGetDir_->activeThreadCount() != 0) ||
                (pPoolFFmpeg_ && pPoolFFmpeg_->activeThreadCount() != 0) )
        {
            // wait 1 sec
            QThread::sleep(1);
            continue;
        }

        break;
    }

    ++gLoopId;
    idManager_->Clear();

    delete pPoolGetDir_;
    pPoolGetDir_ = nullptr;

    delete pPoolFFmpeg_;
    pPoolFFmpeg_ = nullptr;

	taskModel_->ClearAllTasks();

    if(bAppendLog)
        insertLog(TaskKind::App, 0, tr("======== All tasks Cleared ========"));
}



MainWindow::~MainWindow()
{
	closed_ = true;

    delete pDoc_;
    pDoc_ = nullptr;

    delete tableModel_;
    delete taskModel_;
    delete ui;

#ifdef QT_DEBUG
	Q_ASSERT(TaskListData::isAllClear());
	Q_ASSERT(TableItemData::isAllClear());
#endif

}



void MainWindow::insertLog(TaskKind kind, int id, const QString& text, bool bError)
{
    QVector<int> ids;
    ids.append(id);

    QStringList l;
    l.append(text);

    insertLog(kind, ids, l, bError);
}
void MainWindow::insertLog(TaskKind kind,
                           const QVector<int>& ids,
                           const QStringList& texts,
                           bool bError)
{
    Q_UNUSED(bError);
    if(ids.isEmpty())
    {
        Q_ASSERT(texts.isEmpty());
        return;
    }
    QString message;

    message.append(QTime::currentTime().toString());
    message.append(" ");

    Q_ASSERT(ids.count()==texts.count());
    for(int i=0 ; i < texts.length(); ++i)
    {
        QString text=texts[i];
        int id=ids[i];

        QString head;
        switch(kind)
        {
            case TaskKind::GetDir:
            {
                head.append(tr("Iterate"));
                head.append(QString::number(id));
            }
            break;
            case TaskKind::FFMpeg:
            {
                head.append(tr("Thumbnail"));
                head.append(QString::number(id));
            }
            break;
            case TaskKind::SQL:
            {
                head.append(tr("Database"));
				head.append(QString::number(id));
            }
            break;

            case TaskKind::App:
            {
				head.append(tr("Application"));
            }
            break;

//            case TaskKind::Filter:
//            {
//                head.append(tr("Filter"));
//            }
//            break;

            default:
                Q_ASSERT(false);
                return;
        }

        head.append("> ");
        message.append(head);
        message.append(text);
        if( (i+1) != texts.length())
            message.append("\n");
    }
    int scrollMax=ui->txtLog->verticalScrollBar()->maximum();
    int scrollCur=ui->txtLog->verticalScrollBar()->value();

    //int prevcursorPos = ui->txtLog->textCursor().position();
    ui->txtLog->appendPlainText(message);
    //int aftercursorPos = ui->txtLog->textCursor().position();

    //if(prevcursorPos < aftercursorPos)
    if(scrollMax==scrollCur)
    {
        ui->txtLog->verticalScrollBar()->setValue(
                ui->txtLog->verticalScrollBar()->maximum()); // Scrolls to the bottom
    }
}
void MainWindow::resizeDock_obsolete(QDockWidget* dock, const QSize& size)
{
    // width
    switch(this->dockWidgetArea(dock))
    {
        case Qt::DockWidgetArea::LeftDockWidgetArea:
        case Qt::DockWidgetArea::RightDockWidgetArea:
        {
            QList<int> sizes;
            QList<QDockWidget*> docks;

            docks.append(dock);
            sizes.append(size.height());

            resizeDocks(docks,sizes,Qt::Orientation::Vertical);
        }
        break;

        case Qt::DockWidgetArea::TopDockWidgetArea:
        case Qt::DockWidgetArea::BottomDockWidgetArea:
        // height
        {
        QList<QDockWidget*> docks;
            QList<int> sizes;

            docks.append(dock);
            sizes.append(size.width());

            resizeDocks(docks,sizes,Qt::Orientation::Horizontal);
        }
        break;

    default:
        break;
    }
}



void MainWindow::afterGetDir(int loopId, int id,
                             const QString& dirc,
                             const QStringList& filesIn,

                             const QList<qint64> sizes,
                             const QList<qint64> wtimes,

                             const QStringList& salients
                             )
{
    if(loopId != gLoopId)
        return;

    // BlockedBool btPause(&gPaused, true, gPaused);

    Q_UNUSED(id);
    // WaitCursor wc;

    bool needUpdate = false;

    QString dir = normalizeDir(QDir(dirc).absolutePath());

    QStringList filteredFiles;

    // check file is in DB
    for(int ifs = 0 ; ifs < filesIn.count();++ifs)
    {
		const QString& file = filesIn[ifs];
		const QString& sa = salients[ifs];
        const qint64 size = sizes[ifs];
        const qint64 wtime = wtimes[ifs];

        QFileInfo fi(pathCombine(dir, file));

        // QString sa = createSalient(fi.absoluteFilePath(), fi.size());

        if(gpSQL->hasEntry(dir,file,size,wtime,sa))
        {
            if(true) // gpSQL->hasThumb(dir, file))
            {
                insertLog(TaskKind::GetDir, id, tr("Already exists. \"%1\"").
                          arg(fi.absoluteFilePath()));
                continue;
            }
            else
            {
                // thumbs not exist
                // may be crashed or failed to create
                // remove entry
                gpSQL->RemoveEntry(dir, file);
            }
        }

        QStringList dirsDB;
        QStringList filesDB;
        QList<qint64> sizesDB;
        gpSQL->getEntryFromSalient(sa, dirsDB, filesDB, sizesDB);
        bool renamed = false;
        for(int i=0 ; i < filesDB.count(); ++i)
        {
            const QString& dbFile = pathCombine(dirsDB[i], filesDB[i]);
            // same salient in db
            QFileInfo fidb(dbFile);
            if(!fidb.exists())
            {
                // file info in db does not exist on disk
                if(fi.size()==sizesDB[i])
                {
                    // db size is same size with disk
                    // and salient is same ( conditonal queried from db )
                    // we assume file is moved
                    insertLog(TaskKind::GetDir, id, tr("Rename detected. \"%1\" -> \"%2\"").
                              arg(dbFile).
                              arg(pathCombine(dir,file)));
                    if(gpSQL->RenameEntry(dirsDB[i], filesDB[i], dir, file))
                    {
                        if(!tableModel_->RenameEntry(dirsDB[i], filesDB[i], dir, file))
                        {
                            needUpdate = true;
                        }
                    }
                    renamed = true;
                    break;
                }
            }
        }

        if(renamed)
            continue;
        filteredFiles.append(file);
    }
    // now file must be thumbnailed
    afterFilter2(loopId,
                id,
                dir,
                filteredFiles
                );


    if(needUpdate)
        itemChangedCommon();
}
void MainWindow::finished_GetDir(int loopId, int id, const QString& dir)
{
    if(loopId != gLoopId)
        return;

    idManager_->IncrementDone(IDKIND_GetDir);
    Q_ASSERT(idManager_->Get(IDKIND_GetDir) >= idManager_->GetDone(IDKIND_GetDir));

    insertLog(TaskKind::GetDir, id, tr("Scan directory finished. %1").arg(dir));

    checkTaskFinished();
}
void MainWindow::afterFilter2(int loopId,int id,
                             const QString& dir,
                             const QStringList& filteredFiles)
{
    if(loopId != gLoopId)
        return;

    Q_UNUSED(id);



//    QStringList filteredFiles;
//    int ret = gpSQL->filterWithEntry(dir, filesIn, filteredFiles);
//    if(ret != 0)
//    {
//        insertLog(TaskKind::SQL, 0, Sql::getErrorStrig(ret), true);
//    }

    if(filteredFiles.isEmpty())
    {
        insertLog(TaskKind::GetDir, id, tr("No new files found in %1").arg(dir));
    }
    else
    {
        insertLog(TaskKind::GetDir, id, tr("%1 new items found in %2").
                  arg(QString::number(filteredFiles.count())).
                  arg(dir));
    }

    // afterfilter must perform salient check from SQL, for filter-passed files


    QVector<TaskListDataPointer> tasks;
    QVector<int> logids;
    QStringList logtexts;

    for(int i=0 ; i < filteredFiles.length(); ++i)
    {
        Q_ASSERT(isLegalFileExt(optionThumbFormat_));
        QString file = pathCombine(dir, filteredFiles[i]);
        TaskFFmpeg* pTask = new TaskFFmpeg(FFMpeg::GetFFprobe(settings_),
                                           FFMpeg::GetFFmpeg(settings_),
                                           gLoopId,
                                           idManager_->Increment(IDKIND_FFmpeg),
                                           file,
                                           GetTaskPriority(),
                                           optionThumbFormat_);
        pTask->setAutoDelete(true);
//        QObject::connect(pTask, &TaskFFMpeg::sayBorn,
//                         this, &MainWindow::sayBorn);
        QObject::connect(pTask, &TaskFFmpeg::sayHello,
                         this, &MainWindow::sayHello);
        QObject::connect(pTask, &TaskFFmpeg::sayNo,
                         this, &MainWindow::sayNo);
        QObject::connect(pTask, &TaskFFmpeg::sayGoodby,
                         this, &MainWindow::sayGoodby);
        QObject::connect(pTask, &TaskFFmpeg::sayDead,
                         this, &MainWindow::sayDead);
        QObject::connect(pTask, &TaskFFmpeg::finished_FFMpeg,
                         this, &MainWindow::finished_FFMpeg);
        QObject::connect(pTask, &TaskFFmpeg::warning_FFMpeg,
                         this, &MainWindow::warning_FFMpeg);

        tasks.append(TaskListData::Create(pTask->GetId(),pTask->GetMovieFile()));
        getPoolFFmpeg()->start(pTask);

        logids.append(idManager_->Get(IDKIND_FFmpeg));
        logtexts.append(QString(tr("Task registered. %1")).arg(file));
    }
    insertLog(TaskKind::FFMpeg, logids, logtexts);
    taskModel_->AddTasks(tasks);

    checkTaskFinished();


}
void MainWindow::checkTaskFinished()
{
    if(idManager_->isAllTaskFinished())
    {
        onTaskEnded();
        clearAllPool(false);
        insertLog(TaskKind::App, 0, tr("======== All Tasks finished ========"));
    }
}

QString MainWindow::getSelectedVideo(bool bNativeFormat)
{
    QItemSelectionModel *select = ui->tableView->selectionModel();

    if(!select->hasSelection())
        return QString();
    QVariant v = proxyModel_->data(select->selectedIndexes()[0], TableModel::MovieFileFull);
    QString s = v.toString();
    Q_ASSERT(!s.isEmpty());

    return bNativeFormat ? QDir::toNativeSeparators(s) : s;
}
qint64 MainWindow::getSelectedID()
{
    QItemSelectionModel *select = ui->tableView->selectionModel();

    if(!select->hasSelection())
        return -1;

    QVariant v = proxyModel_->data(select->selectedIndexes()[0], TableModel::ID);
    return v.toLongLong();
}


#include "renamedialog.h"
void MainWindow::OnContextRename()
{
    QString oldfull = getSelectedVideo(false);
    if(oldfull.isEmpty()) { Alert(this, TR_NOVIDEO_SELECTED()); return;}

    QString olddir = normalizeDir(QFileInfo(oldfull).absolutePath());
    QString oldname = QFileInfo(oldfull).fileName();
    QString oldext = QFileInfo(oldfull).suffix();
    QString oldbasename = oldname.left(oldname.length()-oldext.length()-1);

    QString targetname;
    QString newbasename;
    QString newext;
    for(;;) {
        RenameDialog dlg(this);
        dlg.setBasename(newbasename.isEmpty() ? oldbasename : newbasename);
        dlg.setExt(newext.isEmpty() ? oldext : newext);
        if(!dlg.exec())
            return;
        newbasename = dlg.basename();
        newext = dlg.ext();
        if(newbasename.isEmpty() && newext.isEmpty())
        {
            Alert(this, tr("Name and extension is empty."));
            continue;
        }

        if(newbasename==oldbasename && newext==oldext)
        {
            // Alert(this,tr("New name is same as old name."));
            return;
        }
        targetname = newbasename + "." + newext;

//        QFile filefull(oldfull);
//        if(!filefull.rename(pathCombine(olddir, targetname)))
//        {
//            Alert(this,
//                QString(tr("Failed to rename file. (%1)")).arg(filefull.errorString()));
//            continue;
//        }

//        if(!QFile::rename(oldfull,pathCombine(olddir, targetname)))
//        {
//            Alert(this,
//                QString(tr("Failed to rename file. (%1)")).arg(oldfull));
//            continue;
//        }

        QString renameError;
        if(!myRename(oldfull,pathCombine(olddir, targetname), renameError))
        {
            Alert(this,
                QString(tr("Failed to rename file. (%1)")).arg(renameError));
            continue;
        }
        break;
    };

    if(!gpSQL->RenameEntry(olddir, oldname, olddir,targetname))
    {
        Alert(this, tr("Failed to rename in database."));
        return;
    }
    tableModel_->RenameEntry(olddir, oldname, olddir, targetname);
}

void MainWindow::OnContextRemoveFromDatabase()
{
    QString movieFile = getSelectedVideo(false);
    if(movieFile.isEmpty()) { Alert(this, TR_NOVIDEO_SELECTED()); return;}

    bool bRemoveFromHardDisk = settings_.valueBool(KEY_MESSAGEBOX_REMOVEFORMEXTERNALMEDIA);
    const bool isTargetFileExists = QFile(movieFile).exists();

    QMessageBox msgbox(this);

    QCheckBox cbRemoveFromMedia(tr("Also remove from external media"));
    cbRemoveFromMedia.setEnabled(isTargetFileExists);
    cbRemoveFromMedia.setChecked(bRemoveFromHardDisk);

    msgbox.setText(tr("Do you want to remove \"%1\" from database?").
                   arg(movieFile));

    msgbox.setIcon(QMessageBox::Icon::Question);
    msgbox.addButton(QMessageBox::Yes);
    msgbox.addButton(QMessageBox::No);
    msgbox.setDefaultButton(QMessageBox::No);
    msgbox.setCheckBox(&cbRemoveFromMedia);

    if(msgbox.exec() != QMessageBox::Yes)
        return;

    if(!bRemoveFromHardDisk && cbRemoveFromMedia.checkState()==Qt::Checked)
    {
        // Newly checked to remove from disk, ask again
        if(!YesNo(this,
              QString(tr("The file \"%1\" will be removed. Are you sure to continue?")).arg(movieFile)))
        {
            return;
        }
    }

    bRemoveFromHardDisk = cbRemoveFromMedia.checkState()==Qt::Checked;
    settings_.setValue(KEY_MESSAGEBOX_REMOVEFORMEXTERNALMEDIA, bRemoveFromHardDisk);

    QString error;
    QString dir,name;
    nomalizeDirAndName(movieFile, dir, name);
    if(dir.isEmpty() || name.isEmpty())
    {
        Alert(this, tr("Directory or name is empty."));
        return;
    }
    gpSQL->DeleteEntryThumbFiles(dir,name,nullptr);

    if(!gpSQL->RemoveEntry(dir,name,&error))
    {
        Alert(this,
              tr("Failed to remove \"%1\".").
              arg(movieFile));
        return;
    }
    tableModel_->RemoveItem(movieFile);

    if(bRemoveFromHardDisk && isTargetFileExists)
    {
        try
        {
            MoveToTrashImpl(movieFile);
        }
        catch(ErrorInfoException& ex)
        {
            Alert(this, ex.getErrorInfo());
        }
    }
}

void MainWindow::OnContextAddTags()
{
    if(!pDoc_)
        return;
    qint64 id = getSelectedID();
    if(id <= 0)
    {
        Alert(this,
              TR_NOVIDEO_SELECTED());
        return;
    }

    QAction* act = (QAction*)QObject::sender();
    qint64 tagid = act->data().toLongLong();
    // check state is already updated
    pDoc_->SetTagged(id, tagid, act->isChecked());
}

void MainWindow::OnContextExternalTools()
{
    const QString movieFileNative = getSelectedVideo(true);
    if(movieFileNative.isEmpty()) { Alert(this, TR_NOVIDEO_SELECTED()); return;}

    QAction* act = (QAction*)QObject::sender();
    int i = act->data().toInt();
    QString exe = externalTools_[i].GetExe();
    QString arg = externalTools_[i].GetArg();
    QString argparsed;

    static QRegExp rx("(\\$\\{\\w+\\})");

    int prevpos = 0;
    int pos = 0;

    while ((pos = rx.indexIn(arg, pos)) != -1)
    {
        argparsed += arg.mid(prevpos,pos-prevpos);
        int matchedlen = rx.matchedLength();
        QString s = arg.mid(pos,matchedlen);// rx.cap(i++);
        if(s=="${filefullpath}")
        {
            argparsed += movieFileNative;
        }
        else if(s=="${directoryfullpath}")
        {
            argparsed += QFileInfo(movieFileNative).dir().absolutePath();
        }
        pos += matchedlen;
        prevpos = pos;
    }
    argparsed += arg.mid(prevpos);




//    QStringList argconst;
//    arg << movieFile;

    QString command;
    command += dq(exe);
    command += " ";
    command += argparsed;
    qDebug() << "QProcess::startDetached:" << command;
    if(!QProcess::startDetached(command))
    {
        QString message;
        message += tr("Failed to start new process.");
        message += "\n\n";
        message += tr("Command line:");
        message += "\n";
        message += command;
        Alert(this, message);
        return;
    }

    if(externalTools_[i].IsCountAsOpen())
    {
        updateOnOpened(getSelectedID(),movieFileNative);
    }
}

void MainWindow::OnContextCopySelectedVideoFilename()
{
    QString movieFile = getSelectedVideo();
    if(movieFile.isEmpty()) { Alert(this, TR_NOVIDEO_SELECTED()); return;}

    QFileInfo fi(movieFile);
    QApplication::clipboard()->setText(fi.fileName());
}

void MainWindow::IDManager::updateStatus()
{
    QString s = QString("D: %1/%2   M: %3/%4").
            arg(idGetDirDone_).arg(idGetDir_).
            // arg(idFilterDone_).arg(idFilter_).
            arg(idFFMpegDone_).arg(idFFMpeg_);

    win_->slTask_->setText(s);
}


void MainWindow::OnDirectorySelectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    Q_UNUSED(selected);
    Q_UNUSED(deselected);

    if (!initialized_ || closed_ || directoryChanging_)
        return;

    if(ui->directoryWidget->IsMissingItemSelectedOrChecked())
    {
        if(!YesNo(this,
              tr("Showing all missing item will take some time. Do you want to continue?")))
        {
            return;
        }
    }
    if(limitManager_)
        limitManager_->Reset();
    itemChangedCommon();
}

void MainWindow::itemChangedCommon(bool bForceRead)
{
	if (!initialized_ || closed_)
		return;

    if ((directoryChanging_ || tagChanging_) && !bForceRead)
		return;

	WaitCursor wc;

    QStringList dirs;
    bool bOnlyMissing = false;
    QList<DirectoryItem*> allSelectedOrChecked = ui->directoryWidget->selectedOrCheckedItems();
    // Add also current item
    // when blank area is clicked, all selection is gone
    // if(allSelectedOrChecked.isEmpty())
    {
        DirectoryItem* currentDi =(DirectoryItem*)ui->directoryWidget->currentItem();
        if(currentDi && !allSelectedOrChecked.contains(currentDi))
        {
            allSelectedOrChecked.append(currentDi);
        }
    }

    foreach(DirectoryItem* item, allSelectedOrChecked)
    {
        if(item->IsAllItem())
        {
            dirs = QStringList();
            break;
        }
        else if(item->IsMissingItem())
        {
            dirs = QStringList();
            bOnlyMissing = true;
            bForceRead = true;
            break;
        }
        else
        {
            Q_ASSERT(item->IsNormalItem());
            dirs.append(item->text());
            continue;
        }
        Q_ASSERT(false);
    }

    // Get Tag selected
    // QSet<qint64> taggedids;
    TagidsInfo tagInfos(TagidsInfo::TAGIDS_INFO_TYPE::TAGIDSINFO_USERSELECTED);
    GetSelectedAndCurrentTagIDs(tagInfos);

    tbShowNonExistant_->setEnabled(!bOnlyMissing);

    const bool isSameReq =
            lastQueriedOnlyMissing_ == bOnlyMissing &&
            lastQueriedDirs_ == dirs  &&
//            lastQueriedIsAllTag_==isAllTag &&
//            lastQueriedTaggedIDs_==taggedids;
            lastQueriedTaggedIds_==tagInfos;
    if(!bForceRead && isSameReq)
    {
        return;
    }
    if (limitManager_ && !isSameReq)
	{
		limitManager_->Reset();
	}
    lastQueriedOnlyMissing_ = bOnlyMissing;
    lastQueriedDirs_ = dirs;
//    lastQueriedIsAllTag_ = isAllTag;
//    lastQueriedTaggedIDs_ = taggedids;
    lastQueriedTaggedIds_ = tagInfos;
    GetSqlAllSetTable(dirs, tagInfos, bOnlyMissing);
}

void MainWindow::GetSelectedAndCurrentTagIDs(TagidsInfo& tagInfos)
{
    QList<qint64> tagids;
	TagItem* tiCurrent = (TagItem*)ui->listTag->currentItem();
	
    for(int i=0 ; i < ui->listTag->count(); ++i)
    {
        TagItem* ti = (TagItem*)ui->listTag->item(i);
        if(ti->IsAllItem())
        {
            if(ti->isSelected() || ti==tiCurrent)
            {
                tagInfos.setAll();
                return;
            }
        }
        else if(ti->IsNotagItem())
        {
            if(ti->isSelected() || ti==tiCurrent)
            {
                tagInfos.setNotags();
                break;
            }
        }
        else if(ti->IsNormalItem())
        {
            Q_ASSERT(!ti->IsAllItem());
            if(ti->isSelected() || ti->IsChecked() || ti==tiCurrent)
            {
                tagids.append(ti->tagid());
            }
        }
        else
        {
            Q_ASSERT(false);
        }
    }

    if(tagInfos.isNotags())
    {
        QList<qint64> allids;
        for(int i=0 ; i < ui->listTag->count(); ++i)
        {
            TagItem* ti = (TagItem*)ui->listTag->item(i);
            if(ti->IsNormalItem())
            {
                allids.append(ti->tagid());
            }
        }
        QSet<qint64> allfileids;;
        pDoc_ && pDoc_->GetTaggedIDs(allids, allfileids);
        tagInfos.setFileIds(allfileids);
        return;
    }
    if(tagids.isEmpty())
    {
        // nothing is selected
        tagInfos.setAll();
        return ;
    }
    QSet<qint64> taggedids;
    pDoc_ && pDoc_->GetTaggedIDs(tagids, taggedids);
    tagInfos.setFileIds(taggedids);
    return;
}

void MainWindow::GetSqlAllSetTable(const QStringList& dirs,
                                   const TagidsInfo& tagInfos,
                                   bool bOnlyMissing)
{
    tableModel_->SetShowMissing(tbShowNonExistant_->isChecked() );
    QElapsedTimer timer;
    timer.start();

	if (limitManager_ && limitManager_->IsNotCounted())
	{
		qlonglong count = gpSQL->GetAllCount(dirs);
		limitManager_->SetAllCount(count);

		size_t cmbcount = (count / limitManager_->GetNumberOfRows());
		if ((count % limitManager_->GetNumberOfRows()) != 0)
			cmbcount++;

        BlockedBool blc(limitManager_->GetBlockPointer());
        cmbLimit_->clear();
		for (size_t i = 0; i < cmbcount; ++i)
		{
            cmbLimit_->addItem(QString("%1").arg(i+1));
            cmbLimit_->setItemData((int)i, Qt::AlignCenter, Qt::TextAlignmentRole);
		}
	}

    // these 2 columns exists in doc, so copy them into main DB.
//    switch(sortManager_.GetCurrentSort())
//    {
//        case SORTCOLUMNMY::SORT_OPENCOUNT:
//        {
//            if(!pDoc_->IsOpenCountAndLastAccessClean())
//            {
//                QMap<qint64,int> opencounts;
//                pDoc_->GetOpenCounts(opencounts);
//                gpSQL->ApplyOpenCount(opencounts);
//                pDoc_->SetOpenCountAndLastAccessClean();
//            }
//        }
//        break;

//        case SORTCOLUMNMY::SORT_LASTACCESS:
//        {
//            if(!pDoc_->IsOpenCountAndLastAccessClean())
//            {
//                QMap<qint64,qint64> lastaccesses;
//                pDoc_->GetLastAccesses(lastaccesses);
//                gpSQL->ApplyLastAccesses(lastaccesses);
//                pDoc_->SetOpenCountAndLastAccessClean();
//            }
//        }
//        break;

//    default:
//        break;
//    }
    QList<TableItemDataPointer> all;
    gpSQL->GetAll(all,
                  dirs,
                  cmbFind_->currentText(),
                  bOnlyMissing,
                  sortManager_.GetCurrentSort(),
                  sortManager_.GetCurrentRev(),
                  limitManager_ ?
                      LimitArg(limitManager_->GetCurrentIndex(), limitManager_->GetNumberOfRows()): LimitArg(),
                  tagInfos);


    UpdateTitle(dirs, bOnlyMissing ? UpdateTitleType::ONLYMISSING : UpdateTitleType::DEFAULT);

    insertLog(TaskKind::App,
              0,
              tr("Querying Database takes %1 milliseconds.").arg(timer.elapsed()));

    // pDoc_->setOpenCountAndLascAccess_obsolete(all);
    // pDoc_->sort(all, sortManager_.GetCurrentSort(), sortManager_.GetCurrentRev());

    timer.start();
    tableModel_->ResetData(all);

    insertLog(TaskKind::App,
              0,
              tr("Resetting data takes %1 milliseconds.").arg(timer.elapsed()));


	tableSortParameterChanged(sortManager_.GetCurrentSort(), sortManager_.GetCurrentRev());
}

void MainWindow::UpdateTitle(const QStringList& dirs, UpdateTitleType utt)
{
    QStringList titledirs;

    switch(utt)
    {
    case UpdateTitleType::ONLYMISSING:
        titledirs << tr("Missing");
        break;

    case UpdateTitleType::DEFAULT:
        if(dirs.empty())
        {
            titledirs << tr("All");
        }
        else
        {
            titledirs = dirs;
        }
        break;
    case UpdateTitleType::INIT:
        break;
    default:
        Q_ASSERT(false);
    }

    QString title;
    if(pDoc_)
    {
        title.append(pDoc_->GetFileName());
        title.append(" - ");
    }
    for(int i=0 ; i < titledirs.count(); ++i)
    {
        title.append(titledirs[i]);
        if((i+1) != titledirs.count())
            title.append(" | ");
    }

    if(title.isEmpty())
        title = APPNAME_DISPLAY;
    else
    {
        title.append(" - ");
        title.append(APPNAME_DISPLAY);
    }

#ifdef QT_DEBUG
    title.append(" (Debug)");
#endif
    this->setWindowTitle(title);
}
void MainWindow::on_action_Top_triggered()
{
	WaitCursor wc;
    ui->tableView->scrollToTop();
}

void MainWindow::on_action_Bottom_triggered()
{
	WaitCursor wc;
    proxyModel_->ensureBottom();
	QApplication::processEvents();
    ui->tableView->scrollToBottom();

    QApplication::processEvents();
    ui->tableView->scrollToBottom();
}


void MainWindow::OndirectoryItemChanged(QListWidgetItem *item)
{
    Q_UNUSED(item);

    if(limitManager_)
        limitManager_->Reset();


    itemChangedCommon();
}

void MainWindow::tableItemCountChanged()
{
    slItemCount_->setText(tr("Items: %1").arg(proxyModel_->GetItemCount()));
}
void MainWindow::tableSortParameterChanged(SORTCOLUMNMY sc, bool rev)
{
    QString text = tr("Sort: %1 %2").
            arg(GetSortColumnName(sc)).
            arg(rev ? "-" : "+");
    slItemSort_->setText(text);
}


bool MainWindow::IsInitialized() const
{
    return initialized_;
}

void MainWindow::on_action_Find_triggered()
{
    QString cur = cmbFind_->currentText();
    if(cur.isEmpty())
        return;

    if(limitManager_)
        limitManager_->Reset();

    itemChangedCommon(true);

    InsertUniqueTextToComboBox(*cmbFind_, cur);
    cmbFind_->setCurrentIndex(0);
}
void MainWindow::on_action_Clear_triggered()
{
    QModelIndex curSel = ui->tableView->currentIndex();
    QString selPath;
    if(curSel.isValid())
    {
        QVariant v = proxyModel_->data(curSel, TableModel::TableRole::MovieFileFull);
        if(v.isValid())
            selPath = v.toString();
    }
    if(limitManager_)
        limitManager_->Reset();

    cmbFind_->setCurrentText(QString());
    itemChangedCommon(true);

    if(!selPath.isEmpty())
    {
        QModelIndex miToSelect = proxyModel_->findIndex(selPath);
        if(miToSelect.isValid())
        {
            ui->tableView->selectionModel()->select(miToSelect,
                                                    QItemSelectionModel::ClearAndSelect);
            proxyModel_->ensureIndex(miToSelect);
            QApplication::processEvents();
            ui->tableView->scrollTo(miToSelect);
        }
    }
}

void MainWindow::OnFindComboEnterPressed()
{
    on_action_Find_triggered();
}



bool MainWindow::HasDirectory(const QString& dir)
{
    QString dirNormlized = normalizeDir(dir);
    for(int i=0 ; i < ui->directoryWidget->count();++i)
    {
        DirectoryItem* di = (DirectoryItem*)ui->directoryWidget->item(i);
        if(di->text()==dirNormlized)
            return true;
    }
    return false;
}

void MainWindow::on_action_Extentions_triggered()
{
    OptionExtension dlg(this, settings_);
    dlg.exec();
}

void MainWindow::on_action_Font_triggered()
{
    setFontCommon2(KEY_FONT_TABLEINFO,
                  [this]() -> QFont { return tableModel_->GetInfoFont();},
                  [this](QFont& font) { tableModel_->SetInfoFont(font);});
}

void MainWindow::on_action_FontDetail_triggered()
{
    setFontCommon2(KEY_FONT_TABLEDETAIL,
                  [this]() -> QFont { return tableModel_->GetDetailFont();},
                  [this](QFont& font) { tableModel_->SetDetailFont(font);});

}
void MainWindow::setFontCommon1(const QString& savekey,
                                QWidget* pWidget)
{
    setFontCommon2(savekey,
                  [pWidget]() -> QFont { return pWidget->font();},
                  [pWidget](QFont& font) { pWidget->setFont(font);});
}

void MainWindow::askRebootClose()
{
    if(!YesNo(this,
         tr("Application needs to restart for the changes to take effect. Do you want to restart application now?")))
    {
        return;
    }
    gReboot = true;
    close();
}
void MainWindow::setFontCommon2(const QString& savekey,
                               std::function<QFont (void)> getfunc,
                               std::function<void(QFont&)> setfunc)
{
    bool ok;
    QFont font = QFontDialog::getFont(
                &ok,
                getfunc(),
                this);
    if (!ok)
    {
        if(!YesNo(this,
              tr("Do you want to set back to default font?")))
        {
            return;
        }
        settings_.setValue(savekey, QString());
        askRebootClose();
        return;
    }

    setfunc(font);
    settings_.setValue(savekey, font.toString());
}



void MainWindow::on_action_New_triggered()
{
    QFileDialog dlg(this);
    dlg.setWindowTitle(tr("Create New Document"));
    dlg.setDirectory(lastSelectedDocumentDir_);
    dlg.setFileMode(QFileDialog::AnyFile);
    dlg.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);

    QStringList filters;
    filters << tr("SceneExplorer Document (*.scexd)")
            << tr("Any files (*)");

    dlg.setNameFilters(filters);
    if(!dlg.exec())
        return;

    QString newFile = dlg.selectedFiles()[0];
    lastSelectedDocumentDir_ = QFileInfo(newFile).absolutePath();

    if(!OpenDocument(newFile, QFile(newFile).exists()))
        return;

    // create a file
    StoreDocument();
}
void MainWindow::on_action_Open_triggered()
{
    QFileDialog dlg(this);
    dlg.setDirectory(lastSelectedDocumentDir_);
    dlg.setFileMode(QFileDialog::ExistingFile);

    QStringList filters;
    filters << tr("SceneExplorer Document (*.scexd)")
            << tr("Any files (*)");

    dlg.setNameFilters(filters);
    if(!dlg.exec())
        return;

    QString file = dlg.selectedFiles()[0];
    lastSelectedDocumentDir_ = QFileInfo(file).absolutePath();
    OpenDocument(file, true);
}

void MainWindow::on_action_Save_triggered()
{
    StoreDocument();
}


bool MainWindow::checkExeCommon(const QString& exe, QString& errString) const
{
    QProcess process;
    process.setProgram(exe);
    process.setArguments( QStringList() <<
                          "-version"
                          );

    process.start(QProcess::ReadOnly);
    if(!process.waitForStarted(-1))
    {
        errString = process.errorString();
        return false;
    }
    if(!process.waitForFinished(-1))
    {
        errString = process.errorString();
        return false;
    }

    if(0 != process.exitCode())
    {
        errString = process.errorString();
        return false;
    }
    return true;
}

bool MainWindow::checkFFprobe(QString& errString) const
{
    return checkExeCommon(FFMpeg::GetFFmpeg(settings_), errString);
}
bool MainWindow::checkFFmpeg(QString& errString) const
{
    return checkExeCommon(FFMpeg::GetFFmpeg(settings_), errString);
}













void MainWindow::initLangMenus()
{
    QString lang = settings_.valueString(KEY_LANGUAGE);
    if(lang.isEmpty() || lang=="English")
        return;

    ui->menu_Language->setTitle(tr("Language") + " (" + ("Language") + ")");
    ui->action_SystemDefault->setText(tr("System default") + " (" + ("System default") + ")");
    ui->action_English->setText(tr("English") + " (" + ("English") + ")");
    ui->action_Japanese->setText(tr("Japanese") + " (" + ("Japanese") + ")");
}

void MainWindow::on_action_Help_triggered()
{
    QString lang = settings_.valueString(KEY_LANGUAGE);
    if(lang.isEmpty())
    {
        lang = GetSystemDefaultLang();
    }


    QString url = "https://github.com/ambiesoft/SceneExplorer/blob/master/";
    if(lang=="Japanese")
    {
        url += "README.jp.md";
    }
    else
    {
        url += "README.md";
    }

    if(!QDesktopServices::openUrl(url))
    {
        Alert(this, tr("failed to launch %1.").arg(url));
        return;
    }

}

bool MainWindow::CreateNewTag(const QString& tag,
                              const QString& yomi,
                              qint64* insertedTag)
{
    if(pDoc_->IsTagExist(tag))
    {
        Alert(this,tr("Tag \"%1\" already exists.").arg(tag));
        return false;
    }

    qint64 dummy;
    if(!insertedTag)
        insertedTag = &dummy;

    if(!pDoc_->Insert(tag, yomi, *insertedTag))
    {
        Alert(this,tr("Failed to insert or replace Tag."));
        return false;
    }

    return LoadTags();
}


void MainWindow::on_listTag_itemSelectionChanged()
{
    if(limitManager_)
        limitManager_->Reset();
    itemChangedCommon();
}


void MainWindow::editTag()
{
    TagItem* ti =(TagItem*) ui->listTag->currentItem();
    if(ti->IsAllItem())
        return;

    if(!pDoc_)
    {
        Alert(this,
              tr("No Document"));
        return;
    }

    QString tag,yomi;
    if(!pDoc_->GetTag(ti->tagid(), tag, yomi))
    {
        Alert(this,
              tr("Failed to get tag data."));
        return;
    }

    TagInputDialog dlg(this);
    dlg.setTag(tag);
    dlg.setYomi(yomi);
    if(!dlg.exec())
        return;

    if(!pDoc_->ReplaceTag(ti->tagid(), dlg.tag(), dlg.yomi()))
    {
        Alert(this,
              tr("Failed to Replace Tag."));
        return;
    }

    ti->setText(dlg.tag());
}
void MainWindow::deleteTag()
{
    TagItem* ti =(TagItem*) ui->listTag->currentItem();
     if(ti->IsAllItem())
         return;

     if(!pDoc_)
     {
         Alert(this,
               tr("No Document"));
         return;
     }

     if(!YesNo(this,
           tr("Are you sure you want to delete Tag \"%1\"").arg(ti->text())))
     {
         return;
     }

     if(!pDoc_->DeleteTag(ti->tagid()))
     {
         Alert(this,
               tr("Failed to delete tag."));
         return;
     }

     delete ui->listTag->takeItem(ui->listTag->row(ti));
}

void MainWindow::checkAllTagCommon(const bool bCheck)
{
    {
        BlockedBool tb(&tagChanging_);
        for(int i=0 ; i < ui->listTag->count();++i)
        {
            TagItem* ti = (TagItem*)ui->listTag->item(i);
            if(ti->IsNormalItem())
            {
                ti->setCheckState(bCheck ? Qt::Checked : Qt::Unchecked);
            }
        }
    }
    itemChangedCommon();
}
void MainWindow::checkAllTag()
{
    checkAllTagCommon(true);
}
void MainWindow::uncheckAllTag()
{
    checkAllTagCommon(false);
}
void MainWindow::showTagContextMenu(const QPoint &pos)
{
    TagItem* ti =(TagItem*) ui->listTag->itemAt(pos);
    if(!ti)
    {
        // no item selected
        // Create menu and insert some actions
        MyContextMenu myMenuFreeArea;
        myMenuFreeArea.addAction(ui->action_AddNewTag);
        myMenuFreeArea.addAction(ui->action_Paste);
		ui->action_Paste->setEnabled(IsClipboardTagDataAvalable());
        myMenuFreeArea.addSeparator();
        myMenuFreeArea.addAction(tr("&Check All"), this, SLOT(checkAllTag()));
        myMenuFreeArea.addAction(tr("&Uncheck All"), this, SLOT(uncheckAllTag()));

        // Show context menu at handling position
        myMenuFreeArea.exec(ui->listTag->mapToGlobal(pos));
    }
    else if(ti->IsAllItem())
    {
        return;
    }
    else if(ti->IsNormalItem())
    {
        // Create menu and insert some actions
        MyContextMenu myMenuItemArea;
        myMenuItemArea.addAction(tr("&Edit"), this, SLOT(editTag()));
        myMenuItemArea.addAction(tr("&Delete"), this, SLOT(deleteTag()));

        // Show context menu at handling position
        myMenuItemArea.exec(ui->listTag->mapToGlobal(pos));
    }
    else if(ti->IsNotagItem())
    {
        return;
    }
    else
    {
        Q_ASSERT(false);
    }
}
QString MainWindow::GetTags(const qint64& id)
{
    if(!pDoc_)
        return QString();

	QSet<qint64> tagids;
    if(!pDoc_->GetTagsFromID(id, tagids))
    {
        Alert(this,
              tr("Failed to Get Tags From ID."));
        return QString();
    }

    QString ret;
    for(const qint64& tagid : tagids) // int i=0; i < tagids.count(); ++i)
    {
        // qint64 tagid = tagids[i];
        QString tag,yomi;
        if(!pDoc_->GetTag(tagid,tag,yomi))
        {
            Alert(this,
                  tr("Failed to GetTag."));
            return QString();
        }
		if (!ret.isEmpty())
			ret += ",";
        ret += tag;
        //if((i+1) != tagids.count())
        //    ret += ",";
    }
    return ret;
}

void MainWindow::on_listTag_itemChanged(QListWidgetItem *)
{
    if(limitManager_)
        limitManager_->Reset();
    itemChangedCommon();
}

void MainWindow::on_action_Output_triggered()
{
    setFontCommon1(KEY_FONT_OUPUT, ui->txtLog);
}

void MainWindow::on_action_FontDirectory_triggered()
{
    setFontCommon1(KEY_FONT_DIRECTORY,ui->directoryWidget);
}

void MainWindow::on_action_FontTask_triggered()
{
    setFontCommon1(KEY_FONT_TASK,ui->listTask);
}

void MainWindow::on_action_FontTag_triggered()
{
    setFontCommon1(KEY_FONT_TAG,ui->listTag);
}

void MainWindow::setMenuFont(QFont& font)
{
	if (!gpMenuFont_)
		gpMenuFont_.reset(new QFont());
    *gpMenuFont_=font;
    ui->menuBar->setFont(font);
    for(QMenu* pM : ui->menuBar->findChildren<QMenu*>())
        pM->setFont(font);
}
void MainWindow::on_action_FontMenu_triggered()
{
    setFontCommon2(KEY_FONT_MENU,
                   [this]()->QFont { return ui->menuBar->font(); },
                   [this] (QFont& font)
                    {
                       setMenuFont(font);
                    }
    );
}

QList<QWidget*> MainWindow::getAllDockingWindow()
{
    return QList<QWidget*> {
        ui->dockDirectory,
        ui->dockOutput,
        ui->dockTag,
        ui->dockTask
    };
}
void MainWindow::on_action_FontDockingWindow_triggered()
{
    QList<QWidget*> allDock = getAllDockingWindow();

    setFontCommon2(KEY_FONT_DOCKINGWINDOW,
                  [allDock]() -> QFont { return allDock[0]->font();},
                  [allDock](QFont& font)
                    {
                        for(QWidget* pW : allDock)
                            pW->setFont(font);
                    }
    );
}

void MainWindow::on_action_FontStatusbar_triggered()
{
    setFontCommon2(KEY_FONT_STATUSBAR,
                   [this]() -> QFont { return ui->statusBar->font();},
                   [this](QFont& font)
                    {
                        ui->statusBar->setFont(font);
                        foreach(QWidget* pW, getAllStatusBarWidgets())
                        {
                            pW->setFont(font);
                        }
                    }
    );
}

void MainWindow::on_action_Refresh_triggered()
{
    itemChangedCommon(true);
}




QList<QWidget*> MainWindow::getAllStatusBarWidgets()
{
    return QList<QWidget*>()
            <<
               slItemSort_ <<
               slItemCount_ <<
               slTask_ <<
               slPaused_ ;

}



void MainWindow::on_action_OpenVideo_triggered()
{
    openVideo(getSelectedID(), getSelectedVideo());
}


void MainWindow::on_action_OpenDirectory_triggered()
{
    if(ui->tableView->hasFocus() && ui->tableView->selectionModel()->hasSelection())
    {
        openVideoDirectory(getSelectedVideo());
        return;
    }
    if(ui->directoryWidget->hasFocus() && !ui->directoryWidget->selectedItems().isEmpty())
    {
        for(QListWidgetItem* qi : ui->directoryWidget->selectedItems())
        {
            DirectoryItem* di = (DirectoryItem*)qi;
            if(di->IsNormalItem())
            {
                QString dir = di->text();
                if(!QDesktopServices::openUrl(QUrl::fromLocalFile(dir)))
                {
                    Alert(this, tr("failed to launch %1.").arg(dir));
                }
            }
        }
    }
}

//void MainWindow::on_action_CopyPath_triggered()
//{
//    if(ui->tableView->hasFocus() && ui->tableView->selectionModel()->hasSelection())
//    {
//        QString movieFile = getSelectedVideo();
//        if(movieFile.isEmpty()) { Alert(this, TR_NOVIDEO_SELECTED()); return;}

//        setClipboardText(movieFile);
//        return;
//    }
//    if(ui->directoryWidget->hasFocus() && !ui->directoryWidget->selectedItems().isEmpty())
//    {
//        QStringList all;
//        for(QListWidgetItem* di : ui->directoryWidget->selectedItems())
//        {
//            all << di->text();
//        }
//        setClipboardText(all.join("\n"));
//    }
//}

void MainWindow::on_action_ScanAllDirectories_triggered()
{
    ScanSelectedDirectory(true);
}


void MainWindow::on_action_ScanSelectedDirectory_triggered()
{
    ScanSelectedDirectory();
}


QThread::Priority* MainWindow::GetTaskPriority()
{
    return taskPriority_ ? taskPriority_.data() : nullptr;
}
int MainWindow::GetTaskPriorityAsInt()
{
    if(!taskPriority_)
        return -1;
    return (int)(*taskPriority_);
}
void MainWindow::SetTaskPriorityAsInt(int priority)
{
    if(priority==-1)
    {
        if(taskPriority_)
            taskPriority_.reset();
        return;
    }

    if(!taskPriority_) {
        taskPriority_.reset(new QThread::Priority);
    }
    *taskPriority_ = (QThread::Priority) priority;
}



void MainWindow::on_action_ScanArbitraryDirectory_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select Directory"),
                                                    lastSelectedScanDir_);
    if(dir.isEmpty())
        return;
    lastSelectedScanDir_ = dir;

    StartScan(dir);
}

void MainWindow::on_directoryWidget_itemDoubleClicked(QListWidgetItem *item)
{
    DirectoryItem* di = (DirectoryItem*)item;
    if(!di)
        return;

    if(!di->IsAllItem())
        return;

    on_action_ShowAllItem_triggered();
}

void MainWindow::on_listTag_itemDoubleClicked(QListWidgetItem *item)
{
    TagItem* ti = (TagItem*)item;
    if(!ti)
        return;

    if(!ti->IsAllItem())
        return;

    on_action_ShowAllItem_triggered();
}


void MainWindow::on_action_FocusItemTable_triggered()
{
    ui->tableView->setFocus();
}





void MainWindow::on_action_ShowAllItem_triggered()
{
    {
        BlockedBool dc(&directoryChanging_);
        BlockedBool tc(&tagChanging_);

        if(limitManager_)
            limitManager_->Reset();

        // clear find
        cmbFind_->setCurrentText(QString());

        // directory, select all
        ui->directoryWidget->clearSelection();
        DirectoryItem* dall = (DirectoryItem*)ui->directoryWidget->item(0);
        Q_ASSERT(dall->IsAllItem());
        dall->setSelected(true);
        ui->directoryWidget->setCurrentItem(dall);

        // tag select all
        ui->listTag->clearSelection();
        TagItem* tall = (TagItem*)ui->listTag->item(0);
        Q_ASSERT(tall->IsAllItem());
        tall->setSelected(true);
        ui->listTag->setCurrentItem(tall);
    }
    itemChangedCommon(true);
}




void MainWindow::on_action_ShowMissingFiles_triggered()
{
    on_ShowMissingClicked_common(ui->action_ShowMissingFiles->isChecked());
}

void MainWindow::on_action_EmptyFindTexts_triggered()
{
    int count = cmbFind_->count();
    if(count==0)
    {
        Info(this, tr("There are no find texts."));
        return;
    }
    if(!YesNo(this, tr("Are you sure you want to empty %1 find texts?").arg(count)))
        return;
    cmbFind_->clear();
}

void MainWindow::on_action_FocusFind_triggered()
{
    cmbFind_->setFocus();
}


void MainWindow::on_action_FocusDirectoryPane_triggered()
{
    ui->directoryWidget->setFocus();
}


void MainWindow::on_action_FocusTagPane_triggered()
{
    ui->listTag->setFocus();
}


void MainWindow::on_action_AddDirectory_triggered()
{
    if(!pDoc_)
        return;

    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    lastSelectedAddDir_);
    if(dir.isEmpty())
        return;
    lastSelectedAddDir_ = dir;

    if(HasDirectory(dir))
    {
        Alert(this, tr("Directory '%1' already exists.").arg(dir));
        return;
    }

    DirectoryItem* newdi=nullptr;
    if(!pDoc_->InsertDirectory(dir, newdi))
    {
        Alert(this,TR_FAILED_TO_INSERT_DIRECTORY_INTO_DATABASE());
        return;
    }

    ui->directoryWidget->addItem(newdi);
}


void MainWindow::on_action_AddNewTag_triggered()
{
    if(!pDoc_)
    {
        Alert(this, tr("No Document"));
        return;
    }

    TagInputDialog dlg(this);
    if(!dlg.exec())
        return;

    QString tag=dlg.tag();
    QString yomi=dlg.yomi();

    qint64 insertedTag = -1;
    if(!CreateNewTag(tag,yomi,&insertedTag))
        return;
    Q_ASSERT(insertedTag >= 0);

    if(tableContextMenuActivaing_)
    {
        // comming from context menu
        qint64 id = getSelectedID();
        if(id >= 0)
            pDoc_->SetTagged(id, insertedTag, true);
    }
}

void MainWindow::on_action_ExternalTools_triggered()
{
    OptionExternalToolsDialog dlg(settings_, this);
    dlg.items_ = externalTools_;
    if(QDialog::Accepted != dlg.exec())
        return;
    externalTools_ = dlg.items_;
}


void MainWindow::on_action_AboutDocument_triggered()
{
    QString title = APPNAME_DISPLAY;
//    QString text;

//    text.append(tr("Executable"));
//    text.append(": ");
//    text.append(QCoreApplication::applicationFilePath());

//    text.append("\n\n");

//    text.append(tr("Document"));
//    text.append(": ");
//    text.append(pDoc_ ? pDoc_->GetFullName(): tr("<No document>"));

//    text.append("\n\n");

//    text.append(tr("Ini file"));
//    text.append(": ");
//    text.append(settings_.fileName());

//    text.append("\n\n");

//    text.append(tr("Database directory"));
//    text.append(": ");
//    text.append(QDir(".").absolutePath());


//    QMessageBox msgbox(this);
//    msgbox.setIcon(QMessageBox::Information);
//    msgbox.setText(text);
//    msgbox.setWindowTitle(title);
//    msgbox.exec();

    DocinfoDialog dlg(this,
                      QCoreApplication::applicationFilePath(),
                      pDoc_ ? pDoc_->GetFullName(): tr("<No document>"),
                      settings_.fileName(),
                      QDir(".").absolutePath());

    if(QDialog::Accepted != dlg.exec())
        return;

}


void MainWindow::on_action_CommandLine_triggered()
{
    QString helpText;
    processCommandLine(&helpText);

    Info(this,helpText);
}


void MainWindow::on_action_AboutQt_triggered()
{
    QMessageBox::aboutQt(this);
}

