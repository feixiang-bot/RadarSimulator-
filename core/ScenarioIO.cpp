// core/ScenarioIO.cpp
#include "core/ScenarioIO.h"
#include "core/targetmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

namespace {

constexpr int kCurrentVersion = 1;

QJsonObject targetToJson(const Target& t) {
    QJsonObject o;
    o["name"]       = t.name;
    o["rangeKm"]    = t.rangeKm();
    o["azimuthDeg"] = t.azimuthDeg();
    o["speedKmh"]   = t.speedKmh;
    o["headingDeg"] = t.headingDeg;
    o["autoMove"]   = t.autoMove;
    return o;
}

} // namespace

bool ScenarioIO::save(const QString& filePath, const TargetManager& mgr, QString* errorMsg) {
    QJsonObject root;
    root["version"]    = kCurrentVersion;
    root["maxRangeKm"] = mgr.maxRangeKm();

    QJsonArray arr;
    for (int i = 0; i < mgr.targetCount(); ++i) {
        arr.append(targetToJson(mgr.targetAt(i)));
    }
    root["targets"] = arr;

    QFile f(filePath);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMsg) *errorMsg = QString("无法打开文件写入：%1").arg(f.errorString());
        return false;
    }
    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (f.write(data) != data.size()) {
        if (errorMsg) *errorMsg = QString("写入不完整：%1").arg(f.errorString());
        return false;
    }
    return true;
}

bool ScenarioIO::load(const QString& filePath, TargetManager& mgr, QString* errorMsg) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (errorMsg) *errorMsg = QString("无法打开文件读取：%1").arg(f.errorString());
        return false;
    }
    const QByteArray data = f.readAll();

    QJsonParseError parseErr;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError) {
        if (errorMsg) *errorMsg = QString("JSON 解析失败：%1").arg(parseErr.errorString());
        return false;
    }
    if (!doc.isObject()) {
        if (errorMsg) *errorMsg = QString("JSON 根节点不是对象");
        return false;
    }
    const QJsonObject root = doc.object();

    // 版本检查
    const int version = root.value("version").toInt(0);
    if (version != kCurrentVersion) {
        if (errorMsg) *errorMsg = QString("不支持的文件版本：%1").arg(version);
        return false;
    }

    // 量程
    const double maxRange = root.value("maxRangeKm").toDouble(10.0);

    // 目标数组
    const QJsonValue tv = root.value("targets");
    if (!tv.isArray()) {
        if (errorMsg) *errorMsg = QString("targets 字段缺失或不是数组");
        return false;
    }
    const QJsonArray arr = tv.toArray();

    // ---- 开始写入：先清空旧数据，再设量程，再逐条添加 ----
    // 清空（倒序删）
    while (mgr.targetCount() > 0) {
        mgr.removeTarget(mgr.targetAt(0).id);
    }

    // 设量程（会触发 maxRangeChanged，UI 同步）
    mgr.setMaxRangeKm(maxRange);

    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        const QString name = o.value("name").toString();
        const double r  = o.value("rangeKm").toDouble(-1.0);
        const double a  = o.value("azimuthDeg").toDouble(0.0);
        const double s  = o.value("speedKmh").toDouble(0.0);
        const double h  = o.value("headingDeg").toDouble(0.0);
        const bool   am = o.value("autoMove").toBool(false);

        if (r < 0.0 || r > maxRange) {
            // 越界目标：跳过（不中断整个加载）
            continue;
        }

        const int id = mgr.addTargetAt(r, a, name);
        if (id < 0) continue;   // addTargetAt 拒绝（越界）
        mgr.setTargetMotion(id, s, h);
        mgr.setTargetAutoMove(id, am);
    }

    return true;
}