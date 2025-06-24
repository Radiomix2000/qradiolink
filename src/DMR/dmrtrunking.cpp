#include "dmrtrunking.h"

DMRTrunking::DMRTrunking(const Settings *settings, Logger *logger, QObject *parent)
    : QObject{parent}
{
    _settings = settings;
    _logger = logger;
}
