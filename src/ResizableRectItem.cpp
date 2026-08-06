#include "../inc/ResizableRectItem.h"
#include <QPen>
#include <QGraphicsScene>
#include <QGraphicsView>

const qreal HANDLE_SIZE = 10.0;

ResizableRectItem::ResizableRectItem(QGraphicsItem* parent)
    : QGraphicsRectItem(parent), m_resizeState(None), m_isResizing(false), m_isMoving(false) {
    setFlags(QGraphicsItem::ItemIsSelectable);
    setAcceptHoverEvents(true);
    setPen(QPen(QColor(255, 255, 255, 128), 2, Qt::DashLine));
}

void ResizableRectItem::setRectChangedCallback(std::function<void(QRect)> cb) {
    m_onRectChanged = cb;
}

ResizableRectItem::ResizeState ResizableRectItem::getResizeState(const QPointF& pos) const {
    QRectF r = rect();
    bool left = qAbs(pos.x() - r.left()) < HANDLE_SIZE;
    bool right = qAbs(pos.x() - r.right()) < HANDLE_SIZE;
    bool top = qAbs(pos.y() - r.top()) < HANDLE_SIZE;
    bool bottom = qAbs(pos.y() - r.bottom()) < HANDLE_SIZE;

    if (top && left) return TopLeft;
    if (top && right) return TopRight;
    if (bottom && left) return BottomLeft;
    if (bottom && right) return BottomRight;
    if (top) return Top;
    if (bottom) return Bottom;
    if (left) return Left;
    if (right) return Right;
    return None;
}

void ResizableRectItem::hoverMoveEvent(QGraphicsSceneHoverEvent* event) {
    if (m_isResizing) {
        QGraphicsRectItem::hoverMoveEvent(event);
        return;
    }
    ResizeState state = getResizeState(event->pos());
    switch (state) {
        case TopLeft:
        case BottomRight:
            setCursor(Qt::SizeFDiagCursor);
            break;
        case TopRight:
        case BottomLeft:
            setCursor(Qt::SizeBDiagCursor);
            break;
        case Left:
        case Right:
            setCursor(Qt::SizeHorCursor);
            break;
        case Top:
        case Bottom:
            setCursor(Qt::SizeVerCursor);
            break;
        default:
            setCursor(Qt::SizeAllCursor);
            break;
    }
    QGraphicsRectItem::hoverMoveEvent(event);
}

void ResizableRectItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* event) {
    if (!m_isResizing) {
        setCursor(Qt::ArrowCursor);
    }
    QGraphicsRectItem::hoverLeaveEvent(event);
}

void ResizableRectItem::mousePressEvent(QGraphicsSceneMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_resizeState = getResizeState(event->pos());
        if (m_resizeState != None) {
            m_isResizing = true;
        } else {
            m_isMoving = true;
            m_lastMousePos = event->scenePos();
        }
        event->accept();
        return;
    }
    QGraphicsRectItem::mousePressEvent(event);
}

void ResizableRectItem::mouseMoveEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isResizing) {
        QRectF r = rect();
        QPointF p = event->pos();
        qreal minSize = 20.0;

        switch (m_resizeState) {
            case TopLeft:
                if (r.right() - p.x() > minSize && r.bottom() - p.y() > minSize)
                    r.setTopLeft(p);
                break;
            case TopRight:
                if (p.x() - r.left() > minSize && r.bottom() - p.y() > minSize)
                    r.setTopRight(p);
                break;
            case BottomLeft:
                if (r.right() - p.x() > minSize && p.y() - r.top() > minSize)
                    r.setBottomLeft(p);
                break;
            case BottomRight:
                if (p.x() - r.left() > minSize && p.y() - r.top() > minSize)
                    r.setBottomRight(p);
                break;
            case Top:
                if (r.bottom() - p.y() > minSize)
                    r.setTop(p.y());
                break;
            case Bottom:
                if (p.y() - r.top() > minSize)
                    r.setBottom(p.y());
                break;
            case Left:
                if (r.right() - p.x() > minSize)
                    r.setLeft(p.x());
                break;
            case Right:
                if (p.x() - r.left() > minSize)
                    r.setRight(p.x());
                break;
            default: break;
        }
        setRect(r);
        
        if (m_onRectChanged) {
            m_onRectChanged(r.toRect());
        }
        
        event->accept();
        return;
    }
    
    if (m_isMoving) {
        QPointF delta = event->scenePos() - m_lastMousePos;
        m_lastMousePos = event->scenePos();
        
        QRectF r = rect();
        r.translate(delta);
        setRect(r);
        
        if (m_onRectChanged) {
            m_onRectChanged(r.toRect());
        }
        
        event->accept();
        return;
    }
    
    QGraphicsRectItem::mouseMoveEvent(event);
}

void ResizableRectItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* event) {
    if (m_isResizing || m_isMoving) {
        m_isResizing = false;
        m_isMoving = false;
        m_resizeState = None;
        event->accept();
        return;
    }
    QGraphicsRectItem::mouseReleaseEvent(event);
}
