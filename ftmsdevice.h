#ifndef FTMSDEVICE_H
#define FTMSDEVICE_H

#include <QObject>
#include <QLowEnergyAdvertisingData>
#include <QLowEnergyController>
#include <QLowEnergyService>
#include <QLowEnergyServiceData>
#include <QLowEnergyCharacteristicData>
#include <QLowEnergyDescriptorData>
#include <QLowEnergyAdvertisingParameters>
#include <QTimer>

class FTMSDevice : public QObject
{
    Q_OBJECT
public:
    explicit FTMSDevice(QObject *parent = nullptr);

public slots:
    void setCurrentPower(qint16 power);
    void setCurrentCadence(quint8 cadence);
    void setCurrentHeartRate(quint8 heartRate);

    void initialize();

    void sendCurrentValues();
    void restartAdvertising();

signals:
    void newTargetPower(quint32 targetPower);
    void newTargetKp(double kp);

private slots:
    void onIncomingControlPointCommand(QLowEnergyCharacteristic c, QByteArray b);

private:
    QLowEnergyService *m_ftmsService;
    QLowEnergyDescriptorData m_ftmsClientConfig;
    QLowEnergyServiceData m_ftmsServiceData;
    QLowEnergyAdvertisingData m_advertisingData;

    QLowEnergyCharacteristicData m_indoorBikeCharData;
    QLowEnergyCharacteristicData m_powerRangeCharData;
    QLowEnergyCharacteristicData m_resistanceRangeCharData;
    QLowEnergyCharacteristicData m_ftmsFeatureCharData;
    QLowEnergyCharacteristicData m_ftmsControlPointCharData;
    QLowEnergyCharacteristicData m_ftmsStatusCharData;

    QLowEnergyDescriptorData m_indoorBikeClientConfig;
    QLowEnergyDescriptorData m_powerRangeClientConfig;
    QLowEnergyDescriptorData m_resistanceRangeClientConfig;
    QLowEnergyDescriptorData m_ftmsFeatureClientConfig;
    QLowEnergyDescriptorData m_ftmsControlPointClientConfig;
    QLowEnergyDescriptorData m_ftmsStatusClientConfig;

    QLowEnergyController *m_controller;

    quint8 m_currentHeartrate;
    qint16 m_currentPower;
    quint16 m_currentCadence;
    quint16 m_currentSpeed;

    QTimer m_updateTimer;
};

#endif // FTMSDEVICE_H
