#include "fsettingsdialog.h"
#include "ui_fsettingsdialog.h"
#include <QDebug>
#include <fartmanager.h>
#include <QMessageBox>
#include <QDesktopServices>


FSettingsDialog::FSettingsDialog(FDB *db, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FSettingsDialog)
{
    ui->setupUi(this);
    this->db = db;

    //Hast to be the Same order as the Stacked Widget.
   ui->listWidget->addItem("General");
   ui->listWidget->addItem("Database");
   ui->listWidget->addItem("Interface");
   ui->listWidget->addItem("Artwork");
   ui->listWidget->addItem("Watched Folders");
   ui->listWidget->setCurrentRow(0);

   ui->le_Stylesheet->setText(db->getTextPref("stylesheet"));
   ui->cb_ScanLibOnStartup->setChecked(db->getBoolPref("ScanLibsOnStartup", true));


   ui->cb_Artwork_UseCache->setChecked(db->getBoolPref("useArtworkCache", true));

   //##########################
   //WatchedFolders
   QList<FWatchedFolder> tmpList = db->getWatchedFoldersList();
    ui->lw_Folder_FolderList->clear();
   for(int i=0;i<tmpList.length();++i)
   {
       watchedFolders.insert(tmpList[i].getDirectory().absolutePath(), tmpList[i]);
       ui->lw_Folder_FolderList->addItem(tmpList[i].getDirectory().absolutePath());
   }

   QList<FLauncher> launchers = db->getLaunchers();
   for(int i = 0; i < launchers.length(); i++)
   {
       FLauncher launcher = launchers.at(i);
       ui->cb_Folder_LauncherList->addItem(launcher.getName(), QVariant(launcher.getDbId()));
   }

   //##########################

}

FSettingsDialog::~FSettingsDialog()
{
    delete ui;
}

void FSettingsDialog::on_listWidget_currentRowChanged(int i)
{
    ui->settingPages->setCurrentIndex(i);
}

void FSettingsDialog::on_pb_selectStylesheet_clicked()
{
    QString stylesheetFile = QFileDialog::getOpenFileName(this, "Choose stylesheet", QDir::homePath(), "*.qss");
    qDebug() << "Stylesheet: " << stylesheetFile;
    if(QFile::exists(stylesheetFile))
    {
        qDebug() << "New stylesheet added: " << stylesheetFile;
        db->updateTextPref("stylesheet", stylesheetFile);
        emit reloadStylesheet();
    }
}

void FSettingsDialog::on_pb_ResetStylesheet_clicked()
{
    db->updateTextPref("stylesheet", ":/stylesheet.qss");
    emit reloadStylesheet();
}

void FSettingsDialog::on_pb_ScanNow_clicked()
{
    FCrawler crawler;
    crawler.scanAllFolders();
    emit reloadLibrary();
}

void FSettingsDialog::on_lw_Folder_FolderList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if(current) {
        selectedFolder = &watchedFolders[current->text()];
        ui->cb_Folder_ForLauncher->setChecked(selectedFolder->forLauncher);
        ui->cb_Folder_LauncherList->setEnabled(selectedFolder->forLauncher);
    }
}

void FSettingsDialog::on_btn_Artwork_DownloadAll_clicked() {
    if(QMessageBox::warning(this, "Please confirm!", "If Fusion is able to find Artwork, existing Artwork will be overwritten!",QMessageBox::Ok, QMessageBox::Cancel) ==QMessageBox::Cancel)
        return;


    runningDownloads = 0;
    totalDownloads = 0;


   gameList = db->getGameList();
   for(int i=0;i<gameList.length();++i) {
       FArtManager *artmanager = new FArtManager();
       connect(artmanager, SIGNAL(startedDownload()), this, SLOT(downloadStarted()));
       connect(artmanager, SIGNAL(finishedDownload()), this, SLOT(downloadFinished()));
       artmanager->getGameData(&gameList[i], "PC");
   }

}

void FSettingsDialog::on_btn_Folder_Add_clicked()
{
    QDir gameDir = QFileDialog::getExistingDirectory(this, "Choose the Lib-Directory", QDir::homePath());
    ui->lw_Folder_FolderList->addItem(gameDir.absolutePath());

    FWatchedFolder w;
    w.setDirectory(gameDir);
    watchedFolders.insert(w.getDirectory().absolutePath(), w);
}

void FSettingsDialog::on_btn_Folder_Remove_clicked()
{
    if(QMessageBox::warning(this, "Please confirm!", "Do you really wan't to remove the Folder '" +selectedFolder->getDirectory().absolutePath()+ "'?\r\nThe containing Games won't be removed.",QMessageBox::Ok, QMessageBox::Cancel)==QMessageBox::Ok) {
        watchedFolders.remove(selectedFolder->getDirectory().absolutePath());

        ui->lw_Folder_FolderList->clear();

        for(int i=0;i<watchedFolders.count();++i)
            ui->lw_Folder_FolderList->addItem(watchedFolders.values()[i].getDirectory().absolutePath());

    }

}

void FSettingsDialog::on_cb_Folder_ForLauncher_clicked()
{
    ui->cb_Folder_LauncherList->setEnabled(ui->cb_Folder_ForLauncher->checkState());

    if(selectedFolder != NULL)
        selectedFolder->forLauncher = (bool)ui->cb_Folder_ForLauncher->checkState();
}

void FSettingsDialog::downloadFinished() {
    --runningDownloads;
    ui->la_Artwork_DownloadStatus->setText("Running Downloads:" + QString::number(runningDownloads));
    if(runningDownloads<=0)
        QMessageBox::information(this, "Downloads finished", "Finished " + QString::number(totalDownloads) + " download(s)");
}

void FSettingsDialog::downloadStarted() {
    ++runningDownloads;
    ++totalDownloads;
    ui->la_Artwork_DownloadStatus->setText("Running Downloads:" + QString::number(runningDownloads));
}

void FSettingsDialog::on_btn_Artwork_openCache_clicked() {
    QDesktopServices::openUrl(FGame::getCacheDir());

}


void FSettingsDialog::on_btn_Artwork_ClearCache_clicked() {
    QDir cacheDir(FGame::getCacheDir());

#if (QT_VERSION < QT_VERSION_CHECK(5, 0, 0))
    cacheDir.setNameFilters(QStringList()<<"*.*");
    QStringList steamFiles = cacheDir.entryList();
    for(int i=0;i<steamFiles.length();++i) {
        cacheDir.remove(steamFiles[i]);
    }
#else
   cacheDir.removeRecursively();
#endif

}



void FSettingsDialog::on_buttonBox_accepted()
{
   db->updateBoolPref("ScanLibsOnStartup", (bool)ui->cb_ScanLibOnStartup->checkState());

   //Artwortk-Page
   db->updateBoolPref("useArtworkCache", (bool)ui->cb_Artwork_UseCache->checkState());


   //##########################
   //WatchedFolders
    db->updateWatchedFolders(watchedFolders.values());

   //##########################
}
