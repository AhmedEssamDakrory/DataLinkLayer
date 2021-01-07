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
    bool correctErrors(cMessage *msg);

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


    void initGoBackN();
    void startGoBackN(MyMessage_Base* msg);
    void sendData();
    const char* fromNetworkLayer();
    void enableNetworkLayer();
    int inc(int seq_num);
    bool between(int a, int b, int c);
    void startTimer(int seq_num);
    void stopTimer(int seq_num);

    //channel effect handling methods
    void toPhysicalLayer(MyMessage_Base* msg);
    void modificationEffect(MyMessage_Base* msg);
    bool duplicateEffect(MyMessage_Base* msg);
    void delaysEffect(MyMessage_Base* msg);
    bool lossEffect();
};

#endif
