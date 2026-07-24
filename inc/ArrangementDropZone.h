#ifndef FOHMEDIASHOW_ARRANGEMENTDROPZONE_H
#define FOHMEDIASHOW_ARRANGEMENTDROPZONE_H

#include <QWidget>
#include "FlowLayout.h"

class ArrangementDropZone : public QWidget {
    Q_OBJECT
public:
    explicit ArrangementDropZone(QWidget* parent = nullptr) : QWidget(parent) {}

    signals:
        void componentDropped(const QString& name, int index);

public slots:
    // Called by DraggableComponentBtn::manualDragReleased
    void onManualDrop(const QString& name, QPoint globalPos) {
    // Convert global position to local widget coordinates
    QPoint localPos = mapFromGlobal(globalPos);

    // Check we are actually within this widget's bounds
    if (!rect().contains(localPos)) return;

    int index = indexAtPosition(localPos);
    emit componentDropped(name, index);
}

private:
    int indexAtPosition(QPoint localPos) const {
        FlowLayout* layout = qobject_cast<FlowLayout*>(this->layout());
        if (!layout) return 0;

        int count = layout->count();
        if (count == 0) return 0;

        // Default to end
        int index = count;

        for (int i = 0; i < count; ++i) {
            QWidget* w = layout->itemAt(i)->widget();
            if (!w) continue;
            if (localPos.x() < w->geometry().center().x()) {
                index = i;
                break;
            }
        }
        return index;
    }
};

#endif //FOHMEDIASHOW_ARRANGEMENTDROPZONE_H