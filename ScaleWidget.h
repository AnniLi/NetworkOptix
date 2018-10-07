#ifndef SCALEWIDGET_H
#define SCALEWIDGET_H

#include <QWidget>
#include <QMap>
#include <QFutureWatcher>
#include <QTimer>

class ScaleWidget : public QWidget
{
    Q_OBJECT
public:
    ScaleWidget(QWidget* parent = nullptr);
    ~ScaleWidget();

    void setLabels(const QMap<int, int>& labels);
signals:
    void currentGroupChanged(const QString& list);

private:
    void paintEvent(QPaintEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;
    void mouseMoveEvent(QMouseEvent*ev) override;

    void drawScale(QPainter* p);
    void recalculate();
    void updateData();
    void updateGroup();

    QMap<int, int> labels_;
    QList<QPair<int, QPair<int, int>>> groups_;
    QFutureWatcher<void> *watcher_ = nullptr;
    QTimer timer_;
    int currentGroup_ = -1;
    QMutex* mutex_;
};

#endif // SCALEWIDGET_H
