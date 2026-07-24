#pragma once

#include <QMap>
#include <QSet>
#include "Arrangement.h"

struct SlideData {
    QStringList lines;
    QMap<QString, QString> layouts; // ScreenName -> LayoutFileName
};

class SlideDeck : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString defaultArrangementName READ defaultArrangementName WRITE setDefaultArrangement NOTIFY defaultArrangementChanged)
    Q_PROPERTY(QString activeArrangementName READ activeArrangementName WRITE setActiveArrangement NOTIFY activeArrangementChanged)
    Q_PROPERTY(QString globalBackgroundMedia READ globalBackgroundMedia WRITE setGlobalBackgroundMedia NOTIFY globalBackgroundMediaChanged)
    Q_PROPERTY(QStringList arrangementNames READ arrangementNames NOTIFY arrangementsChanged)
    Q_PROPERTY(QStringList componentNames READ componentNames NOTIFY componentsChanged)

public:
    explicit SlideDeck(QObject* parent = nullptr);
    explicit SlideDeck(const QString& name, QObject* parent = nullptr);
    ~SlideDeck() override = default;

    QString name() const { return m_name; }
    void setName(const QString& name);

    void copyFrom(SlideDeck* other);

    // ─── Data Layer: Components & Arrangements ───

    // Maps a component name ("Verse 1") to a list of slides,
    // where each slide is a list of lines.
    void setComponents(const QMap<QString, QList<SlideData>>& components);
    void addComponent(const QString& name, const QList<SlideData>& slides);
    void addComponent(const QString& name, const QList<QStringList>& slideLines); // Helper

    int arrangementCount() const { return static_cast<int>(m_arrangements.size()); }
    Arrangement* arrangementAt(int index) const;
    Arrangement* arrangement(const QString& name) const;

    Arrangement* appendArrangement(const QString& name = {});
    Q_INVOKABLE void cloneArrangement(const QString& sourceName, const QString& newName);
    void removeArrangement(int index);

    QStringList arrangementNames() const;

    QString activeArrangementName() const { return m_activeArrangementName; }
    void setActiveArrangement(const QString& name);

    QString defaultArrangementName() const { return m_defaultArrangementName; }
    void setDefaultArrangement(const QString& name);
    
    QString globalBackgroundMedia() const { return m_globalBackgroundMedia; }
    void setGlobalBackgroundMedia(const QString& mediaPath);

    QStringList componentNames() const;
    void setComponentOrder(const QStringList& order) { m_componentOrder = order; }
    QStringList componentOrder() const { return m_componentOrder; }
    QMap<QString, QList<SlideData>> components() const { return m_components; }


    QString sourceFile() const { return m_sourceFile; }
    void setSourceFile(const QString& file) { m_sourceFile = file; }

    QMap<QString, QString> metadata() const { return m_metadata; }
    void setMetadata(const QMap<QString, QString>& meta) { m_metadata = meta; }
    QString metadataValue(const QString& key) const { return m_metadata.value(key); }
    void setMetadataValue(const QString& key, const QString& value) {
        m_metadata.insert(key, value);
    }

signals:
    void nameChanged(const QString& name);


    void arrangementsChanged();
    void defaultArrangementChanged(const QString& name);
    void activeArrangementChanged(const QString& name);
    void globalBackgroundMediaChanged(const QString& mediaPath);
    void componentsChanged();


private:


    QString m_name = QStringLiteral("Untitled Deck");

    // The source data
    QMap<QString, QList<SlideData>> m_components;
    QList<Arrangement*> m_arrangements;
    QString m_activeArrangementName;
    QString m_defaultArrangementName;
    QString m_globalBackgroundMedia;


    QString m_sourceFile;
    QStringList m_componentOrder;
    QMap<QString, QString> m_metadata;
};
