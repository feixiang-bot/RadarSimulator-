// tools/demo_capture.cpp
// A3：演示素材生成器——真实显示雷达窗口运行数秒，定时抓帧，
// 输出一张动画 GIF + 若干 PNG 截图到 assets/，供 README 内嵌。
#include <QApplication>
#include <QDir>
#include <QImage>
#include <QTimer>
#include "core/targetmanager.h"
#include "tools/gifencoder.h"
#include "ui/radarwidget.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    const QString outDir = argc >= 2 ? QString::fromLocal8Bit(argv[1])
                                     : QStringLiteral("assets");
    QDir().mkpath(outDir);

    TargetManager mgr;
    mgr.setMaxRangeKm(10.0);

    // 场景：1 个静止 + 3 个自动运动目标，制造扫描余辉 + 航迹尾迹
    mgr.addTargetAt(6.0, 45.0, "TGT-A");
    const int a = mgr.addTargetAt(5.0, 120.0, "M-01");
    mgr.setTargetMotion(a, 800.0, 200.0);
    mgr.setTargetAutoMove(a, true);
    const int b = mgr.addTargetAt(7.0, 210.0, "M-02");
    mgr.setTargetMotion(b, 650.0, 90.0);
    mgr.setTargetAutoMove(b, true);
    const int c = mgr.addTargetAt(4.0, 300.0, "M-03");
    mgr.setTargetMotion(c, 900.0, 330.0);
    mgr.setTargetAutoMove(c, true);

    RadarWidget radar;
    radar.setTargetManager(&mgr);
    radar.setSweepSpeedDegPerSec(360.0 / 6.0); // 6 秒/圈
    radar.resize(600, 600);
    radar.show();

    // 抓帧（先让余辉/航迹积累约 0.5s 再开始）
    QVector<QImage> frames;
    QTimer* grabTimer = new QTimer(&radar);
    int tick = 0;
    const int kTotalFrames = 24;

    QObject::connect(grabTimer, &QTimer::timeout, [&]() {
        frames.append(radar.grab().toImage());
        if (++tick >= kTotalFrames) {
            grabTimer->stop();
            app.quit();
        }
    });

    QTimer::singleShot(500, [&]() { grabTimer->start(80); }); // 80ms/帧
    QTimer::singleShot(5000, [&]() { app.quit(); });          // 保险超时
    app.exec();

    if (frames.isEmpty()) {
        qWarning("未抓到任何帧");
        return 1;
    }

    // 合成 GIF
    GifEncoder enc;
    for (const QImage& f : frames)
        enc.addFrame(f, 8); // 0.08s/帧
    QString err;
    const QString gifPath = outDir + "/radar_demo.gif";
    if (!enc.save(gifPath, &err)) {
        qWarning("GIF 保存失败: %s", qPrintable(err));
        return 2;
    }

    // 保存 3 张代表性 PNG（约 1/3、2/3、末尾）
    const int n = frames.size();
    const int picks[3] = { n / 3, n * 2 / 3, n - 1 };
    for (int i = 0; i < 3; ++i) {
        const QString png = outDir + QString("/radar_%1.png").arg(i + 1);
        frames[picks[i]].save(png, "PNG");
    }

    qInfo("已生成: %s（%d 帧）", qPrintable(gifPath), n);
    return 0;
}