#include <QApplication>
#include <QtTest>
#include <QObject>
#include <QWidget>
#include "../inc/SlideDeck.h"
#include "../inc/SlideDeckViewModel.h"
#include "../inc/Arrangement.h"
#include "../inc/Slide.h"
#include <QLineEdit>
#include <QListWidget>
#include <QTemporaryDir>
#include "../inc/Show.h"
#include "../inc/ShowSerializer.h"

class GUITests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void init() {}
    void cleanup() {}

    void testArrangementSequenceChange() {

        SlideDeck deck("Test Deck");
        
        QList<QStringList> verseSlides;
        verseSlides.append(QStringList() << "Line 1" << "Line 2");
        verseSlides.append(QStringList() << "Line 3");
        deck.addComponent("Verse 1", verseSlides);

        Arrangement* arr = deck.appendArrangement("Main");
        deck.setActiveArrangement("Main");

        SlideDeckViewModel viewModel(&deck);
        
        arr->setSequence(QStringList() << "Verse 1" << "Verse 1");
        viewModel.buildActiveSlides();
        
        QCOMPARE(viewModel.slideCount(), 4);
    }

    void testSerializationRoundTrip() {
        QTemporaryDir tempDir;
        QVERIFY(tempDir.isValid());
        QString tempPath = tempDir.path();
        
        QString showFile = tempPath + "/TestShow.fohs";
        
        // 1. Create Data
        Show* originalShow = new Show("Test Show");
        originalShow->setSourceFile(showFile);
        
        SlideDeck* deck = originalShow->appendDeck("Song 1");
        deck->setSourceFile(tempPath + "/Song 1.fohd");
        deck->addComponent("Verse 1", {QStringList{"Line 1", "Line 2"}, QStringList{"Line 3"}});
        deck->addComponent("Chorus", {QStringList{"Chorus Line 1"}});
        
        Arrangement* arr = deck->appendArrangement("Main");
        arr->setSequence(QStringList{"Verse 1", "Chorus"});
        deck->setActiveArrangement("Main");
        
        // 2. Save
        ShowSerializer::saveShowToFile(originalShow);
        
        // 3. Load
        Show* loadedShow = ShowSerializer::loadFohsFile(showFile);
        QVERIFY(loadedShow != nullptr);
        
        // 4. Compare
        QCOMPARE(loadedShow->name(), QString("Test Show"));
        QCOMPARE(loadedShow->deckCount(), 1);
        
        SlideDeck* loadedDeck = loadedShow->deckAt(0);
        QCOMPARE(loadedDeck->name(), QString("Song 1"));
        QCOMPARE(loadedDeck->arrangementCount(), 1); // Only "Main" was added
        QCOMPARE(loadedDeck->activeArrangementName(), QString("Main"));
        
        auto comps = loadedDeck->components();
        QVERIFY(comps.contains("Verse 1"));
        QCOMPARE(comps["Verse 1"].size(), 2);
        QCOMPARE(comps["Verse 1"][0].lines.join(" "), QString("Line 1 Line 2"));
        
        // Cleanup
        delete originalShow;
        delete loadedShow;
    }
};

QTEST_MAIN(GUITests)
#include "GUITests.moc"
