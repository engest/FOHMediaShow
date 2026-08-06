#pragma once

#include <QString>
#include "SlideDeck.h"

class ProPresenterImporter {
public:
    static SlideDeck* importProFile(const QString& filePath);
    static QString extractTextFromRtf(const QByteArray& rtf);
};
