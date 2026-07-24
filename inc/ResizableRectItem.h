#ifndef RESIZABLERECTITEM_H
#define RESIZABLERECTITEM_H

#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <functional>

class ResizableRectItem : public QGraphicsRectItem {
public:
    explicit ResizableRectItem(QGraphicsItem* parent = nullptr);

    void setRectChangedCallback(std::function<void(QRect)> cb);

protected:
    void hoverMoveEvent(QGraphicsSceneHoverEvent* event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent* event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent* event) override;

private:
    enum ResizeState { None, TopLeft, TopRight, BottomLeft, BottomRight, Top, Bottom, Left, Right };
    ResizeState m_resizeState;
    bool m_isResizing;
    bool m_isMoving;
    QPointF m_lastMousePos;
    std::function<void(QRect)> m_onRectChanged;
    
    ResizeState getResizeState(const QPointF& pos) const;
};

#endif // RESIZABLERECTITEM_H
