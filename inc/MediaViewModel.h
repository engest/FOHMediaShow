#ifndef MEDIAVIEWMODEL_H
#define MEDIAVIEWMODEL_H

#include <QAbstractListModel>
#include <QStringList>
#include <QUrl>
#include <QFileInfo>

class MediaViewModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString mediaTypeFilter READ mediaTypeFilter WRITE setMediaTypeFilter NOTIFY mediaTypeFilterChanged)

public:
    enum MediaRoles {
        FileNameRole = Qt::UserRole + 1,
        FilePathRole,
        FileUrlRole,
        IsVideoRole
    };

    explicit MediaViewModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QString mediaTypeFilter() const;
    void setMediaTypeFilter(const QString& filter);

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void importFiles(const QList<QUrl>& urls);
    Q_INVOKABLE void renameFile(int index, const QString& newName);
    Q_INVOKABLE void deleteFile(int index);

signals:
    void mediaTypeFilterChanged();

private:
    QString m_mediaTypeFilter; // "images" or "videos"
    QList<QFileInfo> m_files;
    
    bool isVideo(const QFileInfo& fileInfo) const;
    bool isImage(const QFileInfo& fileInfo) const;
};

#endif // MEDIAVIEWMODEL_H
