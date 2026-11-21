// tools/gifencoder.cpp
#include "tools/gifencoder.h"

#include <QColor>
#include <QFile>
#include <QHash>

namespace {

void appendU16(QByteArray& b, quint16 v) {
    b.append(char(v & 0xFF));
    b.append(char((v >> 8) & 0xFF));
}

// GIF LZW 编码（LSB-first 位打包）。
// 输入为 8bit 像素索引，minCodeSize 固定 8。
QByteArray lzwEncode(const QVector<quint8>& indices, int minCodeSize) {
    const int clearCode = 1 << minCodeSize;
    const int endCode   = clearCode + 1;
    int codeSize = minCodeSize + 1;
    int nextCode = endCode + 1;

    QByteArray out;
    quint32 bitBuf = 0;
    int bitCount = 0;

    auto emitCode = [&](int code) {
        bitBuf |= quint32(code) << bitCount;
        bitCount += codeSize;
        while (bitCount >= 8) {
            out.append(char(bitBuf & 0xFF));
            bitBuf >>= 8;
            bitCount -= 8;
        }
    };

    QHash<int, int> dict;
    dict.reserve(4096);
    auto reset = [&]() {
        dict.clear();
        nextCode = endCode + 1;
        codeSize = minCodeSize + 1;
    };

    emitCode(clearCode);

    int prefix = -1;
    for (quint8 k8 : indices) {
        const int k = int(k8);
        if (prefix < 0) { prefix = k; continue; }
        const int key = (prefix << 8) | k;
        const auto it = dict.constFind(key);
        if (it != dict.constEnd()) {
            prefix = it.value();
        } else {
            emitCode(prefix);
            dict.insert(key, nextCode++);
            if (nextCode == (1 << codeSize) && codeSize < 12) {
                codeSize++;
            } else if (nextCode >= (1 << 12)) {
                emitCode(clearCode);
                reset();
            }
            prefix = k;
        }
    }
    if (prefix >= 0) emitCode(prefix);
    emitCode(endCode);

    if (bitCount > 0) out.append(char(bitBuf & 0xFF));
    return out;
}

} // namespace

void GifEncoder::addFrame(const QImage& image, int delayCs) {
    if (image.isNull()) return;
    if (m_frames.isEmpty()) {
        m_width  = image.width();
        m_height = image.height();
    }
    Frame f;
    f.image   = image;
    f.delayCs = delayCs;
    m_frames.append(f);
}

bool GifEncoder::save(const QString& path, QString* err) const {
    if (m_frames.isEmpty()) {
        if (err) *err = QStringLiteral("无帧可写");
        return false;
    }

    QByteArray gif;
    gif.append("GIF89a", 6);

    // Logical Screen Descriptor
    appendU16(gif, quint16(m_width));
    appendU16(gif, quint16(m_height));
    gif.append(char(0x70)); // 无全局色表；color resolution = 7（8bit）
    gif.append(char(0));    // background color index
    gif.append(char(0));    // pixel aspect ratio

    // Netscape 循环扩展（无限循环）
    gif.append(char(0x21));
    gif.append(char(0xFF));
    gif.append(char(0x0B));
    gif.append("NETSCAPE2.0", 11);
    gif.append(char(0x03));
    gif.append(char(0x01));
    appendU16(gif, 0);      // loop count = 0 → 无限
    gif.append(char(0x00));

    for (const Frame& fr : m_frames) {
        const QImage img =
            fr.image.convertToFormat(QImage::Format_Indexed8, Qt::ThresholdDither);
        if (img.isNull()) continue;

        // 调色板补足到 256 项
        QVector<QRgb> colorTable(256);
        const QVector<QRgb> src = img.colorTable();
        for (int i = 0; i < src.size() && i < 256; ++i) colorTable[i] = src[i];

        // Graphic Control Extension（disposal = 0）
        gif.append(char(0x21));
        gif.append(char(0xF9));
        gif.append(char(0x04));
        gif.append(char(0x00));
        appendU16(gif, quint16(fr.delayCs));
        gif.append(char(0x00)); // transparent index
        gif.append(char(0x00)); // terminator

        // Image Descriptor（带局部色表）
        gif.append(char(0x2C));
        appendU16(gif, 0);      // left
        appendU16(gif, 0);      // top
        appendU16(gif, quint16(img.width()));
        appendU16(gif, quint16(img.height()));
        gif.append(char(0x80 | 0x07)); // 局部色表 + 256 色（N=7）

        // Local Color Table
        for (int i = 0; i < 256; ++i) {
            const QRgb c = colorTable[i];
            gif.append(char(qRed(c)));
            gif.append(char(qGreen(c)));
            gif.append(char(qBlue(c)));
        }

        // 像素索引（逐行 8bit）
        QVector<quint8> idx;
        idx.reserve(img.width() * img.height());
        for (int y = 0; y < img.height(); ++y) {
            const uchar* line = img.constScanLine(y);
            for (int x = 0; x < img.width(); ++x)
                idx.append(line[x]);
        }

        // LZW 编码 + 分块
        const int minCodeSize = 8;
        const QByteArray lzw = lzwEncode(idx, minCodeSize);
        gif.append(char(minCodeSize));
        int pos = 0;
        while (pos < lzw.size()) {
            const int chunk = qMin(255, int(lzw.size() - pos));
            gif.append(char(chunk));
            gif.append(lzw.constData() + pos, chunk);
            pos += chunk;
        }
        gif.append(char(0x00)); // 图像数据终止
    }

    gif.append(char(0x3B)); // trailer

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) {
        if (err) *err = f.errorString();
        return false;
    }
    if (f.write(gif) != gif.size()) {
        if (err) *err = QStringLiteral("写入不完整");
        return false;
    }
    return true;
}