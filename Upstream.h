#pragma

#ifndef UPSTREAM_H
#define UPSTREAM_H

#include <iostream>

#include <fstream>
#include <cstdlib>  // Required for rand() and srand()
#include <ctime>    // Required for time()
#include <chrono>
#include <thread>
#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

using namespace std;
const int numONUs = 2;

struct Packet {
    std::string name;
    int source, destination;
    long double bytes, generationTime, queueTime, receptionTime;
    long double distance;
};

struct ONU {
    int onuId;
    long double bandwidthDemand, allocatedBandwidth, remaingBandwidth, ONUdistance, RTT, BUP, q; 
    double weight,  probability = 0.0;;
    bool isOverloaded = false;
    std::vector<Packet> ONUBuffer[numONUs];
};

struct Frame {
    std::vector<Packet> packets;
    long double size = (38880*8) / 2.4e9;
};

void nonGuaranteedBandwidth(std::vector<ONU>& onus, double remainingBandwidth);

const int numberOfPackets = 1000;
Frame frame;

//Delay
int totalFreeBandwidth = 0;
double time_counter = 0, time_passed = 0.000125, delay = 0, delayFairness, SW = 0, q = 0;
long double TransmitionRate = 2.48838 , residual = 0.0, averageDelay = 0; //Gbps

std::vector<long double> totalDelaysPerONU(numONUs, 0.0);
std::vector<Packet> packetQueue;
std::vector<Packet> OLTBuffer;

//Variables for Packets Generation
double sum_variate = 0;
long double start_pktime, end_pktime, mean = 0, transmissionTime, variate, ref_time, ONU_time_counter, sum_rnd, rnd_nb;

long double ONULength[numONUs];
double totalGRR = 0;

long double FrameSize = (38880*8) / 0.000125; 
int BackgroundLoadParameter = 1;

long double Rf = (250*8) / 0.000125; 
long double Ra = (500*8) / 0.000125;


double L = 0.1; //Updating Impact
long double a = 0.000001;//10^(-5)

long double remainingBandwidth = 0;

int numOverloaded = 0,numNonOverloaded = 0;

//Random Generator
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_real_distribution<double> dis(0.0, 1.0);


int getRandomOnuId() {
    // Generate a random number between 2 and 30
    int randomNumber = rand() % numONUs;
    return randomNumber;
}

long double getBackgroundRate(int Onu_Id){
    double BackgroundRate = (0.1 * Onu_Id) / BackgroundLoadParameter;
    return BackgroundRate;
}

long double getRTT(int Distance){
    long double RTT = (Distance) / 2e5; 
    return RTT;
}

void generatePacket(std::vector<ONU>& onus) {
    long double generationTime = 0.0;
    int loops = 0;

    std::ofstream outputFile("generation_times.txt");
    for (int i = 0; i < numONUs; i++) {
     
        start_pktime= 0.000002;
        end_pktime= 0.000005;
        mean = start_pktime;
        ref_time = sum_variate;

        for (int innerLoops = 0; innerLoops < numberOfPackets; innerLoops++) {
            
            if(ONU_time_counter >= 0.00009)
            {
                mean = (long double) mean + (end_pktime - start_pktime)/((60 - ref_time)/0.00009);
                ONU_time_counter = ONU_time_counter - 0.00009;
            }

            //Random long Numbers Between 0-1
            rnd_nb =  dis(gen);
            
            //Poisson IAT
            variate = (-mean)*std::log(rnd_nb); 
            sum_variate = sum_variate + (variate);

            ONU_time_counter = ONU_time_counter + variate;

            //Generating Packet
            Packet packet;
            packet.source = getRandomOnuId();
            packet.bytes = 1500;
            packet.destination = 1; 
            packet.generationTime = sum_variate;
        
            ONULength[packet.source] += packet.bytes;
        
            onus[i].ONUBuffer->push_back(packet);
            outputFile << packet.generationTime << "\n";
        
            loops++;
        }
    }
   outputFile.close(); 
}

double calculateAverageDelay(const std::vector<Packet>& packets){
    double Total_Delay = 0;
    int numberOfPackets = packets.size();

    for (auto& packet : packets) {
        long double delay_in_func = (packet.receptionTime - packet.generationTime);
        totalDelaysPerONU[packet.source] += delay_in_func;
        Total_Delay += delay_in_func; 

    }

    return Total_Delay / packets.size();
}

void generateFrame(std::vector<ONU>& onus, double& time_passed) {
    for(auto& onu : onus) {
        Frame onuFrame; 

        while (!onu.ONUBuffer->empty() && onu.ONUBuffer->front().generationTime <= time_passed  && (onu.ONUBuffer->front().bytes*8 / 0.000125)  <= onu.allocatedBandwidth) {
            
            long double TransmisionTime = ((onu.ONUBuffer->front().bytes * 8) / (2.48838e9));
            long double QueuingDelay = time_passed - TransmisionTime - onu.ONUBuffer->front().generationTime;
            long double PropagationDelay = (onu.RTT / 2);    
            onu.ONUBuffer->front().receptionTime = onu.ONUBuffer->front().generationTime + QueuingDelay + TransmisionTime + PropagationDelay;

            onu.allocatedBandwidth -= (onu.ONUBuffer->front().bytes*8 / 0.000125);

            onuFrame.packets.push_back(onu.ONUBuffer->front());
            onu.ONUBuffer->erase(onu.ONUBuffer->begin());
            onuFrame.size -= onuFrame.packets.back().bytes;     
        }

        if(onuFrame.packets.size() >= 1){
            delay += calculateAverageDelay(onuFrame.packets);
        }
        if(onu.bandwidthDemand - onu.allocatedBandwidth > 0){
            onu.bandwidthDemand = onu.bandwidthDemand - onu.allocatedBandwidth;
        }else{
            onu.bandwidthDemand = 0;
        }
        onuFrame.packets.clear();
        onuFrame.size = 38880;
    }
}
void weight(std::vector<ONU>& onus, double L, long double a) {
    double sumOfWeights = 0.0;
    SW = 0;
    for (auto& onu : onus) {
        onu.weight = 1.0 / numONUs;
    }
    nonGuaranteedBandwidth(onus, remainingBandwidth);
}


int guaranteedBandwidth(std::vector<ONU>& onus) {   
    remainingBandwidth  = FrameSize;
    for (auto& onu:onus) {    
        onu.allocatedBandwidth = min(Rf + Ra, onu.bandwidthDemand);
        remainingBandwidth -= onu.allocatedBandwidth;
    }

    bool unsatisfiedNeedsExist = false;
    for (const auto& onu : onus) {
        if (onu.allocatedBandwidth < onu.bandwidthDemand) {
            unsatisfiedNeedsExist = true;
            break;
        }
    }
    if (unsatisfiedNeedsExist) {
        long double remainingUnsatisfiedBandwidth = 0;
        for (const auto& onu : onus) {
            if (onu.allocatedBandwidth < onu.bandwidthDemand) {
                remainingUnsatisfiedBandwidth += (onu.bandwidthDemand - onu.allocatedBandwidth);
            }
        }
        if (remainingBandwidth >= remainingUnsatisfiedBandwidth) {
            for (auto& onu : onus) {
                if (onu.allocatedBandwidth < onu.bandwidthDemand) {
                    onu.allocatedBandwidth = onu.bandwidthDemand;
                }
            }
        } else {
            weight(onus, L, a);
        }
    }else{
        return 0;
    } 
    return 0;
}

void nonGuaranteedBandwidth(std::vector<ONU>& onus, double remainingBandwidth) {
    for (auto& onu : onus) {
        onu.allocatedBandwidth = onu.weight * remainingBandwidth;
    }
}

double calculateDFI(std::vector<long double> delays) {
    if (delays.empty()) {
        cout << "Error: Delay's table is empty" << endl;
        return -1.0;
    }
    double sumDelay = std::accumulate(delays.begin(), delays.end(), 0.0);
    double numerator = sumDelay * sumDelay;
    double denominator = delays.size() * std::accumulate(delays.begin(), delays.end(), 0.0,[](double partialSum, double delay) {return partialSum + delay * delay;});
    double fairnessIndex = numerator / denominator;
    return fairnessIndex;
}

void getThroughput(){
    double throughput = (38880 / 0.000125) * 8;
    cout << "Throughput is: " << throughput << endl;
}
#endif // UPSTREAM_H