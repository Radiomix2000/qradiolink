#ifndef DMRTRUNKING_H
#define DMRTRUNKING_H

#include <QObject>
#include "src/settings.h"
#include "src/logger.h"

class DMRTrunking : public QObject
{
    Q_OBJECT
public:
    explicit DMRTrunking(const Settings *settings, Logger *logger, QObject *parent = nullptr);

signals:

private:
    const Settings *_settings;
    Logger *_logger;

};

#endif // DMRTRUNKING_H
