
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
bool selectInterface();
void askVerbose();
void manualConfig();
void saveConfig();
bool checkExistingConfig(string&, bool readOnlyMode);
void askInterval();

static short readLag,idleSpeed,idleCheckTriesRX,idleCheckTriesTX,mode;
static string netInterface,rxtxFilePath,configPath = "config";
static double rxLast_, txLast_;
bool verbose;

int main(int argc, char** argv)
{
    if (argc > 1)
        configPath = argv[1];

    initialSetup();
    watchNetwork();

    if (mode == 3)
        pcOff();

    cout << "Bye!" << endl;
    return 0;
}

bool selectInterface()
{
    cout << "Network interface: ";
    cin >> netInterface;
    rxtxFilePath = "/sys/class/net/"+ netInterface + "/statistics/";

    ifstream fch;
    fch.open(rxtxFilePath+"rx_bytes");
    if (!fch.is_open()) {
        cout << "! Oppps, can't find that interface." << endl;
        return false;
    }
    fch.close();
    cout << "* Looks OK, carry on." << endl;

    return true;
}

void askVerbose()
{
    cout << "Verbose mode? (0-1): ";
    cin >> verbose;
}

void askInterval()
{
    cout << "Read interface data interval [s]: ";
    cin >> readLag;
}

void manualConfig()
{
    if (selectInterface()) {
        askInterval();
        cout << "Idle speed [kB/interval]: ";
        cin >> idleSpeed;
        cout<< "Positive idle speed passes: ";
        cin >> idleCheckTriesTX;
        idleCheckTriesRX = idleCheckTriesTX;
        askVerbose();
    } else {
        exit(-1);
    }
}

void saveConfig()
{
    bool sConfig;
    cout << "Save config? (0-1)";
    cin >> sConfig;

    if (sConfig) {
        string cName;
        cout << "Config name: ";
        cin >> cName;
        ofstream configFile;
        configFile.open(cName);
        if (configFile.is_open()) {
            configFile << mode << endl;
            configFile << netInterface << endl;
            configFile << readLag << endl;
            configFile << idleSpeed << endl;
            configFile << idleCheckTriesRX << endl;
            configFile << verbose << endl;

            configFile.close();
            cout << "Config saved." << endl;

        } else
            cout << "! Opps, can't save config file." << endl;
    }
}

bool checkExistingConfig(string &cFilePath, bool readOnlyMode)
{
    ifstream configFile;
    configFile.open(cFilePath);
    if (configFile.is_open()) {
        if (readOnlyMode) {
            cout << endl << "* Great, found config file, reading..." << endl;
            configFile >> mode;
            configFile.close();
            return true;
        }

        configFile >> mode;
        configFile >> netInterface;
        rxtxFilePath = "/sys/class/net/"+ netInterface + "/statistics/";
        configFile >> readLag;
        configFile >> idleSpeed;
        configFile >> idleCheckTriesRX;
        idleCheckTriesTX = idleCheckTriesRX;
        configFile >> verbose;

        cout << "* Current config: \n"
             << "* Mode: " << mode << endl
             << "* Interface: " << netInterface << endl
             << "* Interval: " << readLag << endl
             << "* Idle speed: " << idleSpeed << endl
             << "* Passes: " << idleCheckTriesRX << endl
             << "* Verbose: " << verbose << endl;

        configFile.close();
        return true;
    } else {
        cout << endl << "! Opps, can't read config file." << endl;
        return false;
    }
}

void initialSetup() {
    if (!checkExistingConfig(configPath,true)) {
        cout << endl << "* Select mode [1 - monitor; 2 - shutdownApp; 3 - shutdownPC]: " << endl << "> ";
        cin >> mode;
    }


    switch (mode) {
    case 1:
        cout << "* Monitor mode selected." << endl;
        selectInterface();
        askInterval();
        askVerbose();
        break;
    case 2:
        if (!checkExistingConfig(configPath,false)) {
            manualConfig();
            saveConfig();
        }
        break;
    case 3:
        if (!checkExistingConfig(configPath, false)) {
            manualConfig();
            saveConfig();
        }
    default:
        cout << "! Bad mode selected. Exit." << endl;
        exit(-1);
        break;
    }
}

void watchNetwork() {
    ofstream logFile;
    logFile.open(timeNow(true)+"_sessionLog");
    if (!logFile.is_open())
        cout << "! Error saving session log file. Proceeding without log file." << endl;

    logFile << "Interface: " << netInterface << "  interval: " << readLag << endl;

    cout << "-------------------" << endl
         << "* Cool, setup complete."<< endl
         << "* Launching network interface monitoring..." << endl;

    int rxDiff,txDiff;
    ushort txPass(0),rxPass(0); // cunting how many times traffic speed was below idleSpeed //

    if (mode == 1)
        idleCheckTriesRX = idleCheckTriesTX = 1;

    while ((idleCheckTriesRX > rxPass) || (idleCheckTriesTX > txPass)) {
        rxDiff = readValue(rxLast_,"rx_bytes");
        txDiff = readValue(txLast_,"tx_bytes");

        if (mode != 1) {
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
    //string cmd("echo \"power off\"");
    string cmd("sudo shutdown -hP now");
    system(cmd.c_str());
}
