#pragma once

#include <QString>
#include <QList>

class Show;
class SlideDeck;
class QObject;

class ShowSerializer {
public:
    /// Load a .fohs manifest file and return the constructed Show object.
    /// Caller takes ownership of the returned Show (or it belongs to parentObj).
    static Show* loadFohsFile(const QString& filePath, QObject* parentObj = nullptr);

    /// Load a .fohd file (JSON) and append the parsed SlideDeck to parentShow.
    static void loadFohdFile(const QString& filePath, Show* parentShow, const QString& preferredArrangement = QString());

    /// Load a .fohd file (JSON) into a standalone SlideDeck object.
    static SlideDeck* loadFohdFile(const QString& filePath, QObject* parentObj = nullptr);

    /// Save the Show object and all its decks to their respective files.
    static void saveShowToFile(Show* show);

    /// Save a single SlideDeck to a .fohd JSON file.
    static void saveDeckToFile(SlideDeck* deck);

    /// Load a SlideLayout from a .fohl file
    static class SlideLayout loadLayoutFile(const QString& filePath);

    /// Save a SlideLayout to a .fohl file
    static void saveLayoutToFile(const class SlideLayout& layout, const QString& filePath);
};
