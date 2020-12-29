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
using namespace omnetpp;

enum Events{
    FrameArrival,
    NetworkLayerReady,
    Timeout
};

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
    cMessage frame(cMessage *msg);
    cMessage unframe(cMessage *msg);
    bool correctErrors(cMessage *msg);

    // GoBackN protocol parameters
    int MAX_SEQ;
    int next_frame_to_send;
    int ack_expected;
    int frame_expected;
    int nbuffered;
    char** buffer;

    void initGoBackN();
    void startGoBackN(MyMessage_Base* msg);
    void sendData();
    char* fromNetworkLayer();
    void enableNetworkLayer();
    int inc(int seq_num);
    bool between(int a, int b, int c);
    int getDest();
};

#endif
