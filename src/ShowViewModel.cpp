#include "../inc/ShowViewModel.h"
#include "../inc/Show.h"
#include "../inc/SlideDeck.h"
#include "../inc/ShowSerializer.h"
#include "../inc/Library.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>

ShowViewModel::ShowViewModel(Show* show, QObject* parent)
    : QAbstractListModel(parent)
{
    if (show) {
        m_shows.append(show);
        connectShow(show);
    }
}

void ShowViewModel::connectShow(Show* show) {
    if (!show) return;
    connect(show, &Show::deckAboutToBeAdded, this, [this, show](int index) {
        if (show->deckCount() > 0) {
            int flatIndex = flatIndexForShowDeck(show, index);
            beginInsertRows(QModelIndex(), flatIndex, flatIndex);
        }
    });
    connect(show, &Show::deckAdded, this, &ShowViewModel::onDeckAdded);
    connect(show, &Show::deckAboutToBeRemoved, this, [this, show](int index) {
        if (show->deckCount() > 1) {
            int flatIndex = flatIndexForShowDeck(show, index);
            beginRemoveRows(QModelIndex(), flatIndex, flatIndex);
        }
    });
    connect(show, &Show::deckRemoved, this, &ShowViewModel::onDeckRemoved);
    connect(show, &Show::deckAboutToBeMoved, this, &ShowViewModel::onDeckAboutToBeMoved);
    connect(show, &Show::deckMoved, this, &ShowViewModel::onDeckMoved);
    connect(show, &Show::deckReplaced, this, &ShowViewModel::onDeckReplaced);
    connect(show, &Show::nameChanged, this, &ShowViewModel::onShowNameChanged);
    connect(show, &Show::nameChanged, this, &ShowViewModel::autoSaveShow);
    connect(show, &Show::defaultTransitionTypeChanged, this, &ShowViewModel::defaultTransitionTypeChanged);
    connect(show, &Show::defaultTransitionTypeChanged, this, &ShowViewModel::autoSaveShow);
    connect(show, &Show::defaultTransitionDurationMsChanged, this, &ShowViewModel::defaultTransitionDurationMsChanged);
    connect(show, &Show::defaultTransitionDurationMsChanged, this, &ShowViewModel::autoSaveShow);

    for (int i = 0; i < show->deckCount(); ++i) {
        if (auto* deck = show->deckAt(i)) {
            connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
            connect(deck, &SlideDeck::globalBackgroundMediaChanged, this, [this, show, deck](const QString&) {
                int deckIdx = static_cast<int>(show->decks().indexOf(deck));
                if (deckIdx >= 0) {
                    int flatIdx = flatIndexForShowDeck(show, deckIdx);
                    QModelIndex modelIdx = createIndex(flatIdx, 0);
                    emit dataChanged(modelIdx, modelIdx, {HasBackgroundVideoRole});
                    emit show->dataChanged(show->index(deckIdx, 0), show->index(deckIdx, 0), {Show::HasBackgroundVideoRole});
                    autoSaveShow();
                }
            });
        }
    }
}

void ShowViewModel::disconnectShow(Show* show) {
    if (show) {
        show->disconnect(this);
    }
}

int ShowViewModel::flatIndexForShowDeck(Show* show, int deckIndex) const {
    int flat = 0;
    for (Show* s : m_shows) {
        if (s == show) {
            return flat + deckIndex;
        }
        if (s) {
            int dc = s->deckCount();
            flat += (dc > 0 ? dc : 1);
        }
    }
    return -1;
}

Show* ShowViewModel::showAtFlatIndex(int flatIndex, int* outDeckIndex) const {
    int cur = 0;
    for (Show* s : m_shows) {
        if (!s) continue;
        int dc = s->deckCount();
        int rows = dc > 0 ? dc : 1;
        if (flatIndex >= cur && flatIndex < cur + rows) {
            if (outDeckIndex) {
                *outDeckIndex = (dc > 0) ? (flatIndex - cur) : -1;
            }
            return s;
        }
        cur += rows;
    }
    if (outDeckIndex) *outDeckIndex = -1;
    return nullptr;
}

Show* ShowViewModel::activeShow() const {
    if (m_shows.isEmpty()) return nullptr;
    if (m_activeIndex < 0) return m_shows.first();
    return showAtFlatIndex(m_activeIndex);
}

QList<QObject*> ShowViewModel::showsList() const {
    QList<QObject*> list;
    for (Show* s : m_shows) {
        if (s) list.append(s);
    }
    return list;
}

QVariantList ShowViewModel::loadedShows() const {
    QVariantList list;
    for (int i = 0; i < m_shows.size(); ++i) {
        Show* s = m_shows[i];
        if (!s) continue;
        QVariantMap showMap;
        showMap["index"] = i;
        showMap["name"] = s->name();
        showMap["sourceFile"] = s->sourceFile();
        showMap["deckCount"] = s->deckCount();

        QVariantList decksList;
        for (int d = 0; d < s->deckCount(); ++d) {
            SlideDeck* deck = s->deckAt(d);
            int flatIdx = flatIndexForShowDeck(s, d);
            QVariantMap deckMap;
            deckMap["flatIndex"] = flatIdx;
            deckMap["deckIndex"] = d;
            deckMap["showIndex"] = i;
            deckMap["name"] = deck ? deck->name() : "";
            deckMap["hasBackgroundVideo"] = deck ? !deck->globalBackgroundMedia().isEmpty() : false;
            deckMap["isActive"] = (flatIdx == m_activeIndex);
            decksList.append(deckMap);
        }
        showMap["decks"] = decksList;
        list.append(showMap);
    }
    return list;
}

QString ShowViewModel::showName() const {
    Show* s = activeShow();
    return s ? s->name() : "No Service Loaded";
}

void ShowViewModel::setShowName(const QString& name) {
    Show* s = activeShow();
    if (s && s->name() != name) {
        s->setName(name);
    }
}

void ShowViewModel::onShowNameChanged() {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (senderShow) {
        int startFlat = flatIndexForShowDeck(senderShow, 0);
        int endFlat = startFlat + senderShow->deckCount() - 1;
        if (endFlat >= startFlat) {
            emit dataChanged(createIndex(startFlat, 0), createIndex(endFlat, 0), {ShowNameRole});
        }
        if (senderShow == activeShow()) {
            emit showNameChanged();
        }
        emit loadedShowsChanged();
    }
}

QString ShowViewModel::showProgressText() const {
    Show* s = activeShow();
    if (!s || s->deckCount() == 0) return "";
    
    QStringList result;
    int showStartFlatIndex = flatIndexForShowDeck(s, 0);
    
    for (int i = 0; i < s->deckCount(); ++i) {
        SlideDeck* deck = s->deckAt(i);
        if (deck) {
            if (showStartFlatIndex + i == m_activeIndex) {
                result.append(QString("<font color=\"#FFAA00\"><b>%1</b></font>").arg(deck->name()));
            } else {
                result.append(deck->name());
            }
        }
    }
    return result.join("<br>");
}

QString ShowViewModel::defaultTransitionType() const {
    Show* s = activeShow();
    return s ? s->defaultTransitionType() : "Cut";
}

void ShowViewModel::setDefaultTransitionType(const QString& type) {
    Show* s = activeShow();
    if (s && s->defaultTransitionType() != type) {
        s->setDefaultTransitionType(type);
    }
}

int ShowViewModel::defaultTransitionDurationMs() const {
    Show* s = activeShow();
    return s ? s->defaultTransitionDurationMs() : 500;
}

void ShowViewModel::setDefaultTransitionDurationMs(int duration) {
    Show* s = activeShow();
    if (s && s->defaultTransitionDurationMs() != duration) {
        s->setDefaultTransitionDurationMs(duration);
    }
}

int ShowViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    int total = 0;
    for (Show* s : m_shows) {
        if (s) {
            int dc = s->deckCount();
            total += (dc > 0 ? dc : 1);
        }
    }
    return total;
}

QVariant ShowViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    int deckIdx = -1;
    Show* s = showAtFlatIndex(index.row(), &deckIdx);
    if (!s) return {};
    
    if (deckIdx == -1) {
        // Placeholder row for empty show
        switch (role) {
            case NameRole: return QString("No lyrics in service");
            case IsActiveRole: return false;
            case HasBackgroundVideoRole: return false;
            case ShowNameRole: return s->name();
            case ShowIndexRole: return m_shows.indexOf(s);
            case FlatIndexRole: return index.row();
            default: return {};
        }
    }

    SlideDeck* deck = s->deckAt(deckIdx);
    if (!deck) return {};

    switch (role) {
        case NameRole: return deck->name();
        case IsActiveRole: return index.row() == m_activeIndex;
        case HasBackgroundVideoRole: return !deck->globalBackgroundMedia().isEmpty();
        case ShowNameRole: return s->name();
        case ShowIndexRole: return m_shows.indexOf(s);
        case FlatIndexRole: return index.row();
        default: return {};
    }
}

QHash<int, QByteArray> ShowViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[NameRole] = "deckName";
    roles[IsActiveRole] = "isActive";
    roles[HasBackgroundVideoRole] = "hasBackgroundVideo";
    roles[ShowNameRole] = "showName";
    roles[ShowIndexRole] = "showIndex";
    roles[FlatIndexRole] = "flatIndex";
    return roles;
}

void ShowViewModel::setActiveIndex(int index) {
    if (m_shows.isEmpty() || index < 0 || index >= rowCount()) {
        if (m_activeIndex != -1) {
            m_activeIndex = -1;
            emit activeDeckChanged(nullptr);
            emit activeIndexChanged();
            emit loadedShowsChanged();
            for (Show* s : m_shows) {
                if (s) s->notifyActiveChanged();
            }
        }
        return;
    }

    if (m_activeIndex != index) {
        int oldIndex = m_activeIndex;
        Show* oldShow = activeShow();
        
        m_activeIndex = index;
        Show* newShow = activeShow();
        
        // Deselect previous
        if (oldIndex >= 0 && oldIndex < rowCount()) {
            QModelIndex idx = createIndex(oldIndex, 0);
            emit dataChanged(idx, idx, {IsActiveRole});
        }
        
        QModelIndex newIdx = createIndex(m_activeIndex, 0);
        emit dataChanged(newIdx, newIdx, {IsActiveRole});
        
        for (Show* s : m_shows) {
            if (s) s->notifyActiveChanged();
        }

        emit activeDeckChanged(activeDeck());
        emit showProgressTextChanged();
        emit activeIndexChanged();
        emit loadedShowsChanged();
        
        if (oldShow != newShow) {
            emit showNameChanged();
            emit defaultTransitionTypeChanged();
            emit defaultTransitionDurationMsChanged();
        }
    }
}

void ShowViewModel::loadShow(const QString& filePath) {
    QString path = filePath;
    if (path.startsWith("file://")) {
        path = QUrl(filePath).toLocalFile();
    }
    
    Show* newShow = ShowSerializer::loadFohsFile(path, this);
    if (newShow) {
        int startRow = rowCount();
        int count = newShow->deckCount();
        int rowsForShow = count > 0 ? count : 1;
        
        beginInsertRows(QModelIndex(), startRow, startRow + rowsForShow - 1);
        m_shows.append(newShow);
        connectShow(newShow);
        endInsertRows();
        emit showsChanged();
        emit loadedShowsChanged();
        
        if (m_activeIndex == -1 && count > 0) {
            setActiveIndex(startRow);
        }
    }
}

void ShowViewModel::unloadShow(const QString& showName) {
    for (int i = 0; i < m_shows.size(); ++i) {
        Show* s = m_shows[i];
        if (s && s->name() == showName) {
            int startRow = flatIndexForShowDeck(s, 0);
            int count = s->deckCount();
            int rowsForShow = count > 0 ? count : 1;
            
            beginRemoveRows(QModelIndex(), startRow, startRow + rowsForShow - 1);
            
            disconnectShow(s);
            m_shows.removeAt(i);
            s->deleteLater();
            
            endRemoveRows();
            emit showsChanged();
            emit loadedShowsChanged();
            
            // Fix active index
            if (m_activeIndex >= startRow + rowsForShow) {
                m_activeIndex -= rowsForShow;
            } else if (m_activeIndex >= startRow) {
                m_activeIndex = -1; // Active show was unloaded
                emit activeDeckChanged(nullptr);
            }
            
            for (Show* showObj : m_shows) {
                if (showObj) showObj->notifyActiveChanged();
            }

            emit activeIndexChanged();
            emit showNameChanged();
            emit defaultTransitionTypeChanged();
            emit defaultTransitionDurationMsChanged();
            emit showProgressTextChanged();
            
            if (m_activeIndex == -1 && rowCount() > 0) {
                setActiveIndex(0);
            } else if (m_activeIndex == -1) {
                emit activeDeckChanged(nullptr);
            } else {
                emit activeDeckChanged(activeDeck());
            }
            break;
        }
    }
}

void ShowViewModel::renameShow(const QString& oldName, const QString& newName) {
    for (Show* s : m_shows) {
        if (s && s->name() == oldName) {
            s->setName(newName);
            ShowSerializer::saveShowToFile(s);
            emit loadedShowsChanged();
            break;
        }
    }
}

SlideDeck* ShowViewModel::activeDeck() const {
    if (m_activeIndex < 0 || m_activeIndex >= rowCount()) return nullptr;
    int deckIdx = -1;
    Show* s = showAtFlatIndex(m_activeIndex, &deckIdx);
    if (!s || deckIdx < 0) return nullptr;
    return s->deckAt(deckIdx);
}

void ShowViewModel::onDeckAdded(int index) {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (!senderShow) return;
    
    if (senderShow->deckCount() > 1) {
        endInsertRows();
    } else {
        int flatIndex = flatIndexForShowDeck(senderShow, index);
        emit dataChanged(createIndex(flatIndex, 0), createIndex(flatIndex, 0));
    }
    
    if (auto* deck = senderShow->deckAt(index)) {
        connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
        connect(deck, &SlideDeck::globalBackgroundMediaChanged, this, [this, senderShow, deck](const QString&) {
            int deckIdx = static_cast<int>(senderShow->decks().indexOf(deck));
            if (deckIdx >= 0) {
                int flatIdx = flatIndexForShowDeck(senderShow, deckIdx);
                QModelIndex modelIdx = createIndex(flatIdx, 0);
                emit dataChanged(modelIdx, modelIdx, {HasBackgroundVideoRole});
                emit senderShow->dataChanged(senderShow->index(deckIdx, 0), senderShow->index(deckIdx, 0), {Show::HasBackgroundVideoRole});
                autoSaveShow();
            }
        });
    }
    
    int flatAddedIndex = flatIndexForShowDeck(senderShow, index);
    
    if (m_activeIndex >= flatAddedIndex) {
        m_activeIndex++;
    } else if (m_activeIndex == -1) {
        setActiveIndex(0);
    }
    
    for (Show* s : m_shows) {
        if (s) s->notifyActiveChanged();
    }

    emit showProgressTextChanged();
    emit loadedShowsChanged();
    autoSaveShow();
}

void ShowViewModel::onDeckRemoved(int index) {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (!senderShow) return;
    
    if (senderShow->deckCount() > 0) {
        endRemoveRows();
    } else {
        int flatIndex = flatIndexForShowDeck(senderShow, 0);
        emit dataChanged(createIndex(flatIndex, 0), createIndex(flatIndex, 0));
    }
    
    int flatRemovedIndex = flatIndexForShowDeck(senderShow, index);
    
    if (m_activeIndex == flatRemovedIndex) {
        setActiveIndex(-1);
    } else if (m_activeIndex > flatRemovedIndex) {
        m_activeIndex--;
    }
    
    for (Show* s : m_shows) {
        if (s) s->notifyActiveChanged();
    }

    emit showProgressTextChanged();
    emit loadedShowsChanged();
    autoSaveShow();
}

void ShowViewModel::onDeckAboutToBeMoved(int fromIndex, int toIndex) {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (!senderShow) return;
    int fromFlat = flatIndexForShowDeck(senderShow, fromIndex);
    int toFlat = flatIndexForShowDeck(senderShow, toIndex);
    int dest = (toFlat > fromFlat) ? toFlat + 1 : toFlat;
    beginMoveRows(QModelIndex(), fromFlat, fromFlat, QModelIndex(), dest);
}

void ShowViewModel::onDeckMoved(int fromIndex, int toIndex) {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (!senderShow) return;
    
    endMoveRows();
    
    int showStart = flatIndexForShowDeck(senderShow, 0);
    int fromFlat = showStart + fromIndex;
    int toFlat = showStart + toIndex;
    
    if (m_activeIndex == fromFlat) {
        m_activeIndex = toFlat;
        emit activeIndexChanged();
    } else if (fromFlat < m_activeIndex && toFlat >= m_activeIndex) {
        m_activeIndex--;
        emit activeIndexChanged();
    } else if (fromFlat > m_activeIndex && toFlat <= m_activeIndex) {
        m_activeIndex++;
        emit activeIndexChanged();
    }
    
    for (Show* s : m_shows) {
        if (s) s->notifyActiveChanged();
    }

    emit activeDeckChanged(activeDeck());
    emit showProgressTextChanged();
    emit loadedShowsChanged();
    autoSaveShow();
}

void ShowViewModel::onDeckReplaced(int index) {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (!senderShow) return;
    
    if (auto* deck = senderShow->deckAt(index)) {
        connect(deck, &SlideDeck::activeArrangementChanged, this, &ShowViewModel::autoSaveShow);
    }
    int flatIdx = flatIndexForShowDeck(senderShow, index);
    QModelIndex qidx = createIndex(flatIdx, 0);
    emit dataChanged(qidx, qidx, {NameRole, IsActiveRole});
    
    if (m_activeIndex == flatIdx) {
        emit activeDeckChanged(senderShow->deckAt(index));
    }
    
    for (Show* s : m_shows) {
        if (s) s->notifyActiveChanged();
    }

    emit showProgressTextChanged();
    emit loadedShowsChanged();
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
    Show* s = activeShow();
    if (!s) return;
    QString path = filePath;
    if (path.startsWith("file://")) {
        path = QUrl(filePath).toLocalFile();
    }
    ShowSerializer::loadFohdFile(path, s);
}

void ShowViewModel::removeDeck(int index) {
    if (index < 0 || index >= rowCount()) return;
    int deckIdx = -1;
    Show* s = showAtFlatIndex(index, &deckIdx);
    if (!s || deckIdx < 0) return;
    s->removeDeck(deckIdx);
}

void ShowViewModel::moveDeck(int fromIndex, int toIndex) {
    if (fromIndex < 0 || fromIndex >= rowCount() || toIndex < 0 || toIndex >= rowCount()) return;
    if (fromIndex == toIndex) return;
    
    int fromDeckIdx = -1;
    Show* fromShow = showAtFlatIndex(fromIndex, &fromDeckIdx);
    
    int toDeckIdx = -1;
    Show* toShow = showAtFlatIndex(toIndex, &toDeckIdx);
    
    if (fromShow != toShow) {
        return;
    }
    
    if (fromShow) {
        fromShow->moveDeck(fromDeckIdx, toDeckIdx);
    }
}

void ShowViewModel::autoSaveShow() {
    Show* senderShow = qobject_cast<Show*>(sender());
    if (senderShow && !senderShow->sourceFile().isEmpty()) {
        ShowSerializer::saveShowToFile(senderShow);
    } else {
        autoSaveAllShows();
    }
}

void ShowViewModel::autoSaveAllShows() {
    for (Show* s : m_shows) {
        if (s && !s->sourceFile().isEmpty()) {
            ShowSerializer::saveShowToFile(s);
        }
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
    
    int startRow = rowCount();
    beginInsertRows(QModelIndex(), startRow, startRow);
    m_shows.append(show);
    connectShow(show);
    endInsertRows();
    emit showsChanged();
    emit loadedShowsChanged();
    
    if (m_activeIndex == -1) {
        setActiveIndex(startRow);
    }
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
        for (int i = 0; i < m_shows.size(); ++i) {
            Show* s = m_shows[i];
            if (s && QFileInfo(s->sourceFile()).absoluteFilePath() == QFileInfo(filePath).absoluteFilePath()) {
                unloadShow(s->name());
                break;
            }
        }
    }
}

void ShowViewModel::reloadDeck(const QString& sourceFile) {
    QString fullPath = QDir(Library::slidedecksDir()).absoluteFilePath(sourceFile);
    
    for (Show* s : m_shows) {
        if (!s) continue;
        for (int i = 0; i < s->deckCount(); ++i) {
            SlideDeck* existingDeck = s->deckAt(i);
            if (existingDeck && existingDeck->sourceFile() == sourceFile) {
                QString currentActive = existingDeck->activeArrangementName();
                SlideDeck* newDeck = ShowSerializer::loadFohdFile(fullPath, static_cast<QObject*>(s));
                if (newDeck) {
                    if (newDeck->arrangementNames().contains(currentActive)) {
                        newDeck->setActiveArrangement(currentActive);
                    } else if (newDeck->arrangementCount() > 0) {
                        newDeck->setActiveArrangement(newDeck->arrangementNames().first());
                    } else {
                        newDeck->setActiveArrangement("");
                    }
                    s->replaceDeck(i, newDeck);
                }
            }
        }
    }
}

bool ShowViewModel::isSameShow(int index1, int index2) const {
    if (index1 < 0 || index1 >= rowCount() || index2 < 0 || index2 >= rowCount()) return false;
    return showAtFlatIndex(index1) == showAtFlatIndex(index2);
}

bool ShowViewModel::isFirstDeckOfShow(int flatIndex) const {
    if (flatIndex < 0 || flatIndex >= rowCount()) return false;
    if (flatIndex == 0) return true;
    int curDeckIdx = -1, prevDeckIdx = -1;
    Show* curShow = showAtFlatIndex(flatIndex, &curDeckIdx);
    Show* prevShow = showAtFlatIndex(flatIndex - 1, &prevDeckIdx);
    return curShow != prevShow;
}
