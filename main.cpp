#include <QtCore/qcoreapplication.h>
#include "ftmsdevice.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    FTMSDevice fd;
    fd.initialize();

    QObject::connect(&fd, &FTMSDevice::newTargetPower, [](quint32 targetPower){
        qDebug() << "Got new target power:" << targetPower;
    });

    QObject::connect(&fd, &FTMSDevice::newTargetKp, [](double targetKp){
        qDebug() << "Got new target kp" << targetKp;
    });
    return app.exec();
}
