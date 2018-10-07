#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ScaleWidget.h"
#include <QInputDialog>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>

const int hour = 60 * 60 * 1000;
const int min = 60 * 1000;
const int sec = 1000;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    scaleWidget_ = new ScaleWidget(this);
    ui->scaleLayout->insertWidget(0, scaleWidget_, 1);
    connect(ui->generateLaelsButton, &QPushButton::clicked, this, &MainWindow::generateLabels);
    connect(scaleWidget_, &ScaleWidget::currentGroupChanged, this, &MainWindow::showBookmarks);
    dialog = new QInputDialog(this);
    dialog->setInputMode(QInputDialog::IntInput);
    dialog->setIntMinimum(1);
    dialog->setIntMaximum(1e8);
    dialog->setIntValue(1e8);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::generateLabels()
{
    if (dialog->exec())
    {
        int count = dialog->intValue();
        ui->generateLaelsButton->setEnabled(false);
        ui->generateLaelsButton->setText("Generating...");
        if (!watcher_ || watcher_->isFinished())
        {
            watcher_ = new QFutureWatcher<QMap<int, int>>();
            connect(watcher_, &QFutureWatcher<QMap<int, int>>::finished, [this] () {
                ui->generateLaelsButton->setEnabled(true);
                 ui->generateLaelsButton->setText("Generate");
                if (scaleWidget_)
                    scaleWidget_->setLabels(watcher_->future().result());
            });
            QFuture<QMap<int, int>> future = QtConcurrent::run(this, &MainWindow::generate, count);
            watcher_->setFuture(future);
        }
    }
}

QMap<int, int> MainWindow::generate(int count)
{
    QMap<int, int> labels;
    for (int i = 0; i < count; ++i)
    {
        int time = (qrand() % 24) * hour + (qrand() % 60) * min + (qrand() % 60) * sec + qrand() % 1000;
        int duration = (qrand() % 3) * hour + (qrand() % 60) * min + (qrand() % 60) * sec + qrand() % 1000;
        labels.insertMulti(time, duration);
    }
    return labels;
}

void MainWindow::showBookmarks(QString list)
{
    ui->labelsListLabel->clear();
    ui->labelsListLabel->setText(list);
}
