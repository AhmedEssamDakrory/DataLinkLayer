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

#include "Initializer.h"
#include "Node.h"
#include "MyMessage_m.h"
#include <vector>

Define_Module(Initializer);

void Initializer::initialize()
{
    MyMessage_Base* msg = new MyMessage_Base("");
    scheduleAt(simTime(), msg);
}

void Initializer::handleMessage(cMessage *msg)
{
    if(msg->isSelfMessage()){
        generateRandomPairs();
        // TODO: schedule self message after 30 seconds.
        MyMessage_Base* msg = new MyMessage_Base("");
        scheduleAt(simTime()+par("timeStep"), msg);
    }
}

void Initializer::generateRandomPairs()
{
    EV<<"Resetting......"<<endl;
    std::vector<int> nodes;
    int n = gateSize("outs");
    for(int i = 0 ; i < n ; ++i) {
        nodes.push_back(i);
    }

    simtime_t resetTime = simTime();
    while((int) nodes.size() > 1){
        int p1 = nodes[0];
        nodes.erase(nodes.begin());
        int randomPos = uniform(0, nodes.size());
        int p2 = nodes[randomPos];
        nodes.erase(nodes.begin()+randomPos);
        MyMessage_Base* msg1 = new MyMessage_Base("");
        MyMessage_Base* msg2 = new MyMessage_Base("");
        msg1->setM_Type(INIT);
        msg2->setM_Type(INIT);
        msg1->setAck(p2);
        msg2->setAck(p1);
        msg1->setTimestamp(resetTime);
        msg2->setTimestamp(resetTime);
        send(msg1, "outs", p1);
        send(msg2, "outs", p2);
    }
}
