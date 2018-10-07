#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFutureWatcher>

namespace Ui {
class MainWindow;
}

class ScaleWidget;
class QInputDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    ScaleWidget* scaleWidget_ = nullptr;

    void generateLabels();
    QMap<int, int> generate(int count);
    void showBookmarks(QString list);
    QFutureWatcher<QMap<int, int>> *watcher_ = nullptr;
    QInputDialog* dialog;
};

#endif // MAINWINDOW_H
