#pragma once
#include <QString>

class TargetManager;

// 场景文件读写。返回 true 表示成功，失败时 errorMsg 填错误信息。
namespace ScenarioIO {

bool save(const QString& filePath, const TargetManager& mgr, QString* errorMsg = nullptr);
bool load(const QString& filePath, TargetManager& mgr, QString* errorMsg = nullptr);

} // namespace ScenarioIO