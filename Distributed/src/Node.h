//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __DISTRIBUTED_NODE_H_
#define __DISTRIBUTED_NODE_H_

#include <omnetpp.h>
#include "MyMessage_m.h"
#include <bitset>
#include <string>
#include <vector>
using namespace omnetpp;

enum Events{
    FrameArrival,
    NetworkLayerReady,
    Timeout,
    INIT
};

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:

    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    void frameWithByteStuffing(MyMessage_Base* mmsg);
    char* unframe(MyMessage_Base* mmsg);

    // hamming error correction
    void correctErrors(MyMessage_Base *mmsg);
    void addHamming(MyMessage_Base *mmsg);
    bool isPowerOfTwo(int x);
    std::string binarize(std::string s);
    std::string characterize(std::string s);

    // statistics
    static int generated_frames;
    static int duplicated_frames;
    static int dropped_frames;
    static int retransmitted_frames;
    static int useful_frames;
    static bool printed;


    // GoBackN protocol parameters
    int dest;
    int MAX_SEQ;
    int next_frame_to_send;
    int ack_expected;
    int frame_expected;
    int nbuffered;
    const char** buffer;
    MyMessage_Base** timers;
    bool* isTimerSet;
    omnetpp::simtime_t  lastResetTime;

    std::vector<std::string> msgs;
    int sent_frames = 0;

    void initGoBackN();
    void startGoBackN(MyMessage_Base* msg);
    void sendData(bool retransmitted);
    const char* fromNetworkLayer();
    void enableNetworkLayer();
    int inc(int seq_num);
    bool between(int a, int b, int c);
    void startTimer(int seq_num);
    void stopTimer(int seq_num);
    void printStatistics();

    //channel effect handling methods
    void toPhysicalLayer(MyMessage_Base* msg, bool retransmitted);
    void modificationEffect(MyMessage_Base* msg);
    bool duplicateEffect(MyMessage_Base* msg);
    void delaysEffect(MyMessage_Base* msg);
    bool lossEffect();
};

#endif
