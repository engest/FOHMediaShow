#ifndef LIBRARY_H
#define LIBRARY_H

#include <QString>
#include <QStringList>

namespace Library {
    QString rootDir();
    QString showsDir();
    QString slidedecksDir();
    QString layoutsDir();
    QString mediaDir();

    QStringList listShowFiles();
    QStringList listSlidedeckFiles();

    QString getShowTitle(const QString& filename);
    QString getDeckTitle(const QString& filename);

    QList<QPair<QString, QString>> listShowsWithTitles();
    QList<QPair<QString, QString>> listDecksWithTitles();

    QString importShowFile(const QString& srcPath);
    QString importSlidedeckFile(const QString& srcPath);

    QStringList listLayoutFiles();
    QVariantMap getLayoutProperties(const QString& layoutName);

    void ensureDirectories();

    QString uniqueDestPath(const QString& dir, const QString& filename);
}

#endif // LIBRARY_H
