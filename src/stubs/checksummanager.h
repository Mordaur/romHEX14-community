#pragma once
#include <QObject>
#include <QString>
#include <QByteArray>
struct ChecksumDllInfo {
    QString name, path, description;
    int devNum = 0;
    bool valid = false;
};
enum class ChecksumResult { OK, Mismatch, Unsupported, Error };
class ChecksumManager : public QObject {
    Q_OBJECT
public:
    explicit ChecksumManager(QObject *p = nullptr) : QObject(p) {}
    static ChecksumManager *instance() { static ChecksumManager m; return &m; }
    // Community build ships no checksum algorithms — auto-detect finds nothing
    // (devNum 0), so the export-time checksum guard is a clean no-op here.
    ChecksumDllInfo autoDetect(const QByteArray &, const QString &) const { return {}; }
    ChecksumResult verify(const QByteArray &, const ChecksumDllInfo &, QString &) { return ChecksumResult::Unsupported; }
    ChecksumResult correct(QByteArray &, const ChecksumDllInfo &, QString &) { return ChecksumResult::Unsupported; }
signals:
    void progress(int);
};
