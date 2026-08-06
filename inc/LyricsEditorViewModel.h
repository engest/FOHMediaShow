#include <QtQml/qqmlregistration.h>
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include "SlideDeck.h"
#include "SlideDeckViewModel.h"
#include <QQmlEngine>

class LyricsEditorViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QVariantList libraryFiles READ libraryFiles NOTIFY libraryFilesChanged)
    Q_PROPERTY(QString currentTitle READ currentTitle WRITE setCurrentTitle NOTIFY currentTitleChanged)
    Q_PROPERTY(QString currentArtists READ currentArtists WRITE setCurrentArtists NOTIFY currentArtistsChanged)
    Q_PROPERTY(QString currentCcli READ currentCcli WRITE setCurrentCcli NOTIFY currentCcliChanged)
    Q_PROPERTY(QString currentCopyright READ currentCopyright WRITE setCurrentCopyright NOTIFY currentCopyrightChanged)
    Q_PROPERTY(QString currentKey READ currentKey WRITE setCurrentKey NOTIFY currentKeyChanged)
    Q_PROPERTY(QString currentTempo READ currentTempo WRITE setCurrentTempo NOTIFY currentTempoChanged)
    Q_PROPERTY(QString currentTheme READ currentTheme WRITE setCurrentTheme NOTIFY currentThemeChanged)
    Q_PROPERTY(QString rawLyricsText READ rawLyricsText WRITE setRawLyricsText NOTIFY rawLyricsTextChanged)
    Q_PROPERTY(SlideDeck* currentDeck READ currentDeck NOTIFY currentDeckChanged)
    Q_PROPERTY(QObject* arrangementModel READ arrangementModel CONSTANT)
    Q_PROPERTY(QStringList availableComponents READ availableComponents NOTIFY availableComponentsChanged)
    Q_PROPERTY(QVariantList screenLayoutMappings READ screenLayoutMappings NOTIFY screenLayoutMappingsChanged)
    Q_PROPERTY(QObject* previewDeckModel READ previewDeckModel CONSTANT)

public:
    explicit LyricsEditorViewModel(QObject* parent = nullptr);
    ~LyricsEditorViewModel() override = default;

    QVariantList libraryFiles() const;
    
    QString currentTitle() const;
    void setCurrentTitle(const QString& title);

    QString currentArtists() const;
    void setCurrentArtists(const QString& artists);

    QString currentCcli() const;
    void setCurrentCcli(const QString& ccli);

    QString currentCopyright() const;
    void setCurrentCopyright(const QString& copyright);

    QString currentKey() const;
    void setCurrentKey(const QString& key);

    QString currentTempo() const;
    void setCurrentTempo(const QString& tempo);

    QString currentTheme() const;
    void setCurrentTheme(const QString& theme);

    QString rawLyricsText() const;
    void setRawLyricsText(const QString& text);

    SlideDeck* currentDeck() const;
    Q_INVOKABLE void setDefaultArrangementName(const QString& name);
    Q_INVOKABLE void setActiveArrangementName(const QString& name);
    QObject* arrangementModel() const;
    QObject* previewDeckModel() const;
    QStringList availableComponents() const;
    
    Q_INVOKABLE QVariantList screenLayoutMappings() const;
    Q_INVOKABLE void setScreenLayoutMapping(const QString& screenName, const QString& layoutFile);
    Q_INVOKABLE void removeScreenLayoutMapping(const QString& screenName);
    Q_INVOKABLE void addScreenLayoutMapping(const QString& screenName, const QString& layoutFile);

    Q_INVOKABLE void refreshLibrary();
    Q_INVOKABLE void newDeck();
    Q_INVOKABLE void loadFromLibrary(const QString& filename);
    Q_INVOKABLE void reloadCurrentDeck();
    Q_INVOKABLE void importFile(const QString& filePath);
    Q_INVOKABLE void importProFiles(const QVariantList& fileUrls);
    Q_INVOKABLE void removeLibraryItem(const QString& filename);
    Q_INVOKABLE void saveDeck(bool updateFromText = true);
    
    Q_INVOKABLE QStringList availableLayouts() const;
    
    Q_INVOKABLE void updateDeckFromRawText();
    
    Q_INVOKABLE void addArrangement(const QString& name);
    Q_INVOKABLE void removeArrangement(const QString& name);
    Q_INVOKABLE void cloneArrangement(const QString& sourceName, const QString& newName);

signals:
    void libraryFilesChanged();
    void currentTitleChanged();
    void currentArtistsChanged();
    void currentCcliChanged();
    void currentCopyrightChanged();
    void currentKeyChanged();
    void currentTempoChanged();
    void currentThemeChanged();
    void rawLyricsTextChanged();
    void currentDeckChanged();
    void availableComponentsChanged();
    void screenLayoutMappingsChanged();
    void deckSaved(const QString& sourceFile);

private:
    void updateRawTextFromDeck();

    SlideDeck* m_currentDeck = nullptr;
    QObject* m_arrangementModel = nullptr;
    SlideDeckViewModel* m_previewDeckModel = nullptr;
    QString m_rawLyricsText;
    QMap<QString, QString> m_screenLayouts;
    QVariantList m_libraryFiles;
};
