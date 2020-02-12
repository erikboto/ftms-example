#include "ftmsdevice.h"

#include <QDataStream>
#include <QTimer>

#define FTMSDEVICE_FTMS_UUID 0x1826
#define FTMSDEVICE_INDOOR_BIKE_CHAR_UUID 0x2AD2
#define FTMSDEVICE_POWER_RANGE_CHAR_UUID 0x2AD8
#define FTMSDEVICE_RESISTANCE_RANGE_CHAR_UUID 0x2AD6
#define FTMSDEVICE_FTMS_FEATURE_CHAR_UUID 0x2ACC
#define FTMSDEVICE_FTMS_CONTROL_POINT_CHAR_UUID 0x2AD9
#define FTMSDEVICE_FTMS_STATUS_CHAR_UUID 0x2ADA

FTMSDevice::FTMSDevice(QObject *parent) : QObject(parent),
    m_currentHeartrate(0),
    m_currentPower(0),
    m_currentCadence(0),
    m_currentSpeed(0)
{
}

enum FtmsControlPointCommand {
    FTMS_REQUEST_CONTROL = 0x00,
    FTMS_RESET,
    FTMS_SET_TARGET_SPEED,
    FTMS_SET_TARGET_INCLINATION,
    FTMS_SET_TARGET_RESISTANCE_LEVEL,
    FTMS_SET_TARGET_POWER,
    FTMS_SET_TARGET_HEARTRATE,
    FTMS_START_RESUME,
    FTMS_STOP_PAUSE,
    FTMS_SET_TARGETED_EXP_ENERGY,
    FTMS_SET_TARGETED_STEPS,
    FTMS_SET_TARGETED_STRIDES,
    FTMS_SET_TARGETED_DISTANCE,
    FTMS_SET_TARGETED_TIME,
    FTMS_SET_TARGETED_TIME_TWO_HR_ZONES,
    FTMS_SET_TARGETED_TIME_THREE_HR_ZONES,
    FTMS_SET_TARGETED_TIME_FIVE_HR_ZONES,
    FTMS_SET_INDOOR_BIKE_SIMULATION_PARAMS,
    FTMS_SET_WHEEL_CIRCUMFERENCE,
    FTMS_SPIN_DOWN_CONTROL,
    FTMS_SET_TARGETED_CADENCE,
    FTMS_RESPONSE_CODE = 0x80
};

enum FtmsResultCode {
    FTMS_SUCCESS = 0x01,
    FTMS_NOT_SUPPORTED,
    FTMS_INVALID_PARAMETER,
    FTMS_OPERATION_FAILED,
    FTMS_CONTROL_NOT_PERMITTED
};

void FTMSDevice::initialize()
{
    // Setup up the advertising properly
    m_advertisingData.setDiscoverability(QLowEnergyAdvertisingData::DiscoverabilityGeneral);
    m_advertisingData.setIncludePowerLevel(true); // Switch to false?
    m_advertisingData.setLocalName("M"); // Use short name to save bytes
    m_advertisingData.setServices(QList<QBluetoothUuid>() << QBluetoothUuid((quint16)FTMSDEVICE_FTMS_UUID));

    // Setup of indoorBike characteristic
    m_indoorBikeCharData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_INDOOR_BIKE_CHAR_UUID));
    m_indoorBikeCharData.setProperties(QLowEnergyCharacteristic::Notify);
    m_indoorBikeClientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                        QByteArray(2, 0));
    m_indoorBikeCharData.addDescriptor(m_indoorBikeClientConfig);

    // Setup of powerRange characteristic
    m_powerRangeCharData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_POWER_RANGE_CHAR_UUID));
    QByteArray powerRangeData;
    QDataStream powerRangeCharDataDs(&powerRangeData, QIODevice::ReadWrite);
    qint16 maxPwr = 1400;
    qint16 minPwr = 0;
    quint16 stepPwr = 1;
    powerRangeCharDataDs.setByteOrder(QDataStream::LittleEndian);
    powerRangeCharDataDs << minPwr << maxPwr << stepPwr;
    m_powerRangeCharData.setValue(powerRangeData);;
    m_powerRangeCharData.setProperties(QLowEnergyCharacteristic::Read);
    m_powerRangeClientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    m_powerRangeCharData.addDescriptor(m_powerRangeClientConfig);

    // Setup of resistanceRange characteristic
    m_resistanceRangeCharData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_RESISTANCE_RANGE_CHAR_UUID));
    QByteArray resistanceRangeData;
    QDataStream resistanceRangeCharDataDs(&resistanceRangeData, QIODevice::ReadWrite);
    qint16 maxRes = 70;
    qint16 minRes = 0;
    quint16 stepRes = 1;
    resistanceRangeCharDataDs.setByteOrder(QDataStream::LittleEndian);
    resistanceRangeCharDataDs << minRes << maxRes << stepRes;
    m_resistanceRangeCharData.setValue(resistanceRangeData);
    m_resistanceRangeCharData.setProperties(QLowEnergyCharacteristic::Read);
    m_resistanceRangeClientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    m_resistanceRangeCharData.addDescriptor(m_resistanceRangeClientConfig);

    // Setup of ftmsFeature characteristic
    m_ftmsFeatureCharData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_FTMS_FEATURE_CHAR_UUID));
    QByteArray ftmsFeatureCharDataRaw;
    QDataStream ftmsFeatureCharDataDs(&ftmsFeatureCharDataRaw, QIODevice::ReadWrite);
    ftmsFeatureCharDataDs.setByteOrder(QDataStream::LittleEndian);
    //                   10987654321098765432109876543210
    quint32 features = 0b00000000000000000100000010000010;
    quint32 settings = 0b00000000000000000000000000001100;
    ftmsFeatureCharDataDs << features << settings;
    m_ftmsFeatureCharData.setValue(ftmsFeatureCharDataRaw);
    m_ftmsFeatureCharData.setProperties(QLowEnergyCharacteristic::Read);
    m_ftmsFeatureClientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    m_ftmsFeatureCharData.addDescriptor(m_ftmsFeatureClientConfig);

    // Setup of m_ftmsControlPoint characteristic
    m_ftmsControlPointCharData.setUuid(QBluetoothUuid((quint16)0x2AD9));
    m_ftmsControlPointCharData.setProperties(QLowEnergyCharacteristic::Write | QLowEnergyCharacteristic::Indicate);
    const QLowEnergyDescriptorData cpClientConfig(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    m_ftmsControlPointCharData.addDescriptor(cpClientConfig);

    // Setup of ftmsStatus characteristic
    m_ftmsStatusCharData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_FTMS_STATUS_CHAR_UUID));
    m_ftmsStatusCharData.setProperties(QLowEnergyCharacteristic::Notify);
    m_ftmsStatusClientConfig = QLowEnergyDescriptorData(QBluetoothUuid::ClientCharacteristicConfiguration,
                                                QByteArray(2, 0));
    m_ftmsStatusCharData.addDescriptor(m_ftmsStatusClientConfig);

    // Final setup (Note that order of chars seems to matter?!)
    m_ftmsServiceData.setType(QLowEnergyServiceData::ServiceTypePrimary);
    m_ftmsServiceData.setUuid(QBluetoothUuid((quint16)FTMSDEVICE_FTMS_UUID));
    m_ftmsServiceData.addCharacteristic(m_indoorBikeCharData);
    m_ftmsServiceData.addCharacteristic(m_powerRangeCharData);
    m_ftmsServiceData.addCharacteristic(m_ftmsControlPointCharData);
    m_ftmsServiceData.addCharacteristic(m_ftmsFeatureCharData);
    m_ftmsServiceData.addCharacteristic(m_ftmsStatusCharData);
    m_ftmsServiceData.addCharacteristic(m_resistanceRangeCharData);

    m_controller = QLowEnergyController::createPeripheral();
    m_ftmsService = m_controller->addService(m_ftmsServiceData);


    // Start advertising
    m_controller->startAdvertising(QLowEnergyAdvertisingParameters(), m_advertisingData,
                                   m_advertisingData);

    // Set up connection for incoming FTMS CP requests
    QObject::connect(m_ftmsService, &QLowEnergyService::characteristicChanged, this, &FTMSDevice::onIncomingControlPointCommand);

    connect(&m_updateTimer, &QTimer::timeout, this, &FTMSDevice::sendCurrentValues);
    m_updateTimer.start(1000);

    connect(m_controller, &QLowEnergyController::disconnected, this, &FTMSDevice::restartAdvertising);
}

void FTMSDevice::onIncomingControlPointCommand(QLowEnergyCharacteristic c ,QByteArray b)
{
    qDebug() << "Write to " << c.uuid();
    qDebug() << b;

    Q_ASSERT(b.size() >= 1);

    // Prepare for reply
    QLowEnergyCharacteristic characteristic
            = m_ftmsService->characteristic(QBluetoothUuid((quint16)FTMSDEVICE_FTMS_CONTROL_POINT_CHAR_UUID));
    Q_ASSERT(characteristic.isValid());
    QByteArray reply;
    QDataStream replyDs(&reply, QIODevice::ReadWrite);
    replyDs.setByteOrder(QDataStream::LittleEndian);

    // Set up a stream from parsing command
    QDataStream inData(&b, QIODevice::ReadOnly);
    inData.setByteOrder(QDataStream::LittleEndian);

    quint8 cmd;
    inData >> cmd;

    switch (static_cast<FtmsControlPointCommand>(cmd))
    {
    case FTMS_REQUEST_CONTROL:
        qDebug() << "Control requested";
        replyDs << (quint8)FTMS_RESPONSE_CODE << (quint8)FTMS_REQUEST_CONTROL << (quint8)FTMS_SUCCESS;
        break;
    case FTMS_RESET:
        emit newTargetPower(100);
        replyDs << (quint8)FTMS_RESPONSE_CODE << (quint8)FTMS_RESET << (quint8)FTMS_SUCCESS ;
        break;
    case FTMS_SET_TARGET_RESISTANCE_LEVEL:
    {
        qint8 requestedResistanceLevel;
        inData >> requestedResistanceLevel;
        emit newTargetKp(requestedResistanceLevel/10.0);
        replyDs << (quint8)FTMS_RESPONSE_CODE << (quint8)FTMS_SET_TARGET_RESISTANCE_LEVEL << (quint8)FTMS_SUCCESS ;
    }
        break;
    case FTMS_SET_TARGET_POWER:
    {
        qint16 targetPower;
        inData >> targetPower;
        replyDs << (quint8)FTMS_RESPONSE_CODE << (quint8)FTMS_SET_TARGET_POWER << (quint8)FTMS_SUCCESS;
        emit newTargetPower(targetPower);
        qDebug() << "New Target Power: " << targetPower;
    }
        break;
    default:
        qDebug() << "Unhandled command";
    }

    if (!reply.isEmpty())
    {
        //qDebug() << reply;
        m_ftmsService->writeCharacteristic(characteristic, reply);
    }
};

void FTMSDevice::sendCurrentValues()
{
    //               bits 5432109876543210
    quint16 charFlags = 0b0000000001000100; // insta cadance and power

    QByteArray charData;
    QDataStream charDataDs(&charData, QIODevice::ReadWrite);
    charDataDs.setByteOrder(QDataStream::LittleEndian);
    charDataDs << charFlags << m_currentSpeed << m_currentCadence << m_currentPower;

    QLowEnergyCharacteristic characteristic
            = m_ftmsService->characteristic(QBluetoothUuid((quint16)FTMSDEVICE_INDOOR_BIKE_CHAR_UUID));
    Q_ASSERT(characteristic.isValid());
    m_ftmsService->writeCharacteristic(characteristic, charData); // Potentially causes notification.
}


void FTMSDevice::setCurrentPower(qint16 power)
{
    m_currentPower = power;
}
void FTMSDevice::setCurrentCadence(quint8 cadence)
{
    m_currentCadence = cadence;
}
void FTMSDevice::setCurrentHeartRate(quint8 heartRate)
{
    m_currentHeartrate = heartRate;
}

void FTMSDevice::restartAdvertising()
{
    m_ftmsService = m_controller->addService(m_ftmsServiceData);
    if (m_ftmsService)
    {
        m_controller->startAdvertising(QLowEnergyAdvertisingParameters(),
                                       m_advertisingData, m_advertisingData);
    }
}
