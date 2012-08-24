#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "image/image.hpp"
#include <QFileDialog>
#include <QDateTime>
#include <QMessageBox>
#include <QProgressDialog>
#include <qmessagebox.h>
#include "reconstruction/reconstruction_window.h"
#include "dsi_interface_static_link.h"
#include "prog_interface_static_link.h"
#include "tracking_static_link.h"
#include "tracking/tracking_window.h"
#include "mainwindow.h"
#include "dicom/dicom_parser.h"
#include "ui_mainwindow.h"
#include "simulation.h"
#include "reconstruction/vbcdialog.h"
#include "mapping/atlas.hpp"
#include "libs/vbc/vbc.hpp"

std::vector<atlas> atlas_list;

extern std::auto_ptr<QProgressDialog> progressDialog;
MainWindow::MainWindow(QWidget *parent) :
        QMainWindow(parent),
        ui(new Ui::MainWindow)
{

    QDir dir = QDir::currentPath()+"/atlas";
    QStringList atlas_name_list= dir.entryList(QStringList("*.nii"),QDir::Files|QDir::NoSymLinks);
    for(int index = 0;index < atlas_name_list.size();++index)
    {
        atlas_list.push_back(atlas());
        if(!atlas_list.back().load_from_file((dir.absolutePath() + "/" + atlas_name_list[index]).toLocal8Bit().begin()))
            atlas_list.pop_back();
        atlas_list.back().name = atlas_name_list[index].toLocal8Bit().begin();
    }

    progressDialog.reset(new QProgressDialog);
    ui->setupUi(this);
    ui->recentFib->setColumnCount(3);
    ui->recentFib->setColumnWidth(0,400);
    ui->recentFib->setColumnWidth(1,400);
    ui->recentFib->setColumnWidth(2,200);
    ui->recentSrc->setColumnCount(3);
    ui->recentSrc->setColumnWidth(0,400);
    ui->recentSrc->setColumnWidth(1,400);
    ui->recentSrc->setColumnWidth(2,200);
    QObject::connect(ui->recentFib,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(open_fib_at(int,int)));
    QObject::connect(ui->recentSrc,SIGNAL(cellDoubleClicked(int,int)),this,SLOT(open_src_at(int,int)));
    updateRecentList();

    if (settings.contains("WORK_PATH"))
        ui->workDir->addItems(settings.value("WORK_PATH").toStringList());
    else
        ui->workDir->addItem(QDir::currentPath());

    ui->dockWidget_3->hide();
}

void MainWindow::open_fib_at(int row,int col)
{
    loadFib(ui->recentFib->item(row,1)->text() + "/" +
            ui->recentFib->item(row,0)->text());
}

void MainWindow::open_src_at(int row,int col)
{
    loadSrc(QStringList() << (ui->recentSrc->item(row,1)->text() + "/" +
            ui->recentSrc->item(row,0)->text()));
}


MainWindow::~MainWindow()
{
    QStringList workdir_list;
    for (int index = 0;index < 10 && index < ui->workDir->count();++index)
        workdir_list << ui->workDir->itemText(index);
    std::swap(workdir_list[0],workdir_list[ui->workDir->currentIndex()]);
    settings.setValue("WORK_PATH", workdir_list);
    delete ui;

}

\
void MainWindow::updateRecentList(void)
{
    {
        QStringList file_list = settings.value("recentFibFileList").toStringList();
        for (int index = 0;index < file_list.size();)
            if(!QFileInfo(file_list[index]).exists())
                file_list.removeAt(index);
            else
                ++index;
            ui->recentFib->clear();
            ui->recentFib->setRowCount(file_list.size());
        for (int index = 0;index < file_list.size();++index)
        {
            ui->recentFib->setRowHeight(index,20);
            ui->recentFib->setItem(index, 0, new QTableWidgetItem(QFileInfo(file_list[index]).fileName()));
            ui->recentFib->setItem(index, 1, new QTableWidgetItem(QFileInfo(file_list[index]).absolutePath()));
            ui->recentFib->setItem(index, 2, new QTableWidgetItem(QFileInfo(file_list[index]).created().toString()));
            ui->recentFib->item(index,0)->setFlags(ui->recentFib->item(index,0)->flags() & ~Qt::ItemIsEditable);
            ui->recentFib->item(index,1)->setFlags(ui->recentFib->item(index,1)->flags() & ~Qt::ItemIsEditable);
            ui->recentFib->item(index,2)->setFlags(ui->recentFib->item(index,2)->flags() & ~Qt::ItemIsEditable);
        }
    }
    {
        QStringList file_list = settings.value("recentSrcFileList").toStringList();
        for (int index = 0;index < file_list.size();)
            if(!QFileInfo(file_list[index]).exists())
                file_list.removeAt(index);
            else
                ++index;
        ui->recentSrc->clear();
        ui->recentSrc->setRowCount(file_list.size());
        for (int index = 0;index < file_list.size();++index)
        {
            ui->recentSrc->setRowHeight(index,20);
            ui->recentSrc->setItem(index, 0, new QTableWidgetItem(QFileInfo(file_list[index]).fileName()));
            ui->recentSrc->setItem(index, 1, new QTableWidgetItem(QFileInfo(file_list[index]).absolutePath()));
            ui->recentSrc->setItem(index, 2, new QTableWidgetItem(QFileInfo(file_list[index]).created().toString()));
            ui->recentSrc->item(index,0)->setFlags(ui->recentSrc->item(index,0)->flags() & ~Qt::ItemIsEditable);
            ui->recentSrc->item(index,1)->setFlags(ui->recentSrc->item(index,1)->flags() & ~Qt::ItemIsEditable);
            ui->recentSrc->item(index,2)->setFlags(ui->recentSrc->item(index,2)->flags() & ~Qt::ItemIsEditable);
        }
    }
    QStringList header;
    header << "File Name" << "Directory" << "Date";
    ui->recentFib->setHorizontalHeaderLabels(header);
    ui->recentSrc->setHorizontalHeaderLabels(header);
}

void MainWindow::addFib(QString filename)
{
    // update recent file list
    QStringList files = settings.value("recentFibFileList").toStringList();
    files.removeAll(filename);
    files.prepend(filename);
    while (files.size() > MaxRecentFiles)
        files.removeLast();
    settings.setValue("recentFibFileList", files);
    updateRecentList();
}

void MainWindow::addSrc(QString filename)
{
    // update recent file list
    QStringList files = settings.value("recentSrcFileList").toStringList();
    files.removeAll(filename);
    files.prepend(filename);
    while (files.size() > MaxRecentFiles)
        files.removeLast();
    settings.setValue("recentSrcFileList", files);
    updateRecentList();
}

void MainWindow::loadFib(QString filename)
{
    std::string file_name = filename.toLocal8Bit().begin();
    begin_prog("load fib");
    check_prog(0,1);
    std::auto_ptr<ODFModel> new_handle(new ODFModel);
    if (!new_handle->load_from_file(&*file_name.begin()))
    {
        QMessageBox::information(this,"error","Cannot load the .fib file, please check the memory sufficiency",0);
        return;
    }
    tracking_window* new_mdi = new tracking_window(this,new_handle.release());
    new_mdi->setAttribute(Qt::WA_DeleteOnClose);
    new_mdi->absolute_path = QFileInfo(filename).absolutePath();
    new_mdi->setWindowTitle(filename);
    new_mdi->showNormal();
    addFib(filename);

}

void MainWindow::loadSrc(QStringList filenames)
{
    try
    {
        reconstruction_window* new_mdi = new reconstruction_window(filenames,this);
        new_mdi->setAttribute(Qt::WA_DeleteOnClose);
        new_mdi->show();
        if(filenames.size() == 1)
            addSrc(filenames[0]);
    }
    catch(...)
    {

    }

}


void MainWindow::openRecentFibFile(void)
{
    QAction *action = qobject_cast<QAction *>(sender());
    loadFib(action->data().toString());
}
void MainWindow::openRecentSrcFile(void)
{
    QAction *action = qobject_cast<QAction *>(sender());
    loadSrc(QStringList() << action->data().toString());
}

void MainWindow::on_OpenDICOM_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                                this,
                                "Open Images files",
                                ui->workDir->currentText(),
                                "Image files (*.dcm *.hdr *.nii 2dseq);;All files (*.*)" );
    if ( filenames.isEmpty() )
        return;
    if(QFileInfo(filenames[0]).baseName() != "2dseq")
    {
        QString sel = QString("*.")+QFileInfo(filenames[0]).suffix();
        QDir directory = QFileInfo(filenames[0]).absoluteDir();
        QStringList file_list = directory.entryList(QStringList(sel),QDir::Files|QDir::NoSymLinks);
        if(file_list.size() > filenames.size())
        {
            QString msg =
              QString("There are %1 %2 files in the directory. Select all?").arg(file_list.size()).arg(QFileInfo(filenames[0]).suffix());
            int result = QMessageBox::information(this,"Input images",msg,
                                     QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
            if(result == QMessageBox::Cancel)
                return;
            if(result == QMessageBox::Yes)
            {
                filenames = file_list;
                for(int index = 0;index < filenames.size();++index)
                    filenames[index] = directory.absolutePath() + "/" + filenames[index];
            }
        }
    }
    dicom_parser dp(filenames,this);
    dp.exec();
}

void MainWindow::on_Reconstruction_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                           this,
                           "Open Src files",
                           ui->workDir->currentText(),
                           "Src files (*.src.gz *.src);;All files (*.*)" );
    if (filenames.isEmpty())
        return;
    loadSrc(filenames);
}

void MainWindow::on_FiberTracking_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                           this,
                           "Open Fib files",
                           ui->workDir->currentText(),
                           "Fib files (*.fib.gz *.fib);;All files (*.*)");
    if (filename.isEmpty())
        return;
    if(QFileInfo(filename).suffix() == "nii")
    {
        image::io::nifti header;
        image::basic_image<float,3> I;
        if(!header.load_from_file(filename.toLocal8Bit().begin()))
        {
            QMessageBox::information(this,"DSI Studio","Invalid file format",0);
            return;
        }
        header >> I;
        if(header.nif_header.srow_x[0] < 0)
            image::flip_y(I);
        else
            image::flip_xy(I);
        filename += ".mat";
        image::io::mat mat;
        mat << I;
        mat.add_matrix("voxel_size",header.nif_header.pixdim+1,3,1);
        mat.save_to_file(filename.toLocal8Bit().begin());
        loadFib(filename);
        return;
    }
    loadFib(filename);
}


void RenameDICOMToDir(QString FileName, QString ToDir)
{
    std::string person, sequence, imagename;
    {
        image::io::dicom header;
        if (!header.load_from_file(FileName.toLocal8Bit().begin()))
            return;

        header.get_patient(person);
        header.get_sequence(sequence);
        header.get_image_name(imagename);
    }

    QString Person(person.c_str()), Sequence(sequence.c_str()),
    ImageName(imagename.c_str());

    ToDir += "/";
    ToDir += Person;
    if (!QDir(ToDir).exists())
        QDir(ToDir).mkdir(ToDir);


    ToDir += "/";
    ToDir += Sequence;
    if (!QDir(ToDir).exists())
        QDir(ToDir).mkdir(ToDir);

    ToDir += "/";
    ToDir += ImageName;
    QFile(FileName).rename(FileName,ToDir);
}
void RenameDICOMUnderDirToDir(QString Dir, QString ToDir, bool include_sub)
{
    if(include_sub)
    {
        QStringList dirs = QDir(Dir).entryList(QStringList("*"),
                                                QDir::Dirs | QDir::NoSymLinks | QDir::NoDotAndDotDot);
        for(int index = 0;index < dirs.size();++index)
            RenameDICOMUnderDirToDir(Dir + "/" + dirs[index],ToDir,true);
    }
    QStringList files = QDir(Dir).entryList(QStringList("*"),
                            QDir::Files | QDir::NoSymLinks);
    for(int index = 0;index < files.size();++index)
        RenameDICOMToDir(Dir + "/" + files[index],ToDir);
}


void MainWindow::on_RenameDICOM_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                                this,
                                "Open DICOM files",
                                ui->workDir->currentText(),
                                "All files (*.*)" );
    if ( filenames.isEmpty() )
        return;
    begin_prog("Rename DICOM Files");
    for (unsigned int index = 0;check_prog(index,filenames.size());++index)
        RenameDICOMToDir(filenames[index],QFileInfo(filenames[index]).absolutePath());
}




void MainWindow::on_browseDir_clicked()
{
    QString filename =
        QFileDialog::getExistingDirectory(this,"Browse Directory",
                                          ui->workDir->currentText());
    if ( filename.isEmpty() )
        return;
    ui->workDir->insertItem(0,filename);
    ui->workDir->setCurrentIndex(0);
}

void MainWindow::on_simulateMRI_clicked()
{
    (new Simulation(this,ui->workDir->currentText()))->show();
}

void MainWindow::on_RenameDICOMDir_clicked()
{
    QString path =
        QFileDialog::getExistingDirectory(this,"Browse Directory",
                                          ui->workDir->currentText());
    if ( path.isEmpty() )
        return;
    RenameDICOMUnderDirToDir(path,path,true);
}

void MainWindow::on_vbc_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                                 this,
                                 "Open template file",
                                 ui->workDir->currentText(),
                                 "Fib files (*.fib.gz);;All files (*.*)" );
    if(filename.isEmpty())
        return;
    std::auto_ptr<vbc> vbc_instance(new vbc);
    if(!vbc_instance->load_fiber_template(filename.toLocal8Bit().begin()))
    {
        QMessageBox::information(this,"Error","Cannot open template file",0);
        return;
    }

    VBCDialog* new_mdi = new VBCDialog(this,ui->workDir->currentText(),vbc_instance.release());
    new_mdi->setAttribute(Qt::WA_DeleteOnClose);
    new_mdi->show();
}

void MainWindow::on_averagefib_clicked()
{
    QStringList filenames = QFileDialog::getOpenFileNames(
                                this,
                                "Select Fib files to average",
                                ui->workDir->currentText(),
                                "Fib files (*.fib.gz *.src);;All files (*.*)" );
    if (filenames.isEmpty())
        return;
    QString outfile = QFileDialog::getSaveFileName(
                                this,
                                "Save file to",
                                filenames[0],
                                "Fib files (*.fib.gz *.src);;All files (*.*)" );
    if (outfile.isEmpty())
        return;
    std::vector<std::string> name_list(filenames.count());
    std::vector<const char*> name_list_buf(filenames.count());
    for (unsigned int index = 0;index < filenames.count();++index)
    {
        name_list[index] = filenames[index].toLocal8Bit().begin();
        name_list_buf[index] = name_list[index].c_str();
    }
    std::string out_name = outfile.toLocal8Bit().begin();
    odf_average(out_name.c_str(),
                &*name_list_buf.begin(),
                filenames.count());
}

void MainWindow::on_suprathreshold_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
                                this,
                                "Open directory",
                                ".");
    if (dir.isEmpty())
        return;
    //cluster_statistics(dir.toLocal8Bit().begin());
}
/*
#include "mapping/mni_norm.hpp"
void MainWindow::on_warpImage_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                                this,
                                "Open tr.mat file",
                                ui->workDir->currentText(),
                                "MAT files (*.mat)" );
    if (filename.isEmpty())
        return;
    MNINorm mni;
    if(!mni.load_transformation_matrix(filename.toLocal8Bit().begin()))
    {
        QMessageBox::information(this,"Error","Invalid tr.mat format",0);
        return;
    }

    filename = QFileDialog::getOpenFileName(
                                this,
                                "Open image file",
                                ui->workDir->currentText(),
                                "Nifti files (*.nii)" );
    if (filename.isEmpty())
        return;

    image::basic_image<float,3> I,out;
    {
        image::io::nifti read;
        if(!read.load_from_file(filename.toLocal8Bit().begin()))
        {
            QMessageBox::information(this,"Error","Cannot open image file",0);
            return;
        }
        read >> I;
        if(read.nif_header.srow_x[0] < 0)
            image::flip_y(I);
        else
            image::flip_xy(I);
    }
    mni.warp(I,out);
    {
        image::io::nifti write;
        image::flip_xy(out);
        write << out;
        float voxel_size[3]={1,1,1};
        write.set_voxel_size(voxel_size);
        float ir[12] = {1.0,0.0,0.0,-78.0,
                        0.0,1.0,0.0,-112.0,
                        0.0,0.0,1.0,-50.0};
        write.set_image_transformation(ir);
        write.save_to_file((filename+".warp.nii").toLocal8Bit().begin());
    }
}
*/