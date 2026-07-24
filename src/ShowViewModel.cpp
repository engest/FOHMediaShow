#include "../inc/ShowViewModel.h"
#include "../inc/Show.h"
#include "../inc/SlideDeck.h"
#include <QUrl>
#include "../inc/ShowSerializer.h"
#include "../inc/Library.h"
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QIODevice>
#include <QDir>
ShowViewModel::ShowViewModel(Show* show, QObject* parent)
    : QAbstractListModel(parent) {
    setShow(show);
}

QString ShowViewModel::showName() const {
    return m_show ? m_show->name() : "No Service Loaded";
}

void ShowViewModel::setShowName(const QString& name) {
    if (m_show && m_show->name() != name) {
        m_show->setName(name);
    }
}

QString ShowViewModel::defaultTransitionType() const {
    return m_show ? m_show->defaultTransitionType() : "Cut";
}

void ShowViewModel::setDefaultTransitionType(const QString& type) {
    if (m_show && m_show->defaultTransitionType() != type) {
        m_show->setDefaultTransitionType(type);
    }
}

int ShowViewModel::defaultTransitionDurationMs() const {
    return m_show ? m_show->defaultTransitionDurationMs() : 500;
}

void ShowViewModel::setDefaultTransitionDurationMs(int duration) {
    if (m_show && m_show->defaultTransitionDurationMs() != duration) {
        m_show->setDefaultTransitionDurationMs(duration);
    }
}

void ShowViewModel::setShow(Show* show) {
    beginResetModel();
    if (m_show) {
        disconnect(m_show, nullptr, this, nullptr);
    }
    m_show = show;
    if (m_show) {
        connect(m_show, &Show::deckAboutToBeAdded, this, [this](int index) {
            beginInsertRows(QModelIndex(), index, index);
        });
        connect(m_show, &Show::deckAdded, this, &ShowViewModel::onDeckAdded);
        connect(m_show, &Show::deckAboutToBeRemoved, this, [this](int index) {
            beginRemoveRows(QModelIndex(), index, index);
        });
        connect(m_show, &Show::deckRemoved, this, &ShowViewModel::onDeckRemoved);
        connect(m_show, &Show::deckMoved, this, &ShowViewModel::onDeckMoved);
        connect(m_show, &Show::deckReplaced, this, &ShowViewModel::onDeckReplaced);
        connect(m_show, &Show::nameChanged, this, &ShowViewModel::showNameChanged);
        connect(m_show, &Show::nameChanged, this, &ShowViewModel::autoSaveShow);
        connect(m_show, &Show::defaultTransitionTypeChanged, this, &ShowViewModel::defaultTransitionTypeChanged);
        connect(m_show, &Show::defaultTransitionTypeChanged, this, &ShowViewModel::autoSaveShow);
        connect(m_show, &Show::defaultTransitionDurationMsChanged, this, &ShowViewModel::defaultTransitionDurationMsChanged);
        connect(m_show, &Show::defaultTransitionDurationMsChanged, this, &ShowViewModel::autoSaveShow);
        
        for (int i = 0; i < m_show->deckCount(); ++i) {
            if (auto* deck = m_show->deckAt(i)) {
                connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
                connect(deck, &SlideDeck::globalBackgroundMediaChanged, this, [this, deck](const QString&) {
                    int idx = static_cast<int>(m_show->decks().indexOf(deck));
                    if (idx >= 0) {
                        QModelIndex modelIdx = createIndex(idx, 0);
                        emit dataChanged(modelIdx, modelIdx, {HasBackgroundVideoRole});
                        autoSaveShow();
                    }
                });
            }
        }
        
        m_activeIndex = m_show->deckCount() > 0 ? 0 : -1;
    } else {
        m_activeIndex = -1;
    }
    endResetModel();
    emit showNameChanged();
    emit activeDeckChanged(activeDeck());
    emit defaultTransitionTypeChanged();
    emit defaultTransitionDurationMsChanged();
}

int ShowViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || !m_show) return 0;
    return m_show->deckCount();
}

QVariant ShowViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !m_show || index.row() >= m_show->deckCount()) return {};

    SlideDeck* deck = m_show->deckAt(index.row());
    if (!deck) return {};

    switch (role) {
        case NameRole: return deck->name();
        case IsActiveRole: return index.row() == m_activeIndex;
        case HasBackgroundVideoRole: return !deck->globalBackgroundMedia().isEmpty();
        default: return {};
    }
}

QHash<int, QByteArray> ShowViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "deckName";
    roles[IsActiveRole] = "isActive";
    roles[HasBackgroundVideoRole] = "hasBackgroundVideo";
    return roles;
}

void ShowViewModel::setActiveIndex(int index) {
    if (!m_show) return;
    
    if (index < 0 || index >= m_show->decks().size()) {
        m_activeIndex = -1;
        emit activeDeckChanged(nullptr);
        emit activeIndexChanged();
        return;
    }

    if (m_activeIndex != index) {
        m_activeIndex = index;
        
        // Deselect previous
        for (int i = 0; i < m_show->decks().size(); ++i) {
            QModelIndex idx = createIndex(i, 0);
            emit dataChanged(idx, idx, {IsActiveRole});
        }
        
        emit activeDeckChanged(m_show->decks()[index]);
        emit activeIndexChanged();
    }
}

void ShowViewModel::loadShow(const QString& filePath) {
    // filePath might be a file:// URL from QML FileDialog
    QString path = filePath;
    if (path.startsWith("file://")) {
        path = QUrl(filePath).toLocalFile();
    }
    
    Show* newShow = ShowSerializer::loadFohsFile(path, this);
    if (newShow) {
        if (m_show) {
            m_show->deleteLater();
        }
        setShow(newShow);
    }
}

SlideDeck* ShowViewModel::activeDeck() const {
    if (!m_show || m_activeIndex < 0 || m_activeIndex >= m_show->deckCount()) return nullptr;
    return m_show->deckAt(m_activeIndex);
}

void ShowViewModel::onDeckAdded(int index) {
    endInsertRows();
    if (auto* deck = m_show->deckAt(index)) {
        connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
        connect(deck, &SlideDeck::globalBackgroundMediaChanged, this, [this, deck](const QString&) {
            int idx = static_cast<int>(m_show->decks().indexOf(deck));
            if (idx >= 0) {
                QModelIndex modelIdx = createIndex(idx, 0);
                emit dataChanged(modelIdx, modelIdx, {HasBackgroundVideoRole});
                autoSaveShow();
            }
        });
    }
    if (m_activeIndex >= index) {
        m_activeIndex++; // Shift active index
    } else if (m_activeIndex == -1) {
        setActiveIndex(0);
    }
    autoSaveShow();
}

void ShowViewModel::onDeckRemoved(int index) {
    endRemoveRows();
    if (m_activeIndex == index) {
        setActiveIndex(m_show->deckCount() > 0 ? 0 : -1);
    } else if (m_activeIndex > index) {
        m_activeIndex--;
    }
    autoSaveShow();
}

void ShowViewModel::onDeckMoved(int fromIndex, int toIndex) {
    beginMoveRows(QModelIndex(), fromIndex, fromIndex, QModelIndex(), toIndex > fromIndex ? toIndex + 1 : toIndex);
    endMoveRows();
    // Basic active index adjustment
    if (m_activeIndex == fromIndex) {
        m_activeIndex = toIndex;
    } else if (fromIndex < m_activeIndex && toIndex >= m_activeIndex) {
        m_activeIndex--;
    } else if (fromIndex > m_activeIndex && toIndex <= m_activeIndex) {
        m_activeIndex++;
    }
    autoSaveShow();
}

void ShowViewModel::onDeckReplaced(int index) {
    if (auto* deck = m_show->deckAt(index)) {
        connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
    }
    QModelIndex qidx = createIndex(index, 0);
    emit dataChanged(qidx, qidx, {NameRole, IsActiveRole});
    if (m_activeIndex == index) {
        emit activeDeckChanged(m_show->deckAt(index));
    }
    autoSaveShow();
}

QVariantMap ShowViewModel::getLayoutProperties(const QString& layoutName) const {
    return Library::getLayoutProperties(layoutName);
}

QVariantList ShowViewModel::getAvailableShows() const {
    QVariantList list;
    for (const auto& pair : Library::listShowsWithTitles()) {
        QVariantMap map;
        map["title"] = pair.first;
        map["path"] = pair.second;
        list.append(map);
    }
    return list;
}

void ShowViewModel::importShow(const QString& filePath) {
    QString path = filePath;
    if (path.startsWith("file://")) {
        path = QUrl(filePath).toLocalFile();
    }
    Library::importShowFile(path);
}

QVariantList ShowViewModel::getAvailableDecks() const {
    QVariantList list;
    for (const auto& pair : Library::listDecksWithTitles()) {
        QVariantMap map;
        map["title"] = pair.first;
        map["path"] = pair.second;
        list.append(map);
    }
    return list;
}

void ShowViewModel::addDeck(const QString& filePath) {
    if (!m_show) return;
    QString path = filePath;
    if (path.startsWith("file://")) {
        path = QUrl(filePath).toLocalFile();
    }
    ShowSerializer::loadFohdFile(path, m_show);
}

void ShowViewModel::removeDeck(int index) {
    if (!m_show || index < 0 || index >= m_show->deckCount()) return;
    m_show->removeDeck(index);
}

void ShowViewModel::moveDeck(int fromIndex, int toIndex) {
    if (!m_show || fromIndex < 0 || fromIndex >= m_show->deckCount() || toIndex < 0 || toIndex >= m_show->deckCount()) return;
    if (fromIndex == toIndex) return;
    m_show->moveDeck(fromIndex, toIndex);
}

void ShowViewModel::autoSaveShow() {
    if (m_show && !m_show->sourceFile().isEmpty()) {
        ShowSerializer::saveShowToFile(m_show);
    }
}

void ShowViewModel::newShow(const QString& title) {
    QString safeTitle = title;
    safeTitle.replace(QRegularExpression("[^a-zA-Z0-9 -]"), "");
    if (safeTitle.isEmpty()) safeTitle = "Untitled";
    
    QString filename = safeTitle.replace(" ", "_") + ".fohs";
    QString path = Library::uniqueDestPath(Library::showsDir(), filename);
    
    Show* show = new Show(title, this);
    show->setSourceFile(path);
    ShowSerializer::saveShowToFile(show);
    
    if (m_show) {
        m_show->deleteLater();
    }
    setShow(show);
}

QVariantList ShowViewModel::getShowsWithDecks() const {
    QVariantList list;
    auto shows = Library::listShowsWithTitles();
    for (const auto& pair : shows) {
        QVariantMap map;
        map["title"] = pair.first;
        map["path"] = pair.second; // filename
        
        QStringList decks;
        QFile file(pair.second);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            
            QStringList sections = content.split("---\n", Qt::SkipEmptyParts);
            if (sections.size() >= 2) {
                QStringList deckLines = sections[1].split('\n', Qt::SkipEmptyParts);
                QString currentDeckFile;
                QString currentArrangement;
                
                auto addDeck = [&]() {
                    if (!currentDeckFile.isEmpty()) {
                        QString title = Library::getDeckTitle(currentDeckFile);
                        if (currentArrangement.isEmpty()) {
                            decks.append(title);
                        } else {
                            decks.append(title + " - " + currentArrangement);
                        }
                        currentDeckFile.clear();
                        currentArrangement.clear();
                    }
                };
                
                for (const QString& line : deckLines) {
                    QString trimmed = line.trimmed();
                    if (trimmed.startsWith("- deck:")) {
                        addDeck();
                        int firstQuote = static_cast<int>(trimmed.indexOf('"'));
                        int lastQuote = static_cast<int>(trimmed.lastIndexOf('"'));
                        if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                            currentDeckFile = trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                        }
                    } else if (trimmed.startsWith("arrangement:")) {
                        int firstQuote = static_cast<int>(trimmed.indexOf('"'));
                        int lastQuote = static_cast<int>(trimmed.lastIndexOf('"'));
                        if (firstQuote != -1 && lastQuote != -1 && lastQuote > firstQuote) {
                            currentArrangement = trimmed.mid(firstQuote + 1, lastQuote - firstQuote - 1);
                        }
                    }
                }
                addDeck(); // last one
            }
        }
        map["decks"] = decks;
        list.append(map);
    }
    return list;
}

void ShowViewModel::removeShow(const QString& filePath) {
    QFile file(filePath);
    if (file.remove()) {
        // If it's the currently active show, reset
        if (m_show && QFileInfo(m_show->sourceFile()).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath()) {
            auto* emptyShow = new Show(QStringLiteral("Untitled Show"), this);
            setShow(emptyShow);
        }
    }
}

void ShowViewModel::reloadDeck(const QString& sourceFile) {
    if (!m_show) return;
    
    QString fullPath = QDir(Library::slidedecksDir()).absoluteFilePath(sourceFile);
    
    for (int i = 0; i < m_show->deckCount(); ++i) {
        SlideDeck* existingDeck = m_show->deckAt(i);
        if (existingDeck && existingDeck->sourceFile() == sourceFile) {
            QString currentActive = existingDeck->activeArrangementName();
            SlideDeck* newDeck = ShowSerializer::loadFohdFile(fullPath, static_cast<QObject*>(m_show));
            if (newDeck) {
                if (newDeck->arrangementNames().contains(currentActive)) {
                    newDeck->setActiveArrangement(currentActive);
                } else if (newDeck->arrangementCount() > 0) {
                    newDeck->setActiveArrangement(newDeck->arrangementNames().first());
                } else {
                    newDeck->setActiveArrangement("");
                }
                m_show->replaceDeck(i, newDeck);
            }
        }
    }
}
