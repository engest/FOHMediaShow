#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

class Arrangement : public QObject {
    Q_OBJECT

public:
    explicit Arrangement(QObject* parent = nullptr);
    explicit Arrangement(const QString& name, QObject* parent = nullptr);
    ~Arrangement() override = default;

    /// The name of the arrangement (e.g., "Default", "Short Service")
    QString name() const { return m_name; }
    void setName(const QString& name);

    /// The sequence of components (e.g., ["Verse 1", "Chorus 1", "Verse 1"])
    QStringList sequence() const { return m_sequence; }
    void setSequence(const QStringList& sequence);

    void appendComponent(const QString& componentName);
    void insertComponentAt(int index, const QString& componentName);
    void removeComponentAt(int index);
    void replaceComponentAt(int index, const QString& newComponentName);
    void moveComponentAt(int fromIndex, int toIndex);
    void clear();

    signals:
        void nameChanged(const QString& name);
        void sequenceChanged();
        void componentInserted(int index, const QString& componentName);
        void componentRemoved(int index, const QString& componentName);
        void componentReplaced(int index, const QString& oldName, const QString& newName);
        void componentMoved(int fromIndex, int toIndex);

    private:
    QString m_name = QStringLiteral("New Arrangement");
    QStringList m_sequence;
};
