
// simple-netstat by marazmista
// copy @ 04.2013


#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <ctime>
#include <sstream>

using namespace std;

void initialSetup();
void watchNetwork();
void pcOff();
int readValue(double&, string);
string timeNow(bool);

static short readLag,idleSpeed,idleCheckTriesRX,idleCheckTriesTX;
static string netInterface,rxtxFilePath;
static double rxLast_, txLast_;
bool verbose,monitorMode;

int main()
{
    initialSetup();
    watchNetwork();

    if (!monitorMode)
        pcOff();

    return 0;
}

void initialSetup() {
    ifstream configFile;
    configFile.open("config");
    if (configFile.is_open()) {
        cout << endl << "* Great, found config file, reading..." << endl;

        configFile >> netInterface;
        rxtxFilePath = "/sys/class/net/"+ netInterface + "/statistics/";
        configFile >> readLag;
        configFile >> idleSpeed;
        configFile >> idleCheckTriesRX;
        idleCheckTriesTX = idleCheckTriesRX;
        configFile >> verbose;
        configFile >> monitorMode;

        cout << "* Current config: \n" << "* Interface: " << netInterface << endl
             << "* Interval: " << readLag << endl
             << "* Idle speed: " << idleSpeed << endl
             << "* Passes: " << idleCheckTriesRX << endl
             << "* Verbose: " << verbose << endl
             << "* Monitor mode " << monitorMode << endl;

        configFile.close();
        return;
    }

    cout << endl << "* Config file not found, manual setup." << endl;

    cout << "Network interface: ";
    cin >> netInterface;
    rxtxFilePath = "/sys/class/net/"+ netInterface + "/statistics/";

    ifstream fch;
    fch.open(rxtxFilePath+"rx_bytes");
    if (!fch.is_open()) {
        cout << "! Oppps, can't find that interface." << endl;
        return;
    }
    fch.close();

    cout << "* Looks OK, carry on." << endl;
    cout << "Read interface data interval [s]: ";
    cin >> readLag;
    cout << "Idle speed [kB/interval]: ";
    cin >> idleSpeed;
    cout<< "Positive idle speed passes (meaningless in monitor mode): ";
    cin >> idleCheckTriesTX;
    idleCheckTriesRX = idleCheckTriesTX;
    cout << "Verbose mode? (0-1): ";
    cin >> verbose;
    cout << "Monitor mode? (disable shutdown pc when network traffic drops to nothing) (0-1): ";
    cin >> monitorMode;

    bool sConfig;
    cout << "Save config? (0-1)";
    cin >> sConfig;

    if (sConfig) {
        ofstream configFile;
        configFile.open("config");
        if (configFile.is_open()) {
            configFile << netInterface << endl;
            configFile << readLag << endl;
            configFile << idleSpeed << endl;
            configFile << idleCheckTriesRX << endl;
            configFile << verbose << endl;
            configFile << monitorMode <<endl;

            configFile.close();
            cout << "Config saved. Search for it in app dir." << endl;

        } else
            cout << "! Opps, can't save config file." << endl;
    }
}

void watchNetwork() {
    ofstream logFile;
    logFile.open(timeNow(true)+"_sessionLog");

    if (!logFile.is_open())
        cout << "! Error saving session log file. Proceeding without log file." << endl;

    logFile << "Interface: " << netInterface << "  interval: " << readLag << " idleSpeed: " << idleSpeed << endl;

    cout << "-------------------" << endl
         << "* Cool, setup complete."<< endl
         << "* Launching network interface monitoring..." << endl;

    ushort txPass(0),rxPass(0); // cunting how many times traffic speed was below idleSpeed //
    int rxDiff,txDiff;

    while ((idleCheckTriesRX > rxPass) || (idleCheckTriesTX > txPass)) {
        rxDiff = readValue(rxLast_,"rx_bytes");
        txDiff = readValue(txLast_,"tx_bytes");

        if (!monitorMode) {
            if (rxDiff < idleSpeed)
                rxPass++;
            else
                rxPass = 0;

            if (txDiff < idleSpeed)
                txPass++;
            else
                txPass = 0;
        }

        if (verbose)
            cout << "  (" << timeNow(false) << ")  D:" << rxDiff << " U:" << txDiff << " | dPass: " << rxPass <<  " uPass: " << txPass << endl;

        logFile << timeNow(false) << ";" << rxDiff << ";" << txDiff << ";"<< rxPass << ";" << txPass << endl;
        usleep(readLag*1000*1000);
    }

    logFile.close();
}

string timeNow(bool toLogName) {
    time_t now = time(0);
    tm *localtm = localtime(&now);
    stringstream os;

    if (toLogName) {
        os << localtm->tm_mday << "-" << localtm->tm_mon + 1 << "-" << localtm->tm_year + 1900 << "_"
           << localtm->tm_hour << "-" << localtm->tm_min << "-" << localtm->tm_sec;
        return os.str();
    } else {
        os << localtm->tm_hour << ":" << localtm->tm_min << ":" << localtm->tm_sec;
        return os.str();
    }
}

int readValue(double &lastValue, string filename) {
    ifstream file;
    file.open(rxtxFilePath + filename);

    char tmp[25];
    file.getline(tmp,25);
    file.close();

    double nowValue(stod(tmp));
    double diff((nowValue - lastValue) / 1024);
    lastValue = nowValue;
    return (int)diff;

}

void pcOff() {
   // string cmd("echo \"power off\"");
    string cmd("sudo shutdown -hP now");
    system(cmd.c_str());
}
