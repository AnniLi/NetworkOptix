#include "ScaleWidget.h"
#include <QPainter>
#include <QDebug>
#include <QtConcurrent/QtConcurrent>
#include <QElapsedTimer>
#include <QMouseEvent>

const long long int rangeT = 24 * 60 * 60 * 1000;
const int rangeD = 3 * 60 * 60 * 1000;

ScaleWidget::ScaleWidget(QWidget *parent) : QWidget(parent)
{
    setMinimumHeight(100);
    setMaximumHeight(100);
    timer_.setInterval(500);
    timer_.setSingleShot(true);
    connect(&timer_, &QTimer::timeout, this, &ScaleWidget::updateData);
    setMouseTracking(true);
    mutex_ = new QMutex();
}

ScaleWidget::~ScaleWidget()
{
    if (watcher_)
    {
        watcher_->cancel();
        delete watcher_;
    }
    delete mutex_;
}

void ScaleWidget::setLabels(const QMap<int, int> &labels)
{
    if (!labels.isEmpty())
    {
        labels_ = labels;
        updateData();
    }
}

void ScaleWidget::paintEvent(QPaintEvent *ev)
{
    QPainter p(this);
    QBrush brushBlue(QColor(1, 1, 250, 120));
    QPen penBlue(QColor(1, 1, 250));
    QBrush brushGreen(QColor(1, 250, 1, 120));
    QPen penGreen(QColor(1, 250, 1));
    QPen penText(QColor(1, 1, 1));
    drawScale(&p);

    double koef = width() * scale_ / double(rangeT);
    QPen currentPen;
    QBrush currentBrush;
    QTextOption opt;
    QString currentText;
    opt.setAlignment(Qt::AlignLeft);
    opt.setWrapMode(QTextOption::NoWrap);
    p.translate(QPointF{offset_, 0});
    for (auto it = groups_.cbegin(); it != groups_.cend(); ++it)
    {
        p.save();
        if (it->second.first > 1)
        {
            currentBrush = brushGreen;
            currentPen = penGreen;
            currentText = QString::number(it->second.first);
        }
        else
        {
            currentBrush = brushBlue;
            currentPen = penBlue;
            auto mark = labels_.find(it->first);
            int number = std::distance(labels_.begin(), mark);
            currentText = QString("Bookmark %1").arg(number);
        }
        QRectF currentRect(koef * it->first, 40, koef * it->second.second, 20);
        p.setPen(penText);
        p.drawRect(currentRect);
        p.fillRect(currentRect, currentBrush);
        p.drawText(currentRect, currentText, opt);

        p.restore();
    }
}

void ScaleWidget::resizeEvent(QResizeEvent *ev)
{
    QWidget::resizeEvent(ev);
    timer_.stop();
    timer_.start();
}

void ScaleWidget::mouseMoveEvent(QMouseEvent *ev)
{
    auto pos = ev->pos();
    if (wasPressed_)
    {
        offset_ += pos.x() - prevXPos_;
        offset_ = qMax(qMin(offset_, 0.0), -(width() - 1.0) * (scale_ - 1));
        prevXPos_ = pos.x();
        update();
    }
    else if (pos.y() > 40 && !groups_.isEmpty())
    {
        double koef = width() * scale_ / double(rangeT);
        int newGroup = -1;

        mutex_->lock();

        for (int i = groups_.size() - 1; i >= 0; --i)
        {
            QRectF rect(koef * groups_[i].first + offset_, 40, koef * groups_[i].second.second, 20);
            if (rect.contains(pos))
            {
                newGroup = i;
                break;
            }
        }
        mutex_->unlock();
        if (newGroup != currentGroup_)
        {
            currentGroup_ = newGroup;
            updateGroup();
        }
    }
    else
        QWidget::mouseMoveEvent(ev);
}

void ScaleWidget::mousePressEvent(QMouseEvent *ev)
{
    if (ev->buttons() == Qt::LeftButton)
    {
        prevXPos_ = ev->pos().x();
        wasPressed_ = true;
    }
    QWidget::mousePressEvent(ev);
}

void ScaleWidget::mouseReleaseEvent(QMouseEvent *ev)
{
    wasPressed_ = false;
    QWidget::mousePressEvent(ev);
}

void ScaleWidget::wheelEvent(QWheelEvent *ev)
{
    if (ev->delta() > 0)
    {
        scale_ *= 1.2;
        offset_ *= 1.2;
    }
    else
    {
        scale_ = qMax(1.0, scale_ / 1.2);
        if (scale_ != 1.0)
            offset_ /= 1.2;
    }
    offset_ = qMax(qMin(offset_, 0.0), -(width() - 1.0) * (scale_ - 1));
    timer_.stop();
    timer_.start();
}

void ScaleWidget::drawScale(QPainter *p)
{
    p->save();
    QTextOption opt;
    opt.setAlignment(Qt::AlignHCenter);
    QRectF textRect(QPointF{-10, 15}, QSize(20, 20));
    p->drawRect(0, 0, width() - 1, height() - 1);
    p->translate(QPointF{offset_, 0});
    double partW = (width() - 1) * scale_ / (24.0 );
    for (int i = 0; i <= 24; ++i)
    {
        p->save();
        p->drawLine(QPointF{0, 0}, QPointF{0, 10});
        p->drawText(textRect, QString("%1h").arg(i == 24 ? 0 : i), opt);
        p->restore();
        p->translate(partW, 0);
    }
    p->restore();
}

void ScaleWidget::recalculate()
{
    QList<QPair<int, QPair<int, int>>> newGroups;
    if (!labels_.isEmpty())
    {
        long long int range = rangeT * 100.0 / double (width() * scale_);

        QPair<int, int> info = {1, labels_.begin().value()};
        int key = labels_.begin().key();
        for (auto it = ++labels_.begin(); it != labels_.end(); ++it)
        {
            if (key + range > it.key())
            {
                ++info.first;
                info.second = qMax(info.second, it.key() - key + it.value());
            }
            else
            {
                newGroups.push_back({key, info});
                key = it.key();
                info = {1, it.value()};
            }
        }
        if (key >= 0)
            newGroups.push_back({key, info});
    }
    mutex_->lock();
    groups_ = newGroups;
    mutex_->unlock();
}

void ScaleWidget::updateData()
{
    if (!watcher_ || watcher_->isFinished())
    {
        watcher_ = new QFutureWatcher<void>();
        connect(watcher_, &QFutureWatcher<void>::finished, [this] () {update();});
        QFuture<void> future = QtConcurrent::run(this, &ScaleWidget::recalculate);
        watcher_->setFuture(future);
    }
}

void ScaleWidget::updateGroup()
{
    QMutexLocker locker(mutex_);
    QString marks;
    if (currentGroup_ >= 0 && groups_.size() > currentGroup_)
    {
        int time = groups_[currentGroup_].first;
        auto it = labels_.lowerBound(time);
        int number = std::distance(labels_.begin(), it);
        int count = qMin(groups_[currentGroup_].second.first, 15);
        for (int i = 0; i < count; ++i)
            marks += QString("Bookmark %1\n").arg(number + i);
        if (count < groups_[currentGroup_].second.first)
            marks += QString("+ %1n other bookmarks").arg(groups_[currentGroup_].second.first - count);
    }
    emit currentGroupChanged(marks);
}

