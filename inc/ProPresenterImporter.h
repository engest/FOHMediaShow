#pragma once

#include <QString>
#include "SlideDeck.h"

class ProPresenterImporter {
public:
    static SlideDeck* importProFile(const QString& filePath);
};
