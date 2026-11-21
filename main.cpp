#include <QApplication>
#include <QWidget>
#include <QTableView>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <cmath>

#include <QGroupBox>
#include <QFormLayout>
#include <QComboBox>
#include <QSlider>
#include <QLabel>

#include <QMenuBar>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QMainWindow>

#include "ui/radarwidget.h"
#include "ui/TargetTableModel.h"
#include "core/targetmanager.h"
#include "core/ScenarioIO.h"
#include "core/udptracksource.h"
#include "core/udptracksender.h"
#include <QTimer>
#include <QHash>
#include <QHostAddress>
#include <QStatusBar>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QCheckBox>

// C5：CSV 字段转义——含逗号/双引号/换行时用双引号包裹并转义内部双引号
static QString csvEscape(const QString& s) {
    QString out = s;
    const bool needQuote = out.contains(',') || out.contains('"')
                        || out.contains('\n') || out.contains('\r');
    out.replace('"', "\"\"");
    if (needQuote) out = '"' + out + '"';
    return out;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    TargetManager mgr;
    mgr.setMaxRangeKm(10.0);

    // 初始一个目标
    mgr.addTargetAt(mgr.maxRangeKm() * 0.6, 45.0);

    QMainWindow root;

    RadarWidget* radar = new RadarWidget(&root);
    QTableView*  table = new QTableView(&root);

    TargetTableModel* model = new TargetTableModel(&mgr, table);
    table->setModel(model);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->horizontalHeader()->setStretchLastSection(true);
    table->setColumnWidth(0, 90);
    table->setColumnWidth(1, 90);
    table->setColumnWidth(2, 80);
    table->setColumnWidth(3, 100);
    table->setColumnWidth(4, 80);

    QPushButton* btnAdd = new QPushButton("添加目标");
    QPushButton* btnDel = new QPushButton("删除选中");

    QObject::connect(btnAdd, &QPushButton::clicked, [&]() {
        const double maxR = mgr.maxRangeKm();
        const double r    = maxR * 0.6;                 // 放在 60% 量程处
        const int    n    = mgr.targetCount();
        const double az   = std::fmod(45.0 + n * 30.0, 360.0);
        const int newId = mgr.addTargetAt(r, az);
        if (newId < 0) {
            // 理论上到不了这里，因为 r = maxR * 0.6 永远合法
            // 但保留这个分支，将来改逻辑时不会静默失败
            qWarning("addTargetAt failed");
        }
    });

    QObject::connect(btnDel, &QPushButton::clicked, [&]() {
        const QModelIndexList sel = table->selectionModel()->selectedRows();
        if (sel.isEmpty()) return;
        const int row = sel.first().row();
        if (row < 0 || row >= mgr.targetCount()) return;
        mgr.removeTarget(mgr.targetAt(row).id);
    });

    // 右侧：表格 + 按钮行 + 参数面板
    QWidget* rightPanel = new QWidget(&root);
    QVBoxLayout* rightLay = new QVBoxLayout(rightPanel);
    rightLay->addWidget(table, 1);
    QGroupBox* paramBox = new QGroupBox("参数");
    QFormLayout* form = new QFormLayout(paramBox);

    // 量程
    QComboBox* cmbRange = new QComboBox;
    cmbRange->addItem("2 km",  2.0);
    cmbRange->addItem("5 km",  5.0);
    cmbRange->addItem("10 km", 10.0);
    cmbRange->addItem("20 km", 20.0);
    cmbRange->setCurrentIndex(2); // 10km
    form->addRow("量程", cmbRange);

    // 扫描速度（秒/圈）
    QSlider* sldSweep = new QSlider(Qt::Horizontal);
    sldSweep->setRange(2, 30);     // 2~30 秒/圈
    sldSweep->setValue(12);        // 12 秒/圈
    QLabel* lblSweep = new QLabel("12 s/圈");
    form->addRow("扫描速度", sldSweep);
    form->addRow("", lblSweep);

    // 余辉衰减（alpha，越小拖尾越长）
    QSlider* sldDecay = new QSlider(Qt::Horizontal);
    sldDecay->setRange(200, 252);  // 200~252
    sldDecay->setValue(240);
    QLabel* lblDecay = new QLabel("240（越大拖尾越短）");
    form->addRow("余辉", sldDecay);
    form->addRow("", lblDecay);

    // 航迹长度（每目标历史点数上限）
    QSlider* sldTrail = new QSlider(Qt::Horizontal);
    sldTrail->setRange(10, 500);
    sldTrail->setValue(100);
    QLabel* lblTrail = new QLabel("100 点");
    form->addRow("航迹长度", sldTrail);
    form->addRow("", lblTrail);

    // --- 连接 ---

    // 量程
    QObject::connect(cmbRange, QOverload<int>::of(&QComboBox::currentIndexChanged),
                     [&](int idx) {
                         const double km = cmbRange->itemData(idx).toDouble();
                         mgr.setMaxRangeKm(km);
                     });

    // 扫描速度：秒/圈 → °/s
    QObject::connect(sldSweep, &QSlider::valueChanged, [&](int secPerRound) {
        lblSweep->setText(QString("%1 s/圈").arg(secPerRound));
        const double degPerSec = 360.0 / secPerRound;
        radar->setSweepSpeedDegPerSec(degPerSec);
    });

    // 余辉
    QObject::connect(sldDecay, &QSlider::valueChanged, [&](int a) {
        lblDecay->setText(QString("%1（越大拖尾越长）").arg(a));
        radar->setDecayAlpha(a);
    });

    // 航迹长度
    QObject::connect(sldTrail, &QSlider::valueChanged, [&](int n) {
        lblTrail->setText(QString("%1 点").arg(n));
        mgr.setTrailMaxPoints(n);
    });

    QHBoxLayout* btnLay = new QHBoxLayout();
    btnLay->addWidget(btnAdd);
    btnLay->addWidget(btnDel);
    rightLay->addLayout(btnLay);
    rightLay->addWidget(paramBox);

    // 整体：雷达 | 右侧面板
    // 雷达和右侧面板放进一个 central widget
    QWidget* central = new QWidget(&root);
    QHBoxLayout* mainLay = new QHBoxLayout(central);
    mainLay->addWidget(radar, 1);
    mainLay->addWidget(rightPanel, 0);
    root.setCentralWidget(central);

    radar->setTargetManager(&mgr);

    // ===== D1：状态栏（目标数 / 扫描方位 / 鼠标极坐标） =====
    QLabel* lblTargetCount = new QLabel(&root);
    QLabel* lblSweepAz     = new QLabel(&root);
    QLabel* lblCursorPolar = new QLabel(&root);
    root.statusBar()->addWidget(lblTargetCount);
    root.statusBar()->addWidget(lblSweepAz);
    root.statusBar()->addWidget(lblCursorPolar);

    auto fixTargetCount = [&]() {
        lblTargetCount->setText(QString("目标：%1").arg(mgr.targetCount()));
    };
    fixTargetCount();
    QObject::connect(&mgr, &TargetManager::targetListChanged, &root,
                     [&]() { fixTargetCount(); });
    QObject::connect(radar, &RadarWidget::sweepAngleChanged,
                     [lblSweepAz](double deg) {
                         lblSweepAz->setText(QString("方位：%1°").arg(deg, 0, 'f', 1));
                     });
    QObject::connect(radar, &RadarWidget::cursorPolarChanged,
                     [lblCursorPolar](double rangeKm, double azDeg) {
                         lblCursorPolar->setText(
                             QString("鼠标：%1 km / %2°")
                                 .arg(rangeKm, 0, 'f', 2)
                                 .arg(azDeg, 0, 'f', 1));
                     });

    // ===== D2：右键菜单（删除 / 属性编辑） =====
    QObject::connect(radar, &RadarWidget::targetRemoveRequested,
                     [&](int id) { mgr.removeTarget(id); });
    QObject::connect(radar, &RadarWidget::targetEditRequested,
                     [&](int id) {
                         const Target* t = mgr.findById(id);
                         if (!t) return;

                         QDialog dlg(&root);
                         dlg.setWindowTitle("目标属性");
                         QFormLayout* form = new QFormLayout(&dlg);

                         QLineEdit* edName = new QLineEdit(t->name, &dlg);
                         QDoubleSpinBox* sbRange = new QDoubleSpinBox(&dlg);
                         sbRange->setRange(0.0, mgr.maxRangeKm());
                         sbRange->setDecimals(2);
                         sbRange->setValue(t->rangeKm());
                         QDoubleSpinBox* sbAz = new QDoubleSpinBox(&dlg);
                         sbAz->setRange(0.0, 360.0);
                         sbAz->setDecimals(1);
                         sbAz->setWrapping(true);
                         sbAz->setValue(t->azimuthDeg());
                         QDoubleSpinBox* sbSpeed = new QDoubleSpinBox(&dlg);
                         sbSpeed->setRange(0.0, 100000.0);
                         sbSpeed->setDecimals(1);
                         sbSpeed->setValue(t->speedKmh);
                         QDoubleSpinBox* sbHeading = new QDoubleSpinBox(&dlg);
                         sbHeading->setRange(0.0, 360.0);
                         sbHeading->setDecimals(1);
                         sbHeading->setWrapping(true);
                         sbHeading->setValue(t->headingDeg);
                         QCheckBox* cbAuto = new QCheckBox("自动运动", &dlg);
                         cbAuto->setChecked(t->autoMove);

                         form->addRow("名称", edName);
                         form->addRow("距离 (km)", sbRange);
                         form->addRow("方位 (°)", sbAz);
                         form->addRow("速度 (km/h)", sbSpeed);
                         form->addRow("航向 (°)", sbHeading);
                         form->addRow("", cbAuto);

                         QDialogButtonBox* bb = new QDialogButtonBox(
                             QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
                         QObject::connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
                         QObject::connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
                         form->addRow(bb);

                         if (dlg.exec() != QDialog::Accepted) return;

                         QString nameErr;
                         if (!mgr.renameTarget(id, edName->text(), &nameErr)) {
                             QMessageBox::warning(&root, "名称无效", nameErr);
                             return;
                         }
                         mgr.setTargetPolar(id, sbRange->value(), sbAz->value());
                         mgr.setTargetMotion(id, sbSpeed->value(), sbHeading->value());
                         mgr.setTargetAutoMove(id, cbAuto->isChecked());
                     });

    // ===== C4：UDP 航迹数据源 + 回放演示 =====
    const quint16 kUdpPort = 45454;
    UdpTrackSource udpSource(&root);
    UdpTrackSender udpSender(&root);
    udpSender.setDestination(QHostAddress::LocalHost, kUdpPort);

    QString udpErr;
    if (!udpSource.listen(kUdpPort, &udpErr)) {
        QMessageBox::warning(&root, "UDP 监听失败", udpErr);
    }

    // 外部航迹 id → 本地目标 id 映射（目标被删 / 出界后失效并重建）
    QHash<int, int> extToLocal;
    QObject::connect(&udpSource, &UdpTrackSource::tracksReceived,
        [&](const QVector<UdpTrack>& tracks) {
            for (const UdpTrack& tr : tracks) {
                int localId = extToLocal.value(tr.id, -1);
                if (localId < 0 || !mgr.findById(localId)) {
                    localId = mgr.addTargetAt(
                        tr.rangeKm, tr.azimuthDeg,
                        tr.name.isEmpty() ? QString("EXT-%1").arg(tr.id) : tr.name);
                    if (localId < 0) continue;
                    extToLocal.insert(tr.id, localId);
                }
                mgr.setTargetPolar(localId, tr.rangeKm, tr.azimuthDeg);
                mgr.setTargetMotion(localId, tr.speedKmh, tr.headingDeg);
                mgr.setTargetAutoMove(localId, tr.autoMove);
            }
        });

    // 回放演示：一键广播一个圆周运动目标，验证接收端上屏
    QPushButton* btnReplay = new QPushButton("回放演示");
    QTimer* replayTimer = new QTimer(&root);
    replayTimer->setInterval(100); // 10Hz
    double replayAngle = 0.0;
    QObject::connect(btnReplay, &QPushButton::clicked, [replayTimer, btnReplay]() {
        if (replayTimer->isActive()) {
            replayTimer->stop();
            btnReplay->setText("回放演示");
        } else {
            replayTimer->start();
            btnReplay->setText("停止回放");
        }
    });
    QObject::connect(replayTimer, &QTimer::timeout, [&]() {
        UdpTrack tr;
        tr.id         = 9001;
        tr.name       = "REPLAY";
        tr.rangeKm    = 6.0;
        tr.azimuthDeg = replayAngle;
        tr.speedKmh   = 0.0;
        tr.headingDeg = 0.0;
        tr.autoMove   = false;
        replayAngle   = std::fmod(replayAngle + 3.0, 360.0);
        QString e;
        udpSender.send({ tr }, &e);   // 发送失败忽略（仅演示）
    });
    btnLay->addWidget(btnReplay);

    //添加JSON保存/加载
    QMenuBar* menuBar = root.menuBar();
    QMenu* fileMenu = menuBar->addMenu("文件");

    QAction* actSave = fileMenu->addAction("保存场景...");
    QAction* actLoad = fileMenu->addAction("加载场景...");
    QAction* actExportCsv = fileMenu->addAction("导出 CSV...");
    QAction* actExportPng = fileMenu->addAction("导出截图 (PNG)...");

    QObject::connect(actSave, &QAction::triggered, [&]() {
        QString path = QFileDialog::getSaveFileName(
            &root, "保存场景", QString(), "JSON 文件 (*.json)");
        if (path.isEmpty()) return;
        // D5：用户未输入后缀时自动补 .json
        if (!path.endsWith(QLatin1String(".json"), Qt::CaseInsensitive))
            path += QLatin1String(".json");

        QString err;
        if (!ScenarioIO::save(path, mgr, &err)) {
            QMessageBox::warning(&root, "保存失败", err);
        }
    });

    QObject::connect(actLoad, &QAction::triggered, [&]() {
        const QString path = QFileDialog::getOpenFileName(
            &root, "加载场景", QString(), "JSON 文件 (*.json)");
        if (path.isEmpty()) return;

        QString err;
        if (!ScenarioIO::load(path, mgr, &err)) {
            QMessageBox::warning(&root, "加载失败", err);
        }
    });

    // ===== C5：导出 CSV / 截图 =====
    QObject::connect(actExportCsv, &QAction::triggered, [&]() {
        QString path = QFileDialog::getSaveFileName(
            &root, "导出 CSV", QString(), "CSV 文件 (*.csv)");
        if (path.isEmpty()) return;
        if (!path.endsWith(QLatin1String(".csv"), Qt::CaseInsensitive))
            path += QLatin1String(".csv");

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(&root, "导出失败", f.errorString());
            return;
        }
        QByteArray data;
        data.append("名称,距离(km),方位(°),速度(km/h),航向(°),自动\r\n");
        for (int i = 0; i < mgr.targetCount(); ++i) {
            const Target& t = mgr.targetAt(i);
            data += csvEscape(t.name).toUtf8();
            data += ',';
            data += QString::number(t.rangeKm(), 'f', 2).toUtf8();
            data += ',';
            data += QString::number(t.azimuthDeg(), 'f', 1).toUtf8();
            data += ',';
            data += QString::number(t.speedKmh, 'f', 1).toUtf8();
            data += ',';
            data += QString::number(t.headingDeg, 'f', 1).toUtf8();
            data += ',';
            data += t.autoMove ? "true" : "false";
            data += "\r\n";
        }
        f.write(data);
    });

    QObject::connect(actExportPng, &QAction::triggered, [&]() {
        QString path = QFileDialog::getSaveFileName(
            &root, "导出截图", QString(), "PNG 图片 (*.png)");
        if (path.isEmpty()) return;
        if (!path.endsWith(QLatin1String(".png"), Qt::CaseInsensitive))
            path += QLatin1String(".png");

        if (!radar->grab().save(path, "PNG"))
            QMessageBox::warning(&root, "导出失败", "截图保存失败");
    });

    root.resize(1200, 720);
    root.setWindowTitle("Radar Sim - Stage 9");
    root.show();

    return app.exec();
}