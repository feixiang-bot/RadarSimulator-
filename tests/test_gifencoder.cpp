// tests/test_gifencoder.cpp
#include <QtTest>
#include <QImage>
#include <QImageReader>
#include <QTemporaryDir>
#include "tools/gifencoder.h"

class TestGifEncoder : public QObject {
    Q_OBJECT
private slots:
    void roundTrip();
    void emptyFails();
};

void TestGifEncoder::roundTrip() {
    QImage r(64, 64, QImage::Format_RGB32); r.fill(QColor(255, 0, 0));
    QImage g(64, 64, QImage::Format_RGB32); g.fill(QColor(0, 255, 0));
    QImage b(64, 64, QImage::Format_RGB32); b.fill(QColor(0, 0, 255));

    GifEncoder enc;
    enc.addFrame(r, 10);
    enc.addFrame(g, 20);
    enc.addFrame(b, 30);

    QTemporaryDir dir;
    const QString path = dir.filePath(QStringLiteral("out.gif"));
    QString err;
    QVERIFY2(enc.save(path, &err), qPrintable(err));
    QVERIFY(QFile::exists(path));

    QImageReader reader(path);
    QVERIFY2(reader.canRead(), "qgif 插件不可用或 GIF 无法解码");
    QCOMPARE(reader.imageCount(), 3);
    QCOMPARE(reader.size(), QSize(64, 64));

    int n = 0;
    while (reader.canRead()) {
        const QImage img = reader.read();
        QVERIFY(!img.isNull());
        QCOMPARE(img.size(), QSize(64, 64));
        ++n;
    }
    QCOMPARE(n, 3);
}

void TestGifEncoder::emptyFails() {
    GifEncoder enc;
    QString err;
    QVERIFY(!enc.save(QStringLiteral("none.gif"), &err));
    QVERIFY(!err.isEmpty());
}

QTEST_GUILESS_MAIN(TestGifEncoder)
#include "test_gifencoder.moc"