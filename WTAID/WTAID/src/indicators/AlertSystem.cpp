#include "indicators/AlertSystem.h"

#include <iostream>
#include <algorithm>

AlertSystem::AlertSystem()
    : alertCallback_(nullptr)
    , criticalG_(9.0f)
{
}

void AlertSystem::onAlert(AlertCallback callback)
{
    alertCallback_ = std::move(callback);
}

void AlertSystem::setCriticalG(float critG)
{
    criticalG_ = critG;
}

bool AlertSystem::checkIndicator(const Indicator* indicator, float value)
{
    if (!indicator)
        return false;

    const auto& config = indicator->getConfig();
    bool triggered = false;

    for (const auto& rule : config.alerts)
    {
        bool ruleTriggered = false;
        float threshold = rule.thresholdA;

        // Для алерта "High G" используем динамический порог из FM-базы
        if (rule.name == "High G" && criticalG_ > 0.0f)
        {
            threshold = criticalG_ * 0.95f;  // 95% от критического
        }

        switch (rule.relation)
        {
        case AlertRule::Relation::GREATER:
            ruleTriggered = value > threshold;
            break;
        case AlertRule::Relation::LESS:
            ruleTriggered = value < threshold;
            break;
        case AlertRule::Relation::EQUAL:
            ruleTriggered = std::abs(value - threshold) < 0.01f;
            break;
        case AlertRule::Relation::RANGE:
            ruleTriggered = value > rule.thresholdA &&
                           value < rule.thresholdB;
            break;
        }

        if (ruleTriggered)
        {
            processAlert(config.name, rule, value, rule.colorName);
            triggered = true;
        }
    }

    return triggered;
}

void AlertSystem::checkAll(const std::vector<IndicatorResult>& results)
{
    activeAlerts_.clear();  // Очищаем старые алерты
    
    for (const auto& result : results)
    {
        if (result.alertTriggered)
        {
            activeAlerts_.push_back(result.alertName);
        }
    }
}

std::vector<std::string> AlertSystem::getActiveAlerts() const
{
    return activeAlerts_;
}

void AlertSystem::reset()
{
    activeAlerts_.clear();
}

void AlertSystem::addAlertRule(const std::string& indicatorName,
                                const AlertRule& rule)
{
    alertRules_[indicatorName].push_back(rule);
}

void AlertSystem::processAlert(const std::string& indicatorName,
                                const AlertRule& rule,
                                float value,
                                const std::string& color)
{
    std::string message = "ALERT [" + rule.name + "]: " +
                          indicatorName + " = " + std::to_string(value);

    if (alertCallback_)
    {
        alertCallback_(rule.name, value, color);
    }
    else
    {
        std::cout << message << std::endl;
    }
}
