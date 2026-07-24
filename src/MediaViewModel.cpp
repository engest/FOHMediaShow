#include "../inc/MediaViewModel.h"
#include "../inc/Library.h"
#include <QDir>
#include <QFile>

MediaViewModel::MediaViewModel(QObject* parent) : QAbstractListModel(parent) {
    m_mediaTypeFilter = "images"; // default
    refresh();
}

int MediaViewModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(m_files.size());
}

QVariant MediaViewModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size()) {
        return QVariant();
    }

    const QFileInfo& info = m_files[index.row()];

    switch (role) {
        case FileNameRole: return info.fileName();
        case FilePathRole: return info.absoluteFilePath();
        case FileUrlRole: return QUrl::fromLocalFile(info.absoluteFilePath());
        case IsVideoRole: return isVideo(info);
        default: return QVariant();
    }
}

QHash<int, QByteArray> MediaViewModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[FileNameRole] = "fileName";
    roles[FilePathRole] = "filePath";
    roles[FileUrlRole] = "fileUrl";
    roles[IsVideoRole] = "isVideo";
    return roles;
}

QString MediaViewModel::mediaTypeFilter() const {
    return m_mediaTypeFilter;
}

void MediaViewModel::setMediaTypeFilter(const QString& filter) {
    if (m_mediaTypeFilter != filter) {
        m_mediaTypeFilter = filter;
        emit mediaTypeFilterChanged();
        refresh();
    }
}

void MediaViewModel::refresh() {
    beginResetModel();
    m_files.clear();

    QDir mediaDir(Library::mediaDir());
    QFileInfoList allFiles = mediaDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);

    for (const QFileInfo& info : allFiles) {
        if (m_mediaTypeFilter == "images" && isImage(info)) {
            m_files.append(info);
        } else if (m_mediaTypeFilter == "videos" && isVideo(info)) {
            m_files.append(info);
        } else if (m_mediaTypeFilter == "all" && (isImage(info) || isVideo(info))) {
            m_files.append(info);
        }
    }

    endResetModel();
}

void MediaViewModel::importFiles(const QList<QUrl>& urls) {
    bool changed = false;
    QDir destDir(Library::mediaDir());

    for (const QUrl& url : urls) {
        if (!url.isLocalFile()) continue;
        
        QFileInfo srcInfo(url.toLocalFile());
        if (!srcInfo.exists() || !srcInfo.isFile()) continue;

        QString destPath = Library::uniqueDestPath(destDir.absolutePath(), srcInfo.fileName());
        if (QFile::copy(srcInfo.absoluteFilePath(), destPath)) {
            changed = true;
        }
    }

    if (changed) {
        refresh();
    }
}

void MediaViewModel::renameFile(int index, const QString& newName) {
    if (index < 0 || index >= m_files.size()) return;
    
    QFileInfo info = m_files[index];
    QDir dir = info.absoluteDir();
    QString newPath = dir.filePath(newName);
    
    // Check if new name doesn't have extension, append old one
    if (QFileInfo(newPath).suffix().isEmpty()) {
        newPath += "." + info.suffix();
    }

    if (QFile::exists(newPath)) return; // Prevent overwriting

    if (QFile::rename(info.absoluteFilePath(), newPath)) {
        refresh();
    }
}

void MediaViewModel::deleteFile(int index) {
    if (index < 0 || index >= m_files.size()) return;

    QFileInfo info = m_files[index];
    if (QFile::remove(info.absoluteFilePath())) {
        refresh();
    }
}

bool MediaViewModel::isVideo(const QFileInfo& fileInfo) const {
    QString ext = fileInfo.suffix().toLower();
    return ext == "mp4" || ext == "mov" || ext == "avi" || ext == "mkv" || ext == "webm";
}

bool MediaViewModel::isImage(const QFileInfo& fileInfo) const {
    QString ext = fileInfo.suffix().toLower();
    return ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "gif" || ext == "bmp" || ext == "webp";
}
