#include <QApplication>
#include <QtTest>
#include <QObject>
#include <QWidget>
#include "../inc/SlideDeck.h"
#include "../inc/SlideDeckViewModel.h"
#include "../inc/Arrangement.h"
#include "../inc/ArrangementViewModel.h"
#include "../inc/Slide.h"
#include <QLineEdit>
#include <QListWidget>
#include <QTemporaryDir>
#include "../inc/Show.h"
#include "../inc/ShowViewModel.h"
#include "../inc/ShowSerializer.h"
#include "../inc/LyricsEditorViewModel.h"
#include "../inc/DisplayEngine.h"
#include "../inc/SettingsManager.h"
#include "../inc/ProPresenterImporter.h"

class GUITests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {}
    void cleanupTestCase() {}
    void init() {}
    void cleanup() {}

    void testSettingsManagerPracticeDelay() {
        SettingsManager sm;
        QCOMPARE(sm.practiceDelayMs(), 273);

        bool signalFired = false;
        QObject::connect(&sm, &SettingsManager::practiceDelayMsChanged, [&]() {
            signalFired = true;
        });

        sm.setPracticeDelayMs(350);
        QCOMPARE(sm.practiceDelayMs(), 350);
        QVERIFY(signalFired);
    }

    void testDisplayEngineAudienceDelay() {
        DisplayOptions opts;
        QCOMPARE(opts.audienceDelayMs, 273);

        DisplayEngine engine(opts);
        QCOMPARE(engine.audienceDelayMs(), 273);

        engine.setAudienceDelayMs(300);
        QCOMPARE(engine.audienceDelayMs(), 300);

        engine.setAudienceDelayMs(273);
        QCOMPARE(engine.audienceDelayMs(), 273);
    }

    void testLyricsEditorNewDeckPreviewReset() {
        LyricsEditorViewModel lyricsVM;
        auto* previewModel = qobject_cast<SlideDeckViewModel*>(lyricsVM.previewDeckModel());
        QVERIFY(previewModel != nullptr);

        // 1. Enter and parse first song lyrics
        lyricsVM.setRawLyricsText("[Verse 1]\nLine 1\n\nLine 2\n\n[Chorus]\nChorus 1");
        lyricsVM.updateDeckFromRawText();

        QCOMPARE(previewModel->slideCount(), 3);
        QCOMPARE(previewModel->slideAt(0)->plainText(), QString("Line 1"));
        QCOMPARE(previewModel->slideAt(1)->plainText(), QString("Line 2"));
        QCOMPARE(previewModel->slideAt(2)->plainText(), QString("Chorus 1"));

        // 2. Create a new deck
        lyricsVM.newDeck();

        // 3. Enter and parse second song lyrics
        lyricsVM.setRawLyricsText("[Verse 1]\nNew Song Line 1");
        lyricsVM.updateDeckFromRawText();

        // 4. Verify preview deck model updated to the new deck's slides and not the old one's
        QCOMPARE(previewModel->slideCount(), 1);
        QCOMPARE(previewModel->slideAt(0)->plainText(), QString("New Song Line 1"));
    }

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

    void testShowDeckMove() {
        Show show("Test Show");
        show.appendDeck("Deck 0");
        show.appendDeck("Deck 1");
        show.appendDeck("Deck 2");
        show.appendDeck("Deck 3");

        ShowViewModel showVM(&show);

        QCOMPARE(showVM.rowCount(), 4);
        showVM.setActiveIndex(0);
        QCOMPARE(showVM.activeIndex(), 0);

        // Move Deck 0 to index 2 (between Deck 2 and Deck 3)
        showVM.moveDeck(0, 2);

        QCOMPARE(show.deckAt(0)->name(), QString("Deck 1"));
        QCOMPARE(show.deckAt(1)->name(), QString("Deck 2"));
        QCOMPARE(show.deckAt(2)->name(), QString("Deck 0"));
        QCOMPARE(show.deckAt(3)->name(), QString("Deck 3"));

        // Active index should follow Deck 0 to index 2
        QCOMPARE(showVM.activeIndex(), 2);

        // Move Deck 3 (last) to index 0 (first)
        showVM.moveDeck(3, 0);
        QCOMPARE(show.deckAt(0)->name(), QString("Deck 3"));
        QCOMPARE(show.deckAt(1)->name(), QString("Deck 1"));
        QCOMPARE(show.deckAt(2)->name(), QString("Deck 2"));
        QCOMPARE(show.deckAt(3)->name(), QString("Deck 0"));

        // Active index was 2 (Deck 0), since moving from 3 to 0 shifted items right, Deck 0 is now index 3
        QCOMPARE(showVM.activeIndex(), 3);
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

    void testDeckSwitchingSlideIndexReset() {
        SlideDeck deck1("Deck 1");
        deck1.addComponent("Verse 1", {QStringList{"D1 Line 1"}, QStringList{"D1 Line 2"}, QStringList{"D1 Line 3"}});
        Arrangement* arr1 = deck1.appendArrangement("Main");
        arr1->setSequence(QStringList{"Verse 1"});
        deck1.setActiveArrangement("Main");

        SlideDeck deck2("Deck 2");
        deck2.addComponent("Verse 1", {QStringList{"D2 Line 1"}, QStringList{"D2 Line 2"}});
        Arrangement* arr2 = deck2.appendArrangement("Main");
        arr2->setSequence(QStringList{"Verse 1"});
        deck2.setActiveArrangement("Main");

        SlideDeckViewModel vm;
        vm.setDeck(&deck1);
        QCOMPARE(vm.slideCount(), 3);
        vm.setSelectedSlideIndex(2);
        QCOMPARE(vm.selectedSlideIndex(), 2);

        int indexObservedDuringRebuilt = -1;
        QObject::connect(&vm, &SlideDeckViewModel::slidesRebuilt, [&]() {
            indexObservedDuringRebuilt = vm.selectedSlideIndex();
        });

        // Switch to deck2
        vm.setDeck(&deck2);
        QCOMPARE(indexObservedDuringRebuilt, 0);
        QCOMPARE(vm.selectedSlideIndex(), 0);
    }

    void testProPresenterRtfUnicodeAndApostrophes() {
        // ProPresenter 7 RTF with \u8217 ? (smart right single quote) and \u8216 ? (smart left single quote)
        QByteArray rtf1 = "{\\rtf0\\ansi\\ansicpg1252\\uc1 That I put my faith in Jesus\\par\\u8216 ?Cause He\\u8217 ?s never let me down}";
        QString res1 = ProPresenterImporter::extractTextFromRtf(rtf1);
        QCOMPARE(res1, QString("That I put my faith in Jesus\n'Cause He's never let me down"));

        // ProPresenter RTF with \u8217 and no space before ?
        QByteArray rtf2 = "{\\rtf0\\ansi\\ansicpg1252\\uc1 When everything around me is shaken\\par I\\u8217 ?ve never been more glad}";
        QString res2 = ProPresenterImporter::extractTextFromRtf(rtf2);
        QCOMPARE(res2, QString("When everything around me is shaken\nI've never been more glad"));

        // Hex escape apostrophe \'92 / \'27
        QByteArray rtf3 = "{\\rtf0\\ansi\\ansicpg1252 He\\'92s faithful}";
        QString res3 = ProPresenterImporter::extractTextFromRtf(rtf3);
        QCOMPARE(res3, QString("He's faithful"));
    }

    void testArrangementComponentRemovalSlideIntegrity() {
        SlideDeck deck("Test Arrangement Deck");
        deck.addComponent("Verse 1", {QStringList{"V1-1"}, QStringList{"V1-2"}, QStringList{"V1-3"}});
        deck.addComponent("Chorus", {QStringList{"C-1"}, QStringList{"C-2"}});
        deck.addComponent("Verse 2", {QStringList{"V2-1"}, QStringList{"V2-2"}, QStringList{"V2-3"}});
        deck.addComponent("Bridge", {QStringList{"B-1"}, QStringList{"B-2"}});
        deck.addComponent("Ending", {QStringList{"E-1"}});

        Arrangement* arr = deck.appendArrangement("Main");
        arr->setSequence(QStringList{"Verse 1", "Chorus", "Verse 2", "Chorus", "Bridge", "Ending"});
        deck.setActiveArrangement("Main");

        SlideDeckViewModel vm;
        vm.setDeck(&deck);
        ArrangementViewModel arrVm(&deck);

        QCOMPARE(vm.slideCount(), 13);
        QCOMPARE(arrVm.rowCount(), 6);

        // Verify initial isFirst flags and texts
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("V1-1"));
        QCOMPARE(vm.data(vm.index(1, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("V2-1"));
        QCOMPARE(vm.data(vm.index(8, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(8, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));
        QCOMPARE(vm.data(vm.index(10, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(10, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("B-1"));
        QCOMPARE(vm.data(vm.index(12, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(12, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("E-1"));

        // 1. Remove middle component "Verse 2" (index 2 in sequence)
        arrVm.removeComponent(2);
        QCOMPARE(arrVm.rowCount(), 5);
        QCOMPARE(vm.slideCount(), 10);

        // Verify Verse 1 (0..2)
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Verse 1"));
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("V1-1"));
        QCOMPARE(vm.data(vm.index(1, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);

        // Verify first Chorus (3..4)
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));

        // Verify second Chorus (5..6) which immediately follows the removed Verse 2
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));

        // Verify Bridge (7..8)
        QCOMPARE(vm.data(vm.index(7, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Bridge"));
        QCOMPARE(vm.data(vm.index(7, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(7, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("B-1"));
        QCOMPARE(vm.data(vm.index(8, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);

        // Verify Ending (9)
        QCOMPARE(vm.data(vm.index(9, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Ending"));
        QCOMPARE(vm.data(vm.index(9, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(9, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("E-1"));

        // 2. Remove first component "Verse 1" (index 0)
        arrVm.removeComponent(0);
        QCOMPARE(arrVm.rowCount(), 4);
        QCOMPARE(vm.slideCount(), 7);

        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));
        QCOMPARE(vm.data(vm.index(1, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Bridge"));
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Ending"));
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);

        // 3. Remove last component "Ending" (index 3)
        arrVm.removeComponent(3);
        QCOMPARE(arrVm.rowCount(), 3);
        QCOMPARE(vm.slideCount(), 6);
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Bridge"));
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("B-2"));
    }

    void testArrangementComponentInsertionAndMoveSlideIntegrity() {
        SlideDeck deck("Test Insertion Deck");
        deck.addComponent("Verse 1", {QStringList{"V1-1"}, QStringList{"V1-2"}});
        deck.addComponent("Chorus", {QStringList{"C-1"}, QStringList{"C-2"}});
        deck.addComponent("Bridge", {QStringList{"B-1"}, QStringList{"B-2"}});

        Arrangement* arr = deck.appendArrangement("Main");
        arr->setSequence(QStringList{"Chorus", "Chorus"});
        deck.setActiveArrangement("Main");

        SlideDeckViewModel vm;
        vm.setDeck(&deck);
        ArrangementViewModel arrVm(&deck);

        QCOMPARE(vm.slideCount(), 4);

        // Insert "Verse 1" at index 1 -> ["Chorus", "Verse 1", "Chorus"]
        arrVm.insertComponent(1, "Verse 1");
        QCOMPARE(arrVm.rowCount(), 3);
        QCOMPARE(vm.slideCount(), 6);

        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Verse 1"));
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("V1-1"));
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::IsFirstRole).toBool(), false);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("V1-2"));
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(4, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-1"));

        // Insert "Bridge" at index 0 -> ["Bridge", "Chorus", "Verse 1", "Chorus"]
        arrVm.insertComponent(0, "Bridge");
        QCOMPARE(arrVm.rowCount(), 4);
        QCOMPARE(vm.slideCount(), 8);
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Bridge"));
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(2, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);

        // Move "Bridge" from index 0 to index 3 -> ["Chorus", "Verse 1", "Chorus", "Bridge"]
        arrVm.moveComponent(0, 3);
        QCOMPARE(arrVm.rowCount(), 4);
        QCOMPARE(vm.slideCount(), 8);
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Chorus"));
        QCOMPARE(vm.data(vm.index(0, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::ComponentNameRole).toString(), QString("Bridge"));
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::IsFirstRole).toBool(), true);
        QCOMPARE(vm.data(vm.index(6, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("B-1"));
        QCOMPARE(vm.data(vm.index(7, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("B-2"));
    }

    void testLiveSlidePreservationDuringArrangementEditing() {
        SlideDeck deck("Live Presentation Deck");
        deck.addComponent("Verse 1", {QStringList{"V1-1"}, QStringList{"V1-2"}});
        deck.addComponent("Chorus", {QStringList{"C-1"}, QStringList{"C-2"}});
        deck.addComponent("Bridge", {QStringList{"B-1"}, QStringList{"B-2"}});

        Arrangement* arr = deck.appendArrangement("Main");
        arr->setSequence(QStringList{"Verse 1", "Chorus", "Bridge"});
        deck.setActiveArrangement("Main");

        SlideDeckViewModel vm;
        vm.setDeck(&deck);
        ArrangementViewModel arrVm(&deck);

        DisplayOptions opts;
        DisplayEngine engine(opts);
        engine.setSlidesContent(vm.toSlideDataList());

        QObject::connect(&vm, &SlideDeckViewModel::slidesUpdated, [&]() {
            engine.updateSlidesContent(vm.toSlideDataList(), vm.selectedSlideIndex());
        });
        QObject::connect(&vm, &SlideDeckViewModel::slidesRebuilt, [&]() {
            engine.updateSlidesContent(vm.toSlideDataList(), vm.selectedSlideIndex());
        });

        // 1. Initially select slide index 3 ("C-2")
        vm.setSelectedSlideIndex(3);
        engine.jumpToSlide(3);

        QCOMPARE(vm.selectedSlideIndex(), 3);
        QCOMPARE(engine.currentSlideNumber(), 4);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));
        QCOMPARE(engine.currentSlideText(), QString("C-2"));

        // 2. Insert "Bridge" before the live slide at index 0 -> sequence becomes ["Bridge", "Verse 1", "Chorus", "Bridge"]
        arrVm.insertComponent(0, "Bridge");
        QCOMPARE(vm.slideCount(), 8);
        QCOMPARE(vm.selectedSlideIndex(), 5); // Shifted by 2
        QCOMPARE(engine.currentSlideNumber(), 6);
        QCOMPARE(vm.data(vm.index(5, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));
        QCOMPARE(engine.currentSlideText(), QString("C-2"));

        // 3. Remove "Verse 1" (index 1) before the live slide -> sequence becomes ["Bridge", "Chorus", "Bridge"]
        arrVm.removeComponent(1);
        QCOMPARE(vm.slideCount(), 6);
        QCOMPARE(vm.selectedSlideIndex(), 3); // Shifted back by 2
        QCOMPARE(engine.currentSlideNumber(), 4);
        QCOMPARE(vm.data(vm.index(3, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));
        QCOMPARE(engine.currentSlideText(), QString("C-2"));

        // 4. Move "Chorus" (index 1) to index 0 -> sequence becomes ["Chorus", "Bridge", "Bridge"]
        arrVm.moveComponent(1, 0);
        QCOMPARE(vm.slideCount(), 6);
        QCOMPARE(vm.selectedSlideIndex(), 1); // "Chorus" is now at index 0, so C-2 is at slide index 1
        QCOMPARE(engine.currentSlideNumber(), 2);
        QCOMPARE(vm.data(vm.index(1, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));
        QCOMPARE(engine.currentSlideText(), QString("C-2"));

        // 5. Remove trailing "Bridge" (index 2) after live slide -> sequence becomes ["Chorus", "Bridge"]
        arrVm.removeComponent(2);
        QCOMPARE(vm.slideCount(), 4);
        QCOMPARE(vm.selectedSlideIndex(), 1);
        QCOMPARE(engine.currentSlideNumber(), 2);
        QCOMPARE(vm.data(vm.index(1, 0), SlideDeckViewModel::SlideTextRole).toString(), QString("C-2"));
        QCOMPARE(engine.currentSlideText(), QString("C-2"));
    }
};

QTEST_MAIN(GUITests)
#include "GUITests.moc"
