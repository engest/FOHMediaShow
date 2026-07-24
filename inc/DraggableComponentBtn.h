#ifndef FOHMEDIASHOW_DRAGGABLECOMPONENTBTN_H
#define FOHMEDIASHOW_DRAGGABLECOMPONENTBTN_H

#include "../inc/ThemeManager.h"

#include <QPushButton>
#include <QMouseEvent>
#include <QLabel>
#include <QApplication>
#include <QPainter>

class DraggableComponentBtn : public QPushButton {
    Q_OBJECT
public:
    explicit DraggableComponentBtn(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent) {
        setMouseTracking(true);
        setAttribute(Qt::WA_Hover, true);
    }

    void setDragColor(const QColor& color) {
        m_dragColor = color;
        m_baseColor = color;
        m_hoverColor = color.lighter(130);
        applyBaseStyle();
    }

signals:
    void manualDragReleased(const QString& text, QPoint globalPos);

protected:
    void enterEvent(QEnterEvent* event) override {
        if (!m_dragging) applyHoverStyle();
        QPushButton::enterEvent(event);
    }

    void leaveEvent(QEvent* event) override {
        if (!m_dragging) applyBaseStyle();
        QPushButton::leaveEvent(event);
    }

    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragStartPosition = event->pos();
            m_dragging = false;
        }
        QPushButton::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override {
        if (!(event->buttons() & Qt::LeftButton)) return;
        handleDragMove(event->globalPosition().toPoint(), event->pos());
    }

    void mouseReleaseEvent(QMouseEvent *event) override {
        if (m_dragging && event->button() == Qt::LeftButton) {
            endDrag(event->globalPosition().toPoint());
            return;
        }
        QPushButton::mouseReleaseEvent(event);
    }

    void keyPressEvent(QKeyEvent *event) override {
        if (m_dragging && event->key() == Qt::Key_Escape) {
            cancelDrag();
            return;
        }
        QPushButton::keyPressEvent(event);
    }

    bool eventFilter(QObject* /*obj*/, QEvent* event) override {
        if (!m_dragging) return false;

        if (event->type() == QEvent::MouseMove) {
            auto* me = static_cast<QMouseEvent*>(event);
            handleDragMove(me->globalPosition().toPoint(), me->pos());
            return true;
        }
        if (event->type() == QEvent::MouseButtonRelease) {
            auto* me = static_cast<QMouseEvent*>(event);
            if (me->button() == Qt::LeftButton) {
                endDrag(me->globalPosition().toPoint());
                return true;
            }
        }
        return false;
    }

private:
    void handleDragMove(QPoint globalPos, QPoint localPos) {
        if (!m_dragging) {
            if ((localPos - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance())
                return;

            m_dragging = true;
            qApp->installEventFilter(this);

            QWidget* topLevel = window();
            m_ghost = new QLabel(topLevel);
            m_ghost->setAttribute(Qt::WA_TransparentForMouseEvents);
            m_ghost->setFixedSize(size());
            m_ghost->raise();

            QPixmap pixmap(size());
            pixmap.fill(Qt::transparent);
            {
                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                QColor bg = m_dragColor.isValid() ? m_dragColor : QColor("#555555");
                bg.setAlpha(200);
                painter.setBrush(bg);
                painter.setPen(Qt::NoPen);
                painter.drawRoundedRect(pixmap.rect().adjusted(1,1,-1,-1), 4, 4);
                painter.setPen(QColor("#F2EAD8"));
                painter.setFont(font());
                painter.drawText(pixmap.rect(), Qt::AlignCenter, text());
            }
            m_ghost->setPixmap(pixmap);
            m_ghost->show();
        }

        if (m_ghost) {
            QWidget* topLevel = window();
            QPoint parentPos = topLevel->mapFromGlobal(globalPos);
            m_ghost->move(parentPos - QPoint(m_ghost->width() / 2, m_ghost->height() / 2));
        }
    }

    void endDrag(QPoint globalPos) {
        qApp->removeEventFilter(this);
        m_dragging = false;

        if (m_ghost) {
            m_ghost->hide();
            m_ghost->deleteLater();
            m_ghost = nullptr;
        }

        applyBaseStyle();
        emit manualDragReleased(text(), globalPos);
    }

    void cancelDrag() {
        qApp->removeEventFilter(this);
        m_dragging = false;

        if (m_ghost) {
            m_ghost->hide();
            m_ghost->deleteLater();
            m_ghost = nullptr;
        }

        applyBaseStyle();
    }

    void applyBaseStyle() {
        setStyleSheet(ThemeManager::getBaseStyle(m_baseColor));
    }

    void applyHoverStyle() {
        setStyleSheet(ThemeManager::getHoverStyle(m_hoverColor));
    }

    QPoint m_dragStartPosition;
    QColor m_dragColor;
    QColor m_baseColor;
    QColor m_hoverColor;
    bool m_dragging = false;
    QLabel* m_ghost = nullptr;
};

#endif //FOHMEDIASHOW_DRAGGABLECOMPONENTBTN_H