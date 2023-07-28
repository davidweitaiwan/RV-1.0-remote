//#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "NetworkClient.h"
#include "LogiV3.h"

std::mutex main_lock;
std::mutex sock_lock;

#define SERVER_IP "61.220.23.240"
#define SERVER_PORT 10000

#define DEVICE_NAME "LOGI_WHEEL"
#define CONTROLLER_INDEX 0

#define REMOTE_DEV_NAME "CAR1"

#define SEND_TIMESTAMP 1
#define TIMESTAMP_LOG

void key_Thread(bool& stopF)
{
    getchar();
    main_lock.lock();
    stopF = true;
    main_lock.unlock();
}

void ShowInfoThread(WheelState& logiWheel, SimpleWheelState& wh, NetworkClient* nc, bool& stopF)
{
    while (!stopF)
    {
        main_lock.lock();

        system("cls");

        printf("\n================================================================================\n");//16 * 5
        std::cout << std::right << std::setw(48) << "Wheel Information" << std::setw(32) << \
            (logiWheel.isInit() ? "" : "Uninitialized") << std::endl;
        printf("================================================================================\n");//16 * 5
        std::cout << std::left << std::setw(16) << "Wheel State" << std::right << \
            std::setw(10) << "Direction" << std::setw(10) << "Gas" << std::setw(10) << "Brake" << \
            std::setw(10) << "Clutch" << std::setw(12) << "MotionType" << std::setw(12) << "Gear" << std::endl;
        printf("--------------------------------------------------------------------------------\n");
        std::cout << std::left << std::setw(16) << "Wheel Output" << std::right << \
            std::setw(10) << logiWheel.getWheel() << std::setw(10) << logiWheel.getGas() << std::setw(10) << logiWheel.getBrake() << \
            std::setw(10) << logiWheel.getClutch() << std::setw(12) << logiWheel.getMotionType() << std::setw(12) << logiWheel.getGearString() << std::endl;
        std::cout << std::left << std::setw(16) << "Wheel Value" << std::right << \
            std::setw(10) << wh.getWheelState() << std::setw(10) << wh.getAccelerator() << std::setw(10) << wh.getBrake() << \
            std::setw(10) << wh.getClutch() << std::setw(12) << logiWheel.getMotionType() << std::setw(12) << logiWheel.getGearString() << std::endl;
        printf("--------------------------------------------------------------------------------\n");
        std::cout << std::left << std::setw(16) << "Wheel State" << logiWheel.getMsg().substr(0, 64) << std::endl;
        printf("================================================================================\n\n");//16 * 5

        printf("\n================================================================================\n");//16 * 5
        std::cout << std::right << std::setw(48) << "Socket Information" << std::endl;
        printf("================================================================================\n");//16 * 5
        std::cout << std::left << std::setw(16) << "Message Type" << \
            std::setw(48) << "Content" << std::setw(16) << "State" << std::endl;
        printf("--------------------------------------------------------------------------------\n");
        std::cout << std::left << std::setw(16) << "IPServer" << \
            std::setw(48) << nc->getHostInfo() << std::setw(16) << nc->getIPServerStateStr() << std::endl;
        std::cout << std::left << std::setw(16) << "Worker Send" << \
            std::setw(48) << nc->getRecvData() << std::setw(16) << nc->getControlStateStr() << std::endl;
        std::cout << std::left << std::setw(16) << "Transport Time" << \
            std::setw(48) << nc->getTransTime() << " ms" << std::setw(16) << std::endl;
        printf("================================================================================\n\n");//16 * 5

        main_lock.unlock();
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
}

std::string CreateSendStr(WheelState& ws, SimpleWheelState& sws)
{
    char sendBuf[128];
    sprintf(sendBuf, "%s:%d:%d:%d:%d:%d", ws.getGearString(), sws.getWheelState(), sws.getAccelerator(), sws.getBrake(), sws.getClutch(), ws.getMotionType());
    if (SEND_TIMESTAMP)
        return (std::string)sendBuf + ":" + NetworkClient::getTimestampStr();
    return (std::string)sendBuf;
}

void InitSock(NetworkClient* nc)
{
    nc->Init(SERVER_IP, SERVER_PORT);
    if (false == nc->Connect())
        exit(0);

    nc->Register(DEVICE_NAME);
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));

    while (!nc->isRemote())
    {
        try
        {
            nc->SendMsg(REMOTE_DEV_NAME, "RemoteRegister");
            boost::this_thread::sleep(boost::posix_time::milliseconds(500));
        }
        catch (...)
        {
            std::vector<std::string> devList = nc->getDeviceList();
            std::cout << "Connect to " << REMOTE_DEV_NAME << " device error, existing devices:\n\t";
            for (auto& i : devList)
                std::cout << " [" << i << "] ";
            std::cout << std::endl;
        }
    }
}

void main()
{
	bool stopF = false;
    bool showF = false;
    bool sockF = false;

    WheelState logiWheel(0);
    SimpleWheelState wh(&logiWheel);

    boost::thread logiTH(&main_logi, std::ref(stopF), std::ref(showF), std::ref(logiWheel));
    boost::thread keyTH(key_Thread, std::ref(stopF));

    boost::asio::io_service io_service;
    NetworkClient *nc;
    nc = new NetworkClient(io_service);

    boost::thread showTH(ShowInfoThread, std::ref(logiWheel), std::ref(wh), nc, std::ref(stopF));

    InitSock(nc);

#ifdef TIMESTAMP_LOG
    FILE* fd_tlog = fopen("timestamp_log.txt", "w");
#endif

    std::chrono::steady_clock execClock;
    auto execSt = execClock.now();
    double execTime = 0;

MAIN_LOOP:
	while (nc->isRemote() && nc->getIPServerStateStr() == "Connected")
	{
        wh.Update();
        std::string sendStr = CreateSendStr(logiWheel, wh);
        nc->SendMsg(REMOTE_DEV_NAME, sendStr);

        auto execNow = execClock.now();
        auto tSpan = static_cast<std::chrono::duration<double>>(execNow - execSt);
        execTime = tSpan.count();

#ifdef TIMESTAMP_LOG
        fprintf(fd_tlog, "%.2f,%s,%d,%s\n", execTime, NetworkClient::getTimestampStr().c_str(), (*nc).getTransTime(), sendStr.c_str());
#endif

        if (stopF)
            break;
        boost::this_thread::sleep(boost::posix_time::milliseconds(50));
	}

    if ((!nc->isRemote() || (nc->getIPServerStateStr() == "Disconnect")) && !stopF)// Disconnect event
    {
        nc = new NetworkClient(io_service);
        InitSock(nc);
    }
    if (!stopF)
        goto MAIN_LOOP;
    keyTH.join();
	logiTH.join();

#ifdef TIMESTAMP_LOG
    fclose(fd_tlog);
#endif

	system("pause");

	return;
}