// tools/gifencoder.h
#pragma once
#include <QImage>
#include <QString>
#include <QVector>

// A3：极简 GIF89a 动画编码器（纯逻辑，LSB-first LZW）。
// 用于演示素材生成——把若干帧图像合成一张动画 GIF。
// 不依赖 GUI 事件循环 / QWidget，仅需 Qt::Gui（QImage）。
class GifEncoder {
public:
    struct Frame {
        QImage image;        // 任意格式，保存时内部转 Indexed8
        int     delayCs = 10; // 帧延迟，单位 1/100 秒
    };

    GifEncoder() = default;

    void addFrame(const QImage& image, int delayCs = 10);

    // 写出 GIF 文件；失败返回 false 并填 err。
    bool save(const QString& path, QString* err = nullptr) const;

private:
    QVector<Frame> m_frames;
    int m_width = 0;
    int m_height = 0;
};