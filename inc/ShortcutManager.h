#pragma once

#include <QObject>
#include <QEvent>
#include <QKeyEvent>
#include <QTimer>
#include <functional>

class DisplayEngine;

class ShortcutManager : public QObject {
    Q_OBJECT

public:
    explicit ShortcutManager(DisplayEngine* engine, QObject* parent = nullptr);

    void setIsEditingCallback(std::function<bool()> callback);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void applyPendingNavigation();

private:
    DisplayEngine* m_engine = nullptr;
    
    bool m_navigationPending = false;
    int  m_pendingNavigationDelta = 0;

    std::function<bool()> m_isEditingCallback;
};
