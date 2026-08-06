#include "../inc/ShortcutManager.h"
#include "../inc/DisplayEngine.h"

ShortcutManager::ShortcutManager(DisplayEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine) {
}

void ShortcutManager::setIsEditingCallback(std::function<bool()> callback) {
    m_isEditingCallback = std::move(callback);
}



bool ShortcutManager::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        int key = keyEvent->key();

        // If the user is actively editing a slide card, don't intercept keys
        if (m_isEditingCallback && m_isEditingCallback()) {
            return QObject::eventFilter(obj, event);
        }

        if (m_engine && m_engine->isRunning()) {
            if (key == Qt::Key_Left) {
                if (m_engine->slideLatency() > 0) {
                    if (!m_navigationPending) {
                        m_navigationPending = true;
                        m_pendingNavigationDelta = -1;
                        QTimer::singleShot(m_engine->slideLatency(), this, &ShortcutManager::applyPendingNavigation);
                    }
                } else {
                    m_engine->retreatSlides(1);
                }
                return true;
            }
            if (key == Qt::Key_Right || key == Qt::Key_Space) {
                if (m_engine->slideLatency() > 0) {
                    if (!m_navigationPending) {
                        m_navigationPending = true;
                        m_pendingNavigationDelta = 1;
                        QTimer::singleShot(m_engine->slideLatency(), this, &ShortcutManager::applyPendingNavigation);
                    }
                } else {
                    m_engine->advanceSlides(1);
                }
                return true;
            }
        }
    }
    return QObject::eventFilter(obj, event);
}

void ShortcutManager::applyPendingNavigation() {
    if (!m_engine || !m_engine->isRunning() || !m_navigationPending) return;

    if (m_pendingNavigationDelta > 0) {
        m_engine->advanceSlides(m_pendingNavigationDelta);
    } else if (m_pendingNavigationDelta < 0) {
        m_engine->retreatSlides(-m_pendingNavigationDelta);
    }
    m_navigationPending = false;
    m_pendingNavigationDelta = 0;
}
