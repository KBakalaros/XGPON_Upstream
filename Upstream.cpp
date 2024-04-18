#include "Upstream.h"
#include <iostream>
#include <stdio.h>
#include <chrono>
#include <thread>

using namespace std;

int main() {
    std::vector<ONU> onus(numONUs); // Δημιουργία ενός πίνακα ONUs
    cout<< "ONUs: " << numONUs << endl;
    generatePacket(onus);
    for (int i = 0; i < numONUs; i++) {
        onus[i].onuId = i+1;
        onus[i].bandwidthDemand = (ONULength[i]*8) / 0.000125;//Bandwidth (bps) 
        onus[i].allocatedBandwidth = 0;
        onus[i].ONUdistance = i + 20; 
        onus[i].RTT = getRTT(onus[i].ONUdistance);
    } 
 
    while (!std::all_of(onus.begin(), onus.end(), [](const ONU& onu) { return onu.bandwidthDemand <= 0; })){ 
        guaranteedBandwidth(onus);
        generateFrame(onus, time_passed);
        time_passed += 0.000125; 
    }
    cout<< "DFI: " << calculateDFI(totalDelaysPerONU) << endl;
    cout << "Average Delay is: " <<  delay << "\n" << endl;   
    getThroughput();      
}   